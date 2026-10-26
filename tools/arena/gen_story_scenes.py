#!/usr/bin/env python3
"""Generate Johto Arena storyline scenes (Chief / Show) from Shorty's gen.py data.
Outputs:
  include/constants/arena_story_flags.h
  src/data/arena_story_scenes.h
  data/scripts/arena_story_scenes.inc
"""
import re, sys, os
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Movie handoffs are not in Shorty's gen.py. The checked-in scenes file owns
# SJ6A (M03) and SJ9A (M04). A regen must put both goto hooks back.
MOVIE_HOOKS = (
    ('\tsetflag FLAG_ARENA_SHOW_SJ6A\n', '\tgoto_if_unset FLAG_M03_DONE, Arena_M03_StartFromSJ6A\n'),
    ('\tsetflag FLAG_ARENA_SHOW_SJ9A\n', '\tgoto_if_unset FLAG_M04_DONE, Arena_M04_StartFromSJ9A\n'),
)
HAND_SCENE_MARK = ('ArenaStory_SJ6A', 'ArenaStory_SJ9A')
PART1_EXTRA = (
    'extern const u8 ArenaStory_SJ6A_29a[];',
    'extern const u8 ArenaStory_SJ9A_30a[];',
    '    {MAP_ILEX_FOREST, ARENA_STORYLINE_SHOW, FLAG_ARENA_SHOW_SJ6A, FLAG_ARENA_SHOW_SJ6, 0, 1, ArenaStory_SJ6A_29a},',
    '    {MAP_VIOLET_CITY_POKEMON_CENTER, ARENA_STORYLINE_SHOW, FLAG_ARENA_SHOW_SJ9A, FLAG_ARENA_SHOW_SJ8, 0, 0, ArenaStory_SJ9A_30a},',
)
HAND_FLAGS = (
    '#define FLAG_ARENA_SHOW_SJ6A                0x915',
    '#define FLAG_ARENA_SHOW_SJ9A                0x916',
    '#define FLAG_ARENA_FORCE_LOSS               0x917 // SJ7: opponent cannot be KO\'d / outcome forced to LOST',
)

def _if_blocks(text):
    return re.findall(r'\.if MATT_PART == \d+\n.*?\n\.endif\n', text, re.S)

def reattach_hand_scenes(prev, new):
    """Paste SJ6A/SJ9A blocks a from-scratch regen would omit."""
    if not prev:
        return new
    extra = []
    for block in _if_blocks(prev):
        if not any(mark in block for mark in HAND_SCENE_MARK):
            continue
        m = re.search(r'^(ArenaStory_SJ(?:6A|9A)\w*):', block, re.M)
        if not m:
            continue
        lab = m.group(1)
        if (lab + '::') in new or re.search(r'(?m)^%s:' % re.escape(lab), new):
            continue
        extra.append(block if block.endswith('\n') else block + '\n')
    if not extra:
        return new
    if not new.endswith('\n'):
        new += '\n'
    return new + '\n' + ''.join(extra)

def inject_movie_hooks(text):
    for anchor, hook in MOVIE_HOOKS:
        if hook in text:
            continue
        if anchor not in text:
            sys.exit('gen_story_scenes: movie hook anchor missing after regen: %s' % anchor.strip())
        text = text.replace(anchor, anchor + hook, 1)
    for _, hook in MOVIE_HOOKS:
        if text.count(hook) != 1:
            sys.exit('gen_story_scenes: movie hook not unique: %s' % hook.strip())
    return text

def preserve_story_header(text):
    m = re.search(r'#if MATT_PART == 1\n(.*?)#endif', text, re.S)
    if not m:
        sys.exit('gen_story_scenes: no MATT_PART == 1 scene table')
    block = m.group(0)
    if 'ArenaStory_SJ6A_29a' not in block:
        block = block.replace('#if MATT_PART == 1\n', '#if MATT_PART == 1\n%s\n%s\n' % (PART1_EXTRA[0], PART1_EXTRA[1]), 1)
    if 'FLAG_ARENA_SHOW_SJ6A' not in block:
        block = block.replace('\n};', '\n%s\n%s\n};' % (PART1_EXTRA[2], PART1_EXTRA[3]), 1)
    return text[:m.start()] + block + text[m.end():]

def preserve_story_flags(text):
    missing = [line for line in HAND_FLAGS if line.split()[1] not in text]
    if not missing:
        return text
    needle = '#define TRAINER_ARENA_STORY'
    if needle not in text:
        sys.exit('gen_story_scenes: flags header missing TRAINER_ARENA_STORY')
    return text.replace(needle, '\n'.join(missing) + '\n' + needle, 1)

# 0x915-0x917 are hand-assigned FLAG_ARENA_SHOW_SJ6A / SJ9A / FORCE_LOSS.
# 0x918-0x91B are FLAG_M03_STARTED/DONE/HIDE_ENTEI/HIDE_CHARIZARD. Do not reuse.
# 0x91C-0x91F are FLAG_M04_STARTED/DONE/HIDE_CELEBI/HIDE_SUICUNE. Do not reuse.
# First 55 assignments stay on 0x935-0x95F then 0x909-0x914.
FREE = [0x935 + i for i in range(0x95F - 0x935 + 1)] + [0x909 + i for i in range(0x915 - 0x909)] \
     + [0x924 + i for i in range(6)] + [0x933] + [0x2B1 + i for i in range(11)]
assert FREE[54] == 0x914, hex(FREE[54])
assert all(slot not in FREE for slot in range(0x915, 0x920))

def _selftest():
    inc = os.path.join(ROOT, 'data/scripts/arena_story_scenes.inc')
    live = open(inc).read()
    for _, hook in MOVIE_HOOKS:
        if live.count(hook) != 1:
            sys.exit('selftest: live hook count for %s is %d' % (hook.strip(), live.count(hook)))
    blocks = [b for b in _if_blocks(live) if any(mark in b for mark in HAND_SCENE_MARK)]
    if len(blocks) < 12:
        sys.exit('selftest: expected SJ6A/SJ9A script+text blocks, got %d' % len(blocks))
    stripped = live
    for b in blocks:
        if b not in stripped:
            sys.exit('selftest: block missing on strip')
        stripped = stripped.replace(b, '', 1)
    if 'Arena_M03_StartFromSJ6A' in stripped or 'Arena_M04_StartFromSJ9A' in stripped:
        sys.exit('selftest: strip left a movie hook behind')
    restored = inject_movie_hooks(reattach_hand_scenes(live, stripped))
    if restored.count(MOVIE_HOOKS[0][1]) != 1 or restored.count(MOVIE_HOOKS[1][1]) != 1:
        sys.exit('selftest: restored hooks not unique')
    if 'ArenaStory_SJ6A_29a::' not in restored or 'ArenaStory_SJ9A_30a::' not in restored:
        sys.exit('selftest: scene labels missing after reattach')
    bare = (
        '@ gen\n.if MATT_PART == 1\nArenaStory_SJ6A_29a::\n\tsetflag FLAG_ARENA_SHOW_SJ6A\n\treleaseall\n\tend\n.endif\n'
        '\n.if MATT_PART == 1\nArenaStory_SJ9A_30a::\n\tsetflag FLAG_ARENA_SHOW_SJ9A\n\treleaseall\n\tend\n.endif\n'
    )
    injected = inject_movie_hooks(reattach_hand_scenes(live, bare))
    if injected.count('ArenaStory_SJ6A_29a::') != 1 or injected.count('ArenaStory_SJ9A_30a::') != 1:
        sys.exit('selftest: duplicated a scene the generator already emitted')
    if injected.count(MOVIE_HOOKS[0][1]) != 1 or injected.count(MOVIE_HOOKS[1][1]) != 1:
        sys.exit('selftest: bare inject missed a hook')
    flags = '#define FLAG_ARENA_HIDE_MISTY        0x914\n#define TRAINER_ARENA_STORY          TRAINER_CHANSEY3\n'
    outf = preserve_story_flags(flags)
    if 'FLAG_ARENA_SHOW_SJ6A' not in outf or 'FLAG_ARENA_SHOW_SJ9A' not in outf or 'FLAG_ARENA_FORCE_LOSS' not in outf:
        sys.exit('selftest: flags')
    if preserve_story_flags(outf) != outf:
        sys.exit('selftest: flags not idempotent')
    header = (
        '#if MATT_PART == 1\nextern const u8 ArenaStory_SJ8_29[];\n'
        'static const struct ArenaScene sArenaScenes[] = {\n'
        '    {MAP_X, ARENA_STORYLINE_SHOW, FLAG_ARENA_SHOW_SJ8, 0, 0, 0, ArenaStory_SJ8_29},\n'
        '};\n#endif\n'
    )
    outh = preserve_story_header(header)
    if outh.count('ArenaStory_SJ6A_29a') != 2 or 'FLAG_ARENA_SHOW_SJ9A' not in outh:
        sys.exit('selftest: header\n' + outh)
    if preserve_story_header(outh) != outh:
        sys.exit('selftest: header not idempotent')
    print('m04 hook selftest ok', len(blocks))

if os.environ.get('ARENA_HOOK_SELFTEST') == '1':
    _selftest()
    sys.exit(0)

GEN = '/workspace/ops/arena/gen.py'
src = open(GEN).read().split('out=[')[0]
ns = {}; exec(src, ns)
LINES = {e[0]: e[5] for e in ns['E']}
PART = {e[0]: int(e[1][1]) for e in ns['E']}

# ---- flags (real names -> free slots) ----
FLAGS = ['FLAG_ARENA_NO_WHITEOUT'] + ['FLAG_ARENA_CHIEF_J%d' % i for i in range(1, 19)] + \
    ['FLAG_ARENA_CHIEF_J8_DELIVERY', 'FLAG_ARENA_CHIEF_J15A', 'FLAG_ARENA_CHIEF_J16_WON', 'FLAG_ARENA_CHIEF_CH2_DONE',
     'FLAG_ARENA_SHOW_SJ1', 'FLAG_ARENA_SHOW_SJ2', 'FLAG_ARENA_SHOW_SJ3A', 'FLAG_ARENA_SHOW_SJ3B', 'FLAG_ARENA_SHOW_SJ3C'] + \
    ['FLAG_ARENA_SHOW_SJ%d' % i for i in range(4, 15)] + ['FLAG_ARENA_SHOW_SJ7_LOSS', 'FLAG_ARENA_SHOW_SJ10A', 'FLAG_ARENA_SHOW_SJ10B'] + \
    ['FLAG_ARENA_SHOW_SJ15_%d' % i for i in range(1, 8)] + ['FLAG_ARENA_SHOW_SJ16', 'FLAG_ARENA_SHOW_SJ17', 'FLAG_ARENA_SHOW_SJ18',
     'FLAG_ARENA_SHOW_P2_END', 'FLAG_ARENA_SHOW_CH2_DONE', 'FLAG_ARENA_HIDE_MISTY']
assert len(FLAGS) <= len(FREE)

# ---- story trainers ----
def P(s):
    out = []
    for m in s.split(','):
        n, l = m.strip().rsplit(' ', 1); out.append(('SPECIES_' + n.upper(), int(l)))
    return out
TR = {  # name: (displayName, class, pic, party)
 'CHIEF_A': ('Chief', 'GENTLEMAN', 'GENTLEMAN', P('Murkrow 16, Zubat 15, Dratini 17')),
 'SAILOR':  ('Sailor', 'SAILOR', 'SAILOR', P('Tentacool 25, Machoke 26, Seaking 27')),
 'CHIEF_B': ('Chief', 'GENTLEMAN', 'GENTLEMAN', P('Murkrow 33, Golbat 33, Sneasel 34, Houndoom 34, Dragonair 36')),
 'CHIEF_C': ('Chief', 'GENTLEMAN', 'GENTLEMAN', P('Murkrow 42, Crobat 42, Sneasel 43, Houndoom 44, Gyarados 43, Dragonair 45')),
 'BOSS':    ('Boss', 'LEADER', 'GIOVANNI', P('Weezing 46, Arbok 46, Houndoom 47, Muk 47, Tyranitar 49')),
 'CHIEF_D': ('Chief', 'GENTLEMAN', 'GENTLEMAN', P('Murkrow 64, Crobat 65, Sneasel 65, Houndoom 66, Gyarados 66, Dragonite 68')),
 'CHIEF_F': ('Chief', 'GENTLEMAN', 'GENTLEMAN', P('Tyranitar 72, Crobat 70, Houndoom 71, Gyarados 71, Sneasel 70, Dragonite 74')),
 'NAMBA':   ('Namba', 'SCIENTIST', 'SCIENTIST_FRLG', P('Electabuzz 30, Magneton 30, Scizor 32')),
 'GARY':    ('Gary', 'RIVAL', 'RIVAL_LATE_FRLG', P('Blastoise 48')),
 'HARRISON':('Harrison', 'COOLTRAINER', 'COOLTRAINER_M', P('Blaziken 52')),
 'BRANDON': ('Brandon', 'PYRAMID_KING', 'PYRAMID_KING_BRANDON', P('Regirock 70, Regice 70, Registeel 70')),
}
TRI = {k: i for i, k in enumerate(TR)}

# ---- scenes: (id, story, map, flag, gates[], cond, steps) ----
# steps: 'L a-b' lines, 'B:x' story battle, 'XB:x' scripted-loss story battle,
# SJ11: Whirl Cup forced loss to Briana, Misty vs Katie, then the player's Briana rematch (Matt 10-23).
# 'S:TRAINER' stock battle, 'XS:TRAINER' scripted-loss stock battle, 'G:SPECIES,lvl',
# 'NOCTOWL', 'CZ_OUT', 'CZ_IN', 'I+:ITEM', 'I-:ITEM', 'PC', 'OPT:x' optional battle,
# 'IN:MAP,x,y' portal in, 'OUT:MAP' portal out, 'SET:FLAG'
C, S = 'ARENA_STORYLINE_CHIEF', 'ARENA_STORYLINE_SHOW'
B = lambda n: 'FLAG_BADGE0%d_GET' % n
SC = [
 ('J1', C, 'NEW_BARK_TOWN', 'FLAG_ARENA_CHIEF_J1', [], 0, ['L 0-7']),
 ('J2', C, 'ROUTE30', 'FLAG_ARENA_CHIEF_J2', ['FLAG_ARENA_CHIEF_J1'], 0, ['L 0-7']),
 ('J3', C, 'RUINS_OF_ALPH_PUZZLE_AND_REWARD_CHAMBERS', 'FLAG_ARENA_CHIEF_J3', ['FLAG_ARENA_CHIEF_J2', B(1)], 0, ['L 0-6']),
 ('J4', C, 'SLOWPOKE_WELL_B1F', 'FLAG_ARENA_CHIEF_J4', ['FLAG_ARENA_CHIEF_J3'], 0, ['L 0-7']),
 ('J5', C, 'ILEX_FOREST', 'FLAG_ARENA_CHIEF_J5', ['FLAG_ARENA_CHIEF_J4', B(2)], 0, ['L 0-2', 'B:CHIEF_A', 'L 3-5']),
 ('J6', C, 'GOLDENROD_CITY_UNDERGROUND_STORAGE', 'FLAG_ARENA_CHIEF_J6', ['FLAG_ARENA_CHIEF_J5', B(3)], 0, ['L 0-6']),
 ('J7', C, 'BURNED_TOWER_B1F', 'FLAG_ARENA_CHIEF_J7', ['FLAG_ARENA_CHIEF_J6'], 0, ['L 0-5']),
 ('J8', C, 'OLIVINE_CITY_CAFE', 'FLAG_ARENA_CHIEF_J8', [B(4)], 0, ['L 0-2', 'B:SAILOR', 'L 3-5']),
 ('J8', C, 'CIANWOOD_SHOP', 'FLAG_ARENA_CHIEF_J8_DELIVERY', ['FLAG_ARENA_CHIEF_J8'], 0, ['L 6-7']),
 ('J9', C, 'LAKE_OF_RAGE', 'FLAG_ARENA_CHIEF_J9', ['FLAG_ARENA_CHIEF_J8'], 0, ['L 0-5']),
 ('J10', C, 'GOLDENROD_CITY_RADIO_TOWER_5F', 'FLAG_ARENA_CHIEF_J10', ['FLAG_ARENA_CHIEF_J9'], 0, ['L 0-4', 'B:CHIEF_B', 'L 5-5']),
 ('J11', C, 'DRAGONS_DEN_SHRINE', 'FLAG_ARENA_CHIEF_J11', ['FLAG_ARENA_CHIEF_J10', B(7)], 0, ['L 0-5']),
 ('J12', C, 'VICTORY_ROAD_1F', 'FLAG_ARENA_CHIEF_J12', ['FLAG_ARENA_CHIEF_J11', B(8)], 0, ['L 0-2', 'B:CHIEF_C', 'L 3-4']),
 ('J13', C, 'SSAQUA_CAPTAINS_ROOM', 'FLAG_ARENA_CHIEF_J13', [], 0, ['L 0-4', 'B:BOSS', 'L 5-6']),
 ('J14', C, 'SAFFRON_CITY_SILPH_CO', 'FLAG_ARENA_CHIEF_J14', ['FLAG_ARENA_CHIEF_J13'], 0, ['L 0-6']),
 ('J15', C, 'SEAFOAM_ISLANDS_B1F', 'FLAG_ARENA_CHIEF_J15A', ['FLAG_ARENA_CHIEF_J14'], 0, ['L 0-1']),
 ('J15', C, 'CINNABAR_ISLAND', 'FLAG_ARENA_CHIEF_J15', ['FLAG_ARENA_CHIEF_J15A'], 0, ['L 2-6']),
 ('J16', C, 'MT_SILVER_3F', 'FLAG_ARENA_CHIEF_J16', ['FLAG_ARENA_CHIEF_J15'], 0, ['L 0-2', 'OPT:CHIEF_D', 'L 3-4']),
 ('J17', C, 'BATTLE_FRONTIER_RANKING_HALL', 'FLAG_ARENA_CHIEF_J17', ['FLAG_ARENA_CHIEF_J16'], 0, ['L 0-0', 'B:CHIEF_F', 'L 1-5']),
 ('J18', C, 'BATTLE_FRONTIER_OUTSIDE_WEST', 'FLAG_ARENA_CHIEF_J18', ['FLAG_ARENA_CHIEF_J17'], 0, ['L 0-5', 'SET:FLAG_ARENA_CHIEF_CH2_DONE']),
 # ---- Show ----
 ('SJ1', S, 'NEW_BARK_TOWN_PLAYERS_HOUSE_1F', 'FLAG_ARENA_SHOW_SJ1', [], 0, ['L 0-0', 'G:SPECIES_PIKACHU,5', 'L 1-6', 'FOLLOW']),
 ('SJ2', S, 'AZALEA_TOWN_KURTS_HOUSE', 'FLAG_ARENA_SHOW_SJ2', ['FLAG_ARENA_SHOW_SJ1'], 0, ['L 0-5']),
 ('SJ3', S, 'ROUTE29', 'FLAG_ARENA_SHOW_SJ3A', ['FLAG_ARENA_SHOW_SJ1'], 0, ['L 0-1', 'G:SPECIES_CHIKORITA,5']),
 ('SJ3', S, 'ROUTE30', 'FLAG_ARENA_SHOW_SJ3B', ['FLAG_ARENA_SHOW_SJ1'], 0, ['L 2-3', 'G:SPECIES_CYNDAQUIL,6']),
 ('SJ3', S, 'ROUTE31', 'FLAG_ARENA_SHOW_SJ3C', ['FLAG_ARENA_SHOW_SJ1'], 0, ['L 4-5', 'G:SPECIES_TOTODILE,7']),
 ('SJ4', S, 'ROUTE30', 'FLAG_ARENA_SHOW_SJ4', ['FLAG_ARENA_SHOW_SJ3B'], 0, ['L 0-5', 'G:SPECIES_HERACROSS,9']),
 ('SJ5', S, 'ROUTE37', 'FLAG_ARENA_SHOW_SJ5', ['FLAG_ARENA_SHOW_SJ1', B(3)], 1, ['L 0-3', 'NOCTOWL', 'L 4-4']),
 ('SJ6', S, 'GATE_ROUTE29_ROUTE46', None, ['FLAG_ARENA_SHOW_SJ1'], 2, ['IN:ARENA_CHARICIFIC_VALLEY,10,38']),
 ('SJ6', S, 'ARENA_CHARICIFIC_VALLEY', 'FLAG_ARENA_SHOW_SJ6', [], 0, ['L 0-5', 'CZ_OUT', 'OUT:GATE_ROUTE29_ROUTE46']),
 ('SJ8', S, 'RUINS_OF_ALPH_PUZZLE_AND_REWARD_CHAMBERS', 'FLAG_ARENA_SHOW_SJ8', ['FLAG_ARENA_SHOW_SJ4'], 0, ['L 0-4']),
 ('SJ9', S, 'BURNED_TOWER_B1F', 'FLAG_ARENA_SHOW_SJ9', ['FLAG_ARENA_SHOW_SJ8', B(3)], 0, ['L 0-4']),
 ('SJ10', S, 'OLIVINE_CITY_LIGHTHOUSE', 'FLAG_ARENA_SHOW_SJ10A', [], 0, ['L 0-2']),
 ('SJ10', S, 'CIANWOOD_SHOP', 'FLAG_ARENA_SHOW_SJ10B', ['FLAG_ARENA_SHOW_SJ10A'], 0, ['L 3-3', 'I+:ITEM_SECRET_POTION']),
 ('SJ10', S, 'OLIVINE_CITY_LIGHTHOUSE', 'FLAG_ARENA_SHOW_SJ10', ['FLAG_ARENA_SHOW_SJ10B'], 0, ['I-:ITEM_SECRET_POTION', 'L 4-5']),
 ('SJ11', S, 'WHIRL_ISLANDS_1F', None, ['FLAG_ARENA_SHOW_SJ10'], 0, ['IN:ARENA_WHIRL_CUP,9,5']),
 ('SJ11', S, 'ARENA_WHIRL_CUP', 'FLAG_ARENA_SHOW_SJ11', [], 0,
    ['L 0-0', 'XS:TRAINER_BRIANA', 'L 1-1', 'S:TRAINER_KATIE', 'S:TRAINER_BRIANA', 'L 2-3', 'B:NAMBA', 'L 4-5', 'OUT:WHIRL_ISLANDS_1F']),
 ('SJ12', S, 'LAKE_OF_RAGE', 'FLAG_ARENA_SHOW_SJ12', ['FLAG_ARENA_SHOW_SJ11'], 0, ['L 0-3']),
 ('SJ13', S, 'INDIGO_PLATEAU_POKEMON_CENTER', None, ['FLAG_ARENA_SHOW_SJ12', B(8)], 0, ['IN:ARENA_SILVER_CONFERENCE,9,5']),
 ('SJ13', S, 'ARENA_SILVER_CONFERENCE', 'FLAG_ARENA_SHOW_SJ13', [], 0,
    ['L 0-0', 'CZ_IN', 'L 1-1', 'B:GARY', 'L 2-4', 'XB:HARRISON', 'B:HARRISON', 'L 5-6', 'SET:FLAG_ARENA_SHOW_P2_END', 'OUT:INDIGO_PLATEAU_POKEMON_CENTER']),
 ('SJ14', S, 'PALLET_TOWN_REDS_HOUSE_1F', 'FLAG_ARENA_SHOW_SJ14', [], 0, ['L 0-4']),
]
LOBBY = [('BATTLE_TOWER_LOBBY', 'TOWER'), ('BATTLE_DOME_LOBBY', 'DOME'), ('BATTLE_PALACE_LOBBY', 'PALACE'),
         ('BATTLE_ARENA_LOBBY', 'ARENA'), ('BATTLE_FACTORY_LOBBY', 'FACTORY'), ('BATTLE_PIKE_LOBBY', 'PIKE'),
         ('BATTLE_PYRAMID_LOBBY', 'PYRAMID')]
for i, (m, f) in enumerate(LOBBY):
    SC.append(('SJ15', S, 'BATTLE_FRONTIER_' + m, 'FLAG_ARENA_SHOW_SJ15_%d' % (i + 1), ['FLAG_ARENA_SHOW_SJ14', 'FLAG_SYS_%s_SILVER' % f], 0, ['L 0-3']))
SC += [
 ('SJ16', S, 'BATTLE_FRONTIER_BATTLE_PYRAMID_LOBBY', 'FLAG_ARENA_SHOW_SJ16', ['FLAG_ARENA_SHOW_SJ15_7'], 0,
    ['L 0-0', 'XB:BRANDON', 'L 1-2', 'B:BRANDON', 'L 3-3']),
 ('SJ17', S, 'PALLET_TOWN_LAB', 'FLAG_ARENA_SHOW_SJ17', ['FLAG_ARENA_SHOW_SJ14'], 3, ['L 0-1', 'PC', 'L 2-3']),
 ('SJ18', S, 'BATTLE_FRONTIER_OUTSIDE_EAST', 'FLAG_ARENA_SHOW_SJ18', ['FLAG_ARENA_SHOW_SJ14'], 4, ['L 0-5', 'SET:FLAG_ARENA_SHOW_CH2_DONE']),
]
# SJ7 is wired into the stock Whitney gym script (scripted loss then stock rematch); see arena_story_scenes.inc.

def esc(t): return t.replace('"', '\\"')
def text(line):
    sp, t = line.split('|', 1)
    sp = '{PLAYER}' if sp == 'PLAYER' else sp.title() if sp not in ('BOSS',) else 'Boss'
    if len(t) > 35: sys.exit('over 35: ' + line)
    return '%s:\\n%s$' % (sp, esc(t))

asm, texts = [], []
def emit_lines(sid, rng, lab):
    a, b = map(int, rng.split('-'))
    for i in range(a, b + 1):
        tl = '%s_T%d' % (lab, i)
        if tl not in seen: seen.add(tl); texts.append('%s:\n\t.string "%s"\n' % (tl, text(LINES[sid][i])))
        asm.append('\tmsgbox %s, MSGBOX_DEFAULT' % tl)
seen = set()
def battle(trainer, loss, story=None):
    if story is not None:
        asm.append('\tsetvar VAR_0x8004, %d' % TRI[story])
        asm.append('\tspecial ArenaStory_PrepBattle')
    asm.append('\tcleartrainerflag %s' % trainer)
    if loss: asm.append('\tsetflag FLAG_ARENA_NO_WHITEOUT')
    asm.append('\ttrainerbattle_no_intro %s, ArenaStory_Text_Lose' % trainer)
    if loss:
        asm.append('\tclearflag FLAG_ARENA_NO_WHITEOUT')
        asm.append('\tspecial HealPlayerParty')

ROWS = {1: [], 2: [], 3: []}
for n, (sid, story, mp, flag, gates, cond, steps) in enumerate(SC):
    lab = 'ArenaStory_%s_%d' % (sid, n)
    asm.append('%s::' % lab); asm.append('\tlockall')
    for st in steps:
        k, _, v = st.partition(':') if not st.startswith('L ') else ('L', '', st[2:])
        if k == 'L': emit_lines(sid, v, 'ArenaStory_%s' % sid)
        elif k == 'B': battle('TRAINER_ARENA_STORY', False, v)
        elif k == 'XB': battle('TRAINER_ARENA_STORY', True, v)
        elif k == 'S': battle(v, False)
        elif k == 'XS': battle(v, True)
        elif k == 'OPT':
            asm += ['\tmsgbox ArenaStory_Text_Fight, MSGBOX_YESNO', '\tgoto_if_eq VAR_RESULT, NO, %s_skip' % lab]
            battle('TRAINER_ARENA_STORY', False, v)
            asm += ['\tsetflag FLAG_ARENA_CHIEF_J16_WON', '%s_skip:' % lab]
        elif k == 'G':
            sp, lv = v.split(','); asm += ['\tgivemon %s, %s' % (sp, lv)]
        elif k == 'FOLLOW':  # D2: Misty joins as the NPC follower (FNPC supports one; Brock stays narrative)
            asm += ['.if FNPC_ENABLE_NPC_FOLLOWERS', '\tsetfollowernpc 3, FNPC_ALL', '.endif']
        elif k == 'NOCTOWL':
            asm += ['\tsetflag FLAG_SET_SHINY', '\tsetwildbattle SPECIES_NOCTOWL, 18', '\tdowildbattle',
                    '\tspecial ArenaStory_WasCaught', '\tgoto_if_eq VAR_RESULT, TRUE, %s_c' % lab,
                    '\tgivemon SPECIES_NOCTOWL, 18', '%s_c:' % lab, '\tclearflag FLAG_SET_SHINY']
        elif k == 'CZ_OUT': asm.append('\tspecial ArenaStory_CharizardToLiza')
        elif k == 'CZ_IN': asm.append('\tspecial ArenaStory_CharizardFromLiza')
        elif k == 'I+': asm.append('\tgiveitem %s' % v)
        elif k == 'I-': asm.append('\tremoveitem %s' % v)
        elif k == 'PC': asm += ['\tspecial ShowPokemonStorageSystemPC', '\twaitstate']
        elif k == 'SET': asm.append('\tsetflag %s' % v)
        elif k == 'IN':
            m, x, y = v.split(',')
            asm += ['\tspecial ArenaStory_SaveReturn', '\treleaseall', '\twarp MAP_%s, %s, %s' % (m, x, y), '\twaitstate']
        elif k == 'OUT':
            asm += ['\tsetflag %s' % flag, '\tspecial ArenaStory_LoadReturn', '\treleaseall',
                    '\tgoto_if_eq VAR_RESULT, FALSE, %s_w0' % lab,
                    '\twarp MAP_%s, VAR_0x8005, VAR_0x8006' % v, '\twaitstate',
                    '%s_w0:' % lab, '\twarp MAP_%s, 0' % v, '\twaitstate']
        else: sys.exit('bad step ' + st)
    if not any(s.startswith(('IN:', 'OUT:')) for s in steps):
        asm += ['\tsetflag %s' % flag, '\treleaseall', '\tend']
    asm.append('')
    ROWS[PART[sid]].append('    {MAP_%s, %s, %s, %s, %s, %d, %s},' % (
        mp, story, flag or 'FLAG_SHOW_' + sid, gates[0] if gates else '0', gates[1] if len(gates) > 1 else '0', cond, lab))

# ---- write ----
h = ['// Generated by tools/arena/gen_story_scenes.py from Shorty\'s gen.py. Do not edit.',
     '#ifndef GUARD_ARENA_STORY_FLAGS_H', '#define GUARD_ARENA_STORY_FLAGS_H']
for f, v in zip(FLAGS, FREE): h.append('#define %-28s 0x%X' % (f, v))
h.append('#define TRAINER_ARENA_STORY          TRAINER_CHANSEY3')
h.append('#define ARENA_STORY_TRAINER_COUNT    %d' % len(TR))
h.append('#endif')
flags_path = os.path.join(ROOT, 'include/constants/arena_story_flags.h')
header_path = os.path.join(ROOT, 'src/data/arena_story_scenes.h')
inc_path = os.path.join(ROOT, 'data/scripts/arena_story_scenes.inc')
prev_inc = open(inc_path).read() if os.path.exists(inc_path) else ''
open(flags_path, 'w').write(preserve_story_flags('\n'.join(h) + '\n'))

c = ['// Generated by tools/arena/gen_story_scenes.py. Do not edit.']
for k, (nm, cl, pic, party) in TR.items():
    c.append('static const struct TrainerMon sArenaParty_%s[] = {' % k)
    for sp, lv in party:
        c.append('    {.species = %s, .lvl = %d, .gender = TRAINER_MON_RANDOM_GENDER, .ball = POKEBALL_COUNT, .nature = NATURE_HARDY, .dynamaxLevel = MAX_DYNAMAX_LEVEL},' % (sp, lv))
    c.append('};')
c.append('static const struct Trainer sArenaStoryTrainers[ARENA_STORY_TRAINER_COUNT] = {')
for k, (nm, cl, pic, party) in TR.items():
    c.append('    {.trainerName = _("%s"), .trainerClass = TRAINER_CLASS_%s, .trainerPic = TRAINER_PIC_%s, .encounterMusic = TRAINER_ENCOUNTER_MUSIC_MALE, .battleType = TRAINER_BATTLE_TYPE_SINGLES, .aiFlags = AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_TRY_TO_FAINT, .multiTeamSize = MULTI_TEAM_SIZE_FULL, .party = sArenaParty_%s, .partySize = ARRAY_COUNT(sArenaParty_%s)},' % (nm, cl, pic, k, k))
c.append('};')
for p in (1, 2, 3):
    c.append('#if MATT_PART == %d' % p)
    for r in ROWS[p]:
        lab = r.rsplit(', ', 1)[1].rstrip('},')
        c.append('extern const u8 %s[];' % lab)
    c.append('static const struct ArenaScene sArenaScenes[] = {'); c += ROWS[p]; c.append('};'); c.append('#endif')
open(header_path, 'w').write(preserve_story_header('\n'.join(c) + '\n'))

out = ['@ Generated by tools/arena/gen_story_scenes.py. Do not edit.']
for p in (1, 2, 3):
    pass
# All scene scripts are assembled into every part (only the part's table references them);
# scripts for other parts are wrapped in .if MATT_PART guards to save ROM.
body = '\n'.join(asm)
blocks = body.split('\n\n')
out2 = []
for b in blocks:
    if not b.strip(): continue
    m = re.match(r'ArenaStory_(S?J\d+)_\d+::', b)
    out2.append('.if MATT_PART == %d\n%s\n.endif\n' % (PART[m.group(1)], b))
tt = []
for t in texts:
    m = re.match(r'ArenaStory_(S?J\d+)_T', t)
    tt.append('.if MATT_PART == %d\n%s.endif\n' % (PART[m.group(1)], t))
L7 = LINES['SJ7']
sj7 = """@ D2: Misty object in the player's house is shown only for Show before SJ1.
ArenaStory_MistyOnTransition::
	setflag FLAG_ARENA_HIDE_MISTY
	goto_if_set FLAG_ARENA_SHOW_SJ1, ArenaStory_SJ7_Ret
	callnative ArenaStory_GetStoryline
	goto_if_ne VAR_RESULT, ARENA_STORYLINE_SHOW, ArenaStory_SJ7_Ret
	clearflag FLAG_ARENA_HIDE_MISTY
	return
@ SJ7 (Show, P1): wired into the stock Whitney script (GoldenrodCity_Gym).
ArenaStory_SJ7_Pre::
	callnative ArenaStory_GetStoryline
	goto_if_ne VAR_RESULT, ARENA_STORYLINE_SHOW, ArenaStory_SJ7_Ret
	goto_if_set FLAG_ARENA_SHOW_SJ7_LOSS, ArenaStory_SJ7_Rematch
	msgbox ArenaStory_SJ7_T0, MSGBOX_DEFAULT
	cleartrainerflag TRAINER_WHITNEY_1
	setflag FLAG_ARENA_NO_WHITEOUT
	trainerbattle_no_intro TRAINER_WHITNEY_1, ArenaStory_Text_Lose
	clearflag FLAG_ARENA_NO_WHITEOUT
	special HealPlayerParty
	cleartrainerflag TRAINER_WHITNEY_1
	msgbox ArenaStory_SJ7_T1, MSGBOX_DEFAULT
	msgbox ArenaStory_SJ7_T2, MSGBOX_DEFAULT
	setflag FLAG_ARENA_SHOW_SJ7_LOSS
	release
	end
ArenaStory_SJ7_Rematch::
	msgbox ArenaStory_SJ7_T3, MSGBOX_DEFAULT
ArenaStory_SJ7_Ret::
	return
ArenaStory_SJ7_PostWin::
	callnative ArenaStory_GetStoryline
	goto_if_ne VAR_RESULT, ARENA_STORYLINE_SHOW, ArenaStory_SJ7_Ret
	msgbox ArenaStory_SJ7_T4, MSGBOX_DEFAULT
	return
ArenaStory_SJ7_Badge::
	callnative ArenaStory_GetStoryline
	goto_if_ne VAR_RESULT, ARENA_STORYLINE_SHOW, ArenaStory_SJ7_Ret
	msgbox ArenaStory_SJ7_T5, MSGBOX_DEFAULT
	setflag FLAG_ARENA_SHOW_SJ7
	return
""" + ''.join('ArenaStory_SJ7_T%d:\n\t.string "%s"\n' % (i, text(l)) for i, l in enumerate(L7))
out += [sj7]
out += out2 + tt + ['ArenaStory_Text_Lose:\n\t.string "...$"\n', 'ArenaStory_Text_Fight:\n\t.string "Fight the Chief?$"\n']
open(inc_path, 'w').write(inject_movie_hooks(reattach_hand_scenes(prev_inc, '\n'.join(out) + '\n')))
print('scenes', {p: len(ROWS[p]) for p in ROWS}, 'flags', len(FLAGS), 'trainers', len(TR))
