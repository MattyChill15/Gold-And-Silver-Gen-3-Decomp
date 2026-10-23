#ifndef GUARD_ARENA_STORY_H
#define GUARD_ARENA_STORY_H
#include "constants/arena_story.h"
u8 ArenaStory_Get(void);            // ARENA_STORYLINE_*
void ArenaStory_Set(u8 storyline);
bool8 ArenaStory_IsChief(void);
void ArenaStory_SetChief(bool8 chief);
#endif
