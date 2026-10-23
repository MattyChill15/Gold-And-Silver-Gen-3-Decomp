#!/usr/bin/env python3
"""Headless Arena series playtest (Emerald chapter 3): demo new game, part
transitions P1->P2->P3 and save rejection. Run after building ept_p1..3
(ARENA_PLAYTEST=1). Uses only ROMs built from your own baserom."""
import argparse, json, re, shutil, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))
from ptlib import Emu
ROOT = Path(__file__).resolve().parents[2]
def consts(*files):
    d = {}
    for f in files:
        for m in re.finditer(r'^#define\s+(\w+)\s+(.+?)\s*(?://.*)?$', (ROOT / f).read_text(), re.M): d.setdefault(m.group(1), m.group(2))
    def get(k):
        return int(eval(re.sub(r'\b([A-Za-z_]\w*)\b', lambda m: str(get(m.group(1))), d[k])))
    return get
C = consts('include/constants/flags.h', 'include/constants/vars.h', 'include/constants/global.h', 'include/constants/opponents.h')
results = []
def check(name, ok, detail):
    s = 'PASS' if ok else 'FAIL'; results.append(dict(check=name, status=s, detail=detail)); print(f'{s:5} {name}: {detail}', flush=True)
def boot_demo(e):  # title -> main menu (empty save) -> Select = demo new game
    e.run(1500); e.run(10, 8); e.run(300); e.run(10, 8); e.run(400); e.run(10, 1); e.run(600); e.run(10, 4); e.run(1500)
def boot_continue(e):
    e.run(1500); e.run(10, 8); e.run(300); e.run(10, 8); e.run(400); e.run(10, 1); e.run(600); e.run(10, 1); e.run(1200)
    for _ in range(3): e.run(10, 2); e.run(100)
def emu(a, n, sav): return Emu(a.drv, str(a.d / f'ept_p{n}.gba'), str(a.d / f'ept_p{n}.elf'), str(sav))
def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--drv', required=True); ap.add_argument('--out', default='/tmp/ea_pt')
    ap.add_argument('--fr-sav', default='/workspace/fr/ws/.arena-dev/playtest/p1_complete.sav'); a = ap.parse_args()
    a.d = ROOT; out = Path(a.out); shutil.rmtree(out, ignore_errors=True); out.mkdir(parents=True)
    # P1 demo new game
    s1 = out / 'p1.sav'; e = emu(a, 1, s1); boot_demo(e)
    loc = e.location(); inf = e.in_field()
    check('P1 demo new game reaches field', inf, f'map {loc[2]}.{loc[3]} ({loc[0]},{loc[1]})')
    st = e.cmd(20, C('VAR_ARENA_GAME_STAMP'))
    e.cmd(5, frames=2400); e.close(); shutil.copy(s1, out / 'p1_incomplete.sav')
    check('P1 stamps Emerald identity', st[1] == 0x4541, f'stamp=0x{st[1]:04x}')
    e = emu(a, 1, s1); boot_continue(e)
    check('P1 Continue of own save', e.in_field() and e.r8(e.s('gArenaSeriesRejectedSave')) == 0, f'map {e.location()[2:]}')
    e.cmd(6, C('FLAG_MATT_PART1_COMPLETE'), 1); sv = e.cmd(5, frames=2400); e.close()
    shutil.copy(s1, out / 'p1_complete.sav')
    # P1 -> P2
    s2 = out / 'p2.sav'; shutil.copy(s1, s2); e = emu(a, 2, s2); boot_continue(e)
    rej = e.r8(e.s('gArenaSeriesRejectedSave')); loc = e.location(); inf = e.in_field()
    i2 = e.cmd(22, C('FLAG_MATT_PART2_INIT')) if inf else [0]*4
    check('P1 -> P2 carry-over (Fortree warp + one-time init)', not rej and inf and (loc[2], loc[3]) in ((0, 4), (12, 2)) and i2[0] == 1 and i2[2] == 2,
          f'rejected={rej} map {loc[2]}.{loc[3]} PART2_INIT={i2[0]} part={i2[2]}')
    e.cmd(14, 0, 10, 10 | (10 << 8)); e.run(300); e.cmd(5, frames=2400); e.close()
    e = emu(a, 2, s2); boot_continue(e); loc = e.location()
    check('P2 init runs once (reload stays where saved)', e.in_field() and (loc[2], loc[3]) == (0, 10), f'map {loc[2]}.{loc[3]}')
    e.cmd(6, C('FLAG_MATT_PART2_COMPLETE'), 1); e.cmd(5, frames=2400); e.close()
    # P2 -> P3
    s3 = out / 'p3.sav'; shutil.copy(s2, s3); e = emu(a, 3, s3); boot_continue(e)
    rej = e.r8(e.s('gArenaSeriesRejectedSave')); loc = e.location(); inf = e.in_field()
    i3 = e.cmd(22, C('FLAG_MATT_PART3_INIT')) if inf else [0]*4
    b7 = e.cmd(22, C('FLAG_BADGE07_GET'))[0] if inf else 0
    check('P2 -> P3 carry-over (Mossdeep warp + one-time init)', not rej and inf and (loc[2], loc[3]) in ((0, 6), (14, 3)) and i3[0] == 1 and i3[2] == 3 and b7 == 1,
          f'rejected={rej} map {loc[2]}.{loc[3]} PART3_INIT={i3[0]} part={i3[2]} badge7={b7}')
    e.close()
    # rejections
    def rej_case(name, n, src):
        t = out / f'rej_p{n}_{Path(src).name}'; shutil.copy(src, t)
        e = emu(a, n, t); e.run(1500); r = e.r8(e.s('gArenaSeriesRejectedSave')); st = e.r16(e.s('gSaveFileStatus')); e.close()
        check(name, r == 1, f'gArenaSeriesRejectedSave={r} gSaveFileStatus={st}')
    rej_case('P2 rejects incomplete P1 save', 2, out / 'p1_incomplete.sav')
    if Path(a.fr_sav).exists(): rej_case('P2 rejects FireRed save', 2, a.fr_sav)
    else: check('P2 rejects FireRed save', False, 'FR save missing')
    rej_case('P3 rejects P1-only save', 3, out / 'p1_complete.sav')
    rej_case('P3 rejects FireRed save', 3, a.fr_sav)
    (out / 'playtest-results.json').write_text(json.dumps(results, indent=1))
    print(f'\n{sum(r["status"] == "PASS" for r in results)}/{len(results)} PASS')
if __name__ == '__main__': main()
