#!/usr/bin/env python3
"""D5 storyline play-through WITH WARP SHORTCUTS (not a real walk-through).
For each storyline and part: load a part save, set the storyline, warp to each
scene map in table order (cmd 14), mash A / fight until the scene flag is set.
Prereq flags from earlier scenes are produced by the scenes themselves; only
badges, frontier symbols and night are forced and logged as shortcuts."""
import json, re, shutil, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))
import playtest_johto as PJ
from ptlib import Emu
ROOT = PJ.ROOT; C = PJ.C; CM = lambda k: int(re.search(rf'\b{k} = (\d+)', open(ROOT / 'include/constants/moves.h').read()).group(1))
import types
DRV = sys.argv[1] if len(sys.argv) > 1 else '/workspace/fr/smoke/playtest_drv'
PJ.A = types.SimpleNamespace(drv=DRV)
SAVES = Path(sys.argv[2] if len(sys.argv) > 2 else '/tmp/johto_pt'); OUT = Path('/tmp/johto_story'); OUT.mkdir(exist_ok=True)
src = open(ROOT / 'src/data/arena_story_scenes.h').read()
TABLE = {}
for part, blk in re.findall(r'#if MATT_PART == (\d)(.*?)#endif', src, re.S):
    TABLE[int(part)] = re.findall(r'\{MAP_(\w+), ARENA_STORYLINE_(\w+), (\w+), (\w+), (\w+), (\d), (\w+)\}', blk)
STORY = {'CHIEF': 1, 'SHOW': 2}
def mapdir():
    d = json.load(open(ROOT / 'data/maps/map_groups.json')); r = {}
    for g in d['group_order']:
        for m in d[g]:
            j = json.load(open(ROOT / f'data/maps/{m}/map.json')); r[j['id'][4:]] = j
    return r
MJ = mapdir()
CUSTOM = {'ARENA_CHARICIFIC_VALLEY': (10, 38), 'ARENA_WHIRL_CUP': (9, 5), 'ARENA_SILVER_CONFERENCE': (9, 5)}
results = []; shortcuts = []
def check(n, ok, d):
    s = 'PASS' if ok else 'FAIL'; results.append(dict(check=n, status=s, detail=d)); print(f'{s:5} {n}: {d}', flush=True)
def xy(m):
    if m in CUSTOM: return CUSTOM[m]
    w = MJ[m].get('warp_events') or []
    return (w[0]['x'], w[0]['y'] + 1) if w else (5, 5)
def flag(e, f):
    try: return e.cmd(22, C(f), frames=200)[0]
    except RuntimeError: return 0
def drive(e, f, maxf=24000):
    """Tap A through text; hand battles to Emu.battle; stop when flag f set and back in field."""
    t = 0; battles = 0
    while t < maxf:
        if not e.in_field():
            e.shot(str(OUT / 'live.png')); e.battle(maxf=30000); battles += 1; t += 600; continue
        e.run(20, 1); e.run(20); t += 40
        if t % 2000 == 0: e.shot(str(OUT / 'live.png'))
        if t % 400 == 0 and flag(e, f): 
            e.run(200, 1); e.run(200)
            return True, battles
    return bool(flag(e, f)), battles
def refresh(e):  # full PP + HP on the lead before each scene (shortcut)
    for i, mv in enumerate(('MOVE_PSYSTRIKE', 'MOVE_AURA_SPHERE', 'MOVE_ICE_BEAM', 'MOVE_THUNDERBOLT')): e.cmd(19, CM(mv), i, 0)
    e.cmd(13, 0, 353)
def run(story, part, sav):
    name = {1: 'chief', 2: 'show'}[story]
    s = OUT / f'{name}_p{part}.sav'; shutil.copy(sav, s)
    e = PJ.jemu(part, s); inf = PJ.boot_continue(e)
    if not inf: check(f'[{name} P{part}] boot', False, 'not in field'); e.close(); return
    e.cmd(24, story)
    e.cmd(1, 150, 100, 0); e.cmd(19, CM('MOVE_PSYSTRIKE'), 0, 0); e.cmd(19, CM('MOVE_AURA_SPHERE'), 1, 0)  # strong lead, damaging moves first
    for b in range(1, 9 if part > 1 else 4): e.cmd(6, C(f'FLAG_BADGE0{b}_GET'), 1)
    shortcuts.append(f'{name} P{part}: party set to Mewtwo L100; badges 1-{8 if part>1 else 3} forced')
    for m, st, f, g1, g2, cond, scr in TABLE[part]:
        if STORY[st] != story or flag(e, f): continue
        if m.startswith('ARENA_'): continue  # entered through its portal scene
        if cond == '2': e.cmd(1, 6, 50, 1); shortcuts.append(f'{name}: Charizard L50 added for {scr}')
        if cond in '34':
            for sym in ['TOWER', 'DOME', 'PALACE', 'ARENA', 'FACTORY', 'PIKE', 'PYRAMID']: e.cmd(6, C(f'FLAG_SYS_{sym}_SILVER'), 1)
            shortcuts.append(f'{name}: 7 silver symbols forced for {scr}')
        if g2.startswith('FLAG_SYS_'): e.cmd(6, C(g2), 1); shortcuts.append(f'{name}: {g2} forced for {scr}')
        if cond == '1':
            check(f'[{name}] {scr} on {m}', None, 'SKIPPED: needs night RTC; flag forced'); results[-1]['status'] = 'SKIP'
            e.cmd(6, C(f), 1); continue
        g, n = PJ.mapid('MAP_' + m); x, y = xy(m)
        refresh(e)
        e.cmd(14, g, n, x | y << 8); e.run(300)
        if scr.startswith('ArenaStory_SJ7') : pass
        ok, nb = drive(e, f)
        e.shot(str(OUT / f'{name}_{scr}.png'))
        check(f'[{name}] {scr} on {m}', ok, f'flag {f}={int(ok)} battles={nb} loc={e.location()}')
    if story == 2 and part == 1:  # SJ7 is on Whitney's talk script: run its hooks directly
        a = e.s('ArenaStory_SJ7_Pre')
        for i in range(2):
            e.cmd(21, a & 0xffff, a >> 16, C('FLAG_SHOW_SJ7_LOSS')); drive(e, 'FLAG_SHOW_SJ7_LOSS', 6000)
        b = e.s('ArenaStory_SJ7_Badge'); e.cmd(21, b & 0xffff, b >> 16, C('FLAG_SHOW_SJ7')); ok, nb = drive(e, 'FLAG_SHOW_SJ7', 6000)
        check('[show] SJ7 Whitney hooks (Pre x2, Badge) via cmd 21', ok and flag(e, 'FLAG_SHOW_SJ7_LOSS') == 1, f'SJ7_LOSS={flag(e,"FLAG_SHOW_SJ7_LOSS")} SJ7={int(ok)} battles={nb}')
    if story == 2 and part == 2:
        check('[show] P2_END set after Harrison rematch', flag(e, 'FLAG_SHOW_P2_END') == 1, '')
    e.close()
ONLY = sys.argv[3] if len(sys.argv) > 3 else None
for story in (1, 2):
    nm = {1: 'chief', 2: 'show'}[story]
    for part, sav in ((1, SAVES / 'classic_p1_complete.sav'), (2, SAVES / 'classic_p1_complete.sav'), (3, SAVES / 'classic_p2_complete.sav')):
        if ONLY and ONLY != f'{nm}{part}': continue
        run(story, part, sav)
(OUT / f'story-results-{ONLY or "all"}.json').write_text(json.dumps(dict(results=results, shortcuts=shortcuts), indent=1))
print(f"\n{sum(r['status']=='PASS' for r in results)}/{len(results)} PASS, {sum(r['status']=='SKIP' for r in results)} SKIP")
