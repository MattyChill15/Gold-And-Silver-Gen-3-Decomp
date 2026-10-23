"""Tiny host side of the headless playtest mailbox (see playtest.py)."""
import subprocess, re

class Emu:
    def __init__(self, drv, rom, elf, sav):
        self.p = subprocess.Popen([drv, rom, sav], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
        out = subprocess.run(['arm-none-eabi-nm', elf], capture_output=True, text=True).stdout
        self.sym = {}
        for l in out.splitlines():
            parts = l.split()
            if len(parts) == 3:
                self.sym.setdefault(parts[2], int(parts[0], 16))
        self.box = self.sym.get('gArenaPlaytest')
    def _c(self, line):
        self.p.stdin.write(line + '\n'); self.p.stdin.flush()
        return self.p.stdout.readline().strip()
    def run(self, n, keys=0): self._c(f'run {n} {keys}')
    def shot(self, f): self._c(f'shot {f}')
    def r32(self, a): return int(self._c(f'r32 {a}'))
    def r16(self, a): return int(self._c(f'r16 {a}'))
    def r8(self, a): return int(self._c(f'r8 {a}'))
    def w8(self, a, v): self._c(f'w8 {a} {v}')
    def w16(self, a, v): self._c(f'w16 {a} {v}')
    def s(self, name, off=0): return self.sym[name] + off
    def cb2(self): return self.r32(self.s('gMain', 4))
    def in_field(self): return self.cb2() == self.sym['CB2_Overworld'] | 1 or self.cb2() == self.sym['CB2_Overworld']
    def cmd(self, c, a=0, b=0, cc=0, frames=600):
        bx = self.box
        self.w8(bx + 1, 0); self.w16(bx + 2, a); self.w16(bx + 4, b); self.w16(bx + 6, cc); self.w8(bx, c)
        for _ in range(frames // 2):
            self.run(2)
            if self.r8(bx + 1) == c:
                return [self.r32(bx + 8 + 4 * i) for i in range(4)]
        self.shot('/tmp/pt_fail.png')
        raise RuntimeError(f'mailbox cmd {c} not executed (not in overworld?)')
    def location(self):
        sb1 = self.r32(self.s('gSaveBlock1Ptr'))
        return self.r16(sb1), self.r16(sb1 + 2), self.r8(sb1 + 4), self.r8(sb1 + 5)
    def battle(self, maxf=60000, tap_every=90):
        """Run until the field is back. Taps A to advance prompts/text."""
        f = 0
        while self.in_field() and f < 900:
            self.run(10); f += 10
        started = not self.in_field()
        peaks = {}
        pm = self.sym.get('CB2_UpdatePartyMenu')
        while f < maxf:
            if pm and (self.cb2() | 1) == (pm | 1):
                # forced switch after a faint: move to the next mon and send it out
                if self.r16(self.s('gBattleMons') + 0x28) == 0:
                    peaks['faintMenus'] = peaks.get('faintMenus', 0) + 1
                    self.run(6, 128); self.run(10); self.run(4, 1); self.run(20); self.run(4, 1); self.run(30)
                else:  # optional "switch?" menu: keep the current mon
                    self.run(4, 2); self.run(20); self.run(4, 2); self.run(40)
            for _ in range(max(1, (tap_every - 4) // 4)):
                self.run(4)
                for k in ('sSeShake', 'sMattSwitch', 'gArenaItemsUsed'):
                    if k in self.sym:
                        peaks[k] = max(peaks.get(k, 0), self.r8(self.s(k)))
                if 'gArenaMoveTelemetry' in self.sym:
                    peaks['healed'] = max(peaks.get('healed', 0), self.r32(self.s('gArenaMoveTelemetry')))
            self.run(4, 1); f += tap_every
            if 'gBattlerPartyIndexes' in self.sym and not self.in_field():
                peaks['switchedTo'] = max(peaks.get('switchedTo', 0), self.r16(self.s('gBattlerPartyIndexes')))
            if self.in_field():
                self.run(60)
                if self.in_field():
                    break
        self.last = (started, self.in_field(), f, peaks)
        return self.last
    def close(self):
        try: self._c('quit')
        except Exception: pass
        self.p.wait(timeout=30)
