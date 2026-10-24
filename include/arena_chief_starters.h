#ifndef GUARD_ARENA_CHIEF_STARTERS_H
#define GUARD_ARENA_CHIEF_STARTERS_H
struct Pokemon;
bool8 ArenaChief_Active(void);
u16 ArenaChief_StarterSpecies(u16 species);
void ArenaChief_StarterVars(void);
void ArenaChief_RemapTrainerParty(struct Pokemon *party, u8 count, u8 trainerClass);
#endif
