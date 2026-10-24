#!/usr/bin/env python3
"""Checklist v2 items 4 (carry-over detail) and 9 (save/load/reset/cold boot) on jpt_p1..3.
Shortcut: saves come from playtest_johto.py chain (badge/GAME_CLEAR/Red flags forced there)."""
import sys, json, shutil, types
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))
import playtest_johto as PJ
PJ.A = types.SimpleNamespace(drv='/workspace/fr/smoke/playtest_drv')
C = PJ.C; SRC = Path(sys.argv[1] if len(sys.argv) > 1 else '/tmp/johto_v2'); OUT = Path('/tmp/johto_v2_items'); OUT.mkdir(exist_ok=True)
res = []
def check(n, ok, d): res.append(dict(check=n, status='PASS' if ok else 'FAIL', detail=d)); print('PASS' if ok else 'FAIL', n, d, flush=True)
FL = ['FLAG_BADGE01_GET', 'FLAG_BADGE04_GET', 'FLAG_MATT_PART1_COMPLETE', 'FLAG_SYS_GAME_CLEAR', 'FLAG_MATT_PART2_COMPLETE']
def snap(e):
    i = e.cmd(23); pp = e.s('gParties')
    party = [(e.r16(pp + 100*k + 0) , e.r8(pp + 100*k + 84)) for k in range(i[0])]  # (personality lo, level)
    return dict(party=i[0], pc=i[1], species=(i[2] & 0xffff, i[2] >> 16), story=i[3] & 0xff, levels=[p[1] for p in party], pers=[p[0] for p in party],
                flags={f: e.cmd(22, C(f))[0] for f in FL})
snaps = {}
for lab in ('classic', 'chief', 'show'):
    for n, sv in ((1, f'{lab}_p1_complete.sav'), (2, f'{lab}_p1_complete.sav'), (3, f'{lab}_p2_complete.sav')):
        t = OUT / f'carry_{lab}_p{n}.sav'; shutil.copy(SRC / sv, t)
        e = PJ.jemu(n, t); inf = PJ.boot_continue(e); snaps[(lab, n)] = snap(e) if inf else None; e.shot(str(OUT / f'carry_{lab}_p{n}.png')); e.close()
    a, b, c = snaps[(lab, 1)], snaps[(lab, 2)], snaps[(lab, 3)]
    keep = lambda x, y: x and y and all(x[k] == y[k] for k in ('party', 'pc', 'species', 'story', 'levels', 'pers'))
    check(f'[{lab}] item4 P1->P2 party/PC/storyline identical', keep(a, b), f'P1={a} P2={b}')
    check(f'[{lab}] item4 P2->P3 party/PC/storyline identical', keep(b, c), f'P3={c}')
    # every flag set in an earlier part keeps its value (BADGE01 is 0 in these saves: the chain forces BADGE04 only)
    carried = lambda x, y: x and y and all(y['flags'][f] >= x['flags'][f] for f in FL)
    check(f'[{lab}] item4 flags set earlier stay set (P1->P2->P3) and P3 has PART1+PART2+GAME_CLEAR',
          carried(a, b) and carried(b, c) and all(c['flags'][f] for f in ('FLAG_BADGE04_GET', 'FLAG_MATT_PART1_COMPLETE', 'FLAG_SYS_GAME_CLEAR', 'FLAG_MATT_PART2_COMPLETE')),
          f"P1={a and a['flags']} P2={b and b['flags']} P3={c and c['flags']}")
# item 9: per part cold boot -> save -> soft reset -> continue -> location/party same; then cold boot again
for n, sv in ((1, 'classic_p1_complete.sav'), (2, 'classic_p1_complete.sav'), (3, 'classic_p2_complete.sav')):
    t = OUT / f'stab_p{n}.sav'; shutil.copy(SRC / sv, t)
    e = PJ.jemu(n, t); inf1 = PJ.boot_continue(e); loc1 = e.location()[:4]; s1 = snap(e) if inf1 else None
    e.run(16, 16); e.run(30); loc_pre = e.location()[:4]; PJ.save(e)
    e.run(10, 15); e.run(30, 15); e.run(200)  # soft reset A+B+Select+Start
    inf2 = PJ.boot_continue(e); loc2 = e.location()[:4]; s2 = snap(e) if inf2 else None; e.shot(str(OUT / f'stab_p{n}_after_reset.png'))
    for _ in range(30): e.run(20, 1); e.run(20)  # idle/A mash 1200 frames: no crash
    alive = e.in_field(); e.close()
    e = PJ.jemu(n, t); inf3 = PJ.boot_continue(e); loc3 = e.location()[:4]; e.shot(str(OUT / f'stab_p{n}_coldboot.png')); e.close()
    check(f'item9 P{n} cold boot/save/soft reset/load/cold boot', inf1 and inf2 and inf3 and alive and loc2 == loc_pre and loc3 == loc_pre and s1 and s2 and s1['party'] == s2['party'],
          f'boot={inf1} loc={loc1} moved/saved={loc_pre} afterReset={inf2} {loc2} alive={alive} coldboot={inf3} {loc3}')
json.dump(dict(results=res, shortcuts=['input saves produced by playtest_johto.py chain (part-complete flags forced)']), open(OUT / 'v2_items_4_9.json', 'w'), indent=1)
print(sum(r['status'] == 'PASS' for r in res), '/', len(res))
