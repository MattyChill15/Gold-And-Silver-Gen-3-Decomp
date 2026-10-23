#include "global.h"
#include "event_data.h"
#include "script.h"
#include "constants/arena_story.h"

// Storyline scene slots, one table per non-Classic storyline. All NULL until
// Shorty's scenes land; with a NULL slot (or Classic selected) the native stock
// Gold/Silver script runs unchanged.
static const u8 *const sChiefStoryScenes[ARENA_STORY_HOOK_COUNT] = {0};
static const u8 *const sShowStoryScenes[ARENA_STORY_HOOK_COUNT] = {0};

static const u8 *const *const sStoryScenes[ARENA_STORYLINE_COUNT] = {
    [ARENA_STORYLINE_CLASSIC] = NULL,
    [ARENA_STORYLINE_CHIEF]   = sChiefStoryScenes,
    [ARENA_STORYLINE_SHOW]    = sShowStoryScenes,
};

u8 ArenaStory_Get(void)
{
    u8 v = FlagGet(FLAG_ARENA_STORY_BIT0) | (FlagGet(FLAG_ARENA_STORY_BIT1) << 1);
    return v < ARENA_STORYLINE_COUNT ? v : ARENA_STORYLINE_CLASSIC;
}

void ArenaStory_Set(u8 storyline)
{
    if (storyline >= ARENA_STORYLINE_COUNT)
        storyline = ARENA_STORYLINE_CLASSIC;
    if (storyline & 1) FlagSet(FLAG_ARENA_STORY_BIT0); else FlagClear(FLAG_ARENA_STORY_BIT0);
    if (storyline & 2) FlagSet(FLAG_ARENA_STORY_BIT1); else FlagClear(FLAG_ARENA_STORY_BIT1);
}

bool8 ArenaStory_IsChief(void)
{
    return ArenaStory_Get() == ARENA_STORYLINE_CHIEF;
}

void ArenaStory_SetChief(bool8 chief)
{
    ArenaStory_Set(chief ? ARENA_STORYLINE_CHIEF : ARENA_STORYLINE_CLASSIC);
}

// Usage in a map script:
//   setvar VAR_0x8004, ARENA_STORY_HOOK_x
//   callnative ArenaStory_TryHook
//   goto_if_eq VAR_RESULT, TRUE, <skip Classic scene>
// VAR_RESULT = TRUE if a storyline scene was called (it returns to the caller).
void ArenaStory_TryHook(struct ScriptContext *ctx)
{
    u16 hook = gSpecialVar_0x8004;
    const u8 *const *scenes = sStoryScenes[ArenaStory_Get()];
    gSpecialVar_Result = FALSE;
    if (scenes == NULL || hook >= ARENA_STORY_HOOK_COUNT || scenes[hook] == NULL)
        return;
    ScriptCall(ctx, scenes[hook]);
    gSpecialVar_Result = TRUE;
}

// Script special: VAR_RESULT = current storyline (ARENA_STORYLINE_*).
void ArenaStory_GetStoryline(void)
{
    gSpecialVar_Result = ArenaStory_Get();
}

// Demo/practice preset: Johto keeps the stock New Bark intro flags.
void ArenaJohto_DemoIntroFlags(void)
{
}
