import json,re,os
d=json.load(open('src/data/wild_encounters.json'))
sp={}
for l in open('include/constants/species.h'):
    m=re.match(r'\s*(SPECIES_\w+)\s*=\s*(\d+)\s*,',l)
    if m: sp[m.group(1)]=int(m.group(2))
maps={}
for g in d['wild_encounter_groups']:
    for e in g['encounters']:
        m=e.get('map');
        if not m: continue
        for k,v in e.items():
            if isinstance(v,dict) and 'mons' in v:
                for mm in v['mons']: maps.setdefault(m,set()).add(mm['species'])
P1=set('NEW_BARK_TOWN CHERRYGROVE_CITY VIOLET_CITY AZALEA_TOWN GOLDENROD_CITY ECRUTEAK_CITY OLIVINE_CITY OLIVINE_CITY_PORT_OUTSIDE ROUTE29 ROUTE30 ROUTE31 ROUTE32 ROUTE33 ROUTE34 ROUTE35 ROUTE36 ROUTE37 ROUTE38 ROUTE39 SPROUT_TOWER_2F SPROUT_TOWER_3F UNION_CAVE_1F UNION_CAVE_B1F UNION_CAVE_B2F SLOWPOKE_WELL_B1F SLOWPOKE_WELL_B2F ILEX_FOREST NATIONAL_PARK_BUG_CONTEST NATIONAL_PARK_NORMAL BURNED_TOWER_1F BURNED_TOWER_B1F RUINS_OF_ALPH_B1F RUINS_OF_ALPH_OUTSIDE DARK_CAVE_SOUTH_SIDE'.split())
P2=set('ROUTE40 ROUTE41 ROUTE42 ROUTE43 ROUTE44 ROUTE45 ROUTE46 ROUTE26 ROUTE27 CIANWOOD_CITY WHIRL_ISLANDS_1F WHIRL_ISLANDS_B1F WHIRL_ISLANDS_B2F MAHOGANYTOWN LAKE_OF_RAGE ROCKET_HIDEOUT_B1F ICE_PATH_1F ICE_PATH_B1F ICE_PATH_B2F ICE_PATH_B3F ICE_PATH_B4F BLACKTHORN_CITY DRAGONS_DEN_CAVERN DARK_CAVE_NORTH_SIDE MT_MORTAR_1F_NORTH MT_MORTAR_1F_SOUTH MT_MORTAR_2F MT_MORTAR_B1F TOHJO_FALLS_CAVERN VICTORY_ROAD_1F VICTORY_ROAD_B1F VICTORY_ROAD_B2F TIN_TOWER_3F TIN_TOWER_4F TIN_TOWER_5F TIN_TOWER_6F TIN_TOWER_7F TIN_TOWER_8F TIN_TOWER_9F'.split())
kanto=re.compile(r'ROUTE([1-9]|1\d|2[0-5]|28)$|PALLET|VIRIDIAN|PEWTER|CERULEAN|VERMILION|LAVENDER|CELADON|SAFFRON|FUCHSIA|CINNABAR|MT_MOON|ROCK_TUNNEL|DIGLETTS|SEAFOAM|MT_SILVER|VICTORY_ROAD_KANTO|VICTORY_ROAD_MOLTRES|SAFARI_ZONE')
def dex(s):
    n=sp.get(s,0)
    return n if 1<=n<=386 else 0
cost=lambda n: os.path.getsize('.arena-dev/pmd/%04d.blob'%n)+os.path.getsize('.arena-dev/pmd/%04d.gbapal'%n)
parts={'P1':set(),'P2':set(),'P3':set()}
for m,s in maps.items():
    k=m[4:]
    p='P1' if k in P1 else 'P2' if k in P2 else 'P3' if kanto.search(k) else None
    if p: parts[p]|={dex(x) for x in s if dex(x)}
for p,s in parts.items():
    print(p,len(s),sum(cost(n) for n in s))
print('budget',int(0.85*32*1024*1024)-(35640036-8860292))
u=parts['P1']|parts['P2']|parts['P3']
print('union',len(u),sum(cost(n) for n in u))
import glob,collections
a=collections.Counter()
for n in parts['P1']:
    for f in glob.glob('.arena-dev/pmd/%04d-*.4bpp'%n): a[f.split('-')[-1]]+=os.path.getsize(f)
print('P1 raw 4bpp by anim',dict(a))
