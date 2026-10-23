#!/usr/bin/env python3
"""Matt: split the arena art roster across Emerald Parts 1-3 under a ROM byte budget.
Each part: own-map wilds (+their families) first, then species the player can carry in
from earlier parts, then dex fill so the union of the three parts covers all 386."""
import json, re, os, sys
R = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.chdir(R)
BUDGET = int(sys.argv[1]) if len(sys.argv) > 1 else 8_900_000
STARTERS = ['VIBRAVA', 'MUDKIP', 'ABRA']
old = {}
for f in ['tools/arena/roster.json', 'tools/arena/roster_part2.json']:
    for e in json.load(open(f)): old.setdefault(e['species'], e)
# dex numbers / names for all 386 from blobs + species constants
names = {}
for l in open('include/constants/species.h'):
    m = re.match(r'#define SPECIES_(\w+)\s+(\d+)', l)
    if m and 1 <= int(m.group(2)) <= 251: names[m.group(1)] = int(m.group(2))
for l in open('include/constants/pokedex.h'):
    m = re.match(r'\s+NATIONAL_DEX_(\w+),', l)
    if m: pass
natl = [m.group(1) for m in re.finditer(r'NATIONAL_DEX_(\w+),', open('include/constants/pokedex.h').read().split('enum {',1)[1].split('}',1)[0])]
natl = [n for n in natl if n != 'NONE'][:386]
dex = {n: i + 1 for i, n in enumerate(natl)}
cost = {n: os.path.getsize(f'.arena-dev/pmd/{dex[n]:04d}.blob') for n in natl}
# evolution families
evo = {}
for m in re.finditer(r'\[SPECIES_(\w+)\]\s*=\s*\{(.*?)\}\s*,\s*\n', open('src/data/pokemon/evolution.h').read(), re.S):
    for t in re.findall(r'SPECIES_(\w+)\}', m.group(2)): evo.setdefault(m.group(1), set()).add(t)
def fam(s):
    out, todo = set(), [s]
    while todo:
        x = todo.pop()
        if x in out: continue
        out.add(x); todo += list(evo.get(x, ()))
    return out
def wild(f):
    d = json.load(open(f))['wild_encounter_groups'][0]['encounters']
    out = {}
    for e in d:
        s = set()
        for v in e.values():
            if isinstance(v, dict) and 'mons' in v:
                s |= {m['species'].replace('SPECIES_', '') for m in v['mons']}
        out[e['base_label']] = s
    return out
P1W, P2W = wild('src/data/wild_encounters.json'), wild('src/data/wild_encounters_part2.json')
P3_RE = r'Route12[9]|Route13|SeafloorCavern|CaveOfOrigin|SkyPillar|VictoryRoad|Sootopolis|MeteorFalls_B1F|MeteorFalls_StevensCave|Underwater_Route12[6-9]|Underwater_Sootopolis|ArtisanCave|DesertUnderpass|AlteringCave|SealedChamber|MirageTower|MagmaHideout|AquaHideout|SafariZone_North[EW]|SafariZone_South[EW]|AbandonedShip_HiddenFloor|Underwater_SealedChamber|ShoalCave_LowTideIceRoom|ScorchedSlab|TerraCave|MarineCave|SouthernIsland|NavelRock|FarawayIsland|BirthIsland'
P2_RE = r'Route12[0-8]|Route119|MtPyre|Lilycove|SafariZone|Mossdeep|ShoalCave|Underwater_Route124|Underwater_Route126|NewMauville|AbandonedShip|ScorchedSlab'
def part_of(label):
    if re.search(P3_RE, label): return 3
    if re.search(P2_RE, label): return 2
    return 1
own = {1: set(STARTERS), 2: set(), 3: set()}
for lab, s in P1W.items():
    if part_of(lab) == 1: own[1] |= s
for lab, s in P2W.items():
    p = part_of(lab)
    if p >= 2: own[p] |= s
LEGEND = ['REGIROCK', 'REGICE', 'REGISTEEL', 'LATIAS', 'LATIOS', 'KYOGRE', 'GROUDON', 'RAYQUAZA', 'JIRACHI', 'DEOXYS']
own[3] |= set(LEGEND)
parts, union = {}, set()
for p in (1, 2, 3):
    pick, used = [], 0
    uncovered = sorted(set(natl) - union - set().union(*(own[q] for q in range(p + 1, 4))), key=lambda n: dex[n]) if p == 3 else []
    tiers = [sorted(own[p]), uncovered,
             sorted(set().union(*(fam(x) for x in own[p])) - own[p]),
             sorted(set().union(*(own[q] for q in range(1, p))) | set().union(*(parts[q] for q in range(1, p)) if p > 1 else set())),
             sorted(set(natl), key=lambda n: dex[n])]
    seen = set()
    for t, tier in enumerate(tiers):
        for n in tier:
            if n in seen or n not in cost: continue
            if t == 5 and p < 3: continue
            if used + cost[n] > BUDGET: continue
            seen.add(n); used += cost[n]
    parts[p] = seen; union |= seen
    print(f'part {p}: {len(seen)} species, {used} B; own {len(own[p])} missing {sorted(own[p]-seen)}')
print('union', len(union), 'missing', sorted(set(natl) - union, key=lambda n: dex[n]))
for p, f in ((1, 'roster.json'), (2, 'roster_part2.json'), (3, 'roster_part3.json')):
    out = []
    for n in sorted(parts[p], key=lambda n: dex[n]):
        e = dict(old.get(n) or {'dex': f'{dex[n]:04d}', 'species': n, 'level': 20})
        out.append(e)
    json.dump(out, open('tools/arena/' + f, 'w'), indent=1)
