#include "global.h"
#include "event_data.h"
#include "script.h"
#include "constants/arena_story.h"

// Chief Story scene slots. All NULL until Shorty's scenes land; with a NULL
// slot (or Classic selected) the native stock Gold/Silver script runs unchanged.
static const u8 *const sChiefStoryScenes[ARENA_STORY_HOOK_COUNT] = {0};

bool8 ArenaStory_IsChief(void)
{
    return FlagGet(FLAG_ARENA_CHIEF_STORY);
}

void ArenaStory_SetChief(bool8 chief)
{
    if (chief) FlagSet(FLAG_ARENA_CHIEF_STORY);
    else FlagClear(FLAG_ARENA_CHIEF_STORY);
}

// Usage in a map script:
//   setvar VAR_0x8004, ARENA_STORY_HOOK_x
//   callnative ArenaStory_TryHook
//   goto_if_eq VAR_RESULT, TRUE, <skip Classic scene>
// VAR_RESULT = TRUE if a Chief scene was called (it returns to the caller).
void ArenaStory_TryHook(struct ScriptContext *ctx)
{
    u16 hook = gSpecialVar_0x8004;
    gSpecialVar_Result = FALSE;
    if (!ArenaStory_IsChief() || hook >= ARENA_STORY_HOOK_COUNT || sChiefStoryScenes[hook] == NULL)
        return;
    ScriptCall(ctx, sChiefStoryScenes[hook]);
    gSpecialVar_Result = TRUE;
}

// Demo/practice preset: Johto keeps the stock New Bark intro flags.
void ArenaJohto_DemoIntroFlags(void)
{
}
