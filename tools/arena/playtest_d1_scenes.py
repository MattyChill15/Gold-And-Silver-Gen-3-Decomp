import sys, shutil, json, hashlib
sys.argv=['x','/workspace/fr/smoke/playtest_drv','/tmp/johto_pt','none']  # ONLY=none -> main loop no-op
sys.path.insert(0,'tools/arena')
import playtest_story as S
PJ=S.PJ; C=S.C; OUT=S.Path('/tmp/johto_d1')
CASES=[(1,1,'NEW_BARK_TOWN','FLAG_ARENA_CHIEF_J1','ArenaStory_J1_0','classic_p1_complete.sav'),
 (1,2,'NEW_BARK_TOWN_PLAYERS_HOUSE_1F','FLAG_ARENA_SHOW_SJ1','ArenaStory_SJ1_20','classic_p1_complete.sav'),
 (2,1,'OLIVINE_CITY_CAFE','FLAG_ARENA_CHIEF_J8','ArenaStory_J8_7','classic_p1_complete.sav'),
 (2,2,'OLIVINE_CITY_LIGHTHOUSE','FLAG_ARENA_SHOW_SJ10A','ArenaStory_SJ10_31','classic_p1_complete.sav'),
 (3,1,'SSAQUA_CAPTAINS_ROOM','FLAG_ARENA_CHIEF_J13','ArenaStory_J13_13','classic_p2_complete.sav'),
 (3,2,'PALLET_TOWN_REDS_HOUSE_1F','FLAG_ARENA_SHOW_SJ14','ArenaStory_SJ14_39','classic_p2_complete.sav')]
res=[]
import os
for part,story,m,f,scr,sv in [c for c in CASES if c[0]>=int(os.environ.get('FROM','1'))]:
    nm={1:'chief',2:'show'}[story]; s=OUT/f'{nm}_p{part}.sav'; shutil.copy(f'/tmp/johto_pt/{sv}',s)
    e=PJ.jemu(part,s); PJ.boot_continue(e)
    e.cmd(24,story); e.cmd(30,1); e.cmd(1,150,100,0)
    for b in range(1,9 if part>1 else 4): e.cmd(6,C(f'FLAG_BADGE0{b}_GET'),1)
    S.refresh(e); pre=S.flag(e,f)
    g,n=PJ.mapid('MAP_'+m); x,y=S.xy(m); e.cmd(14,g,n,x|y<<8)
    shots=[]; t=0; nb=0
    for i in range(400):
        if not e.in_field():
            nb+=1; S.battle_win(e); e.shot(str(OUT/f'{nm}_p{part}_{scr}_battle{nb}.png'))
            cb=e.cb2()&~1; print('  after battle_win cb2',next((k for k,v in e.sym.items() if (v&~1)==cb),hex(cb)),flush=True)
            if nb>3: break
            continue
        e.run(60)
        if i<40 and i%2==0: p=OUT/f'{nm}_p{part}_{scr}_{i:02d}.png'; e.shot(str(p)); shots.append(str(p))
        e.run(10,1); e.run(10)
        if i%5==0 and S.flag(e,f): break
    post=S.flag(e,f); p=OUT/f'{nm}_p{part}_{scr}_end.png'; e.shot(str(p))
    res.append(dict(part=part,mode=nm,map=m,script=scr,flag=f,flag_before=pre,flag_after=post,battles=nb,shots=shots))
    print(res[-1]['part'],nm,scr,pre,post,nb,flush=True); e.close()
json.dump(res,open(OUT/f"d1_scenes_raw_{os.environ.get('FROM','1')}.json",'w'),indent=1)
