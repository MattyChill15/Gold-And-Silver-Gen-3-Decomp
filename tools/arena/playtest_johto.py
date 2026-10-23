#!/usr/bin/env python3
"""Headless Arena series playtest (Johto chapter 2, 3 parts): FR P2 -> Johto P1
import, P1 -> P2 -> P3 carry-over, P3 -> Emerald P1 import, save rejections and
storyline (Classic/Chief/Show) carry. Build jpt_p1..3 (ARENA_PLAYTEST=1) first.
Uses only ROMs built from your own baserom."""
import argparse, json, re, shutil, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))
from ptlib import Emu
ROOT = Path(__file__).resolve().parents[2]
EA = Path('/workspace/ea/ws')
import subprocess
def consts(root, *files):
    cache = {}
    def get(k):
        if k not in cache:
            src = '#include "global.h"\n' + ''.join(f'#include "{f[8:]}"\n' for f in files) + f'ARENA_PT_VALUE {k}\n'
            o = subprocess.run(['cpp', '-P', '-I', str(root / 'include'), '-I', str(root / 'gflib'), '-I', str(root)], input=src, capture_output=True, text=True, cwd=root).stdout
            v = o.rsplit('ARENA_PT_VALUE', 1)[1].strip()
            cache[k] = int(eval(v.replace('u', '')))
        return cache[k]
    return get
C = consts(ROOT, 'include/constants/flags.h', 'include/constants/vars.h')
CE = consts(EA, 'include/constants/flags.h')
CEV = consts(EA, 'include/constants/vars.h')
results = []
def check(name, ok, detail):
    s = 'PASS' if ok else 'FAIL'; results.append(dict(check=name, status=s, detail=detail)); print(f'{s:5} {name}: {detail}', flush=True)
def boot_new(e):  # title -> main menu -> New Game -> mash through intro/game-mode/naming
    e.run(1500); e.run(10, 8); e.run(300); e.run(10, 8); e.run(400); e.run(10, 1); e.run(600)
    seq = [1, 256, 1, 1, 8, 1]
    for i in range(900):
        e.run(5, seq[i % len(seq)]); e.run(25)
        if e.in_field(): return True
    return False
def boot_continue(e):
    e.run(1500); e.run(10, 8); e.run(300); e.run(10, 8); e.run(400); e.run(10, 1); e.run(1200)
    for _ in range(3): e.run(10, 2); e.run(100)
    return e.in_field()
def ea_boot_demo(e):
    e.run(1500); e.run(10, 8); e.run(300); e.run(10, 8); e.run(400); e.run(10, 1); e.run(600); e.run(10, 4); e.run(1500)
def jemu(n, sav): return Emu(A.drv, str(ROOT / f'jpt_p{n}.gba'), str(ROOT / f'jpt_p{n}.elf'), str(sav))
def save(e): e.cmd(5, frames=2400)
def settle(e): e.run(120)  # FieldTick syncs part flags
def rec(e):
    r = e.cmd(9); return dict(ok=r[0], src=r[1] & 0xff, segs=(r[1] >> 8) & 0xff, party=(r[1] >> 16) & 0xff)
def chain(out, story, fr_sav, label):
    """FR P2 -> J P1 (story set) -> P2 -> P3 -> Red; returns final save path."""
    s = out / f'{label}.sav'; shutil.copy(fr_sav, s)
    e = jemu(1, s); pend = None
    e.run(1500); pend = e.r8(e.s('gArenaSeriesImportPending')); e.close()
    e = jemu(1, s); inf = boot_new(e)
    i = e.cmd(23) if inf else [0]*4
    sp = (i[2] & 0xffff, i[2] >> 16)
    check(f'[{label}] FR P2 -> Johto P1 import', pend == 1 and inf and i[0] >= 1 and (i[3] >> 8) == 1,
          f'importPending={pend} field={inf} party={i[0]} pcMons={i[1]} slot0/1 species={sp} imported={i[3]>>8}')
    e.cmd(24, story); e.cmd(6, C('FLAG_BADGE04_GET'), 1); settle(e)
    p1c = e.cmd(22, C('FLAG_MATT_PART1_COMPLETE'))[0]; save(e); e.close()
    check(f'[{label}] Morty badge -> PART1_COMPLETE synced and saved', p1c == 1, f'PART1_COMPLETE={p1c} storyline set={story}')
    shutil.copy(s, out / f'{label}_p1_complete.sav')
    e = jemu(2, s); inf = boot_continue(e); rej = e.r8(e.s('gArenaSeriesRejectedSave'))
    i = e.cmd(23) if inf else [0]*4; part = e.cmd(22, 0)[2] if inf else 0
    check(f'[{label}] Johto P1 -> P2 carry-over', inf and not rej and part == 2 and (i[3] & 0xff) == story,
          f'rejected={rej} field={inf} part={part} party={i[0]} storyline={i[3]&0xff}')
    e.cmd(6, C('FLAG_SYS_GAME_CLEAR'), 1); settle(e)
    p2c = e.cmd(22, C('FLAG_MATT_PART2_COMPLETE'))[0]; save(e); e.close()
    check(f'[{label}] Lance (GAME_CLEAR) -> PART2_COMPLETE synced and saved', p2c == 1, f'PART2_COMPLETE={p2c}')
    shutil.copy(s, out / f'{label}_p2_complete.sav')
    e = jemu(3, s); inf = boot_continue(e); rej = e.r8(e.s('gArenaSeriesRejectedSave'))
    i = e.cmd(23) if inf else [0]*4; part = e.cmd(22, 0)[2] if inf else 0
    check(f'[{label}] Johto P2 -> P3 carry-over', inf and not rej and part == 3 and (i[3] & 0xff) == story,
          f'rejected={rej} field={inf} part={part} party={i[0]} storyline={i[3]&0xff}')
    e.cmd(6, C('FLAG_DEFEATED_RED'), 1); settle(e); save(e)
    g = e.cmd(22, 0); r = rec(e); e.close()
    sf = g[1] & 0xff
    check(f'[{label}] Red beaten -> record completes as SEG_JOHTO', r['ok'] and r['src'] == 2 and (r['segs'] & 2) and ((sf >> 2) & 3) == story,
          f"record src={r['src']} segsComplete=0x{r['segs']:02x} party={r['party']} seriesFlags=0x{sf:02x} (story bits={(sf>>2)&3})")
    return s
def ea_import(out, jsav, story, label):
    t = out / f'{label}_ea.sav'; shutil.copy(jsav, t)
    e = Emu(A.drv, str(EA / 'ept_p1.gba'), str(EA / 'ept_p1.elf'), str(t))
    e.run(1500); pend = e.r8(e.s('gArenaSeriesImportPending')); e.close()
    e = Emu(A.drv, str(EA / 'ept_p1.gba'), str(EA / 'ept_p1.elf'), str(t)); inf = boot_new(e); pc = e.r8(e.s('gPlayerPartyCount'))
    imp = e.cmd(22, CE('FLAG_ARENA_SERIES_IMPORTED'))[0] if inf else 0; e.shot(str(out / f'{label}_ea.png'))
    est = e.cmd(20, CEV('VAR_ARENA_STORYLINE'))[0] if inf else -1
    save(e); sf = e.cmd(22, 0)[1] & 0xff; e.close()
    check(f'[{label}] Johto P3 -> Emerald P1 import', pend == 1 and inf and pc == 2 and imp == 1,
          f'importPending={pend} field={inf} party={pc} imported={imp}')
    check(f'[{label}] Emerald P1 keeps storyline and re-saves it', est == story and ((sf >> 2) & 3) == story,
          f'VAR_ARENA_STORYLINE={est} record story bits after Emerald save={(sf>>2)&3} expected={story}')
MAPS = None
def mapid(name):
    global MAPS
    if MAPS is None:
        MAPS = {}; d = json.load(open(ROOT / 'data/maps/map_groups.json'))
        for gi, g in enumerate(d['group_order']):
            for ni, m in enumerate(d[g]): MAPS[json.load(open(ROOT / f'data/maps/{m}/map.json'))['id']] = (gi, ni)
    return MAPS[name]
def gate(n, sav, target, expect_blocked, out):
    # D3: ask the part's gate (cmd 25, ArenaJohto_IsMapBlocked) and warp if open. Function-level, not input-driven.
    t = out / f'gate_p{n}_{target}.sav'; shutil.copy(sav, t)
    e = jemu(n, t); inf = boot_continue(e); before = e.location()[2:4] if inf else None
    g, m = mapid(target); bl = e.cmd(25, g, m, 1)[0]; e.run(600)
    after = e.location()[2:4]; e.shot(str(out / f'gate_p{n}_{target}.png')); e.close()
    blocked = inf and bl == 1 and after == before and after != (g, m)
    ok = inf and (blocked if expect_blocked else (bl == 0 and after == (g, m)))
    check(f'D3 P{n} {"blocks" if expect_blocked else "allows"} {target}', ok, f'gate={bl} field={inf} before={before} after={after} target={(g, m)}')
def main():
    global A
    ap = argparse.ArgumentParser(); ap.add_argument('--drv', default='/workspace/fr/smoke/playtest_drv'); ap.add_argument('--out', default='/tmp/johto_pt')
    ap.add_argument('--fr-sav', default=str(ROOT / '.arena-dev/pt/fr_p2_complete.sav')); A = ap.parse_args()
    out = Path(A.out); shutil.rmtree(out, ignore_errors=True); out.mkdir(parents=True)
    names = {0: 'classic', 1: 'chief', 2: 'show'}
    finals = {}
    for st in (0, 1, 2):
        finals[st] = chain(out, st, A.fr_sav, names[st])
        ea_import(out, finals[st], st, names[st])
    gate(1, out / 'classic_p1_complete.sav', 'MAP_CIANWOOD_CITY', True, out)
    gate(1, out / 'classic_p1_complete.sav', 'MAP_PALLET_TOWN', True, out)
    gate(1, out / 'classic_p1_complete.sav', 'MAP_VIOLET_CITY', False, out)
    gate(2, out / 'classic_p2_complete.sav', 'MAP_PALLET_TOWN', True, out)
    gate(2, out / 'classic_p2_complete.sav', 'MAP_CIANWOOD_CITY', False, out)
    gate(3, finals[0], 'MAP_PALLET_TOWN', False, out)
    def rej_case(name, n, src):
        t = out / f'rej_p{n}_{Path(src).name}'; shutil.copy(src, t)
        e = jemu(n, t); e.run(1500); r = e.r8(e.s('gArenaSeriesRejectedSave')); e.close()
        check(name, r == 1, f'gArenaSeriesRejectedSave={r}')
    # incomplete P1: import + save without badge
    s = out / 'p1_incomplete.sav'; shutil.copy(A.fr_sav, s); e = jemu(1, s); boot_new(e); save(e); e.close()
    rej_case('P2 rejects incomplete Johto P1 save', 2, s)
    rej_case('P2 rejects FireRed save', 2, A.fr_sav)
    rej_case('P3 rejects P1-only save', 3, out / 'classic_p1_complete.sav')
    rej_case('P3 rejects FireRed save', 3, A.fr_sav)
    (out / 'playtest-results.json').write_text(json.dumps(results, indent=1))
    print(f'\n{sum(r["status"] == "PASS" for r in results)}/{len(results)} PASS')
if __name__ == '__main__': main()
