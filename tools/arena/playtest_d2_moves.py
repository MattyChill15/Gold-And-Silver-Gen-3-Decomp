#!/usr/bin/env python3
"""Checklist v2 item 6 (D2): after the real SJ1 scene, Misty + Pikachu followers through a door warp,
a map-edge transition, Surf (water) and back; then SJ2 Brock swap.
Shortcuts (labeled): storyline set (cmd 24), warps to scene/test spots (cmd 14), lead Pikachu L5 (cmd 1),
Mewtwo slot 2 taught Surf (cmd 1/19), badges 1-4 forced in P1; water leg in P2 with badges 1-8 forced (Surf = badge 5), lead Pikachu raised to L100 + VAR_REPEL_STEP_COUNT=250 so wild battles don't interrupt; save made after SJ2 in P1 (cmd 6)."""
import sys, json, shutil
sys.argv = ['x', '/workspace/fr/smoke/playtest_drv', '/tmp/johto_v2', 'none']
sys.path.insert(0, 'tools/arena')
import playtest_story as S
PJ = S.PJ; C = S.C; OUT = S.Path('/tmp/johto_v2_d2'); OUT.mkdir(exist_ok=True)
res = []; sc = []
def check(n, ok, d): res.append(dict(check=n, status='PASS' if ok else 'FAIL', detail=d)); print('PASS' if ok else 'FAIL', n, d, flush=True)
def surfing(e): return e.r8(e.s('gPlayerAvatar')) & 8
def fol(e): o = S.objs(e); return o.get(253), o.get(254)
def state(e, tag):
    p = OUT / f'd2_{tag}.png'; e.shot(str(p)); n, m = fol(e); loc = e.location()
    return dict(tag=tag, loc=loc, npc=n, mon=m, surf=bool(surfing(e)), field=e.in_field(), shot=str(p))
def near(st):
    px, py = st['loc'][0], st['loc'][1]
    return st['npc'] and st['mon'] and all(abs(o[1] - px) + abs(o[2] - py) <= 3 for o in (st['npc'], st['mon']))
s = OUT / 'd2.sav'; shutil.copy('/tmp/johto_v2/classic_p1_complete.sav', s)
e = PJ.jemu(1, s); PJ.boot_continue(e); e.cmd(24, 2); e.cmd(30, 1)
for b in (1, 2, 3, 4): e.cmd(6, C(f'FLAG_BADGE0{b}_GET'), 1)
g, n = PJ.mapid('MAP_NEW_BARK_TOWN_PLAYERS_HOUSE_1F'); e.cmd(14, g, n, 9 | 7 << 8); e.run(300)
ok, _ = S.drive(e, 'FLAG_ARENA_SHOW_SJ1')
e.cmd(1, 25, 5, 0); e.cmd(1, 150, 100, 1); e.cmd(19, S.CM('MOVE_SURF'), 0, 1)
g, n = PJ.mapid('MAP_NEW_BARK_TOWN_PLAYERS_HOUSE_1F'); e.cmd(14, g, n, 9 | 6 << 8); e.run(300)
for _ in range(3): e.run(16, 128); e.run(24)
s0 = state(e, 'indoor_before_door')
for _ in range(4): e.run(16, 128); e.run(40)  # step onto the door mat -> exits to New Bark
e.run(200); s1 = state(e, 'after_door_exit')
check('D2 door: Misty+Pikachu follow out of player house', ok and s1['loc'][2:] == PJ.mapid('MAP_NEW_BARK_TOWN') and near(s1), f'{s0} -> {s1}')
g, n = PJ.mapid('MAP_NEW_BARK_TOWN'); e.cmd(14, g, n, 1 | 12 << 8); e.run(240)
for _ in range(6): e.run(16, 32); e.run(24)
e.run(120); s2 = state(e, 'after_edge_to_route29')
check('D2 map transition New Bark -> Route 29 (walk west)', s2['loc'][2:] == PJ.mapid('MAP_ROUTE29') and near(s2), str(s2))
g, n = PJ.mapid('MAP_AZALEA_TOWN_KURTS_HOUSE'); x, y = S.xy('AZALEA_TOWN_KURTS_HOUSE'); e.cmd(14, g, n, x | y << 8); e.run(300)
ok2, _ = S.drive(e, 'FLAG_ARENA_SHOW_SJ2'); s6 = state(e, 'after_SJ2'); sb3 = e.r32(e.s('gSaveBlock3Ptr'))
check('D2 swap: Brock replaces Misty after SJ2', ok2 and e.r16(sb3 + 12 + 18) == 302 and s6['npc'] and s6['npc'][0] == 302, f'followerGfx={e.r16(sb3+12+18)} (302=Brock, 303=Misty) {s6}')
PJ.save(e); e.close()
# water: Surf is badge 5, which P1 never grants (part gate), so the water leg runs in P2 on the same save (followers carried in SaveBlock3)
e = PJ.jemu(2, s); PJ.boot_continue(e)
for b in range(1, 9): e.cmd(6, C(f'FLAG_BADGE0{b}_GET'), 1)
e.cmd(1, 25, 100, 0); e.cmd(1, 150, 100, 1); e.cmd(19, S.CM('MOVE_SURF'), 0, 1)
e.cmd(20, C('VAR_REPEL_STEP_COUNT'), 250, 1)  # shortcut: repel steps (no wild battles on the water leg)
g, n = PJ.mapid('MAP_NEW_BARK_TOWN'); e.cmd(14, g, n, 24 | 12 << 8); e.run(240); e.run(8, 16); e.run(20)  # face east to the pond
for _ in range(12):
    if surfing(e): break
    e.run(6, 1); e.run(60)
for _ in range(3): e.run(16, 16); e.run(30)
e.run(120); s3 = state(e, 'surfing')
check('D2 water: Surf with followers, no softlock', s3['surf'] and s3['field'], str(s3) + ' (followers hidden/on blob while surfing per FNPC)')
for _ in range(6): e.run(16, 32); e.run(30)
e.run(200); s4 = state(e, 'after_dismount')
check('D2 water: dismount, followers back and trailing', not s4['surf'] and near(s4), str(s4))
for _ in range(4): e.run(16, 128); e.run(24)
s5 = state(e, 'walk_after_surf'); check('D2 no softlock after water (player still moves)', s5['loc'] != s4['loc'] and s5['field'], str(s5))
e.close()
json.dump(dict(results=res, shortcuts=__doc__.split('Shortcuts (labeled): ')[1]), open(OUT / 'd2_results.json', 'w'), indent=1)
print(sum(r['status'] == 'PASS' for r in res), '/', len(res))
