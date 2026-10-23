#!/usr/bin/env python3
"""Persistent, local-only mGBA lab. No ROM/save material enters Git."""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import shutil
import socket
import socketserver
import struct
import subprocess
import sys
import time
import tempfile

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / '.arena-dev'
LOCAL = ROOT / '.arena-dev'
# macOS Unix sockets have a ~104-byte path limit. Nested isolated workspaces
# need a short, per-user/per-workspace private directory, never a shared socket.
SOCKET_DIR = Path('/tmp') / ('emerald-lab-' + str(os.getuid()) + '-' + hashlib.sha256(str(ROOT).encode()).hexdigest()[:12])
SOCKET_DIR.mkdir(mode=0o700, exist_ok=True)
if SOCKET_DIR.is_symlink() or SOCKET_DIR.stat().st_uid != os.getuid() or SOCKET_DIR.stat().st_mode & 0o077:
    raise RuntimeError('Unsafe local emulator socket directory')
SOCKET = SOCKET_DIR / 'lab.sock'
KEYS = {'A': 1, 'B': 2, 'SELECT': 4, 'START': 8, 'RIGHT': 16,
        'LEFT': 32, 'UP': 64, 'DOWN': 128, 'R': 256, 'L': 512, 'NONE': 0}


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class Lab:
    def __init__(self):
        self.runner = None
        self.live_speed = 4  # User requested the exact controlled core visible.
        self.mode = 'lab'
        self.reload()

    def reload(self, mode=None):
        if mode is not None: self.mode = mode
        if self.runner:
            self.raw('quit', reply=False)
            self.runner.wait(timeout=5)
        base = 'arena_lab' if self.mode == 'lab' else 'pokeemerald'
        rom = ROOT / (base + '.gba')
        elf = ROOT / (base + '.elf')
        self.rom_path = rom
        if not rom.exists() or not elf.exists():
            raise RuntimeError('Build first: ./tools/arena/dev.sh build')
        self.sha = digest(rom)
        self.release = WORK / 'builds' / self.sha
        self.release.mkdir(parents=True, exist_ok=True)
        # Multiple disposable test cores can share one content-addressed build.
        # Never truncate a ROM another core has open/mapped during reload.
        for source, name in ((rom, 'game.gba'), (elf, 'game.elf')):
            destination = self.release / name
            if destination.exists() and digest(destination) == digest(source):
                continue
            with tempfile.NamedTemporaryFile(dir=self.release, prefix=name+'.', delete=False) as tmp:
                temporary = Path(tmp.name)
            try:
                shutil.copy2(source, temporary)
                os.replace(temporary, destination)
            finally:
                temporary.unlink(missing_ok=True)
        self.symbols = {}
        self.names = {}
        nm = subprocess.check_output(['arm-none-eabi-nm', '-n', str(elf)], text=True)
        for line in nm.splitlines():
            parts = line.split()
            if len(parts) == 3 and re.fullmatch('[0-9a-fA-F]+', parts[0]):
                addr = int(parts[0], 16)
                self.symbols[parts[2]] = addr
                self.names[addr] = parts[2]
        if self.mode == 'lab' and 'gArenaLabMailbox' not in self.symbols:
            raise RuntimeError('Not a lab build: missing native development mailbox')
        bundle = WORK / 'Esmeralda Laboratorio.app/Contents'
        (bundle / 'MacOS').mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / 'tools/arena/Emerald-Lab-Info.plist', bundle / 'Info.plist')
        executable = bundle / 'MacOS/arena-runner'
        if not executable.exists() or digest(executable) != digest(LOCAL / 'arena-runner'):
            shutil.copy2(LOCAL / 'arena-runner', executable)
        self.runner = subprocess.Popen([str(executable), str(self.release / 'game.gba')],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
            text=True, bufsize=1)
        if self.runner.stdout.readline().strip() != 'ready':
            raise RuntimeError('mGBA driver failed to start')
        if self.live_speed is not None: self.raw(f'view {self.live_speed}')
        return {'rom_sha256': self.sha, 'pid': self.runner.pid}

    def raw(self, command, reply=True):
        self.runner.stdin.write(command + '\n')
        self.runner.stdin.flush()
        if not reply:
            return
        import select
        if not select.select([self.runner.stdout], [], [], 30)[0]:
            raise TimeoutError('mGBA command exceeded 30 s: ' + command)
        line = self.runner.stdout.readline().strip()
        if not line:
            raise RuntimeError('mGBA driver stopped')
        if line == 'failed':
            raise RuntimeError('mGBA rejected command: ' + command)
        return line

    def read(self, address, size):
        if isinstance(address, str):
            address = self.symbols[address]
        return bytes.fromhex(self.raw(f'read {address:x} {size}'))

    def u32(self, address):
        return int.from_bytes(self.read(address, 4), 'little')

    def frame(self, frames=1, keys=0):
        if not 0 <= frames <= 18000:
            raise ValueError('Frame count must be 0..18000')
        # Bound each core request even with a live window running at real speed.
        while frames:
            chunk = min(frames, 600)
            self.raw(f'frame {chunk} {keys}')
            frames -= chunk

    def key(self, keys, frames, release=True):
        value = 0
        for name in keys.upper().split('+'):
            value |= KEYS[name]
        self.frame(frames, value)
        if release: self.frame(1, 0)

    def task_names(self):
        tasks = self.read('gTasks', 16 * 40)
        return [self.names.get(int.from_bytes(tasks[i:i+4], 'little') & ~1, '?')
                for i in range(0, len(tasks), 40) if tasks[i+4]]

    def cb2(self):
        return self.names.get(self.u32(self.symbols['gMain'] + 4) & ~1, '?')

    def telemetry(self):
        v = struct.unpack('<10I2H4h2H4B', self.read('gRealtimeArenaTelemetry', 60))
        return dict(entries=v[0], frames=v[1], shots=list(v[2:4]), hits=list(v[4:6]),
            misses=v[6], dodges=v[7], exits=v[8], exp_before=v[9], hp_before=v[10],
            last_damage=v[11], x=list(v[12:14]), y=list(v[14:16]), speed=list(v[16:18]),
            active=bool(v[18]), paused=bool(v[19]), faint_exit=bool(v[20]), move_slot=v[21])

    def ai_telemetry(self):
        if 'gArenaAiTelemetry' not in self.symbols: return None
        v = struct.unpack('<7I8B4h', self.read('gArenaAiTelemetry', 44))
        return dict(decisions=v[0], paths=v[1], dodges=v[2], blocked_shots=list(v[3:5]),
            wall_blocks=list(v[5:7]), level=v[7], reaction_frames=v[8], aim_error_px=v[9],
            style=v[10], state=['reposition', 'aim', 'recover', 'evade'][min(v[11],3)],
            facing=list(v[12:14]), goal=list(v[15:17]), waypoint=list(v[17:19]))

    def sprite_telemetry(self):
        if 'gArenaSpriteTelemetry' not in self.symbols: return None
        v = struct.unpack('<8B2I', self.read('gArenaSpriteTelemetry', 16))
        return dict(pmd=list(v[:2]), animation=list(v[2:4]), frame=list(v[4:6]),
            direction=list(v[6:8]), uploads=list(v[8:10]))

    def combat_telemetry(self):
        if 'gArenaCombatTelemetry' not in self.symbols: return None
        v = struct.unpack('<2I8H4B', self.read('gArenaCombatTelemetry', 28))
        return dict(cancelled=list(v[:2]), last_move=list(v[2:4]), cooldown=list(v[4:6]),
            windup=list(v[6:8]), dash_cooldown=list(v[8:10]), pending_slot=list(v[10:12]),
            aim_blocked=bool(v[12]), aim_visible=bool(v[13]))

    def feedback_telemetry(self):
        if 'gArenaFeedbackTelemetry' not in self.symbols: return None
        v = struct.unpack('<3I2H4B', self.read('gArenaFeedbackTelemetry', 20))
        return dict(impacts=v[0], particles_spawned=v[1], wall_impacts=v[2], hitstop=v[3],
            live_particles=v[4], number_life=list(v[5:7]), attack_buffer=v[7], dash_buffer=v[8])

    def move_telemetry(self):
        if 'gArenaMoveTelemetry' not in self.symbols: return None
        v = struct.unpack('<6I8B2H', self.read('gArenaMoveTelemetry', 36))
        return dict(healed=list(v[:2]), stat_changes=list(v[2:4]), rush_walls=list(v[4:6]),
            kind=list(v[6:8]), phase=list(v[8:10]), range=list(v[10:12]),
            active=list(v[12:14]), action_move=list(v[14:16]))

    def physics_telemetry(self):
        if 'gArenaPhysicsTelemetry' not in self.symbols: return None
        v=struct.unpack('<8I2h2H16B',self.read('gArenaPhysicsTelemetry',56))
        return dict(impacts=v[0],broken=v[1],spawned=v[2],recycled=v[3],bounces=v[4],chains=v[5],
            blast_serial=v[6],steps=v[7],blast=list(v[8:10]),live=v[10],peak=v[11],
            hp=list(v[12:19]),fuse=list(v[19:26]),solid_mask=v[26])

    def frame_telemetry(self):
        if 'gArenaFrameTelemetry' not in self.symbols:return None
        v=struct.unpack('<4I12H',self.read('gArenaFrameTelemetry',40))
        return dict(updates=v[0],missed_vblanks=v[1],max_gap=v[2],last_vblank=v[3],
            scanlines=list(v[4:10]),peak_scanlines=list(v[10:16]))

    def party(self):
        count = min(6, self.read('gPlayerPartyCount', 1)[0])
        raw = self.read('gPlayerParty', 100 * count)
        orders = list(itertools.permutations(range(4)))
        mons = []
        for i in range(count):
            p = raw[i*100:(i+1)*100]
            personality, ot = struct.unpack_from('<II', p)
            key = personality ^ ot
            data = b''.join(struct.pack('<I', v ^ key) for v in struct.unpack('<12I', p[32:80]))
            order = orders[personality % 24]
            growth = data[order.index(0)*12:order.index(0)*12+12]
            attacks = data[order.index(1)*12:order.index(1)*12+12]
            evs = data[order.index(2)*12:order.index(2)*12+6]
            species, item, exp = struct.unpack_from('<HHI', growth)
            checksum = sum(struct.unpack('<24H', data)) & 0xffff
            mons.append(dict(slot=i, species=species, item=item, exp=exp, level=p[84],
                status=int.from_bytes(p[80:84],'little'),
                hp=int.from_bytes(p[86:88], 'little'), max_hp=int.from_bytes(p[88:90], 'little'),
                speed=int.from_bytes(p[94:96], 'little'), moves=list(struct.unpack('<4H', attacks[:8])),
                pp=list(attacks[8:12]), evs=list(evs),
                checksum_ok=checksum == int.from_bytes(p[28:30], 'little')))
        return mons

    def battlers(self):
        # Native layout is documented in include/pokemon.h (BattlePokemon).
        # Symbol addresses still come from this build's ELF, never ROM offsets.
        count = min(4, self.read('gBattlersCount', 1)[0])
        raw = self.read('gBattleMons', count * 88) if count else b''
        mons = []
        for i in range(count):
            p = raw[i*88:(i+1)*88]
            species, attack, defense, speed, sp_attack, sp_defense = struct.unpack_from('<6H', p)
            mons.append(dict(species=species, attack=attack, defense=defense, speed=speed,
                sp_attack=sp_attack, sp_defense=sp_defense, moves=list(struct.unpack_from('<4H',p,12)),
                stat_stages=list(p[24:32]), ability=p[32], types=list(p[33:35]), pp=list(p[36:40]),
                hp=int.from_bytes(p[40:42],'little'), level=p[42], max_hp=int.from_bytes(p[44:46],'little'),
                item=int.from_bytes(p[46:48],'little'), personality=int.from_bytes(p[72:76],'little'),
                status=int.from_bytes(p[76:80],'little'), volatile_status=int.from_bytes(p[80:84],'little')))
        return mons

    def status(self):
        save = self.u32('gSaveBlock1Ptr')
        pos = list(struct.unpack('<hh', self.read(save, 4))) if 0x02000000 <= save < 0x02040000 else None
        return dict(rom_sha256=self.sha, mode=self.mode, stale=digest(self.rom_path) != self.sha,
            cb2=self.cb2(), battle_main=self.names.get(self.u32('gBattleMainFunc') & ~1, '?'),
            tasks=self.task_names(), position=pos, map_index=list(self.read(save + 4, 2)) if pos else None,
            party=self.party(), arena=self.telemetry(), ai=self.ai_telemetry(), sprites=self.sprite_telemetry(),
            combat=self.combat_telemetry(), battlers=self.battlers(), capture=self.capture_telemetry(),
            feedback=self.feedback_telemetry(),
            biome=self.read('gArenaBiome',1)[0] if 'gArenaBiome' in self.symbols else None,
            environment=self.read('gBattleEnvironment',1)[0],
            moves=self.move_telemetry(),
            physics=self.physics_telemetry(), frame_timing=self.frame_telemetry(),
            intro=dict(zip(['started','elapsed','skipped'],struct.unpack('<3I',self.read('gArenaIntroTelemetry',12)))) if 'gArenaIntroTelemetry' in self.symbols else None,
            result_flow=dict(zip(['started','elapsed','ui_fallbacks','texts_skipped','animations_skipped','exp_updates'],
                struct.unpack('<6I',self.read('gArenaResultTelemetry',24)))) if 'gArenaResultTelemetry' in self.symbols else None,
            field_locked=bool(self.read('sLockFieldControls', 1)[0]),
            field_script=self.read('sGlobalScriptContextStatus', 1)[0],
            battle_outcome=self.read('gBattleOutcome', 1)[0])

    def capture_telemetry(self):
        if 'gArenaCaptureTelemetry' not in self.symbols: return None
        v=struct.unpack('<8I4H4h4B',self.read('gArenaCaptureTelemetry',52))
        return dict(zip(['throws','hits','misses','escaped','caught','cancelled','empty','full',
            'balls','age','species','hp','aim_x','aim_y','ball_x','ball_y','state','shakes','destination','ball_sprite'],v))

    def boot(self, save=None, release=False):
        save_path = self.safe_path(save, 'saves', '.sav', existing=True) if save else None
        self.reload('release' if release else 'lab')
        if save:
            self.raw(f'loadsave {save_path}')
        # Traverse the real intro/title/menu with buttons. Stop before NEW GAME.
        for _ in range(100):
            if 'Task_HandleMainMenuInput' in self.task_names():
                break
            self.frame(1, KEYS['A'])
            self.frame(45, 0)
        else:
            raise RuntimeError('Did not reach the main menu in 4600 frames')
        if save:
            self.key('A', 1)  # CONTINUE from the real cartridge save
        else:
            self.key('SELECT', 1)  # Explicit new demo; guarded by SAVE_STATUS_EMPTY
        for _ in range(100):
            self.frame(10)
            if self.cb2() == 'CB2_Overworld' and (self.mode == 'release' or self.u32('gArenaLabMailbox') == 0x414C4142):
                self.frame(80)
                return self.status()
        raise RuntimeError('Demo/CONTINUE did not reach the overworld')

    def mail(self, command):
        if self.mode != 'lab': raise RuntimeError('Release ROM has no test mailbox; use normal game controls')
        addr = self.symbols['gArenaLabMailbox']
        if self.cb2() != 'CB2_Overworld':
            raise RuntimeError('Native lab command requires the free overworld')
        completed = self.u32(addr + 16)
        self.raw(f'write32 {addr+4:x} {command:x}')
        for _ in range(600):
            self.frame(1)
            if self.u32(addr + 16) != completed:
                return self.u32(addr + 12)
        self.raw(f'write32 {addr+4:x} 0')  # Never leave an unacknowledged command armed.
        raise RuntimeError('Overworld command stayed blocked by a script/menu')

    def species_id(self, species):
        if not species.isdigit():
            definitions = (ROOT / 'include/constants/species.h').read_text()
            match = re.search(r'^#define SPECIES_' + re.escape(species.upper()) + r'\s+(\d+)\b', definitions, re.M)
            if not match:
                raise ValueError('Unknown species: ' + species)
            species = match[1]
        return int(species)

    def fixture_params(self, species, level):
        if self.mode != 'lab': raise RuntimeError('Release ROM has no test fixtures')
        species = self.species_id(species)
        if not 1 <= level <= 100:
            raise ValueError('Level must be 1..100')
        addr = self.symbols['gArenaLabMailbox']
        self.raw(f'write16 {addr+8:x} {int(species):x}')
        self.raw(f'write8 {addr+10:x} {level:x}')

    def battle(self, species, level, classic, elemental_duel=False):
        self.fixture_params(species, level)
        addr = self.symbols['gArenaLabMailbox']
        self.raw(f'write8 {addr+11:x} {int(classic):x}')
        if elemental_duel and (classic or species not in ('CHARIZARD','HARIYAMA','BLASTOISE','MEWTWO','SCEPTILE','DRAGONITE','MAGMAR')):
            raise ValueError('Unsupported native elemental/decoy fixture')
        if self.mail(18 if elemental_duel else 1):
            raise RuntimeError('Encounter rejected by native lab')
        for _ in range(130):
            if self.telemetry()['active']:
                if self.cb2() == 'CB2_Arena':
                    self.frame(2)
                    return self.status()
                self.frame(2)
                continue
            if self.cb2() == 'BattleMainCB2' and self.names.get(self.u32('gBattleMainFunc') & ~1) == 'HandleTurnActionSelectionState':
                self.frame(20)
                if self.cb2() == 'BattleMainCB2':
                    return self.status()
                continue
            self.frame(1, 1)
            self.frame(15, 0)
        raise RuntimeError('Battle did not reach the arena/action selection')

    def biome_map(self, biome):
        if biome not in range(5):
            raise ValueError('Unknown biome fixture')
        self.raw(f'write16 {self.symbols["gArenaLabMailbox"]+8:x} {biome:x}')
        if self.mail(15):
            raise RuntimeError('Native map fixture rejected')
        self.frame(180)
        if biome == 1:
            if self.mail(16): raise RuntimeError('Surf fixture rejected')
            self.frame(30)
        return self.status()

    def trainer(self, trainer):
        definitions = (ROOT / 'include/constants/opponents.h').read_text()
        match = re.search(r'^#define TRAINER_' + re.escape(trainer.upper()) + r'\s+(\d+)\b', definitions, re.M)
        if not match:
            raise ValueError('Unknown trainer: ' + trainer)
        addr = self.symbols['gArenaLabMailbox']
        self.raw(f'write16 {addr+8:x} {int(match[1]):x}')
        self.raw(f'write8 {addr+11:x} 0')
        if self.mail(14):
            raise RuntimeError('Trainer rejected by native lab')
        for _ in range(200):
            if self.cb2() == 'CB2_Arena':
                self.frame(2)
                return self.status()
            if self.cb2() == 'BattleMainCB2' and self.names.get(self.u32('gBattleMainFunc') & ~1) == 'HandleTurnActionSelectionState':
                self.frame(20)
                if self.cb2() == 'BattleMainCB2':
                    return self.status()
                continue
            self.frame(3, 1)
            self.frame(15, 0)
        raise RuntimeError('Trainer did not reach arena/action selection')

    def safe_path(self, name, folder, suffix, existing=False):
        if not re.fullmatch(r'[A-Za-z0-9_-]+', name):
            raise ValueError('Use a simple name: letters, digits, - or _')
        base = WORK / folder
        if folder == 'states':
            base /= self.sha
        base.mkdir(parents=True, exist_ok=True)
        path = base / (name + suffix)
        if existing and not path.exists():
            raise ValueError('No matching artifact for this build: ' + str(path))
        return path

    def handle(self, q):
        action = q['action']
        if action not in ('status', 'reload', 'boot', 'stop') and digest(self.rom_path) != self.sha:
            raise RuntimeError('Build changed. Use boot or reload; old RAM is not valid for new code.')
        if action == 'status': return self.status()
        if action == 'view':
            if not 0 <= q['speed'] <= 8: raise ValueError('Speed must be 0 (fast)..8')
            self.live_speed = q['speed']
            self.raw(f'view {self.live_speed}')
            return {'live': True, 'speed': self.live_speed, 'same_core': True}
        if action == 'reload': return self.reload()
        if action == 'boot': return self.boot(q.get('save'), q.get('release', False))
        if action == 'press': self.key(q['keys'], q['frames']); return self.status()
        if action == 'hold': self.key(q['keys'], q['frames'], release=False); return self.status()
        if action == 'wait': self.frame(q['frames']); return self.status()
        if action in ('battle', 'trainer'):
            result = self.battle(q['species'], q['level'], q['classic'], q.get('elemental_duel',False)) if action == 'battle' else self.trainer(q['trainer'])
            # Ordinary tests expect playable input, not a presentation phase.
            # Capture/intro tests opt into the first presentation frame.
            addr = self.symbols.get('gArenaSendoutTelemetry')
            if addr and not q.get('intro', False) and self.cb2() == 'CB2_Arena':
                for _ in range(180):
                    if not int.from_bytes(self.read(addr, 2), 'little'): break
                    self.frame(1, 0)
                if int.from_bytes(self.read(addr, 2), 'little'): raise RuntimeError('Send-out did not finish')
                self.frame(3, 0)  # Publish playable coordinates after presentation ends.
                result = self.status()
            return result
        if action == 'biome-map': return self.biome_map(q['biome'])
        if action == 'heal': return {'native_result': self.mail(3), 'party': self.party()}
        if action == 'fixture-adventure':
            if q.get('confirm') != 'replace-private-copy-party':
                raise ValueError('Explicit private-save preparation confirmation required')
            if self.mail(7): raise RuntimeError('Native adventure preparation rejected')
            self.frame(240)
            return self.status()
        if action == 'fixture-lead':
            self.fixture_params(q['species'], q['level'])
            if self.mail(4): raise RuntimeError('Native lead fixture rejected')
            return self.status()
        if action == 'fixture-music':
            if self.mode != 'lab' or q['song'] not in (359,470):raise ValueError('Private soundtrack fixture only')
            self.fixture_params(str(q['song']),1)
            if self.mail(13):raise RuntimeError('Soundtrack fixture rejected')
            return self.status()
        if action == 'fixture-cut':
            if not 0<=q['slot']<4:raise ValueError('Move slot must be 0..3')
            self.fixture_params('15',q['slot']+1) # MOVE_CUT, not a species mutation.
            if self.mail(12):raise RuntimeError('Native HM compatibility rejected this fixture')
            return self.status()
        if action == 'fixture-elements':
            if self.mode != 'lab': raise RuntimeError('Private fixture only')
            if self.mail(17): raise RuntimeError('Native elemental loadout fixture rejected')
            return self.status()
        if action == 'fixture-balls':
            if self.mode != 'lab' or not 0 <= q['count'] <= 999: raise ValueError('Private fixture only: 0..999 balls')
            addr=self.symbols['gArenaLabMailbox']
            self.raw(f"write16 {addr+8:x} {q['count']:x}")
            if self.mail(8): raise RuntimeError('Native inventory fixture rejected')
            return self.status()
        if action == 'fixture-storage-full':
            if q.get('confirm') != 'replace-disposable-boxes': raise ValueError('Explicit fixture confirmation required')
            if self.mail(9): raise RuntimeError('Full storage fixture rejected')
            return self.status()
        if action == 'audit-capture':
            self.fixture_params(q['species'],1)
            if self.mail(10): raise RuntimeError('Native audit rejected')
            return dict(zip(['balls','stored','party_count','seen','captures','caught','pc_location','level','ball','bad_egg'],
                struct.unpack('<10I',self.read('gArenaLabCaptureAudit',40))))
        if action in ('fixture-team', 'fixture-box'):
            if self.mode != 'lab': raise RuntimeError('Release ROM has no test fixtures')
            if not 1 <= len(q['mons']) <= 6: raise ValueError('Team must have 1..6 Pokemon')
            team = []
            for spec in q['mons']:
                species, level = spec.split(':')
                species, level = self.species_id(species), int(level)
                if not 1 <= species < 412 or not 1 <= level <= 100:
                    raise ValueError('Invalid native species/level')
                team.append((species, level))
            addr = self.symbols['gArenaLabMailbox']
            for i, (species, level) in enumerate(team):
                self.raw(f'write16 {addr+20+i*2:x} {species:x}')
                self.raw(f'write8 {addr+32+i:x} {level:x}')
            self.raw(f'write8 {addr+38:x} {len(team):x}')
            result = self.mail(5 if action == 'fixture-team' else 6)
            if result: raise RuntimeError('Native fixture rejected (box slots must be empty)' if result == 3 else 'Native team fixture rejected')
            return self.status()
        if action == 'inspect':
            if not 1 <= q['size'] <= 4096: raise ValueError('Size must be 1..4096')
            addr = self.symbols[q['symbol']]
            if q.get('deref'): addr = self.u32(addr)
            addr += q.get('offset', 0)
            if not 0x02000000 <= addr < 0x0e000000: raise ValueError('Not readable game memory')
            return {'symbol': q['symbol'], 'address': hex(addr), 'hex': self.read(addr, q['size']).hex()}
        if action in ('snapshot', 'restore'):
            path = self.safe_path(q['name'], 'states', '.state', action == 'restore')
            self.raw(('state ' if action == 'snapshot' else 'loadstate ') + str(path))
            if action == 'snapshot':
                path.with_suffix('.json').write_text(json.dumps(self.status(), indent=2) + '\n')
            return {'path': str(path), 'status': self.status()}
        if action in ('save', 'export-save'):
            if action == 'save':
                result = self.mail(2)
                if result != 1: raise RuntimeError('Native save failed: ' + str(result))
            path = self.safe_path(q['name'], 'saves', '.sav')
            self.raw('save ' + str(path))
            return {'path': str(path), 'sha256': digest(path), 'party': self.party()}
        if action == 'screenshot':
            path = self.safe_path(q['name'], 'screenshots', '.png')
            self.raw('screenshot ' + str(path))
            path.with_suffix('.json').write_text(json.dumps(self.status(), indent=2) + '\n')
            return {'path': str(path), 'rom_sha256': self.sha}
        if action == 'play':
            # A separate sandbox: the visual emulator cannot overwrite lab/user saves.
            folder = WORK / 'play' / (self.sha[:12] + '-' + str(time.time_ns()))
            folder.mkdir(parents=True)
            (folder / 'config').mkdir()
            shutil.copy2(self.release / 'game.gba', folder / 'game.gba')
            self.raw('state ' + str(folder / 'game.state'))
            self.raw('save ' + str(folder / 'game.sav'))
            bundle = WORK / 'mGBA Arena.app' / 'Contents'
            (bundle / 'MacOS').mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / 'tools/arena/mGBA-Arena-Info.plist', bundle / 'Info.plist')
            executable = bundle / 'MacOS/mgba'
            if not executable.exists() or digest(executable) != digest(LOCAL / 'mgba/build/sdl/mgba'):
                shutil.copy2(LOCAL / 'mgba/build/sdl/mgba', executable)
            with (folder / 'emulator.log').open('w') as log:
                p = subprocess.Popen([str(executable), '-4',
                    '-C', 'logLevel=7', '-C', 'rewindEnable=0', '-C', 'useBios=0', '-C', 'skipBios=1',
                    '-t', str(folder / 'game.state'), str(folder / 'game.gba')],
                    stdout=log, stderr=log, start_new_session=True,
                    env=dict(os.environ, XDG_CONFIG_HOME=str(folder / 'config')))
            return {'pid': p.pid, 'folder': str(folder), 'rom_sha256': self.sha}
        raise ValueError('Unknown action ' + action)


class Handler(socketserver.StreamRequestHandler):
    def handle(self):
        began = time.monotonic()
        line = self.rfile.readline(65536)
        if not line:
            return
        try:
            q = json.loads(line)
            if q['action'] == 'stop':
                result = {'stopped': True}
                self.server.stop_requested = True
            else:
                result = self.server.lab.handle(q)
            response = {'ok': True, 'elapsed_s': round(time.monotonic() - began, 3), 'result': result}
        except Exception as e:
            response = {'ok': False, 'error': str(e)}
        with (WORK / 'commands.jsonl').open('a') as history:
            history.write(json.dumps({'time': time.time(), 'request': line.decode().strip(),
                                      'response': response}) + '\n')
        self.wfile.write((json.dumps(response) + '\n').encode())


def serve():
    WORK.mkdir(mode=0o700, parents=True, exist_ok=True)
    if SOCKET.exists():
        try:
            with socket.socket(socket.AF_UNIX) as s: s.connect(str(SOCKET))
        except ConnectionRefusedError: SOCKET.unlink()
        else: raise RuntimeError('Lab already running')
    with socketserver.UnixStreamServer(str(SOCKET), Handler) as server:
        os.chmod(SOCKET, 0o600)
        server.lab = Lab()
        server.stop_requested = False
        while not server.stop_requested: server.handle_request()
        server.lab.raw('quit', reply=False)
        server.lab.runner.wait(timeout=5)
    SOCKET.unlink(missing_ok=True)


def call(q, autostart=True):
    WORK.mkdir(mode=0o700, parents=True, exist_ok=True)
    for attempt in range(2):
        try:
            with socket.socket(socket.AF_UNIX) as s:
                s.settimeout(360)
                s.connect(str(SOCKET))
                s.sendall((json.dumps(q) + '\n').encode())
                return json.loads(s.makefile('rb').readline())
        except (FileNotFoundError, ConnectionRefusedError):
            if not autostart or attempt: raise
            with (WORK / 'server.log').open('a') as log:
                subprocess.Popen([sys.executable, str(Path(__file__).resolve()), 'serve'],
                    stdout=log, stderr=log, start_new_session=True)
            for _ in range(100):
                time.sleep(.05)
                try:
                    with socket.socket(socket.AF_UNIX) as probe: probe.connect(str(SOCKET))
                    break
                except (FileNotFoundError, ConnectionRefusedError): pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='action', required=True)
    for name in ('serve', 'status', 'reload', 'stop', 'heal', 'play'): sub.add_parser(name)
    p = sub.add_parser('boot'); p.add_argument('--save')
    p.add_argument('--release', action='store_true')
    p = sub.add_parser('view'); p.add_argument('--speed', type=int, default=1)
    for name in ('press', 'hold'):
        p = sub.add_parser(name); p.add_argument('keys'); p.add_argument('frames', type=int, nargs='?', default=1)
    p = sub.add_parser('wait'); p.add_argument('frames', type=int)
    p = sub.add_parser('inspect'); p.add_argument('symbol'); p.add_argument('size', type=int, nargs='?', default=4)
    p.add_argument('--offset', type=int, default=0); p.add_argument('--deref', action='store_true')
    p = sub.add_parser('battle'); p.add_argument('--species', default='POOCHYENA'); p.add_argument('--level', type=int, default=5); p.add_argument('--classic', action='store_true')
    p = sub.add_parser('fixture-balls'); p.add_argument('--count',type=int,required=True)
    p = sub.add_parser('fixture-cut');p.add_argument('--slot',type=int,default=0)
    p = sub.add_parser('fixture-lead', help='REPLACES slot 0 in the disposable lab, using native CreateMon')
    p.add_argument('--species', default='TREECKO'); p.add_argument('--level', type=int, required=True)
    p = sub.add_parser('fixture-team', help='REPLACES disposable team using native CreateMon, never release')
    p.add_argument('--mons', nargs='+', required=True, help='SPECIES:LEVEL, at most six')
    p = sub.add_parser('fixture-box', help='Fill empty Box 1 slots with native fixture Pokemon, never release')
    p.add_argument('--mons', nargs='+', required=True, help='SPECIES:LEVEL, at most six')
    for name in ('snapshot', 'restore', 'save', 'export-save', 'screenshot'):
        p = sub.add_parser(name); p.add_argument('name')
    args = vars(parser.parse_args())
    if args['action'] == 'serve': serve(); return
    response = call(args)
    print(json.dumps(response, indent=2))
    if not response['ok']: sys.exit(1)


if __name__ == '__main__': main()
