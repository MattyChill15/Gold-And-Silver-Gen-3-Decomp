// Arena series save hub (see docs/SERIES_SAVE.md). Shared verbatim between
// the FireRed (chapter 1) and Emerald (chapter 3) trees.
#include "global.h"
#include "fieldmap.h"
#include "arena_series.h"
#include "save.h"
#include "load_save.h"
#ifdef FIRERED
#include "gba/flash_internal.h"
#else
#include "agb_flash.h"
#include "matt_config.h"
void ReadFlash(u16 sectorNum, u32 offset, u8 *dest, u32 size);
#endif
#include "event_data.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "pokedex.h"
#include "item.h"
#include "money.h"
#include "malloc.h"
#include "random.h"
#include "string_util.h"
#include "constants/items.h"
#include "constants/flags.h"
#include "constants/moves.h"
#include "constants/trainers.h"

#ifndef MATT_PART
#define MATT_PART 1
#endif
#ifdef FIRERED
#define ARENA_GAME_STAMP ARENA_GAME_STAMP_FIRERED
#if MATT_PART == 2
#define ARENA_SEG ARENA_SEG_FIRERED
#define ARENA_CHAPTER_CLEAR() FlagGet(FLAG_SYS_GAME_CLEAR)
#else
#define ARENA_SEG ARENA_SEG_FIRERED_P1
#define ARENA_CHAPTER_CLEAR() FlagGet(FLAG_MATT_PART1_COMPLETE) /* Part 1 ends at Erika */
#endif
#define ARENA_DEX_REWARD_FLAG FLAG_HIDDEN_ITEM_UNUSED_0x07
#define ARENA_SCALING_ACTIVE() FALSE /* chapter 1: never scales story levels */
static const u8 sChapterText[] = _("CHAPTER 1 COMPLETE!");
#else
#define ARENA_GAME_STAMP ARENA_GAME_STAMP_EMERALD
#if MATT_PART == 3
#define ARENA_SEG ARENA_SEG_EMERALD_P3
#define ARENA_CHAPTER_CLEAR() FlagGet(FLAG_SYS_GAME_CLEAR)
#elif MATT_PART == 2
#define ARENA_SEG ARENA_SEG_EMERALD_P2
#define ARENA_CHAPTER_CLEAR() FlagGet(FLAG_MATT_PART2_COMPLETE) /* Part 2 ends at Tate & Liza */
#else
#define ARENA_SEG ARENA_SEG_EMERALD_P1
#define ARENA_CHAPTER_CLEAR() FlagGet(FLAG_MATT_PART1_COMPLETE) /* Part 1 ends at Winona */
#endif
#define ARENA_DEX_REWARD_FLAG FLAG_ARENA_DEX_REWARD
#define ARENA_SCALING_ACTIVE() FlagGet(FLAG_ARENA_SERIES_IMPORTED)
static const u8 sChapterText[] = _("CHAPTER 3 COMPLETE!");
#endif

STATIC_ASSERT(sizeof(struct ArenaSeriesRecord) == 0x2F4, ArenaSeriesRecordSize);

EWRAM_DATA const u8 *const *gArenaSeriesToast = NULL;
extern u32 gSaveCounter;
EWRAM_DATA u8 gArenaSeriesDexDirty = 0;
EWRAM_DATA u8 gArenaRematchPending = 0;
EWRAM_DATA u8 gArenaSeriesImportPending = 0;

static const u8 sDexToast1[] = _("SERIES DEX COMPLETE 386");
static const u8 sDexToast2[] = _("+10 RARE CANDY");
static const u8 sDexToast3[] = _("+1 LUCKY EGG");
static const u8 *const sDexToast[] = {sDexToast1, sDexToast2, sDexToast3};

static u32 RecordChecksum(const struct ArenaSeriesRecord *r)
{
    const u32 *w = (const u32 *)r;
    u32 i, sum = 0;
    for (i = 0; i < offsetof(struct ArenaSeriesRecord, checksum) / 4; i++)
        sum += w[i];
    return sum;
}

static struct ArenaSeriesRecord *SectorRecord(void)
{
    return (struct ArenaSeriesRecord *)&gSaveDataBuffer.data[4];
}

// Reads flash sector 31 into gSaveDataBuffer; TRUE if it holds a valid record.
static bool8 LoadSectorRecord(void)
{
    struct ArenaSeriesRecord *r = SectorRecord();
    if (gFlashMemoryPresent != TRUE)
        return FALSE;
    ReadFlash(ARENA_SERIES_SECTOR, 0, (u8 *)&gSaveDataBuffer, SECTOR_SIZE);
    return *(u32 *)gSaveDataBuffer.data == SPECIAL_SECTOR_SENTINEL
        && r->magic == ARENA_SERIES_MAGIC && r->version >= 1
        && r->size >= sizeof(struct ArenaSeriesRecord)
        && r->checksum == RecordChecksum(r);
}

bool8 ArenaSeries_Read(struct ArenaSeriesRecord *dst)
{
    if (!LoadSectorRecord())
        return FALSE;
    memcpy(dst, SectorRecord(), sizeof(*dst));
    return TRUE;
}

static u8 CountBadges(void)
{
    u8 i, n = 0;
    for (i = 0; i < 8; i++)
        if (FlagGet(FLAG_BADGE01_GET + i))
            n++;
    return n;
}

bool8 ArenaSeries_WriteChapterComplete(void)
{
    struct ArenaSeriesRecord *r = SectorRecord();
    u32 i;
    if (!LoadSectorRecord())
        memset(&gSaveDataBuffer, 0, sizeof(gSaveDataBuffer));
    *(u32 *)gSaveDataBuffer.data = SPECIAL_SECTOR_SENTINEL;
    r->magic = ARENA_SERIES_MAGIC;
    r->version = ARENA_SERIES_VERSION;
    r->size = sizeof(struct ArenaSeriesRecord);
    r->sourceSegment = ARENA_SEG;
    if (ARENA_CHAPTER_CLEAR())
        r->segmentsComplete |= 1 << (ARENA_SEG - 1);
    r->badges[ARENA_SEG - 1] = CountBadges();
    r->money = GetMoney(&gSaveBlock1Ptr->money);
    r->playerGender = gSaveBlock2Ptr->playerGender;
    memcpy(r->playerName, gSaveBlock2Ptr->playerName, 8);
    memcpy(r->trainerId, gSaveBlock2Ptr->playerTrainerId, 4);
    if (FlagGet(ARENA_DEX_REWARD_FLAG))
        r->seriesFlags |= ARENA_SERIES_FLAG_DEX_REWARD;
#ifdef FIRERED
    r->seriesFlags &= ~(ARENA_SERIES_FLAG_YELLOW | ARENA_SERIES_FLAG_CHIEF_STORY);
    if (VarGet(VAR_ARENA_GAME_MODE) == ARENA_GAME_MODE_YELLOW)
        r->seriesFlags |= ARENA_SERIES_FLAG_YELLOW;
    if (VarGet(VAR_ARENA_STORYLINE) == 1)
        r->seriesFlags |= ARENA_SERIES_FLAG_CHIEF_STORY;
#endif
    for (i = 0; i < ARENA_SERIES_DEX_BYTES; i++)
    {
        r->dexSeen[i] |= gSaveBlock1Ptr->dexSeen[i];
        r->dexCaught[i] |= gSaveBlock1Ptr->dexCaught[i];
    }
    r->partyCount = gPlayerPartyCount;
    for (i = 0; i < PARTY_SIZE; i++)
        r->party[i] = gPlayerParty[i];
    r->writeCount++;
    r->saveCounter = gSaveCounter;
    r->checksum = RecordChecksum(r);
    return ProgramFlashSectorAndVerify(ARENA_SERIES_SECTOR, (u8 *)&gSaveDataBuffer) == 0;
}

// Called after every successful normal/HoF save once the chapter is clear,
// so the hub always mirrors the newest team, boxes slot and dex.
void ArenaSeries_OnSaved(u8 saveType)
{
    if ((saveType == SAVE_NORMAL || saveType == SAVE_HALL_OF_FAME) && ARENA_CHAPTER_CLEAR())
        ArenaSeries_WriteChapterComplete();
}

// ---- Part save identity --------------------------------------------------
// Every save made by a series build carries VAR_ARENA_GAME_STAMP. A later
// part only continues a save from the earlier part of the same game, so e.g.
// a FireRed save copied next to an Emerald Part 2 ROM boots as an empty file.
EWRAM_DATA u8 gArenaSeriesRejectedSave = 0;

void ArenaSeries_StampNewGame(void)
{
    VarSet(VAR_ARENA_GAME_STAMP, ARENA_GAME_STAMP);
}

static bool8 PartSaveAccepted(u16 stamp)
{
    if (stamp != 0 && stamp != ARENA_GAME_STAMP)
        return FALSE; // another game's save
#if MATT_PART >= 2
    if (stamp != ARENA_GAME_STAMP)
        return FALSE; // later parts need a stamped series save
#if MATT_PART == 3
    if (!FlagGet(FLAG_MATT_PART2_COMPLETE) && !FlagGet(FLAG_MATT_PART3_INIT))
        return FALSE;
#else
    if (!FlagGet(FLAG_MATT_PART1_COMPLETE))
        return FALSE;
#endif
#endif
    return TRUE;
}

void ArenaSeries_ValidatePartSave(void)
{
    gArenaSeriesRejectedSave = 0;
    if (gSaveFileStatus != SAVE_STATUS_OK || gArenaSeriesImportPending)
        return;
    if (!PartSaveAccepted(VarGet(VAR_ARENA_GAME_STAMP)))
    {
        gArenaSeriesRejectedSave = 1;
        gSaveFileStatus = SAVE_STATUS_EMPTY;
        return;
    }
#if defined(FIRERED) && MATT_PART == 1
    // Older single-ROM saves already past Erika: mark the Part 1 checkpoint
    // so the next save can be carried into Part 2.
    if (FlagGet(FLAG_BADGE04_GET))
        FlagSet(FLAG_MATT_PART1_COMPLETE);
#endif
    VarSet(VAR_ARENA_GAME_STAMP, ARENA_GAME_STAMP);
}

const u8 *ArenaSeries_ChapterCompleteText(void)
{
    return sChapterText;
}

static void GiveReward(u16 item, u16 n)
{
    if (!AddBagItem(item, n))
        AddPCItem(item, n);
}

#if ARENA_PLAYTEST
static void ArenaPlaytest_Run(void);
#endif
void ArenaSeries_FieldTick(void)
{
#if ARENA_PLAYTEST
    ArenaPlaytest_Run();
#endif
    if (!gArenaSeriesDexDirty)
        return;
    gArenaSeriesDexDirty = 0;
    if (FlagGet(ARENA_DEX_REWARD_FLAG) || GetNationalPokedexCount(FLAG_GET_CAUGHT) < ARENA_SERIES_DEX_TOTAL)
        return;
    FlagSet(ARENA_DEX_REWARD_FLAG);
    GiveReward(ITEM_RARE_CANDY, 10);
    GiveReward(ITEM_LUCKY_EGG, 1);
    gArenaSeriesToast = sDexToast;
}

// ---- Level scaling -----------------------------------------------------
u8 ArenaSeries_PartyTargetLevel(void)
{
    u8 top[3] = {0, 0, 0};
    u32 i, n = 0, sum = 0;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        u8 lv;
        if (!GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) || GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG))
            continue;
        lv = GetMonData(&gPlayerParty[i], MON_DATA_LEVEL);
        if (lv > top[0]) { top[2] = top[1]; top[1] = top[0]; top[0] = lv; }
        else if (lv > top[1]) { top[2] = top[1]; top[1] = lv; }
        else if (lv > top[2]) top[2] = lv;
    }
    for (i = 0; i < 3; i++)
        if (top[i]) { sum += top[i]; n++; }
    return n ? sum / n : 0;
}

static u8 CapLevel(s32 lv)
{
    return lv > MAX_LEVEL ? MAX_LEVEL : lv < 1 ? 1 : lv;
}

u8 ArenaSeries_ScaleWildLevel(u8 level)
{
    s32 t;
    if (!ARENA_SCALING_ACTIVE())
        return level;
    t = ArenaSeries_PartyTargetLevel() - 6 + (Random() % 5); // top-3 avg -6..-2
    return t > level ? CapLevel(t) : level;
}

u8 ArenaSeries_ScaleTrainerLevel(u8 level, u8 trainerClass)
{
    s32 t = ArenaSeries_PartyTargetLevel(), lv = level;
    bool8 boss = trainerClass == TRAINER_CLASS_LEADER || trainerClass == TRAINER_CLASS_ELITE_FOUR
              || trainerClass == TRAINER_CLASS_CHAMPION;
    if (gArenaRematchPending)
        lv = max(level + 10, t + 3);                   // gym rematch
    else if (boss && trainerClass != TRAINER_CLASS_LEADER && FlagGet(FLAG_SYS_GAME_CLEAR))
        lv = max(level + 5, t + 2);                    // E4/champion rematch
    else if (ARENA_SCALING_ACTIVE())
        lv = max(level, boss ? t + 1 : t - 3);         // carried-over team
    return CapLevel(lv);
}

void ArenaSeries_BeginRematch(void)
{
    gArenaRematchPending = 1;
}

// ---- HM field use with the badge alone ----------------------------------
bool8 ArenaSeries_HmBadgeOk(u16 move)
{
    u8 badge;
    switch (move)
    {
#ifdef FIRERED
    case MOVE_FLASH:      badge = 1; break;
    case MOVE_CUT:        badge = 2; break;
    case MOVE_FLY:        badge = 3; break;
    case MOVE_STRENGTH:   badge = 4; break;
    case MOVE_SURF:       badge = 5; break;
    case MOVE_ROCK_SMASH: badge = 6; break;
    case MOVE_WATERFALL:  badge = 7; break;
#else
    case MOVE_CUT:        badge = 1; break;
    case MOVE_FLASH:      badge = 2; break;
    case MOVE_ROCK_SMASH: badge = 3; break;
    case MOVE_STRENGTH:   badge = 4; break;
    case MOVE_SURF:       badge = 5; break;
    case MOVE_FLY:        badge = 6; break;
    case MOVE_DIVE:       badge = 7; break;
    case MOVE_WATERFALL:  badge = 8; break;
#endif
    default: return FALSE;
    }
    return FlagGet(FLAG_BADGE01_GET + badge - 1);
}

s8 ArenaSeries_FirstFieldMon(void)
{
    s8 i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (!GetMonData(&gPlayerParty[i], MON_DATA_SPECIES))
            break;
        if (!GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG))
            return i;
    }
    return -1;
}

// ---- Emerald: import an earlier chapter -------------------------------
#ifndef FIRERED
static u16 SectorChecksum(const void *data, u16 size)
{
    const u32 *w = data;
    u32 i, sum = 0;
    for (i = 0; i < size / 4; i++)
        sum += w[i];
    return (u16)((sum >> 16) + sum);
}

void ArenaSeries_CheckImportAtBoot(void)
{
    struct ArenaSeriesRecord *r = SectorRecord();
    u16 status = gSaveFileStatus;
    u32 counter = gSaveCounter;
    gArenaSeriesImportPending = 0;
    if (status == SAVE_STATUS_NO_FLASH || !LoadSectorRecord())
        return;
#if MATT_PART >= 2
    return; // later parts continue from the previous part's save
#endif
    if (r->sourceSegment != ARENA_SEG_FIRERED && r->sourceSegment != ARENA_SEG_JOHTO)
        return; // only completed earlier chapters import (not FireRed Part 1)
    // The slot still holds that chapter's own save (same counter), or no
    // valid Emerald save exists: offer a clean New Game that imports.
    if (status != SAVE_STATUS_OK || counter == r->saveCounter)
    {
        gArenaSeriesImportPending = 1;
        gSaveFileStatus = SAVE_STATUS_EMPTY;
    }
}

// Copies PC boxes from the slot written with the record (pret sectors 5-13;
// struct PokemonStorage is byte-identical in FireRed and Emerald).
static bool8 ImportBoxes(u32 saveCounter)
{
    u32 slot, i, best = 2, off;
    for (slot = 0; slot < NUM_SAVE_SLOTS && best == 2; slot++)
    {
        u32 found = 0;
        for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
        {
            struct SaveSector *s = &gSaveDataBuffer;
            ReadFlash(slot * NUM_SECTORS_PER_SLOT + i, 0, (u8 *)s, SECTOR_SIZE);
            if (s->signature != SECTOR_SIGNATURE || s->counter != saveCounter
             || s->id < SECTOR_ID_PKMN_STORAGE_START || s->id > SECTOR_ID_PKMN_STORAGE_END)
                continue;
            off = (s->id - SECTOR_ID_PKMN_STORAGE_START) * SECTOR_DATA_SIZE;
            if (s->checksum == SectorChecksum(s->data, min(SECTOR_DATA_SIZE, sizeof(struct PokemonStorage) - off)))
                found |= 1 << (s->id - SECTOR_ID_PKMN_STORAGE_START);
        }
        if (found == (1 << (SECTOR_ID_PKMN_STORAGE_END - SECTOR_ID_PKMN_STORAGE_START + 1)) - 1)
            best = slot;
    }
    if (best == 2)
        return FALSE;
    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
    {
        struct SaveSector *s = &gSaveDataBuffer;
        ReadFlash(best * NUM_SECTORS_PER_SLOT + i, 0, (u8 *)s, SECTOR_SIZE);
        if (s->signature != SECTOR_SIGNATURE || s->id < SECTOR_ID_PKMN_STORAGE_START || s->id > SECTOR_ID_PKMN_STORAGE_END)
            continue;
        off = (s->id - SECTOR_ID_PKMN_STORAGE_START) * SECTOR_DATA_SIZE;
        memcpy((u8 *)gPokemonStoragePtr + off, s->data, min(SECTOR_DATA_SIZE, sizeof(struct PokemonStorage) - off));
    }
    return TRUE;
}

static void SanitizeMon(struct Pokemon *mon)
{
    u16 item = GetMonData(mon, MON_DATA_HELD_ITEM);
    if (item >= ITEM_HM01 || item > ITEMS_COUNT) { item = ITEM_NONE; SetMonData(mon, MON_DATA_HELD_ITEM, &item); }
    CalculateMonStats(mon);
}

void ArenaSeries_ApplyImport(void)
{
    struct ArenaSeriesRecord *r;
    u32 i, b, box, pos;
    if (!gArenaSeriesImportPending)
        return;
    gArenaSeriesImportPending = 0;
    r = Alloc(sizeof(*r));
    if (r == NULL || !ArenaSeries_Read(r))
    {
        Free(r);
        return;
    }
    memcpy(gSaveBlock2Ptr->playerName, r->playerName, 8);
    gSaveBlock2Ptr->playerGender = r->playerGender;
    memcpy(gSaveBlock2Ptr->playerTrainerId, r->trainerId, 4); // same OT: no "traded" disobedience
    SetMoney(&gSaveBlock1Ptr->money, r->money);
    ZeroPlayerPartyMons();
    for (i = 0; i < PARTY_SIZE && i < r->partyCount; i++)
    {
        gPlayerParty[i] = r->party[i];
        SanitizeMon(&gPlayerParty[i]);
    }
    CalculatePlayerPartyCount();
    ImportBoxes(r->saveCounter);
    for (box = 0; box < TOTAL_BOXES_COUNT; box++)
        for (pos = 0; pos < IN_BOX_COUNT; pos++)
        {
            u16 item = GetBoxMonData(&gPokemonStoragePtr->boxes[box][pos], MON_DATA_HELD_ITEM);
            if (item >= ITEM_HM01) { item = ITEM_NONE; SetBoxMonData(&gPokemonStoragePtr->boxes[box][pos], MON_DATA_HELD_ITEM, &item); }
        }
    EnableNationalPokedex();
    for (i = 1; i <= ARENA_SERIES_DEX_TOTAL; i++)
    {
        b = 1 << ((i - 1) & 7);
        if (r->dexSeen[(i - 1) >> 3] & b)
            GetSetPokedexFlag(i, FLAG_SET_SEEN);
        if (r->dexCaught[(i - 1) >> 3] & b)
            GetSetPokedexFlag(i, FLAG_SET_CAUGHT);
    }
    if (r->seriesFlags & ARENA_SERIES_FLAG_DEX_REWARD)
        FlagSet(ARENA_DEX_REWARD_FLAG);
    FlagSet(FLAG_ARENA_SERIES_IMPORTED);
    gArenaSeriesDexDirty = 1;
    Free(r);
}
#else
void ArenaSeries_CheckImportAtBoot(void) {}
void ArenaSeries_ApplyImport(void) {}
#endif

#if ARENA_PLAYTEST
// ---- Headless playtest mailbox (tools/arena/playtest.py) ----------------
// The host writes args then cmd; the overworld tick runs it, fills result
// and sets done = cmd. Only compiled into ARENA_PLAYTEST=1 test ROMs.
#include "script.h"
#include "script_pokemon_util.h"
#include "field_player_avatar.h"
#include "overworld.h"
#ifdef FIRERED
#include "field_fadetransition.h"
extern u32 gArenaSummaryCards;
extern u8 gArenaItemsUsed;
#else
#include "field_screen_effect.h"
#endif
static u32 PtCountStorageMons(void)
{
    u32 b, p, n = 0;
    for (b = 0; b < TOTAL_BOXES_COUNT; b++)
        for (p = 0; p < IN_BOX_COUNT; p++)
            if (GetBoxMonDataAt(b, p, MON_DATA_SPECIES))
                n++;
    return n;
}
struct ArenaPlaytestBox { u8 cmd, done; u16 a, b, c; u32 result[4]; };
EWRAM_DATA struct ArenaPlaytestBox gArenaPlaytest = {0};
void ArenaPlaytest_StartTrainer(u16 trainerId);
extern const u8 EventScript_ArenaPracticeBattle[];

static void ArenaPlaytest_Run(void)
{
    struct ArenaPlaytestBox *m = &gArenaPlaytest;
    u32 i;
    u8 cmd = m->cmd;
    if (!cmd)
        return;
    m->cmd = 0;
    memset(m->result, 0, sizeof(m->result));
    switch (cmd)
    {
    case 1: // party slot c = species a at level b (c==0 clears the party first)
        if (m->c == 0)
            ZeroPlayerPartyMons();
        CreateMon(&gPlayerParty[m->c], m->a, m->b, USE_RANDOM_IVS, FALSE, 0, OT_ID_PLAYER_ID, 0);
        CalculatePlayerPartyCount();
        m->result[0] = gPlayerPartyCount;
        break;
    case 2: // give item a x b
        m->result[0] = AddBagItem(m->a, m->b);
        break;
#ifdef FIRERED
    case 3: // wild battle species a level b
        CreateScriptedWildMon(m->a, m->b, ITEM_NONE);
        ScriptContext_SetupScript(EventScript_ArenaPracticeBattle);
        break;
    case 4: // trainer battle a (b=1: gym rematch)
        gArenaRematchPending = m->b;
        ArenaPlaytest_StartTrainer(m->a);
        break;
#endif
    case 5: // save (same as start menu: snapshot the map view first)
        SaveMapView();
        m->result[0] = TrySavingData(SAVE_NORMAL);
        m->result[1] = gSaveCounter;
        break;
    case 6: // flag a := b
        if (m->b) FlagSet(m->a); else FlagClear(m->a);
        break;
    case 7: // catch every national dex entry
        for (i = 1; i <= ARENA_SERIES_DEX_TOTAL; i++)
        {
            GetSetPokedexFlag(i, FLAG_SET_SEEN);
            GetSetPokedexFlag(i, FLAG_SET_CAUGHT);
        }
        m->result[0] = GetNationalPokedexCount(FLAG_GET_CAUGHT);
        break;
    case 8: // scaling query: a = base level, b = trainer class
        m->result[0] = ArenaSeries_PartyTargetLevel();
        m->result[1] = ArenaSeries_ScaleTrainerLevel(m->a, m->b);
        m->result[2] = ArenaSeries_ScaleWildLevel(m->a);
        m->result[3] = FlagGet(FLAG_SYS_GAME_CLEAR);
        break;
    case 9: // read record
    {
        struct ArenaSeriesRecord *r = Alloc(sizeof(*r));
        if (r && ArenaSeries_Read(r))
        {
            m->result[0] = 1;
            m->result[1] = r->sourceSegment | (r->segmentsComplete << 8) | (r->partyCount << 16) | (r->badges[ARENA_SEG - 1] << 24);
            m->result[2] = r->money;
            m->result[3] = r->saveCounter;
        }
        Free(r);
        break;
    }
#ifdef FIRERED
    case 10: // HM badge check for move a; result[1] = slot checkpartymove would use
        m->result[0] = ArenaSeries_HmBadgeOk(m->a);
        m->result[1] = ArenaSeries_FirstFieldMon();
        m->result[2] = PartyHasMonWithSurf();
        break;
    case 11: // bag quantity check: item a, qty b
        m->result[0] = CheckBagHasItem(m->a, m->b);
        m->result[1] = gSaveBlock2Ptr->optionsTextSpeed;
        break;
#endif
    case 12: // level / exp of party slot a
        m->result[0] = GetMonData(&gPlayerParty[m->a], MON_DATA_LEVEL);
        m->result[1] = GetMonData(&gPlayerParty[m->a], MON_DATA_EXP);
        m->result[2] = GetMonData(&gPlayerParty[m->a], MON_DATA_SPECIES);
        m->result[3] = GetMonData(&gPlayerParty[m->a], MON_DATA_HP);
        break;
    case 13: // party slot a hp := b
        i = m->b;
        SetMonData(&gPlayerParty[m->a], MON_DATA_HP, &i);
        break;
    case 14: // warp to map group a, num b, x = c & 0xFF, y = c >> 8
        SetWarpDestination(m->a, m->b, -1, m->c & 0xFF, m->c >> 8);
        DoWarp();
        break;
#ifdef FIRERED
    case 15: // party slot a held item := b
        i = m->b;
        SetMonData(&gPlayerParty[m->a], MON_DATA_HELD_ITEM, &i);
        break;
    case 19: // party slot c: move slot b := move a (full PP)
        i = m->a;
        SetMonData(&gPlayerParty[m->c], MON_DATA_MOVE1 + m->b, &i);
        i = GetMovePP(m->a);
        SetMonData(&gPlayerParty[m->c], MON_DATA_PP1 + m->b, &i);
        break;
#endif
    case 18: // remove item a x b
        m->result[0] = RemoveBagItem(m->a, m->b);
        break;
#ifdef FIRERED
    case 17: // party slot a: exp one point short of the next level
    {
        u16 sp = GetMonData(&gPlayerParty[m->a], MON_DATA_SPECIES);
        u8 lv = GetMonData(&gPlayerParty[m->a], MON_DATA_LEVEL);
        i = gExperienceTables[gSpeciesInfo[sp].growthRate][lv + 1] - 1;
        SetMonData(&gPlayerParty[m->a], MON_DATA_EXP, &i);
        m->result[0] = i;
        break;
    }
#endif
    case 20: // var a (c=1: set to b); result = value, game stamp, save status, rejected
        if (m->c)
            VarSet(m->a, m->b);
        m->result[0] = VarGet(m->a);
        m->result[1] = VarGet(VAR_ARENA_GAME_STAMP);
        m->result[2] = gSaveFileStatus;
        m->result[3] = gArenaSeriesRejectedSave;
        break;
#ifdef FIRERED
    case 21: // run event script at address a | b << 16; result[0] = flag c
        ScriptContext_SetupScript((const u8 *)(m->a | ((u32)m->b << 16)));
        m->result[0] = FlagGet(m->c);
        break;
#endif
    case 22: // flag a; record seriesFlags; MATT_PART
    {
        struct ArenaSeriesRecord *r = Alloc(sizeof(*r));
        m->result[0] = FlagGet(m->a);
        if (r && ArenaSeries_Read(r))
            m->result[1] = r->seriesFlags | (r->sourceSegment << 8) | (r->segmentsComplete << 16);
        m->result[2] = MATT_PART;
        Free(r);
        break;
    }
#ifdef FIRERED
    case 16: // counters
        m->result[0] = gArenaSummaryCards;
        m->result[1] = gPlayerPartyCount;
        m->result[2] = PtCountStorageMons();
        m->result[3] = gArenaItemsUsed;
        break;
#endif
    }
    m->done = cmd;
}
#endif
