#!/usr/bin/env python3
"""Chief Story starters check (Johto ch2): Elm's lab ball choice under Classic vs Chief.
Shortcut (labelled): demo new game (boot_new), storyline via playtest cmd 24, warp into the lab and run the
*_Choice script directly; rival battle started via cmd 21 running the Cherrygrove trainerbattle label using the trainer the Cherrygrove script would pick."""
import json, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))
from ptlib import Emu
from playtest_johto import consts, boot_new, ROOT
C = consts(ROOT, 'include/constants/flags.h', 'include/constants/vars.h', 'include/constants/opponents.h')
ORD = "GAEM GAME GEAM GEMA GMAE GMEA AGEM AGME AEGM AEMG AMGE AMEG EGAM EGMA EAGM EAMG EMGA EMAG MGAE MGEA MAGE MAEG MEGA MEAG".split()
def species(e, base):
    pid, ot = e.r32(base), e.r32(base + 4)
    g = ORD[pid % 24].index('G')
    return (e.r32(base + 0x20 + 12 * g) ^ pid ^ ot) & 0x7ff
NAMES = {152: 'CHIKORITA', 155: 'CYNDAQUIL', 158: 'TOTODILE', 215: 'SNEASEL', 228: 'HOUNDOUR', 147: 'DRATINI'}  # expansion species ids = national dex
nm = lambda s: NAMES.get(s, str(s))
RIV = {0: 'TRAINER_RIVAL_CYNDAQUIL_1', 1: 'TRAINER_RIVAL_TOTODILE_1', 2: 'TRAINER_RIVAL_CHIKORITA_1'}
out = ROOT / '.arena-dev/pt/results/chief'; out.mkdir(parents=True, exist_ok=True)
n = int(sys.argv[1]) if len(sys.argv) > 1 else 1
res = []
for story, sname in ((0, 'classic'), (1, 'chief')):
    for ball in ('Chikorita', 'Cyndaquil', 'Totodile'):
        sav = out / f'j{n}_{sname}_{ball}.sav'
        if sav.exists(): sav.unlink()
        e = Emu('/workspace/fr/smoke/playtest_drv', str(ROOT / f'jpt_p{n}.gba'), str(ROOT / f'jpt_p{n}.elf'), str(sav))
        inf = boot_new(e)
        e.cmd(24, story); e.cmd(1, 0, 0, 0); e.cmd(6, C('FLAG_SYS_POKEMON_GET'), 0)
        e.cmd(20, C('VAR_STARTER_MON'), 7, 1)
        e.cmd(14, 1, 0, 9 | (5 << 8)); e.run(600)
        sym = e.s(f'NewBarkTown_Lab_EventScript_{ball}Ball_Choice')
        e.cmd(21, sym & 0xffff, sym >> 16); e.run(240)
        shot = out / f'j{n}_{sname}_{ball}_confirm.png'; e.shot(str(shot))
        e.run(4, 1)
        for _ in range(12): e.run(60); e.run(4, 2)
        e.run(200)
        lv_sp = e.cmd(12, 0)
        smon = e.cmd(20, C('VAR_STARTER_MON'))[0]
        rival = RIV.get(smon)
        en = None; bshot = None
        if rival:
            bs = e.s('CherryGroveCity_Silver_Battle_' + rival.split('_')[2].capitalize()); e.cmd(21, bs & 0xffff, bs >> 16); e.run(900)
            en = species(e, e.r32(e.s('gEnemyPartyPtr')))
            bshot = out / f'j{n}_{sname}_{ball}_rival.png'; e.shot(str(bshot))
        r = dict(part=n, story=sname, ball=ball, field=inf, player_starter=nm(lv_sp[2]), level=lv_sp[0], VAR_STARTER_MON=smon,
                 rival_trainer=rival, rival_lead=nm(en) if en is not None else None, confirm_png=str(shot), rival_png=str(bshot))
        print(r, flush=True); res.append(r)
json.dump(res, open(out / f'johto_p{n}_chief_starters.json', 'w'), indent=1)
