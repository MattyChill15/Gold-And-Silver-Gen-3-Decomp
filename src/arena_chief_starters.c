// Matt: Chief Story starters (Chief storyline only; Classic/Show unchanged).
#include "global.h"
#include "pokemon.h"
#include "event_data.h"
#include "arena_story.h"
#include "constants/arena_story.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "arena_chief_starters.h"

struct ChiefRival { u16 from, to, minLevel, low; };
static const u16 sChiefStarters[][2] = {
    {SPECIES_CHIKORITA, SPECIES_SNEASEL},
    {SPECIES_CYNDAQUIL, SPECIES_HOUNDOUR},
    {SPECIES_TOTODILE, SPECIES_DRATINI},
};
static const struct ChiefRival sChiefRival[] = {
    {SPECIES_CHIKORITA, SPECIES_SNEASEL, 0, SPECIES_SNEASEL},
    {SPECIES_BAYLEEF, SPECIES_SNEASEL, 0, SPECIES_SNEASEL},
    {SPECIES_MEGANIUM, SPECIES_WEAVILE, 0, SPECIES_WEAVILE},
    {SPECIES_CYNDAQUIL, SPECIES_HOUNDOUR, 0, SPECIES_HOUNDOUR},
    {SPECIES_QUILAVA, SPECIES_HOUNDOOM, 24, SPECIES_HOUNDOUR},
    {SPECIES_TYPHLOSION, SPECIES_HOUNDOOM, 0, SPECIES_HOUNDOOM},
    {SPECIES_TOTODILE, SPECIES_DRATINI, 0, SPECIES_DRATINI},
    {SPECIES_CROCONAW, SPECIES_DRAGONAIR, 30, SPECIES_DRATINI},
    {SPECIES_FERALIGATR, SPECIES_DRAGONITE, 55, SPECIES_DRAGONAIR},
};

bool8 ArenaChief_Active(void) { return (ArenaStory_Get() == ARENA_STORYLINE_CHIEF); }

u16 ArenaChief_StarterSpecies(u16 species)
{
    u32 i;
    if (!ArenaChief_Active())
        return species;
    for (i = 0; i < ARRAY_COUNT(sChiefStarters); i++)
        if (sChiefStarters[i][0] == species)
            return sChiefStarters[i][1];
    return species;
}

// callnative: remap VAR_TEMP_2 (player starter) / VAR_TEMP_3 (rival starter); VAR_RESULT = Chief?
void ArenaChief_StarterVars(void)
{
    gSpecialVar_Result = ArenaChief_Active();
    if (!gSpecialVar_Result)
        return;
    VarSet(VAR_TEMP_2, ArenaChief_StarterSpecies(VarGet(VAR_TEMP_2)));
    VarSet(VAR_TEMP_3, ArenaChief_StarterSpecies(VarGet(VAR_TEMP_3)));
}

static void ChiefRebuildMon(struct Pokemon *mon, u16 species)
{
    u32 i, ivs[NUM_STATS];
    u8 level = GetMonData(mon, MON_DATA_LEVEL);
    u32 pid = GetMonData(mon, MON_DATA_PERSONALITY);
    u16 item = GetMonData(mon, MON_DATA_HELD_ITEM);
    for (i = 0; i < NUM_STATS; i++)
        ivs[i] = GetMonData(mon, MON_DATA_HP_IV + i);
    CreateMon(mon, species, level, pid, OTID_STRUCT_RANDOM_NO_SHINY);
    for (i = 0; i < NUM_STATS; i++)
        SetMonData(mon, MON_DATA_HP_IV + i, &ivs[i]);
    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
    CalculateMonStats(mon);
}

// Rival parties: swap the default starter line for the Chief line (level-legal stage, default level-up moves).
void ArenaChief_RemapTrainerParty(struct Pokemon *party, u8 count, u8 trainerClass)
{
    u32 i, j;
    if (!ArenaChief_Active() || !(trainerClass == TRAINER_CLASS_RIVAL))
        return;
    for (i = 0; i < count && i < PARTY_SIZE; i++)
    {
        u16 sp = GetMonData(&party[i], MON_DATA_SPECIES);
        u8 lv = GetMonData(&party[i], MON_DATA_LEVEL);
        for (j = 0; j < ARRAY_COUNT(sChiefRival); j++)
        {
            if (sChiefRival[j].from == sp)
            {
                ChiefRebuildMon(&party[i], lv >= sChiefRival[j].minLevel ? sChiefRival[j].to : sChiefRival[j].low);
                break;
            }
        }
    }
}
