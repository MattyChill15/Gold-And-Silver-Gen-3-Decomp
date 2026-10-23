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
SHORTCUTS_PP = []
import os
LOSS = os.environ.get('ARENA_LOSS') == '1'  # test mode: really lose every FLAG_ARENA_NO_WHITEOUT battle
OUTCOMES = []  # (scene, noWhiteoutFlag, gBattleOutcome, map after)
def nowhite(e):
    sb1 = e.r32(e.s('gSaveBlock1Ptr')); fl = C('FLAG_ARENA_NO_WHITEOUT')
    return (e.r8(sb1 + 4960 + fl // 8) >> (fl % 8)) & 1
def lose(e, maxf=30000):
    # shortcut (loss test): while FLAG_ARENA_NO_WHITEOUT is set in battle, lead held at 1 HP and foe HP refilled, so the player genuinely loses
    bm = e.s('gBattleMons'); bcb = e.s('BattleMainCB2') | 1; f = 0; hit = 0
    while f < 1500 and (e.cb2() | 1) != bcb and not e.in_field(): e.run(4); e.run(4, 1); f += 8
    if not nowhite(e):  # ordinary battle: normal win driver
        battle_win(e); return 0
    while f < maxf:
        if hit == 1 and not nowhite(e):  # scripted-loss battle over (flag cleared by script): record its outcome
            hit = 2; OUTCOMES.append((CUR[0] + ':loss', 1, e.r8(e.s('gBattleOutcome')), e.location()[:2])); print('  outcome', OUTCOMES[-1], flush=True)
        if not e.in_field() and nowhite(e):
            # shortcut (loss test): every 8 frames lead's battle moves -> Splash, lead HP<=1, foe HP full, bench HP 0 (no switch-in)
            hit = 1
            for i in range(4): e.w16(bm + 12 + 2 * i, 150)
            if e.r16(bm + 42) > 1: e.w16(bm + 42, 1)
            e.w16(bm + 140 + 42, e.r16(bm + 140 + 46))
            pp = e.s('gParties')  # gParties[0] = player party
            for k in range(1, 6): e.w16(pp + 100 * k + 86, 0)
        e.run(4); e.run(4, 1); f += 8
        if hit and f % 400 == 0 and os.environ.get('ARENA_DBG'): print('   dbg', e.r16(bm), e.r16(bm+12), e.r16(bm+42), e.r16(bm+140), e.r16(bm+140+42), e.r16(bm+280+42), e.r8(e.s('gBattleOutcome')), flush=True)
        if f % 200 == 0 and e.in_field():
            e.run(60)
            if e.in_field(): break
    return 1 if hit == 1 else 0  # 2 = loss already recorded; this outcome is the rematch
ASSIST = os.environ.get('ARENA_ASSIST') == '1'; ASSISTED = []
def battle_win(e):
    if not ASSIST: return e.battle(maxf=30000)
    # shortcut (win assist): lead battle HP and PP refilled every 8 frames so one Mewtwo can clear 6-mon L70+ teams
    bm = e.s('gBattleMons'); f = 0; ASSISTED.append(CUR[0])
    while f < 40000:
        if not e.in_field():
            e.w16(bm + 42, e.r16(bm + 46))
            for i in range(4): e.w8(bm + 37 + i, 10)  # PP too
        e.run(4); e.run(4, 1); f += 8
        if f % 200 == 0 and e.in_field():
            e.run(60)
            if e.in_field(): break
def drive(e, f, maxf=24000):
    """Tap A through text; hand battles to Emu.battle; stop when flag f set and back in field."""
    t = 0; battles = 0; topup = False
    while t < maxf:
        if not e.in_field():
            e.shot(str(OUT / f'live_{ONLY}.png'))
            cb = e.cb2() & ~1; nm = next((k for k, v in e.sym.items() if (v & ~1) == cb), hex(cb))
            print(f'  non-field cb2={nm} t={t}', flush=True)
            if nm == 'CB2_PokeStorage':  # SJ17 opens the PC: leave it with B (no box changes)
                for _ in range(60):
                    if (e.cb2() | 1) != (e.s('CB2_PokeStorage') | 1): break
                    e.run(6, 2); e.run(40); e.run(6, 2); e.run(40); e.run(6, 1); e.run(40)  # B,B -> 'exit box?' -> A (yes); converges if a menu was open
                t += 400; continue
            nw = lose(e) if LOSS else (battle_win(e), nowhite(e))[1]
            OUTCOMES.append((CUR[0], nw, e.r8(e.s('gBattleOutcome')), e.location()[:2])); print('  outcome', OUTCOMES[-1], flush=True)
            battles += 1; t += 600; topup = True; continue
        if topup: refresh(e); topup = False; SHORTCUTS_PP.append(battles)  # shortcut: PP/HP top-up between back-to-back scene battles
        e.run(20, 1); e.run(20); t += 40
        if t % 2000 == 0: e.shot(str(OUT / f'live_{ONLY}.png'))
        if t % 400 == 0 and flag(e, f): 
            e.run(200, 1); e.run(200)
            return True, battles
    return bool(flag(e, f)), battles
CUR = ['']
def refresh(e):  # full PP + HP on the lead before each scene (shortcut)
    for i, mv in enumerate(('MOVE_ICE_BEAM', 'MOVE_AURA_SPHERE', 'MOVE_PSYSTRIKE', 'MOVE_THUNDERBOLT')): e.cmd(19, CM(mv), i, 0)
    e.cmd(13, 0, 353)
def run(story, part, sav):
    name = {1: 'chief', 2: 'show'}[story]
    s = OUT / f'{name}_p{part}.sav'; shutil.copy(sav, s)
    e = PJ.jemu(part, s); inf = PJ.boot_continue(e)
    if not inf: check(f'[{name} P{part}] boot', False, 'not in field'); e.close(); return
    e.cmd(24, story); e.cmd(30, 1)  # shortcut: classic battle engine (harness can't drive realtime)
    shortcuts.append(f'{name} P{part}: story battles run in classic engine (playtest cmd 30)')
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
            tod = e.cmd(31, 22, 0)[0]; shortcuts.append(f'{name}: game clock set to 22:00 (cmd 31) for {scr}, time of day={tod}')
        g, n = PJ.mapid('MAP_' + m); x, y = xy(m)
        refresh(e)
        e.cmd(14, g, n, x | y << 8); e.run(300)
        if scr.startswith('ArenaStory_SJ7') : pass
        CUR[0] = scr; ok, nb = drive(e, f)
        e.shot(str(OUT / f'{name}_{scr}.png'))
        check(f'[{name}] {scr} on {m}', ok, f'flag {f}={int(ok)} battles={nb} loc={e.location()}')
    if story == 2 and part == 3:
        # SJ16 shares the Pyramid lobby with SJ15_46 and fires straight after it; check it explicitly.
        check('[show] SJ16 set (Pyramid lose->rematch, fires after SJ15_46)', flag(e, 'FLAG_ARENA_SHOW_SJ16') == 1, f'SJ16={flag(e, "FLAG_ARENA_SHOW_SJ16")}')
    if story == 2 and part == 1:
        # SJ4 shares Route 30 with SJ3B and fires straight after it; check it explicitly.
        check('[show] SJ4 set (fires after SJ3B on ROUTE30)', flag(e, 'FLAG_ARENA_SHOW_SJ4') == 1, f'SJ4={flag(e, "FLAG_ARENA_SHOW_SJ4")}')
        # SJ7 hooks are `call`ed from the stock Whitney script, so drive the real gym:
        # talk -> forced loss (SJ7_LOSS) -> talk -> rematch win (state 3) -> state 4 -> badge (SJ7).
        e.cmd(6, C('FLAG_BADGE03_GET'), 0); refresh(e)
        g, n = PJ.mapid('MAP_GOLDENROD_CITY_GYM'); e.cmd(14, g, n, 15 | 10 << 8); e.run(400)
        CUR[0] = 'SJ7_Whitney'; e.run(10, 64); e.run(30); ok, nb = drive(e, 'FLAG_ARENA_SHOW_SJ7_LOSS', 8000)
        nb2 = 0
        for _ in range(40):  # talk again: rematch (stock Whitney battle) -> state 3
            st = e.cmd(20, C('VAR_GOLDENROD_CITY_STATE'))[0] if e.in_field() else -1
            if st == 3: break
            if not e.in_field(): e.battle(maxf=30000); nb2 += 1; continue
            refresh(e); e.run(10, 64); e.run(20); e.run(20, 1); e.run(200)
        nb += nb2
        check('[show] SJ7 forced loss then rematch at Whitney', ok and st == 3, f'SJ7_LOSS={int(ok)} state={st} battles={nb}')
        e.cmd(20, C('VAR_GOLDENROD_CITY_STATE'), 4, 1); shortcuts.append('show: Bridget trigger skipped, VAR_GOLDENROD_CITY_STATE=4 forced')
        e.run(10, 64); e.run(30); ok, nb = drive(e, 'FLAG_ARENA_SHOW_SJ7', 8000)
        for _ in range(60): e.run(20, 1); e.run(20)  # finish the badge script
        check('[show] SJ7 badge hook', ok and flag(e, 'FLAG_BADGE03_GET') == 1, f'SJ7={int(ok)} badge3={flag(e, "FLAG_BADGE03_GET")}')
    if story == 2 and part == 2:
        check('[show] P2_END set after Harrison rematch', flag(e, 'FLAG_ARENA_SHOW_P2_END') == 1, '')
    e.close()
ONLY = sys.argv[3] if len(sys.argv) > 3 else None
for story in (1, 2):
    nm = {1: 'chief', 2: 'show'}[story]
    for part, sav in ((1, SAVES / 'classic_p1_complete.sav'), (2, SAVES / 'classic_p1_complete.sav'), (3, SAVES / 'classic_p2_complete.sav')):
        if ONLY and ONLY != f'{nm}{part}': continue
        run(story, part, sav)
if LOSS: shortcuts.append('loss test: in FLAG_ARENA_NO_WHITEOUT battles the lead is held at 1 HP with Splash-only moves, bench HP 0, foe HP refilled (memory writes) to force a real loss')
nwb = [o for o in OUTCOMES if o[1] == 1]
if nwb: check(f'no-whiteout battles {"lost" if LOSS else "won"} without blackout', all(o[2] == (2 if LOSS else 1) for o in nwb) and all(o[3] != (10, 10) for o in OUTCOMES), str(nwb))
shortcuts.append(f'win assist: lead HP+PP refilled in battle (memory write) for {sorted(set(ASSISTED))}') if ASSISTED else None
shortcuts.append(f'harness: lead PP/HP topped up (cmd 19/13) between back-to-back battles inside one scene, {len(SHORTCUTS_PP)} times') if SHORTCUTS_PP else None
(OUT / f'story-results-{ONLY or "all"}{"-loss" if LOSS else ""}.json').write_text(json.dumps(dict(results=results, shortcuts=shortcuts, battle_outcomes=OUTCOMES, outcome_key='1=won 2=lost'), indent=1))
print(f"\n{sum(r['status']=='PASS' for r in results)}/{len(results)} PASS, {sum(r['status']=='SKIP' for r in results)} SKIP")
