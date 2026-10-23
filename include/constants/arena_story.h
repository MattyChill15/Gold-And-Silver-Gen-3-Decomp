#ifndef GUARD_CONSTANTS_ARENA_STORY_H
#define GUARD_CONSTANTS_ARENA_STORY_H
// Storyline selector (2 bits: FLAG_ARENA_STORY_BIT0/BIT1). Classic = stock G/S scripts.
#define ARENA_STORYLINE_CLASSIC 0
#define ARENA_STORYLINE_CHIEF   1
#define ARENA_STORYLINE_SHOW    2 // anime-based Show Story (content pending)
#define ARENA_STORYLINE_COUNT   3
// Hook slots for storyline scenes (content written separately by Shorty).
// A map script calls:  setvar VAR_0x8004, ARENA_STORY_HOOK_x / special ArenaStory_TryHook
// VAR_RESULT = TRUE when a storyline scene ran (script should skip its Classic scene).
#define ARENA_STORY_HOOK_NEW_GAME          0
#define ARENA_STORY_HOOK_NEW_BARK_START    1
#define ARENA_STORY_HOOK_ELM_STARTER       2
#define ARENA_STORY_HOOK_RIVAL_1           3
#define ARENA_STORY_HOOK_GYM_BADGE         4  // VAR_0x8005 = badge index 0..7 (Johto), 8..15 (Kanto)
#define ARENA_STORY_HOOK_TEAM_ROCKET_HQ    5
#define ARENA_STORY_HOOK_RADIO_TOWER       6
#define ARENA_STORY_HOOK_ELITE_FOUR_ENTRY  7
#define ARENA_STORY_HOOK_LANCE_DEFEATED    8  // Part 1 ending
#define ARENA_STORY_HOOK_PART2_START       9
#define ARENA_STORY_HOOK_KANTO_ARRIVAL     10
#define ARENA_STORY_HOOK_RED_DEFEATED      11
#define ARENA_STORY_HOOK_FRONTIER_ENTRY    12
#define ARENA_STORY_HOOK_SERIES_HANDOFF    13 // before writing the Johto -> Emerald record
#define ARENA_STORY_HOOK_COUNT             14
#endif
