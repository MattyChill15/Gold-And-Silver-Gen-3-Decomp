// Arena series save hub (see docs/SERIES_SAVE.md). Shared verbatim between
// the FireRed (chapter 1) and Emerald (chapter 3) trees.
#include "global.h"
#include "rtc.h"
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
#ifdef ARENA_JOHTO
#include "arena_mon_xfer.h"
#include "pokeball.h"
#include "constants/characters.h"
#include "arena_story.h"
#include "data/arena_xfer_tables.h"
#endif

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
#elif defined(ARENA_JOHTO)
#define ARENA_GAME_STAMP ARENA_GAME_STAMP_JOHTO
// Three parts (Matt 2026-10-23): P1 New Bark..Ecruteak (Fog Badge), P2 to Lance,
// P3 Kanto/Mt Silver/Frontier to Red. P3 completes as ARENA_SEG_JOHTO for Emerald.
#if MATT_PART == 3
#define ARENA_SEG ARENA_SEG_JOHTO
#define ARENA_CHAPTER_CLEAR() FlagGet(FLAG_DEFEATED_RED) /* Part 3 ends at Red */
#elif MATT_PART == 2
#define ARENA_SEG ARENA_SEG_JOHTO_P2
#define ARENA_CHAPTER_CLEAR() FlagGet(FLAG_MATT_PART2_COMPLETE) /* Part 2 ends at Lance */
#else
#define ARENA_SEG ARENA_SEG_JOHTO_P1
#define ARENA_CHAPTER_CLEAR() FlagGet(FLAG_MATT_PART1_COMPLETE) /* Part 1 ends at Morty (Ecruteak) */
#endif
#define ARENA_DEX_REWARD_FLAG FLAG_ARENA_DEX_REWARD
#define ARENA_SCALING_ACTIVE() FlagGet(FLAG_ARENA_SERIES_IMPORTED)
static const u8 sChapterText[] = _("CHAPTER 2 COMPLETE!");
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


#if defined(ARENA_JOHTO) || !defined(FIRERED)
#ifndef ARENA_JOHTO
#include "arena_mon_xfer.h"
#include "data/arena_xfer_tables.h"
#endif
// Expansion-numbered fields -> pret (Emerald) numbering. FALSE: no Gen 1-3 species.
static bool8 XferExpFieldsToPret(struct ArenaMonFields *f)
{
    u32 i, n;
    u16 species = f->species;
    for (i = 0; species > ARENA_SERIES_DEX_TOTAL && i < ARRAY_COUNT(sXferExpFormToBase); i++)
        if (sXferExpFormToBase[i][0] == species)
            species = sXferExpFormToBase[i][1];
    f->species = species <= ARENA_SERIES_DEX_TOTAL ? sXferNationalToPretSpecies[species] : 0;
    if (f->species == 0)
        return FALSE;
    f->heldItem = f->heldItem < XFER_EXP_ITEM_COUNT ? sXferExpItemToEa[f->heldItem] : 0;
    f->ball = f->ball < ARRAY_COUNT(sXferExpBallToPret) ? sXferExpBallToPret[f->ball] : 4;
    f->abilityNum = f->abilityNum == 1; // hidden abilities do not exist in pret
    for (i = 0, n = 0; i < 4; i++)
    {
        if (f->moves[i] == 0 || f->moves[i] > 354) // MOVE_PSYCHO_BOOST: later-gen moves dropped
            continue;
        f->moves[n] = f->moves[i];
        f->pp[n] = f->pp[i];
        n++;
    }
    for (; n < 4; n++)
        f->moves[n] = f->pp[n] = 0;
    return TRUE;
}
#endif

#ifdef ARENA_JOHTO
// ---- Johto (expansion) <-> pret mon conversion -------------------------
static const u8 sXferRibbonFields[] =
{
    MON_DATA_COOL_RIBBON, MON_DATA_BEAUTY_RIBBON, MON_DATA_CUTE_RIBBON, MON_DATA_SMART_RIBBON, MON_DATA_TOUGH_RIBBON,
    MON_DATA_CHAMPION_RIBBON, MON_DATA_WINNING_RIBBON, MON_DATA_VICTORY_RIBBON, MON_DATA_ARTIST_RIBBON,
    MON_DATA_EFFORT_RIBBON, MON_DATA_MARINE_RIBBON, MON_DATA_LAND_RIBBON, MON_DATA_SKY_RIBBON,
    MON_DATA_COUNTRY_RIBBON, MON_DATA_NATIONAL_RIBBON, MON_DATA_EARTH_RIBBON, MON_DATA_WORLD_RIBBON,
};

static u32 RibbonValue(u32 ribbons, u32 i)
{
    return i < 5 ? (ribbons >> (3 * i)) & 7 : (ribbons >> (15 + i - 5)) & 1;
}

// FireRed (pret) box mon bytes -> native box mon. FALSE leaves dst empty.
static bool8 Johto_ImportPretBoxMon(const u8 *raw, struct BoxPokemon *dst)
{
    struct ArenaMonFields f;
    u32 i, v;
    u16 species;
    u8 nick[POKEMON_NAME_LENGTH + 1];
    ZeroBoxMonData(dst);
    if (!ArenaXfer_DecodePret(raw, &f) || f.isBadEgg || f.species >= ARRAY_COUNT(sXferPretSpeciesToNational))
        return FALSE;
    species = sXferPretSpeciesToNational[f.species]; // expansion species ids are national for 1-386
    if (species == SPECIES_NONE)
        return FALSE;
    CreateBoxMonLegacy(dst, species, 1, 0, TRUE, f.personality, OT_ID_PRESET, f.otId);
    memset(nick, EOS, sizeof(nick));
    memcpy(nick, f.nickname, 10);
    SetBoxMonData(dst, MON_DATA_NICKNAME, nick);
    SetBoxMonData(dst, MON_DATA_OT_NAME, f.otName);
    v = f.language;    SetBoxMonData(dst, MON_DATA_LANGUAGE, &v);
    v = f.markings;    SetBoxMonData(dst, MON_DATA_MARKINGS, &v);
    v = f.heldItem < XFER_FR_ITEM_COUNT ? sXferFrItemToExp[f.heldItem] : ITEM_NONE;
    SetBoxMonData(dst, MON_DATA_HELD_ITEM, &v);
    SetBoxMonData(dst, MON_DATA_EXP, &f.exp);
    v = f.ppBonuses;   SetBoxMonData(dst, MON_DATA_PP_BONUSES, &v);
    v = f.friendship;  SetBoxMonData(dst, MON_DATA_FRIENDSHIP, &v);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        v = f.moves[i] <= MOVE_PSYCHO_BOOST ? f.moves[i] : MOVE_NONE; // Gen 1-3 move ids match
        SetBoxMonData(dst, MON_DATA_MOVE1 + i, &v);
        v = v ? f.pp[i] : 0;
        SetBoxMonData(dst, MON_DATA_PP1 + i, &v);
    }
    for (i = 0; i < NUM_STATS; i++)
    {
        v = f.evs[i]; SetBoxMonData(dst, MON_DATA_HP_EV + i, &v);
        v = f.ivs[i]; SetBoxMonData(dst, MON_DATA_HP_IV + i, &v);
    }
    v = f.contest[0]; SetBoxMonData(dst, MON_DATA_COOL, &v);
    v = f.contest[1]; SetBoxMonData(dst, MON_DATA_BEAUTY, &v);
    v = f.contest[2]; SetBoxMonData(dst, MON_DATA_CUTE, &v);
    v = f.contest[3]; SetBoxMonData(dst, MON_DATA_SMART, &v);
    v = f.contest[4]; SetBoxMonData(dst, MON_DATA_TOUGH, &v);
    v = f.contest[5]; SetBoxMonData(dst, MON_DATA_SHEEN, &v);
    v = f.pokerus;     SetBoxMonData(dst, MON_DATA_POKERUS, &v);
    v = f.metLocation; SetBoxMonData(dst, MON_DATA_MET_LOCATION, &v);
    v = f.metLevel;    SetBoxMonData(dst, MON_DATA_MET_LEVEL, &v);
    v = f.metGame;     SetBoxMonData(dst, MON_DATA_MET_GAME, &v);
    v = f.otGender;    SetBoxMonData(dst, MON_DATA_OT_GENDER, &v);
    v = ItemIdToBallId(f.ball && f.ball < XFER_FR_ITEM_COUNT ? sXferFrItemToExp[f.ball] : ITEM_POKE_BALL);
    SetBoxMonData(dst, MON_DATA_POKEBALL, &v);
    v = f.abilityNum;  SetBoxMonData(dst, MON_DATA_ABILITY_NUM, &v);
    v = f.fateful;     SetBoxMonData(dst, MON_DATA_MODERN_FATEFUL_ENCOUNTER, &v);
    for (i = 0; i < ARRAY_COUNT(sXferRibbonFields); i++)
    {
        v = RibbonValue(f.ribbons, i);
        SetBoxMonData(dst, sXferRibbonFields[i], &v);
    }
    if (f.isEgg)
    {
        v = TRUE;
        SetBoxMonData(dst, MON_DATA_IS_EGG, &v);
    }
    return TRUE;
}

// Native box mon -> pret fields (Emerald numbering), via the same decoder
// Emerald uses on Johto PC boxes.
static bool8 Johto_ExportFields(const struct BoxPokemon *src, struct ArenaMonFields *f)
{
    if (!ArenaXfer_DecodeExpansion((const u8 *)src, f))
        return FALSE;
    return XferExpFieldsToPret(f);
}

// Native party mon -> 100-byte pret party mon (Emerald numbering).
static void Johto_EncodePartyMon(struct Pokemon *mon, u8 *dst)
{
    struct ArenaMonFields f;
    u32 i;
    u16 *stats = (u16 *)(dst + 86);
    memset(dst, 0, ARENA_PARTYMON_SIZE_PRET);
    if (!Johto_ExportFields(&mon->box, &f))
        return;
    ArenaXfer_EncodePret(&f, dst);
    *(u32 *)(dst + 80) = GetMonData(mon, MON_DATA_STATUS);
    dst[84] = GetMonData(mon, MON_DATA_LEVEL);
    dst[85] = 0xFF; // no mail
    stats[0] = GetMonData(mon, MON_DATA_HP);
    stats[1] = GetMonData(mon, MON_DATA_MAX_HP);
    for (i = 0; i < NUM_STATS - 1; i++)
        stats[2 + i] = GetMonData(mon, MON_DATA_ATK + i);
}

// 100-byte FireRed party mon -> native party mon.
static bool8 Johto_ImportPretPartyMon(const u8 *src, struct Pokemon *mon)
{
    u32 hp, status;
    ZeroMonData(mon);
    if (!Johto_ImportPretBoxMon(src, &mon->box))
        return FALSE;
    CalculateMonStats(mon);
    hp = *(const u16 *)(src + 86);
    status = *(const u32 *)(src + 80);
    if (hp > GetMonData(mon, MON_DATA_MAX_HP) || hp == 0)
        hp = GetMonData(mon, MON_DATA_MAX_HP);
    SetMonData(mon, MON_DATA_HP, &hp);
    SetMonData(mon, MON_DATA_STATUS, &status);
    return TRUE;
}
#endif

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
#ifdef ARENA_JOHTO
    r->seriesFlags = (r->seriesFlags & ~ARENA_SERIES_STORY_MASK) | (ArenaStory_Get() << ARENA_SERIES_STORY_SHIFT);
#endif
    for (i = 0; i < ARENA_SERIES_DEX_BYTES; i++)
    {
        r->dexSeen[i] |= gSaveBlock1Ptr->dexSeen[i];
        r->dexCaught[i] |= gSaveBlock1Ptr->dexCaught[i];
    }
    r->partyCount = gPlayerPartyCount;
    for (i = 0; i < PARTY_SIZE; i++)
#ifdef ARENA_JOHTO
        Johto_EncodePartyMon(&gPlayerParty[i], r->party[i]);
#else
        r->party[i] = gPlayerParty[i];
#endif
    r->writeCount++;
    r->saveCounter = gSaveCounter;
    r->checksum = RecordChecksum(r);
    return ProgramFlashSectorAndVerify(ARENA_SERIES_SECTOR, (u8 *)&gSaveDataBuffer) == 0;
}

// Called after every successful normal/HoF save once the chapter is clear,
// so the hub always mirrors the newest team, boxes slot and dex.
#ifdef ARENA_JOHTO
static void JohtoSyncPartFlags(void)
{
#if MATT_PART == 1
    if (FlagGet(FLAG_BADGE04_GET)) FlagSet(FLAG_MATT_PART1_COMPLETE);
#elif MATT_PART == 2
    if (FlagGet(FLAG_SYS_GAME_CLEAR)) FlagSet(FLAG_MATT_PART2_COMPLETE);
#endif
}
#endif
void ArenaSeries_OnSaved(u8 saveType)
{
#ifdef ARENA_JOHTO
    JohtoSyncPartFlags();
#endif
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
#ifdef ARENA_JOHTO
    JohtoSyncPartFlags(); // before any save, so the flag is in the slot the next part reads
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
#ifdef ARENA_JOHTO
    if (r->sourceSegment != ARENA_SEG_FIRERED)
        return; // Johto continues the completed FireRed chapter only
#else
    if (r->sourceSegment != ARENA_SEG_FIRERED && r->sourceSegment != ARENA_SEG_JOHTO)
        return; // only completed earlier chapters import (not FireRed Part 1)
#endif
    // The slot still holds that chapter's own save (same counter), or no
    // valid Emerald save exists: offer a clean New Game that imports.
    if (status != SAVE_STATUS_OK || counter == r->saveCounter)
    {
        gArenaSeriesImportPending = 1;
        gSaveFileStatus = SAVE_STATUS_EMPTY;
    }
}

// Copies PC boxes from the slot written with the record (sectors 5-13).
// currentBox..boxWallpapers sit at the same offsets in every tree; the mons
// themselves are converted when the source format differs (FireRed -> Johto,
// Johto -> Emerald). srcSize = the source game's sizeof(struct PokemonStorage).
#define PRET_STORAGE_COPY 0x83D0
static bool8 ImportBoxes(u32 saveCounter, u32 srcSize)
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
            if (off < srcSize && s->checksum == SectorChecksum(s->data, min(SECTOR_DATA_SIZE, srcSize - off)))
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
        if (off < PRET_STORAGE_COPY)
            memcpy((u8 *)gPokemonStoragePtr + off, s->data, min(SECTOR_DATA_SIZE, PRET_STORAGE_COPY - off));
    }
    return TRUE;
}

// Converts every PC slot in place after ImportBoxes. Returns mons kept.
static u32 ConvertImportedBoxes(u8 sourceSegment)
{
    u32 box, pos, kept = 0;
    u8 raw[ARENA_BOXMON_SIZE];
    for (box = 0; box < TOTAL_BOXES_COUNT; box++)
        for (pos = 0; pos < IN_BOX_COUNT; pos++)
        {
            struct BoxPokemon *mon = &gPokemonStoragePtr->boxes[box][pos];
            memcpy(raw, mon, ARENA_BOXMON_SIZE);
#ifdef ARENA_JOHTO
            kept += Johto_ImportPretBoxMon(raw, mon);
#else
            if (sourceSegment == ARENA_SEG_JOHTO)
            {
                struct ArenaMonFields f;
                if (ArenaXfer_DecodeExpansion(raw, &f) && !f.isBadEgg && XferExpFieldsToPret(&f))
                    ArenaXfer_EncodePret(&f, (u8 *)mon);
                else
                    memset(mon, 0, ARENA_BOXMON_SIZE);
            }
            if (GetBoxMonData(mon, MON_DATA_SPECIES))
            {
                u16 item = GetBoxMonData(mon, MON_DATA_HELD_ITEM);
                if (item >= ITEM_HM01) { item = ITEM_NONE; SetBoxMonData(mon, MON_DATA_HELD_ITEM, &item); }
                kept++;
            }
#endif
        }
    return kept;
}

#ifndef ARENA_JOHTO
static void SanitizeMon(struct Pokemon *mon)
{
    u16 item = GetMonData(mon, MON_DATA_HELD_ITEM);
    if (item >= ITEM_HM01 || item > ITEMS_COUNT) { item = ITEM_NONE; SetMonData(mon, MON_DATA_HELD_ITEM, &item); }
    CalculateMonStats(mon);
}
#endif

void ArenaSeries_ApplyImport(void)
{
    struct ArenaSeriesRecord *r;
    u32 i, b;
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
    for (i = 0, b = 0; i < PARTY_SIZE && i < r->partyCount; i++)
    {
#ifdef ARENA_JOHTO
        if (Johto_ImportPretPartyMon(r->party[i], &gPlayerParty[b]))
            b++;
#else
        gPlayerParty[i] = r->party[i];
        SanitizeMon(&gPlayerParty[i]);
#endif
    }
    CalculatePlayerPartyCount();
#ifdef ARENA_JOHTO
    if (ImportBoxes(r->saveCounter, PRET_STORAGE_COPY))
        ConvertImportedBoxes(r->sourceSegment);
#else
    if (ImportBoxes(r->saveCounter, r->sourceSegment == ARENA_SEG_JOHTO ? ARENA_JOHTO_STORAGE_SIZE : sizeof(struct PokemonStorage)))
        ConvertImportedBoxes(r->sourceSegment);
#endif
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
#ifdef ARENA_JOHTO
    ArenaStory_Set((r->seriesFlags & ARENA_SERIES_STORY_MASK) >> ARENA_SERIES_STORY_SHIFT); // storyline carries over
#endif
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
#include "move.h"
#include "field_player_avatar.h"
#include "overworld.h"
#ifdef FIRERED
#include "field_fadetransition.h"
extern u32 gArenaSummaryCards;
extern u8 gArenaItemsUsed;
#else
#include "field_screen_effect.h"
#endif
bool8 gArenaPlaytestClassic;
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
        CreateMonLegacy(&gPlayerParty[m->c], m->a, m->b, USE_RANDOM_IVS, FALSE, 0, OT_ID_PLAYER_ID, 0);
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
#endif
    case 19: // party slot c: move slot b := move a (full PP)
        i = m->a;
        SetMonData(&gPlayerParty[m->c], MON_DATA_MOVE1 + m->b, &i);
        i = GetMovePP(m->a);
        SetMonData(&gPlayerParty[m->c], MON_DATA_PP1 + m->b, &i);
        break;
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
#if defined(FIRERED) || defined(ARENA_JOHTO)
    case 21: // run event script at address a | b << 16; result[0] = flag c
        ScriptContext_SetupScript((const u8 *)(m->a | ((u32)m->b << 16)));
        m->result[0] = FlagGet(m->c);
        break;
#endif
#ifndef FIRERED
    case 31: // game clock := hour a, minute b (real RTC offset, like setting the clock)
        RtcInitLocalTimeOffset(m->a, m->b);
        RtcCalcLocalTime();
        m->result[0] = GetTimeOfDay();
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
#ifdef ARENA_JOHTO
    case 23: // import check: party count, PC mons, species slot 0|1<<16, storyline
        m->result[0] = gPlayerPartyCount;
        m->result[1] = PtCountStorageMons();
        m->result[2] = GetMonData(&gPlayerParty[0], MON_DATA_SPECIES) | (GetMonData(&gPlayerParty[1], MON_DATA_SPECIES) << 16);
        m->result[3] = ArenaStory_Get() | (FlagGet(FLAG_ARENA_SERIES_IMPORTED) << 8);
        break;
    case 30: // D5 shortcut: a=1 runs trainer battles in the classic engine
        gArenaPlaytestClassic = m->a;
        m->result[0] = gArenaPlaytestClassic;
        break;
    case 24: // storyline := a
        ArenaStory_Set(m->a);
        m->result[0] = ArenaStory_Get();
        break;
    case 25: // D3 gate: is map group a / num b closed in this part? c=1: warp there if open
        m->result[0] = ArenaJohto_IsMapBlocked(m->a, m->b);
        if (m->c && !m->result[0])
        {
            SetWarpDestination(m->a, m->b, WARP_ID_NONE, 5, 5);
            DoWarp();
        }
        break;
#endif
    }
    m->done = cmd;
}
#endif
