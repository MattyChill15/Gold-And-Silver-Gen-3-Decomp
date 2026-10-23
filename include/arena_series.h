#ifndef GUARD_ARENA_SERIES_H
#define GUARD_ARENA_SERIES_H
// Arena series save hub. Format: docs/SERIES_SAVE.md (keep in sync).
#define ARENA_SERIES_MAGIC      0x53524E41 // "ANRS" little-endian
#define ARENA_SERIES_VERSION    1
#define ARENA_SERIES_SECTOR     31         // flash sector, same in FR and Emerald
#define ARENA_SERIES_DEX_BYTES  52         // 416 bits >= 386 National Dex
#define ARENA_SERIES_DEX_TOTAL  386

// Record "segment" ids. Ch2 (Johto) is reserved.
#define ARENA_SEG_FIRERED       1
#define ARENA_SEG_JOHTO         2
#define ARENA_SEG_EMERALD_P1    3
#define ARENA_SEG_EMERALD_P2    4
#define ARENA_SEG_FIRERED_P1    5 // FireRed Part 1 checkpoint (Erika); the chapter itself completes as ARENA_SEG_FIRERED in Part 2
#define ARENA_SEG_EMERALD_P3    6
#define ARENA_SEG_JOHTO_P1      7 // Johto Part 1 checkpoint (Lance); the chapter completes as ARENA_SEG_JOHTO in Part 2

// Save identity stamped into each part's save (VAR_ARENA_GAME_STAMP).
#define ARENA_GAME_STAMP_FIRERED 0x4652 // "FR"
#define ARENA_GAME_STAMP_EMERALD 0x4541 // "EA"
#define ARENA_GAME_STAMP_JOHTO   0x4F4A // "JO"
// Johto (pokeemerald-expansion) PC storage: same offsets as pret up to the
// wallpapers, then fusion slots; the last storage sector holds this much.
#define ARENA_JOHTO_STORAGE_SIZE 0x8560

#define ARENA_SERIES_FLAG_DEX_REWARD 1 // 386 reward claimed in some chapter
#define ARENA_SERIES_FLAG_YELLOW     2 // chapter 1 played in Yellow mode (Pikachu starter, Eevee rival)
#define ARENA_SERIES_FLAG_CHIEF_STORY 4 // storyline: Chief Story (stored only for now)

struct ArenaSeriesRecord
{
    /*0x000*/ u32 magic;
    /*0x004*/ u16 version;
    /*0x006*/ u16 size;            // sizeof(struct ArenaSeriesRecord)
    /*0x008*/ u8 sourceSegment;    // segment that wrote this copy
    /*0x009*/ u8 segmentsComplete; // bit (seg-1)
    /*0x00A*/ u8 partyCount;
    /*0x00B*/ u8 playerGender;
    /*0x00C*/ u8 badges[8];        // badge count per segment, [seg-1]
    /*0x014*/ u32 money;
    /*0x018*/ u8 playerName[8];
    /*0x020*/ u8 trainerId[4];
    /*0x024*/ u32 seriesFlags;
    /*0x028*/ u8 dexSeen[ARENA_SERIES_DEX_BYTES];
    /*0x05C*/ u8 dexCaught[ARENA_SERIES_DEX_BYTES];
#ifdef ARENA_JOHTO
    /*0x090*/ u8 party[6][100];   // always pret (FR/Emerald) party format
#else
    /*0x090*/ struct Pokemon party[6];
#endif
    /*0x2E8*/ u32 writeCount;
    /*0x2EC*/ u32 saveCounter;     // pret gSaveCounter of the slot saved with it
    /*0x2F0*/ u32 checksum;        // sum of u32 words 0x000..0x2EF
}; // 0x2F4 bytes

extern const u8 *const *gArenaSeriesToast;
extern u8 gArenaSeriesDexDirty;
extern u8 gArenaRematchPending;

bool8 ArenaSeries_Read(struct ArenaSeriesRecord *dst);
bool8 ArenaSeries_WriteChapterComplete(void);
void ArenaSeries_FieldTick(void);
u8 ArenaSeries_PartyTargetLevel(void);
u8 ArenaSeries_ScaleWildLevel(u8 level);
u8 ArenaSeries_ScaleTrainerLevel(u8 level, u8 trainerClass);
void ArenaSeries_BeginRematch(void);
bool8 ArenaSeries_HmBadgeOk(u16 move);
s8 ArenaSeries_FirstFieldMon(void);
const u8 *ArenaSeries_ChapterCompleteText(void);
void ArenaSeries_OnSaved(u8 saveType);
// Emerald only: import a FireRed (earlier-chapter) save on New Game.
extern u8 gArenaSeriesImportPending;
void ArenaSeries_CheckImportAtBoot(void);
void ArenaSeries_ValidatePartSave(void);
void ArenaSeries_StampNewGame(void);
extern u8 gArenaSeriesRejectedSave;
void ArenaSeries_ApplyImport(void);
#endif
