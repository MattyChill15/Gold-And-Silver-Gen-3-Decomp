"""Release-ROM smoke (johto_pN.gba, no playtest mailbox): boot, R+Start debug probe, walk,
save via Start menu, cold boot + Continue, compare location. P2/P3 boot from the series-test chain saves (classic_pN_complete.sav)
made by playtest_johto.py (shortcut: those saves were written by the jpt_pN playtest ROMs)."""
import sys, shutil, json, hashlib; sys.path.insert(0, 'tools/arena'); sys.argv = ['x']
from ptlib import Emu; import playtest_johto as PJ
O = '/tmp/johto_rel_smoke'; D = '/workspace/fr/smoke/playtest_drv'; res = []
def ck(n, ok, d): res.append(dict(check=n, status='PASS' if ok else 'FAIL', detail=d)); print('PASS' if ok else 'FAIL', n, d, flush=True)
def emu(n): return Emu(D, f'johto_p{n}.gba', f'johto_p{n}.elf', f'{O}/rel_p{n}.sav')
def loc(e):
    sb1 = e.r32(e.s('gSaveBlock1Ptr')); return (e.r16(sb1), e.r16(sb1+2), e.r8(sb1+4), e.r8(sb1+5))
for n in (1, 2, 3):
    if n > 1: shutil.copy(f'/tmp/johto_v2/classic_p{n-1}_complete.sav', f'{O}/rel_p{n}.sav')
    else: open(f'{O}/rel_p1.sav', 'wb').close()
    e = emu(n); ok = PJ.boot_new(e) if n == 1 else PJ.boot_continue(e); e.run(200)
    ck(f'P{n} boots to field', ok and e.in_field(), loc(e))
    cb = e.cb2(); e.run(3, 264); e.run(120); e.shot(f'{O}/p{n}_r_start.png')
    ck(f'P{n} R+Start opens no debug menu', e.in_field() and e.cb2() == cb, 'see p%d_r_start.png' % n)
    e.run(3, 2); e.run(60)
    for _ in range(2): e.run(16, 32); e.run(8)   # walk left
    e.run(60); before = loc(e)
    e.run(3, 8); e.run(100); e.shot(f'{O}/p{n}_startmenu.png')
    for _ in range(2): e.run(3, 128); e.run(20)
    e.run(3, 1); e.run(150)
    for _ in range(6): e.run(3, 1); e.run(120)
    for _ in range(4): e.run(3, 2); e.run(60)
    e.shot(f'{O}/p{n}_after_save.png'); e.p.kill()
    e = emu(n); ok = PJ.boot_continue(e); e.run(200); after = loc(e); e.shot(f'{O}/p{n}_continue.png')
    ck(f'P{n} save -> cold boot -> Continue keeps location', ok and after == before, f'{before} -> {after}')
    e.p.kill()
md5 = {f'johto_p{n}': hashlib.md5(open(f'johto_p{n}.gba','rb').read()).hexdigest() for n in (1,2,3)}
json.dump(dict(roms=md5, results=res), open(f'{O}/release_smoke.json', 'w'), indent=1)
print(sum(r['status']=='PASS' for r in res), '/', len(res))
