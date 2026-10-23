#include "global.h"
#include "arena_series.h"
#include "matt_config.h"
// Matt mod state (reset every battle in RealtimeArena_ResetBattle).
static EWRAM_INIT u8 sMattPrompt = 0;     // 0 not asked, 1 asking, 2 answered
static EWRAM_INIT u8 sMattCatchMode = 0;  // 1 = weaken then throw balls
static EWRAM_INIT u8 sMattStatusUsed = 0; // bit per move slot: setup/status used
static EWRAM_INIT u8 sMattThrowWait = 0;
static EWRAM_INIT u8 sMattBackoff = 0;
static EWRAM_INIT u16 sMattBall = 0;
static EWRAM_INIT u16 sMattLeftoversClock[2] = {0};
static EWRAM_INIT u8 sProtectChain[2] = {0};
static EWRAM_INIT u16 sMattProtectGap = 0; // bit15: must attack before next guard
static EWRAM_INIT u8 sEndureLife[2] = {0};
// v10 move state (12 bytes): last hit taken (Counter/Mirror Coat), its
// category and age; Destiny Bond window; Perish count; Encore timer; SE shake.
static EWRAM_INIT u16 sLastHit[2] = {0};
static EWRAM_INIT u8 sLastHitAge[2] = {0}, sLastHitSpecial = 0, sDestinyLife[2] = {0}, sPerish[2] = {0}, sEncore[2] = {0}, sSeShake = 0;
static EWRAM_INIT u16 sMattItemCd = 0;
static EWRAM_INIT u8 sMattItemMsg[32] = {0};
static EWRAM_INIT u8 sMattChoice = 0;    // 0 TRY CATCH, 1 CATCH (100%), 2 FIGHT
static EWRAM_INIT u8 sMattForceCatch = 0;
static EWRAM_INIT u8 sMattShots = 0;
// Auto-switch: bits 0-1 switches used this battle, bits 2-7 cooldown (x15 frames).
static EWRAM_INIT u8 sMattSwitch = 0;
#include "realtime_arena.h"
void ArenaJohto_DemoIntroFlags(void);
#include "arena_capture.h"
#include "item.h"
#include "trig.h"
#include "malloc.h"
#include "pokedex.h"
#include "arena_lab.h"
#include "arena_navigation.h"
#include "arena_sprites.h"
#include "arena_feedback.h"
#include "arena_moves.h"
#include "arena_move_fx.h"
#include "arena_physics.h"
#include "arena_terrain.h"
#include "arena_render.h"
#include "arena_psychic.h"
#include "event_object_movement.h"
#include "graphics.h"
#include "constants/event_objects.h"
#include "field_weather.h"
#include "fonts.h"
#include "constants/weather.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_scripts.h"
#include "battle_util.h"
#include "battle_setup.h"
#include "bg.h"
#include "data.h"
#include "decompress.h"
#include "event_data.h"
#include "field_screen_effect.h"
#include "gpu_regs.h"
#include "main.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "reshow_battle_screen.h"
#include "save.h"
#include "scanline_effect.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "util.h"
#include "random.h"
#include "constants/abilities.h"
#include "constants/game_stat.h"
#include "constants/battle_move_effects.h"
#include "constants/heal_locations.h"
#include "constants/items.h"
#include "constants/trainers.h"
#include "constants/hold_effects.h"
#include "constants/moves.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define Q 256
#define SHOTS_COUNT ARENA_BOLT_SLOTS
#define SHOT_TAG 0xA710
#define MON_TAG 0xA720
#define AI_REPOSITION 0
#define AI_AIM 1
#define AI_RECOVER 2
#define AI_EVADE 3
// PMDCollab rows: down, down-right, right, up-right, up, up-left, left, down-left.
#define FACE_UP 4
#define FACE_DOWN 0
#define FACE_LEFT 6
#define FACE_RIGHT 2

struct ArenaBody
{
    s32 x, y;
    s32 aimX, aimY;
    u16 cooldown, dashCooldown;
    u8 sprite, moveSlot, dash, flash, facing, moving, shadow;
    const struct ArenaSpriteSet *art;
    u32 animClock;
    u16 shotTimer, shotElapsed;
    u8 animation, drawnFrame, drawnDirection, shotFacing, shotSlot;
    s8 dashX, dashY;
    s16 attackX, attackY;
    u8 actionLife, actionAge, connected, terrainMask,manualAim;
    s16 knockX,knockY;
    u16 burnClock;
    u16 weatherClock;
    u8 statusActions;
    u16 rollAngle;
    u8 rollStage, rollDistance, flat;
};

struct ArenaShot
{
    s32 x, y, vx, vy;
    u16 move;
    u8 side, sprite, life, direction, age, elementMask;
    u8 trail[2];
};

// DidierRLopes, PR #1: original throw choreography/state.
struct ArenaToss
{
    s32 damage;
    s16 height[2];
    s16 holdX, holdY;
    s16 drawX[2], drawY[2];
    s16 scaleX[2], scaleY[2];
    u16 rotation[2];
    u8 priority[2], animation[2], direction[2], hold[2];
    u8 phase, timer, attacker, shake, fatal, space, blackout;
};
struct ArenaState
{
    struct ArenaBody bodies[2];
    struct ArenaShot shots[SHOTS_COUNT];
    MainCallback savedCB1;
    u16 frame;
    u8 active, paused, classic, resultTimer;
    u8 lastAttacker, lastTarget, hudDirty;
    u32 aiRandom;
    struct ArenaPoint goal, waypoint, observed, previous;
    u16 thinkTimer, aimTimer, goalTimer;
    u8 aiState, style, reaction, aimError, cueSprite, aimSprite;
    u8 stalledTicks;
    u8 hitstop, attackBuffer, dashBuffer;
    s8 dashRequestX, dashRequestY;
    u32 lastBlast;
    struct ArenaToss toss;
};

static EWRAM_INIT struct ArenaState sArena = {};
// Persistent environment within one encounter, including native party switches.
static EWRAM_INIT bool8 sEnvironmentInitialized=FALSE;
// Remember the living field occupants across native result/party handoffs.
static EWRAM_DATA struct {
    s32 x, y;
    u32 personality;
    u8 party, facing, alive;
    u16 cooldown, dashCooldown, burnClock;
    u8 moveSlot, statusActions;
} sArenaResident[2] = {};
static EWRAM_INIT u8 sArenaSendoutMask = 3;
static EWRAM_INIT u8 sFlameReach[2]={},sFlameElementMask[2]={};
// starts, active ticks, native hit opportunities, prop hits, peak reach.
EWRAM_DATA u32 gArenaFlameTelemetry[5]={};
static bool8 PsychicBusy(u8 side);
static bool8 PsychicStart(u8 side,s32 targetX,s32 targetY);
// Slot four is an arena-only fallback, never written into a native moveset.
#define ARENA_STRUGGLE_SLOT MAX_MON_MOVES
#define ARENA_NO_MOVE (MAX_MON_MOVES+1)
static u16 ArenaMoveId(u8 side,u8 slot)
{return slot==ARENA_STRUGGLE_SLOT?MOVE_STRUGGLE:slot<MAX_MON_MOVES?gBattleMons[side].moves[slot]:MOVE_NONE;}
static bool8 NeedsStruggle(u8 side)
{
    u32 n;
    for(n=0;n<MAX_MON_MOVES;n++)
        if(gBattleMons[side].pp[n] && ArenaMoves_Get(gBattleMons[side].moves[n])
            && GetMovePower(gBattleMons[side].moves[n]))return FALSE;
    return TRUE;
}
static u8 ArenaMovePP(u8 side,u8 slot)
{return slot==ARENA_STRUGGLE_SLOT?NeedsStruggle(side):slot<MAX_MON_MOVES?gBattleMons[side].pp[slot]:0;}
static bool8 BattleHasPsychic(void)
{
    u32 side,slot;
    for(side=0;side<2;side++)for(slot=0;slot<MAX_MON_MOVES;slot++)
        if(ArenaMoveId(side,slot)==MOVE_PSYCHIC)return TRUE;
    return FALSE;
}
static void PsychicReset(void);
static void TickPsychic(void);
static void DrawPsychicAuras(void);
static void ElementsInit(void);
static void ElementsResetBattle(void);
static void ElementsTick(void);
static void ElementsDraw(void);
static void StormReset(void);
static void StormStart(void);
static void StormBiome(const u32 *tiles,const u16 *pal);
static void StormSprites(void);
static u16 StormSpeed(u8 side,u16 speed);
static bool8 StormEvade(void);
static bool8 StormActive(void);
static void StormDraw(u16 budget);
static bool8 SpecialBusy(u8 side);
static void DiveStart(u8 side,u16 move);
static bool8 SpecialInvulnerable(u8 side);
static bool8 WarpStart(u8 side);
static bool8 TossStart(u8 side);
static void SpecialReset(void);
static void SpecialHide(void);
static void SpecialTick(void);
static void SpecialDraw(void);
static bool8 TossActive(void);
extern u32 gArenaWarpTossTelemetry[8];
static bool8 ElementReact(struct ArenaShot *shot);
static void GuardReset(void);
static void GuardRebind(void);
static void GuardTick(void);
static void GuardDraw(void);
static bool8 ClashBusy(void);
static void ClashReset(void);
static void GuardStart(u8 side);
static bool8 GuardIntercept(u8 side,u16 move);
static bool8 GuardBusy(void);
static bool8 ElementMotion(u8 side,s32 dx,s32 dy);
// Matt: reclaimed 4 KiB EWRAM. The arena's two 2 KiB frame buffers share the
// flash sector buffer, which is only used by save/load (never during a battle).
#include "save.h"
STATIC_ASSERT(sizeof(gSaveDataBuffer) >= 2 * 512 * sizeof(u32), ArenaFrameTilesFit);
#define sMonFrameTiles ((u32 (*)[512])(void *)&gSaveDataBuffer)
static EWRAM_INIT bool8 sDemoRequested = FALSE;
EWRAM_DATA struct RealtimeArenaTelemetry gRealtimeArenaTelemetry = {};
EWRAM_DATA struct ArenaAiTelemetry gArenaAiTelemetry = {};
EWRAM_DATA struct ArenaSpriteTelemetry gArenaSpriteTelemetry = {};
EWRAM_DATA struct ArenaCombatTelemetry gArenaCombatTelemetry = {};
EWRAM_DATA struct ArenaMoveTelemetry gArenaMoveTelemetry = {};
// Acid Defense drops, Bite interrupts, Color Change activations.
EWRAM_DATA u32 gArenaAdventureTelemetry[3] = {};
EWRAM_DATA u32 gArenaStatusTelemetry[4] = {}; // burn applications[2], pulses[2]
EWRAM_DATA struct ArenaFrameTelemetry gArenaFrameTelemetry = {};
EWRAM_DATA bool8 gRealtimeArenaRestoringFaint = FALSE;
EWRAM_DATA bool8 gRealtimeArenaQuietResult = FALSE;
EWRAM_DATA bool8 gRealtimeArenaQuietIntro = FALSE;
EWRAM_DATA u16 gArenaQuietDisplay = 0;
EWRAM_DATA struct ArenaResultTelemetry gArenaResultTelemetry = {};
EWRAM_DATA struct ArenaIntroTelemetry gArenaIntroTelemetry = {};
EWRAM_DATA u16 gArenaRenderTelemetry[8] = {};
static EWRAM_DATA u8 sHudRecovery=0;
static EWRAM_INIT bool8 sMoveMenuReady=FALSE;
EWRAM_DATA u8 gArenaMenuRequest=0; // 1: native bag, 2: native party
EWRAM_DATA u16 gArenaCompatibilityReason=ARENA_COMPAT_OK;
static EWRAM_DATA u8 sPausePage=0;
static EWRAM_DATA u8 sPauseChoice=0;
static EWRAM_INIT u16 sBideDamage[2]={};
static void ArenaExpFieldTick(void);

static const struct BgTemplate sArenaBgs[] =
{
    { .bg = 0, .charBaseIndex = 0, .mapBaseIndex = 31,
      .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0 },
    { .bg = 1, .charBaseIndex = 1, .mapBaseIndex = 30,
      .screenSize = 0, .paletteMode = 1, .priority = 2, .baseTile = 0 },
    { .bg = 2, .charBaseIndex = 3, .mapBaseIndex = 29,
      .screenSize = 0, .paletteMode = 0, .priority = 2, .baseTile = 0 }
};
static const u32 sForestTiles[] = INCBIN_U32(".arena-dev/art/forest.8bpp");
static const u16 sForestPalette[] = INCBIN_U16(".arena-dev/art/forest.gbapal");
static const u16 sForestMap[] = INCBIN_U16(".arena-dev/art/forest.bin");
static const struct WindowTemplate sArenaWindows[] =
{
    { .bg = 0, .tilemapLeft = 0, .tilemapTop = 0,
      .width = 30, .height = 3, .paletteNum = 0, .baseBlock = 1 },
    { .bg = 0, .tilemapLeft = 1, .tilemapTop = 6,
      .width = 28, .height = 9, .paletteNum = 0, .baseBlock = 91 },
    { .bg = 0, .tilemapLeft = 0, .tilemapTop = 18,
      .width = 30, .height = 2, .paletteNum = 0, .baseBlock = 343 },
    DUMMY_WIN_TEMPLATE
};
static const u16 sArenaPalette[16] =
{
    RGB(3, 6, 9), RGB(4, 9, 12), RGB(6, 13, 15), RGB(10, 20, 19),
    RGB(29, 31, 29), RGB(8, 29, 23), RGB(31, 14, 8), RGB(29, 24, 9),
    RGB(12, 15, 17), RGB(2, 3, 6), RGB(31, 31, 31),
    RGB(3, 6, 9), RGB(2, 6, 9)
};
static const u32 sShotTiles[8] =
{
    0x00022000, 0x00211200, 0x02111120, 0x21111112,
    0x21111112, 0x02111120, 0x00211200, 0x00022000
};
// Shadows share unused shot-palette entries, preserving their exact colors and
// leaving two OBJ palettes for the original trainer artwork (16-slot GBA limit).
static const u16 sPlayerShotPalette[16] = {RGB_BLACK, RGB(10,31,25), RGB_WHITE, RGB(7,11,10), RGB(4,8,8)};
static const u16 sEnemyShotPalette[16] = {RGB_BLACK, RGB(31,10,5), RGB(31,25,10)};
static const struct SpriteSheet sShotSheet = {sShotTiles, sizeof(sShotTiles), SHOT_TAG};
static const struct SpritePalette sShotPalettes[] =
{
    {sPlayerShotPalette, SHOT_TAG}, {sEnemyShotPalette, SHOT_TAG + 1}
};
static const struct OamData sShotOam =
{
    .shape = SPRITE_SHAPE(8x8), .size = SPRITE_SIZE(8x8), .priority = 0
};
static const struct SpriteTemplate sShotTemplate =
{
    .tileTag = SHOT_TAG, .paletteTag = SHOT_TAG, .oam = &sShotOam,
    .anims = gDummySpriteAnimTable, .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCallbackDummy
};
static const struct OamData sMonOam =
{
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .shape = SPRITE_SHAPE(64x64), .size = SPRITE_SIZE(64x64), .priority = 0
};
static const struct SpriteTemplate sMonTemplate =
{
    .tileTag = MON_TAG, .paletteTag = TAG_NONE, .oam = &sMonOam,
    .anims = gDummySpriteAnimTable, .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCallbackDummy
};
static const u32 sShadowTiles[] = {0x00000000, 0x33300000, 0x44433300, 0x44444330, 0x44433300, 0x33300000, 0x00000000, 0x00000000, 0x00000000, 0x00000333, 0x00333444, 0x03344444, 0x00333444, 0x00000333, 0x00000000, 0x00000000};
static const struct SpriteSheet sShadowSheet = {sShadowTiles, sizeof(sShadowTiles), SHOT_TAG + 2};
static const struct OamData sShadowOam =
{
    .shape = SPRITE_SHAPE(16x8), .size = SPRITE_SIZE(16x8), .priority = 1
};
static const struct SpriteTemplate sShadowTemplate =
{
    .tileTag = SHOT_TAG + 2, .paletteTag = SHOT_TAG, .oam = &sShadowOam,
    .anims = gDummySpriteAnimTable, .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCallbackDummy
};
static const struct OamData sAimOam =
{
    .shape = SPRITE_SHAPE(32x32), .size = SPRITE_SIZE(32x32), .priority = 1
};
static const struct SpriteTemplate sAimTemplate =
{
    .tileTag = SHOT_TAG + 3, .paletteTag = SHOT_TAG, .oam = &sAimOam,
    .anims = gDummySpriteAnimTable, .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCallbackDummy
};
static const u8 sTextPaused[] = _("ARROWS: PICK MOVE   START: PLAY");
static const u8 sTextPP[] = _(" PP ");
static const u8 sMoveDirections[4][7] = {_("UP"),_("RIGHT"),_("DOWN"),_("LEFT")};
static const u8 sMoveKinds[6][8] = {_("CLASSIC"),_("HIT"),_("DASH"),_("SHOT"),_("CONE"),_("BOOST")};
static const u8 sMoveStatusLabel[] = _("STATUS");
#if ARENA_LAB
static const u8 sDemoName[] = _("GERMAN");
#else
static const u8 sDemoName[] = _("ARENA");
#endif

// Native encounter shortcut for the opt-in practice SAVE, not a ROM mailbox.
// Curated levels retain genuine compatible moves from each native learnset.
static const struct {u16 species;u8 level;} sPracticeRivals[] =
{
    {SPECIES_EEVEE,36}, {SPECIES_BLASTOISE,36}, {SPECIES_SCIZOR,15},
    {SPECIES_BLAZIKEN,20}, {SPECIES_DRAGONITE,30}, {SPECIES_CHARIZARD,36},
    {SPECIES_SCEPTILE,43}, {SPECIES_RAYQUAZA,60}, {SPECIES_MEWTWO,66}
};
static EWRAM_INIT u8 sPracticeRival = 0;

void RealtimeArena_PracticeTick(void)
{
    extern const u8 EventScript_ArenaPracticeBattle[];
    u32 i;
    ArenaExpFieldTick();
    if (!FlagGet(FLAG_ARENA_PRACTICE) || gPaletteFade.active
        || ArePlayerFieldControlsLocked() || ScriptContext_IsEnabled()
        || (JOY_HELD(L_BUTTON | R_BUTTON) != (L_BUTTON | R_BUTTON))
        || !JOY_NEW(L_BUTTON | R_BUTTON)) return;
    for (i = 0; i < gPlayerPartyCount; i++)
        if (GetMonData(&gPlayerParty[i], MON_DATA_HP) && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG)) break;
    if (i == gPlayerPartyCount) return;
    CreateScriptedWildMon(sPracticeRivals[sPracticeRival].species,
                         sPracticeRivals[sPracticeRival].level, ITEM_NONE);
    sPracticeRival = (sPracticeRival + 1) % ARRAY_COUNT(sPracticeRivals);
    ScriptContext_SetupScript(EventScript_ArenaPracticeBattle);
}

static void CB2_ArenaInit(void);
static void CB2_Arena(void);
static void ArenaExit(bool8 fainted);
static void CaptureHud(void);
static void AiChooseGoal(void);
static u16 AiRandom(void);

static s32 Abs(s32 n) { return n < 0 ? -n : n; }
static s32 Clamp(s32 n, s32 lo, s32 hi) { return n < lo ? lo : n > hi ? hi : n; }
static u16 MonVisualScale(u8 side)
{
    (void)side;
    return 256;
}
static void MonVisualScaleApply(void)
{
    u32 side;
    for(side=0;side<2;side++)if(sArena.bodies[side].art&&MonVisualScale(side)!=256)
    {
        struct Sprite *p=&gSprites[sArena.bodies[side].sprite];
        struct OamMatrix *m=&gOamMatrices[p->oam.matrixNum];
        u16 scale=MonVisualScale(side);
        p->oam.affineMode=ST_OAM_AFFINE_DOUBLE;
        CalcCenterToCornerVec(p,p->oam.shape,p->oam.size,p->oam.affineMode);
        SetOamMatrix(p->oam.matrixNum,m->a*256/scale,m->b*256/scale,m->c*256/scale,m->d*256/scale);
    }
}

// Ghost types phase through the arena walls and through the objects inside
// it: leaving through one edge brings them in from the opposite one, and rocks,
// logs, bushes, crystals and pods never block their movement. Only a battler
// with the Ghost type; attacks and projectiles still interact with cover.
#define ARENA_PHASE_MARGIN 14
EWRAM_DATA struct ArenaGhostTelemetry gArenaGhostTelemetry = {};

static bool8 GhostBody(u8 side) { return IS_BATTLER_OF_TYPE(side, TYPE_GHOST); }

// Inside a wall or overlapping a solid object: drawn translucent.
static bool8 Phasing(u8 side)
{
    const struct ArenaBody *body = &sArena.bodies[side];
    return GhostBody(side) && (body->x < ARENA_MIN_X * Q || body->x > ARENA_MAX_X * Q
        || body->y < ARENA_MIN_Y * Q || body->y > ARENA_MAX_Y * Q
        || !ArenaNav_LineClear(body->x / Q, body->y / Q, body->x / Q, body->y / Q, ARENA_BODY_RADIUS));
}

// Once a ghost has pushed a body length into a wall, it reappears inside the
// opposite wall, still moving the same way. One glow marks each side.
static void PhaseWrap(u8 side, s32 dx, s32 dy)
{
    struct ArenaBody *body = &sArena.bodies[side];
    s16 exitX = body->x / Q, exitY = body->y / Q;
    bool8 crossed = FALSE;
    if (dx < 0 && body->x <= (ARENA_MIN_X - ARENA_PHASE_MARGIN) * Q)
    { body->x = (ARENA_MAX_X + ARENA_PHASE_MARGIN) * Q; crossed = TRUE; }
    else if (dx > 0 && body->x >= (ARENA_MAX_X + ARENA_PHASE_MARGIN) * Q)
    { body->x = (ARENA_MIN_X - ARENA_PHASE_MARGIN) * Q; crossed = TRUE; }
    if (dy < 0 && body->y <= (ARENA_MIN_Y - ARENA_PHASE_MARGIN) * Q)
    { body->y = (ARENA_MAX_Y + ARENA_PHASE_MARGIN) * Q; crossed = TRUE; }
    else if (dy > 0 && body->y >= (ARENA_MAX_Y + ARENA_PHASE_MARGIN) * Q)
    { body->y = (ARENA_MIN_Y - ARENA_PHASE_MARGIN) * Q; crossed = TRUE; }
    if (!crossed) return;
    gArenaGhostTelemetry.crossings[side]++;
    ArenaFeedback_CaptureGlow(exitX, exitY, TRUE);
    ArenaFeedback_CaptureGlow(body->x / Q, body->y / Q, FALSE);
    PlaySE(SE_M_TELEPORT);
}

// Real-time Rollout. The wind-up tumbles the body in place, slowly at first,
// then it rolls along its facing, gaining speed; power doubles for every
// ROLLOUT_STAGE_PX rolled (up to x8) and once more after Defense Curl. Cover
// in the way is hit with the roll's strength and rolled through if it breaks.
#define ROLLOUT_STAGE_PX 40
#define ROLLOUT_MAX_STAGE 3
#define ROLLOUT_SPIN 0x1000
// Frames a rolled-over rival stays flattened; it springs back over the last few.
#define ROLLOUT_FLAT_FRAMES 18
#define ROLLOUT_FLAT_SPRING 10
// Camera elevation used to project the roll (35 degrees): sin and cos in Q8.
#define ROLLOUT_CAM_SIN 147
#define ROLLOUT_CAM_COS 210
EWRAM_DATA u8 gArenaRolloutStage = 0;

static bool8 Rolling(u8 side)
{
    const struct ArenaBody *body = &sArena.bodies[side];
    return body->shotTimer && ArenaMoveId(side,body->shotSlot) == MOVE_ROLLOUT;
}

// A third of full speed at launch, full speed half a second in.
static s32 RolloutSpeed(const struct ArenaBody *body, const struct ArenaMoveProfile *p)
{
    return p->speed * (85 + 171 * min(30, body->actionAge) / 30) / 256;
}

static void RolloutEnd(struct ArenaBody *body)
{
    body->shotTimer = 0;
    body->actionLife = 1;
}

#include "arena_rollout.inc"
#include "arena_double_team.inc"
#include "arena_cover.inc"
#include "arena_barrier.inc"

static bool8 SupportedMove(u16 move)
{
    return ArenaMoves_Get(move) != NULL;
}

static u8 FirstMove(u8 side, bool8 needsPP)
{
    u32 i;
    // Start on an attack when possible. Adding Growl/Screech must not change
    // a previously playable lead into an apparently harmless default action.
    for (i = 0; i < MAX_MON_MOVES; i++)
        if (SupportedMove(ArenaMoveId(side,i)) && GetMovePower(ArenaMoveId(side,i))
            && (!needsPP || ArenaMovePP(side,i))) return i;
    // Legal status-only sets (e.g. newly caught Ralts/Cascoon) remain usable.
    // Never hide an unsupported action such as Bide behind an adapted buff.
    for (i = 0; i < MAX_MON_MOVES; i++)
        if (ArenaMoveId(side,i) && !SupportedMove(ArenaMoveId(side,i)))
            return ARENA_NO_MOVE;
    if(needsPP && NeedsStruggle(side))return ARENA_STRUGGLE_SLOT;
    for (i = 0; i < MAX_MON_MOVES; i++)
        if (SupportedMove(ArenaMoveId(side,i))
            && (!needsPP || ArenaMovePP(side,i))) return i;
    return ARENA_NO_MOVE;
}

static bool8 SupportedBattler(u8 side)
{
    u32 i;
    // Matt mod: held items never force classic. Oran/Sitrus, Leftovers and
    // status-cure berries are handled in the arena; everything else is inert
    // (native damage still applies type-boost items like Charcoal).
    gArenaCompatibilityReason=ARENA_COMPAT_STATUS|(side<<8);
    if (
           (gBattleMons[side].status1 & ~(STATUS1_ANY|STATUS1_TOXIC_COUNTER))
        || (gBattleMons[side].volatiles.confusionTurns||gBattleMons[side].volatiles.wrapped||gBattleMons[side].volatiles.escapePrevention||gBattleMons[side].volatiles.substitute||gBattleMons[side].volatiles.infatuation||gBattleMons[side].volatiles.transformed||gBattleMons[side].volatiles.foresight||gBattleMons[side].volatiles.cursed||gBattleMons[side].volatiles.nightmare)
        || gBattleMons[side].volatiles.leechSeed || gBattleMons[side].volatiles.root || gBattleMons[side].volatiles.lockOn || gBattleMons[side].volatiles.mudSport || gBattleMons[side].hp == 0)
        return FALSE;
    gArenaCompatibilityReason=ARENA_COMPAT_MOVES|(side<<8);
    if(FirstMove(side,TRUE)==ARENA_NO_MOVE)return FALSE;
    gArenaCompatibilityReason=ARENA_COMPAT_ABILITY|(side<<8);
    switch (gBattleMons[side].ability)
    {
    case ABILITY_NONE:
    case ABILITY_VOLT_ABSORB: // Matt mod: passive here (no absorb heal in arena)
    case ABILITY_SOUNDPROOF:
    case ABILITY_STENCH:
    case ABILITY_OVERGROW:
    case ABILITY_BLAZE:
    case ABILITY_TORRENT:
    case ABILITY_SWARM:
    case ABILITY_RUN_AWAY:
    case ABILITY_PICKUP:
    case ABILITY_KEEN_EYE:
    case ABILITY_SHIELD_DUST:
    case ABILITY_COMPOUND_EYES:
    case ABILITY_HUSTLE:
    case ABILITY_HUGE_POWER:
    case ABILITY_PURE_POWER:
    case ABILITY_BATTLE_ARMOR:
    case ABILITY_SHELL_ARMOR:
    case ABILITY_LEVITATE:
    case ABILITY_WONDER_GUARD:
    case ABILITY_INNER_FOCUS:
    case ABILITY_INTIMIDATE:
    case ABILITY_CHLOROPHYLL: // No weather is admitted by Eligible().
    case ABILITY_INSOMNIA:   // Native SetMoveEffect rejects sleep.
    case ABILITY_GUTS:       // Native damage handles its burn interaction.
    case ABILITY_LIGHTNING_ROD: // Gen III redirection only, doubles excluded.
    case ABILITY_COLOR_CHANGE:
    case ABILITY_WATER_VEIL: // Original SetMoveEffect prevents burns.
    case ABILITY_SWIFT_SWIM: // Weather encounters remain classic.
    case ABILITY_RAIN_DISH:
    case ABILITY_DRIZZLE: // Native intro owns activation; Rain Dance owns flood.
    case ABILITY_DROUGHT: // Matt v13: native intro sets sun, like Drizzle.
    case ABILITY_SERENE_GRACE: // Matt v13: native secondary-chance code only.
    case ABILITY_FORECAST: // Matt v13: form change not drawn in the arena.
    case ABILITY_DAMP: // Explosion/Selfdestruct remain classic moves.
    case ABILITY_STURDY: // Gen III OHKO prevention; OHKO moves remain classic.
    case ABILITY_ROCK_HEAD: // Recoil moves remain classic.
    case ABILITY_PLUS: // Partner-only effects; doubles remain classic.
    case ABILITY_MINUS:
    case ABILITY_THICK_FAT: // Native CalculateBaseDamage handles Fire/Ice.
    case ABILITY_EARLY_BIRD: // Two sleep units per active residual interval.
    case ABILITY_ILLUMINATE: // Encounter-rate ability; no in-battle effect.
    case ABILITY_HYPER_CUTTER: // ChangeStatBuffs keeps its Attack protection.
    case ABILITY_PRESSURE: // Fire charges an extra PP for opposing targeted actions.
    case ABILITY_AIR_LOCK: // Weather encounters are still excluded by Eligible().
    case ABILITY_SAND_VEIL:
    case ABILITY_CLOUD_NINE:
    case ABILITY_CLEAR_BODY: // Native ChangeStatBuffs retains rejection.
    case ABILITY_WHITE_SMOKE:
    case ABILITY_LIMBER: // Native SetMoveEffect rejects paralysis.
    case ABILITY_OBLIVIOUS: // Infatuation moves still unsupported.
    case ABILITY_OWN_TEMPO: // Confusion moves still unsupported.
    case ABILITY_VITAL_SPIRIT: // Native SetMoveEffect rejects sleep.
    case ABILITY_MAGMA_ARMOR: // Native SetMoveEffect rejects freezing.
    case ABILITY_MARVEL_SCALE: // Native CalculateBaseDamage applies Defense.
    case ABILITY_STICKY_HOLD: // Item-removal moves are not arena profiles.
    case ABILITY_SUCTION_CUPS: // Forced-switch moves are not arena profiles.
    case ABILITY_NATURAL_CURE: // Native switch transaction cures the outgoing mon.
    case ABILITY_FLASH_FIRE:
    case ABILITY_WATER_ABSORB:
    case ABILITY_SHED_SKIN: // Timed native-rate cure below, including saved burns.
    case ABILITY_SYNCHRONIZE:
    case ABILITY_IMMUNITY:
    case ABILITY_STATIC:
    case ABILITY_POISON_POINT:
    case ABILITY_FLAME_BODY:
    case ABILITY_EFFECT_SPORE:
    case ABILITY_ROUGH_SKIN:   // v10: contact recoil 1/16 in the arena.
    case ABILITY_LIQUID_OOZE:  // v10: drain moves hurt the drainer.
    case ABILITY_MAGNET_PULL:  // Trapping only: nobody switches/flees mid-arena.
    case ABILITY_ARENA_TRAP:
    case ABILITY_SHADOW_TAG:
    case ABILITY_TRACE:        // Inert in the arena (no ability copy).
    case ABILITY_TRUANT:       // v10: every other action is skipped (extra recovery).
        return TRUE;
    case ABILITY_CUTE_CHARM:
        // Do not silently suppress a contact ability. These can enter only if
        // the opposing arena moves cannot trigger it; otherwise the complete
        // encounter stays in native classic mode, including its status rolls.
        for (i = 0; i <= MAX_MON_MOVES; i++)
            if (SupportedMove(ArenaMoveId(side ^ 1,i))
                && ArenaMovePP(side ^ 1,i)
                && MoveMakesContact(ArenaMoveId(side ^ 1,i)))
            {
                gArenaCompatibilityReason=ARENA_COMPAT_CONTACT|(side<<8);
                return FALSE;
            }
        return TRUE;
    default: // Gen 4+ abilities stay in native classic mode.
        break;
    }
    return FALSE;
}

void RealtimeArena_ResetBattle(void)
{
    memset(sArenaResident, 0, sizeof(sArenaResident));
    sArenaSendoutMask = 3;
    sMattPrompt = sMattCatchMode = sMattStatusUsed = sMattThrowWait = sMattBackoff = 0;
    sMattLeftoversClock[0] = sMattLeftoversClock[1] = 0;
    sProtectChain[0] = sProtectChain[1] = 0; sMattItemCd = 0; sMattProtectGap = 0; sEndureLife[0] = sEndureLife[1] = 0;
    sLastHit[0] = sLastHit[1] = 0; sLastHitAge[0] = sLastHitAge[1] = 0; sLastHitSpecial = 0;
    sDestinyLife[0] = sDestinyLife[1] = sPerish[0] = sPerish[1] = sEncore[0] = sEncore[1] = sSeShake = 0;
    sMattBall = ITEM_POKE_BALL; sMattChoice = 0; sMattForceCatch = 0; sMattShots = 0; sMattSwitch = 4<<2;
    gArenaMenuRequest=0;
    sPausePage=0;
    memset(gArenaEvolutionTelemetry, 0, sizeof(gArenaEvolutionTelemetry));
    gRealtimeArenaQuietIntro = FALSE;
    ElementsResetBattle();
    StormReset();
    CoverReset();
    GuardReset();
    DecoyReset();
    sEnvironmentInitialized=FALSE;
    memset(&sArena, 0, sizeof(sArena));
    gRealtimeArenaTelemetry.active = FALSE;
    gRealtimeArenaRestoringFaint = FALSE;
    gRealtimeArenaQuietResult = FALSE;
    memset(&gArenaResultTelemetry,0,sizeof(gArenaResultTelemetry));
    memset(gArenaAdventureTelemetry,0,sizeof(gArenaAdventureTelemetry));
    memset(gArenaStatusTelemetry,0,sizeof(gArenaStatusTelemetry));
    gArenaIntroTelemetry.started=gMain.vblankCounter1;
    gArenaIntroTelemetry.elapsed=gArenaIntroTelemetry.skipped=0;
}

static bool8 Eligible(void)
{
    gArenaCompatibilityReason=ARENA_COMPAT_LAB;
#if ARENA_LAB
    if (gArenaLabMailbox.classic) return FALSE;
#endif
    // FIRST_BATTLE is still a 1v1 with a real starter and native return script.
    gArenaCompatibilityReason=ARENA_COMPAT_FORMAT;
    if (sArena.classic || gBattlersCount != 2 || (gBattleTypeFlags & ~(BATTLE_TYPE_FIRST_BATTLE | BATTLE_TYPE_IS_MASTER | BATTLE_TYPE_TRAINER))
        || gAbsentBattlerFlags)
        return FALSE;
    if(!SupportedBattler(0) || !SupportedBattler(1))return FALSE;
    gArenaCompatibilityReason=ARENA_COMPAT_OK;
    return TRUE;
}

bool8 RealtimeArena_CanSkipIntro(void)
{
    // Native first-turn scripts still apply abilities/weather; only their
    // classic presentation is suppressed before the arena's own send-out.
    return Eligible();
}

bool8 RealtimeArena_TryStart(void)
{
    if(!Eligible())
    {
        // Quiet faint resolution intentionally keeps the arena framebuffer.
        // If the replacement needs classic mechanics, rebuild its complete UI
        // before a controller writes a ChooseAction menu into that framebuffer.
        if(gRealtimeArenaQuietResult)
        {
            gRealtimeArenaQuietIntro=FALSE;
            RealtimeArena_ShowResultUi();
            return TRUE;
        }
        // An unsupported replacement must retain the real classic fallback.
        if(gRealtimeArenaQuietIntro)
        {gRealtimeArenaQuietIntro=FALSE;SetGpuReg(REG_OFFSET_DISPCNT,gArenaQuietDisplay);}
        return FALSE;
    }
    // Wait inside selection rather than issuing a classic ChooseAction command
    // while the intro/fade is still completing. That menu would own the controller.
    if (gBattleControllerExecFlags || gPaletteFade.active) return TRUE;
    sArena.savedCB1 = gMain.callback1;
    gMain.callback1 = NULL;
    SetMainCallback2(CB2_ArenaInit);
    return TRUE;
}

static void VBlank_Arena(void)
{
    LoadOam();
    ArenaRender_Flush();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

#define H(n) ((((n)&192)==64?15:0)|(((n)&48)==16?240:0)|(((n)&12)==4?3840:0)|(((n)&3)==1?61440:0))
static const u16 sHudRowMask[256]={
    H(0),H(1),H(2),H(3),H(4),H(5),H(6),H(7),H(8),H(9),H(10),H(11),H(12),H(13),H(14),H(15),
    H(16),H(17),H(18),H(19),H(20),H(21),H(22),H(23),H(24),H(25),H(26),H(27),H(28),H(29),H(30),H(31),
    H(32),H(33),H(34),H(35),H(36),H(37),H(38),H(39),H(40),H(41),H(42),H(43),H(44),H(45),H(46),H(47),
    H(48),H(49),H(50),H(51),H(52),H(53),H(54),H(55),H(56),H(57),H(58),H(59),H(60),H(61),H(62),H(63),
    H(64),H(65),H(66),H(67),H(68),H(69),H(70),H(71),H(72),H(73),H(74),H(75),H(76),H(77),H(78),H(79),
    H(80),H(81),H(82),H(83),H(84),H(85),H(86),H(87),H(88),H(89),H(90),H(91),H(92),H(93),H(94),H(95),
    H(96),H(97),H(98),H(99),H(100),H(101),H(102),H(103),H(104),H(105),H(106),H(107),H(108),H(109),H(110),H(111),
    H(112),H(113),H(114),H(115),H(116),H(117),H(118),H(119),H(120),H(121),H(122),H(123),H(124),H(125),H(126),H(127),
    H(128),H(129),H(130),H(131),H(132),H(133),H(134),H(135),H(136),H(137),H(138),H(139),H(140),H(141),H(142),H(143),
    H(144),H(145),H(146),H(147),H(148),H(149),H(150),H(151),H(152),H(153),H(154),H(155),H(156),H(157),H(158),H(159),
    H(160),H(161),H(162),H(163),H(164),H(165),H(166),H(167),H(168),H(169),H(170),H(171),H(172),H(173),H(174),H(175),
    H(176),H(177),H(178),H(179),H(180),H(181),H(182),H(183),H(184),H(185),H(186),H(187),H(188),H(189),H(190),H(191),
    H(192),H(193),H(194),H(195),H(196),H(197),H(198),H(199),H(200),H(201),H(202),H(203),H(204),H(205),H(206),H(207),
    H(208),H(209),H(210),H(211),H(212),H(213),H(214),H(215),H(216),H(217),H(218),H(219),H(220),H(221),H(222),H(223),
    H(224),H(225),H(226),H(227),H(228),H(229),H(230),H(231),H(232),H(233),H(234),H(235),H(236),H(237),H(238),H(239),
    H(240),H(241),H(242),H(243),H(244),H(245),H(246),H(247),H(248),H(249),H(250),H(251),H(252),H(253),H(254),H(255)
};
#undef H

static void PrintWindow(u8 window, u32 x, u32 y, const u8 *text, u8 color)
{
    // Same original Emerald glyphs, tightly bounded HUD blit. Invoking the full
    // dialogue state machine per HP/PP change stalled several hardware frames.
    // Foreground only: this strip is cleared once, shadow/background are both 9.
    u32 n=0,tiles=sArenaWindows[window].width,height=sArenaWindows[window].height*8;
    while(*text!=EOS&&n++<32)
    {
        u32 glyph=*text++,width=gFontSmallLatinGlyphWidths[glyph],row;
        const u16 *src=gFontSmallLatinGlyphs+glyph*32;
        if(x+width>tiles*8)break;
        for(row=0;row<13&&y+row<height;row++)
        {
            u32 bits=src[row<8?row:row+8],py=y+row,shift=(x&7)*4;
            u32 mask=sHudRowMask[bits>>8]|(sHudRowMask[bits&255]<<16);
            u32 ink,colorWord=0x11111111*color;
            u32 *dest=(u32*)gWindows[window].tileData+((py/8)*tiles+x/8)*8+(py&7);
            if(width<8)mask&=0xffffffff>>((8-width)*4);
            ink=mask&colorWord;
            *dest=(*dest&~(mask<<shift))|(ink<<shift);
            if(shift&&x/8<tiles-1)dest[8]=(dest[8]&~(mask>>(32-shift)))|(ink>>(32-shift));
        }
        x+=width;
    }
}

static void Print(u32 x,u32 y,const u8 *text,u8 color)
{
    PrintWindow(0,x,y,text,color);
}

static void WindowRect(u8 window,u32 x,u32 top,u32 width,u32 height,u32 color)
{
    u32 y;
    for(y=top;y<top+height;y++)
    {
        u32 at=x,left=width;
        while(left)
        {
            u32 count=min(left,8-(at&7)),shift=(at&7)*4;
            u32 mask=(0xffffffff>>((8-count)*4))<<shift;
            u32 *dest=(u32*)gWindows[window].tileData+((y/8)*sArenaWindows[window].width+at/8)*8+(y&7);
            *dest=(*dest&~mask)|(0x11111111*color&mask);
            left-=count;at+=count;
        }
    }
}

static void HudRect(u32 x,u32 top,u32 width,u32 height,u32 color)
{
    WindowRect(0,x,top,width,height,color);
}

static void PrintPopup(u32 x, u32 y, const u8 *text, u8 color)
{
    PrintWindow(1,x,y,text,color);
}

static void DrawStage(void)
{
    // Art is on BG1. Clearing an overlay never redraws terrain or erases rocks.
    // Keep the cached HUD pixels too: pause must not erase unchanged HP/names.
    ClearWindowTilemap(1);
    sMoveMenuReady=FALSE;
    CopyWindowToVram(1, COPYWIN_MAP);
}

static u8 RecoveryPips(void)
{
    const struct ArenaBody *body=&sArena.bodies[0];
    const struct ArenaMoveProfile *p=ArenaMoves_Get(ArenaMoveId(0,body->shotSlot));
    if(body->shotTimer || body->actionLife)return 0;
    if(!body->cooldown)return 8;
    return 8-min(8,body->cooldown*8/max(1,p?p->recovery:30));
}

static const u8 sQualityLevel[] = _("LVL ");
#include "arena_hud.inc"
#include "arena_quality.inc"

static void DrawHud(void)
{
    u32 side;
    u8 text[64];
    u8 *end;
    DrawStatusHud();
    if (sArena.paused)
    {
        if(sPausePage==3)
        {
            static const u8 labels[4][18]={_("POKEMON"),_("BAG"),_("MOVES"),_("RESUME")};
            static const u8 title[]=_("PAUSED");
            static const u8 help[]=_("UP/DOWN: CHOOSE   A: OPEN");
            FillWindowPixelBuffer(1,PIXEL_FILL(9));
            PrintPopup(8,3,title,5);
            for(side=0;side<4;side++)
            {
                if(side==sPauseChoice)WindowRect(1,7,18+side*11,4,7,5);
                PrintPopup(18,16+side*11,labels[side],side==sPauseChoice?5:4);
            }
            HudTinyWindow(1,7,67,help,7,52);
            PutWindowTilemap(1);CopyWindowToVram(1,COPYWIN_FULL);
        }
        else if(sPausePage)
        {
            FillWindowPixelBuffer(1, PIXEL_FILL(9));
            PrintPopup(7,4,sPausePage==1?sQualityBag:sQualityParty,5);
            PrintPopup(7,21,sPausePage==1?sQualityBagHelp:sQualityPartyHelp,4);
            PrintPopup(7,37,sQualityConfirm,4);
            PrintPopup(7,55,sQualityTabs,7);
            PutWindowTilemap(1);
            CopyWindowToVram(1,COPYWIN_FULL);
        }
        else
        {
        if(!sMoveMenuReady)
        {
        FillWindowPixelBuffer(1, PIXEL_FILL(9));
        PrintPopup(5, 3, sTextPaused, 7);
        for(side=0;side<MAX_MON_MOVES;side++)
        {
            u16 move=ArenaMoveId(0,side);
            const struct ArenaMoveProfile *p=ArenaMoves_Get(move);
            u8 pp=ArenaMovePP(0,side);
            u8 color=!p?8:!pp?6:4;
            u8 y=17+side*12;
            PrintPopup(5,y,sMoveDirections[side],color);
            PrintPopup(43,y,GetMoveName(move),color);
            PrintPopup(132,y,p&&!GetMovePower(move)?sMoveStatusLabel:sMoveKinds[p?p->kind:0],color);
            end=StringCopy(text,sTextPP);
            ConvertIntToDecimalStringN(end,pp,STR_CONV_MODE_LEFT_ALIGN,2);
            PrintPopup(180,y,text,color);
        }
        sMoveMenuReady=TRUE;
        }
        for(side=0;side<MAX_MON_MOVES;side++)
            WindowRect(1,0,19+side*12,3,8,side==sArena.bodies[0].moveSlot?5:9);
        PutWindowTilemap(1);
        CopyWindowToVram(1, COPYWIN_FULL);
        if(NeedsStruggle(0))
        {static const u8 help[]=_("A: STRUGGLE  B: MENU");HudTinyWindow(1,5,67,help,7,52);}
        else HudTinyWindow(1,5,67,sQualityTabs,7,52);
        CopyWindowToVram(1,COPYWIN_GFX);
        }
    }
    CopyWindowToVram(0, COPYWIN_GFX);
    CaptureHud();
    sArena.hudDirty = FALSE;
}

static void LoadAimMarker(void)
{
    u32 tiles[128] = {0};
    struct SpriteSheet sheet = {tiles, sizeof(tiles), SHOT_TAG + 3};
    u32 x, y;
    // Code-native pixel art: four small corners around the target. No opaque
    // center, so the actor's animation stays completely visible.
    for (y = 0; y < 32; y++)
        for (x = 0; x < 32; x++)
            if (((y == 4 || y == 27) && ((x >= 4 && x <= 10) || (x >= 21 && x <= 27)))
                || ((x == 4 || x == 27) && ((y >= 4 && y <= 10) || (y >= 21 && y <= 27))))
            {
                u32 tile = (y / 8) * 4 + x / 8;
                tiles[tile * 8 + y % 8] |= 1 << ((x % 8) * 4);
            }
    LoadSpriteSheet(&sheet);
}

#define TRAINER_TAG 0xA7A0
static void RefreshTrainerFacing(struct Sprite *sprite)
{
    sprite->oam.matrixNum=(sprite->oam.matrixNum&7)|(sprite->hFlip?8:0);
}
// Read-only test evidence: sprite ids, shadow ids, palette slots, graphics ids.
EWRAM_DATA u8 gArenaTrainerTelemetry[8] = {};
static const struct OamData sTrainerOam =
{
    .shape = SPRITE_SHAPE(16x32), .size = SPRITE_SIZE(16x32), .priority = 1
};

static u8 OpponentGraphics(void)
{
    u8 pic = GetTrainerPicFromId(TRAINER_BATTLE_PARAM.opponentA);
    if (pic >= TRAINER_PIC_ELITE_FOUR_SIDNEY && pic <= TRAINER_PIC_LEADER_WINONA)
        return OBJ_EVENT_GFX_SIDNEY + pic - TRAINER_PIC_ELITE_FOUR_SIDNEY;
    switch (pic)
    {
    case TRAINER_PIC_HIKER: return OBJ_EVENT_GFX_HIKER;
    case TRAINER_PIC_BUG_CATCHER: return OBJ_EVENT_GFX_BUG_CATCHER;
    case TRAINER_PIC_LASS: return OBJ_EVENT_GFX_LASS;
    case TRAINER_PIC_YOUNGSTER: return OBJ_EVENT_GFX_YOUNGSTER;
    case TRAINER_PIC_CAMPER: return OBJ_EVENT_GFX_CAMPER;
    case TRAINER_PIC_PICNICKER: return OBJ_EVENT_GFX_PICNICKER;
    case TRAINER_PIC_BEAUTY: return OBJ_EVENT_GFX_BEAUTY;
    case TRAINER_PIC_FISHERMAN: return OBJ_EVENT_GFX_FISHERMAN;
    case TRAINER_PIC_SAILOR: return OBJ_EVENT_GFX_SAILOR;
    case TRAINER_PIC_GENTLEMAN: return OBJ_EVENT_GFX_GENTLEMAN;
    case TRAINER_PIC_RICH_BOY: return OBJ_EVENT_GFX_RICH_BOY;
    case TRAINER_PIC_BLACK_BELT: return OBJ_EVENT_GFX_BLACK_BELT;
    case TRAINER_PIC_HEX_MANIAC: return OBJ_EVENT_GFX_HEX_MANIAC;
    case TRAINER_PIC_NINJA_BOY: return OBJ_EVENT_GFX_NINJA_BOY;
    case TRAINER_PIC_POKEFAN_M: return OBJ_EVENT_GFX_POKEFAN_M;
    case TRAINER_PIC_POKEFAN_F: return OBJ_EVENT_GFX_POKEFAN_F;
    case TRAINER_PIC_EXPERT_M: return OBJ_EVENT_GFX_EXPERT_M;
    case TRAINER_PIC_EXPERT_F: return OBJ_EVENT_GFX_EXPERT_F;
    case TRAINER_PIC_AQUA_GRUNT_M: return OBJ_EVENT_GFX_AQUA_MEMBER_M;
    case TRAINER_PIC_AQUA_GRUNT_F: return OBJ_EVENT_GFX_AQUA_MEMBER_F;
    case TRAINER_PIC_MAGMA_GRUNT_M: return OBJ_EVENT_GFX_MAGMA_MEMBER_M;
    case TRAINER_PIC_MAGMA_GRUNT_F: return OBJ_EVENT_GFX_MAGMA_MEMBER_F;
    case TRAINER_PIC_SWIMMER_M: return OBJ_EVENT_GFX_SWIMMER_M;
    case TRAINER_PIC_SWIMMER_F: return OBJ_EVENT_GFX_SWIMMER_F;
    case TRAINER_PIC_LEADER_JUAN: return OBJ_EVENT_GFX_JUAN;
    case TRAINER_PIC_CHAMPION_WALLACE: return OBJ_EVENT_GFX_WALLACE;
    case TRAINER_PIC_STEVEN: return OBJ_EVENT_GFX_STEVEN;
    case TRAINER_PIC_MAY: return OBJ_EVENT_GFX_MAY_NORMAL;
    case TRAINER_PIC_BRENDAN: return OBJ_EVENT_GFX_BRENDAN_NORMAL;
    default: return OBJ_EVENT_GFX_BOY_1;
    }
}

static void CreateSidelineTrainers(void)
{
    u32 i;
    memset(gArenaTrainerTelemetry, MAX_SPRITES, sizeof(gArenaTrainerTelemetry));
    for (i = 0; i < ((gBattleTypeFlags & BATTLE_TYPE_TRAINER)?2:1); i++)
    {
        bool8 female = (gSaveBlock2Ptr->playerGender != 0) ^ (i != 0);
        u8 graphicsId = i && (gBattleTypeFlags & BATTLE_TYPE_TRAINER) ? OpponentGraphics()
            : female ? OBJ_EVENT_GFX_MAY_NORMAL : OBJ_EVENT_GFX_BRENDAN_NORMAL;
        const struct ObjectEventGraphicsInfo *info = GetObjectEventGraphicsInfo(graphicsId);
        struct SpriteSheet sheet = {info->images[2].data, 256, TRAINER_TAG + i};
        struct SpritePalette pal = {ArenaObjectPalette(info->paletteTag), TRAINER_TAG + i};
        struct SpriteTemplate template =
        {
            .tileTag = TRAINER_TAG + i, .paletteTag = TRAINER_TAG + i,
            .oam = &sTrainerOam, .anims = gDummySpriteAnimTable,
            .images = NULL, .affineAnims = gDummySpriteAffineAnimTable,
            .callback = SpriteCallbackDummy
        };
        u8 id, shadow, palette;
        s16 x = i ? 232 : 8;
        s16 y = i ? 50 : 118;
        // Wild opponents remain wild. The opposite character is a spectator,
        // not a trainer battle. Keep both on the grass edge, behind combatants.
        if (LoadSpriteSheet(&sheet) == 0xFFFF) continue;
        palette = LoadSpritePalette(&pal);
        if (palette == 0xFF) { FreeSpriteTilesByTag(TRAINER_TAG + i); continue; }
        id = CreateSprite(&template, x, y, 12);
        if (id == MAX_SPRITES)
        {
            FreeSpriteTilesByTag(TRAINER_TAG + i);
            FreeSpritePaletteByTag(TRAINER_TAG + i);
            continue;
        }
        // The source side pose faces west; left trainer must face east.
        gSprites[id].hFlip = !i;
        RefreshTrainerFacing(&gSprites[id]);
        shadow = CreateSprite(&sShadowTemplate, x, y + 13, 13);
        gArenaTrainerTelemetry[i] = id;
        gArenaTrainerTelemetry[2+i] = shadow;
        gArenaTrainerTelemetry[4+i] = palette;
        gArenaTrainerTelemetry[6+i] = graphicsId;
    }
}

#include "arena_capture.inc"
#include "arena_biomes.inc"
#include "arena_toss.inc"
#include "arena_sendout.inc"
#include "arena_evolution.inc"

static void CB2_ArenaInit(void)
{
    u32 i;
    sArenaSendoutMask = 3;
    for(i=0;i<2;i++)
        if(sArenaResident[i].alive && gBattleMons[i].hp
            && sArenaResident[i].party==gBattlerPartyIndexes[i]
            && sArenaResident[i].personality==gBattleMons[i].personality)
            sArenaSendoutMask &= ~(1 << i);
    gRealtimeArenaQuietIntro = FALSE;
    gRealtimeArenaQuietResult = FALSE;
    SetVBlankCallback(NULL);
    SetHBlankCallback(NULL);
    ScanlineEffect_Stop();
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_FORCED_BLANK);
    FreeAllWindowBuffers();
    ResetTasks();
    ResetSpriteData();
    DecoyRebind();
    CoverRebind();
    BarrierReset();
    GuardRebind();
    ClashReset();
    SpecialReset();
    ArenaRender_Reset();
    FreeAllSpritePalettes();
    gReservedSpritePaletteCount = 2;
    ResetPaletteFade();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sArenaBgs, ARRAY_COUNT(sArenaBgs));
    InitWindows(sArenaWindows);
    sHudCached=FALSE;
    DeactivateAllTextPrinters();
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_OBJ);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(12,4));
    SetGpuReg(REG_OFFSET_BLDY, 0);
    LoadArenaBiome();
    LoadBgTiles(2, sSpaceTiles, sizeof(sSpaceTiles), 192);
    LoadBgTilemap(2, sSpaceMap, sizeof(sSpaceMap), 0);
    LoadPalette(sArenaPalette, 0, sizeof(sArenaPalette));
    DrawStage();
    ArenaNav_Init();
    if(!sEnvironmentInitialized)
    {
        ArenaPhysics_Init();
        sEnvironmentInitialized=TRUE;
    }
    else
    {
        // A party change must not respawn broken cover or erase wet ground.
        for(i=0;i<ARENA_OBSTACLES;i++)
        {
            gArenaProps[i].reserved=0;
            ArenaNav_SetObstacle(i,!gArenaProps[i].broken);
        }
    }
    LoadSpriteSheet(&sShotSheet);
    // The volcanic plateau is deliberately open: no indoor furniture or
    // invisible obstacles floating above its basalt floor.
    if(gArenaStage==16)
        for(i=0;i<ARENA_OBSTACLES;i++)
        {gArenaProps[i].broken=TRUE;gArenaProps[i].hp=0;ArenaNav_SetObstacle(i,FALSE);}
    LoadSpritePalette(&sShotPalettes[0]);
    LoadSpritePalette(&sShotPalettes[1]);
    if(gArenaStage==16)
    {
        static const u16 basaltShadow[]={RGB(3,2,2),RGB(1,1,1)};
        LoadPalette(basaltShadow,OBJ_PLTT_ID(IndexOfSpritePaletteTag(SHOT_TAG))+3,sizeof(basaltShadow));
    }
    LoadSpriteSheet(&sShadowSheet);
    LoadAimMarker();
    ArenaFeedback_Init();
    ArenaMoveFx_Init();
    ArenaTerrain_Init();
    ElementsInit();
    StormSprites();
    memset(sFlameReach,0,sizeof(sFlameReach));
    memset(sFlameElementMask,0,sizeof(sFlameElementMask));
    memset(gArenaFlameTelemetry,0,sizeof(gArenaFlameTelemetry));
    PsychicReset();
    ArenaPsychicFx_Init(BattleHasPsychic());
    memset(sArena.bodies, 0, sizeof(sArena.bodies));
    memset(sArena.shots, 0, sizeof(sArena.shots));
    memset(&gArenaSpriteTelemetry, 0, sizeof(gArenaSpriteTelemetry));
    memset(&gArenaCombatTelemetry, 0, sizeof(gArenaCombatTelemetry));
    memset(&gArenaMoveTelemetry, 0, sizeof(gArenaMoveTelemetry));
    memset(&gArenaFrameTelemetry,0,sizeof(gArenaFrameTelemetry));
    memset(&gArenaGhostTelemetry,0,sizeof(gArenaGhostTelemetry));
    memset(gArenaRenderTelemetry,0,sizeof(gArenaRenderTelemetry));
    gArenaFrameTelemetry.lastVBlank=gMain.vblankCounter1;
    sArena.lastBlast=0;
    RollBallInit();
    for (i = 0; i < 2; i++)
    {
        struct ArenaBody *body = &sArena.bodies[i];
        struct SpriteTemplate template = sMonTemplate;
        struct SpriteSheet sheet;
        u32 front;
        body->art = ArenaSprites_Get(gBattleMons[i].species);
        gArenaSpriteTelemetry.pmd[i] = body->art != NULL;
        if (body->art)
        {
            ArenaSprites_Decode(&body->art->animations[ARENA_ANIM_IDLE], 0, 0, sMonFrameTiles[i]);
            sheet.data = sMonFrameTiles[i];
            sheet.size = 2048; sheet.tag = MON_TAG + i * 2;
            LoadSpriteSheet(&sheet);
            LoadPalette(body->art->palette, OBJ_PLTT_ID(i), PLTT_SIZE_4BPP);
            template.tileTag = MON_TAG + i * 2;
        }
        else
        {
            // Other species retain the native fallback until their art is imported.
            for (front = 0; front < 2; front++)
            {
                void *picBuf = AllocZeroed(MON_PIC_SIZE);
                LoadSpecialPokePic(picBuf, gBattleMons[i].species, gBattleMons[i].personality, front);
                sheet.data = picBuf; sheet.size = 2048; sheet.tag = MON_TAG + i * 2 + front;
                LoadSpriteSheet(&sheet);
                Free(picBuf);
            }
            LoadPalette(GetMonSpritePalFromSpeciesAndPersonality(gBattleMons[i].species,
                gBattleMons[i].isShiny, gBattleMons[i].personality), OBJ_PLTT_ID(i), PLTT_SIZE_4BPP);
            template.tileTag = MON_TAG + i * 2 + (i ? 1 : 0);
        }
        body->x = 120 * Q;
        body->y = (i ? ARENA_MIN_Y : ARENA_MAX_Y) * Q;
        body->facing = i ? FACE_DOWN : FACE_UP;
        if(!(sArenaSendoutMask & (1 << i)))
        {
            body->x=sArenaResident[i].x;body->y=sArenaResident[i].y;
            body->facing=sArenaResident[i].facing;
        }
        body->moveSlot = FirstMove(i, TRUE);
        body->cooldown = i ? 30 : 15;
        if(!(sArenaSendoutMask & (1 << i)))
        {
            body->cooldown=sArenaResident[i].cooldown;
            body->dashCooldown=sArenaResident[i].dashCooldown;
            body->burnClock=sArenaResident[i].burnClock;
            body->statusActions=sArenaResident[i].statusActions;
            if(sArenaResident[i].moveSlot<=MAX_MON_MOVES
                && SupportedMove(ArenaMoveId(i,sArenaResident[i].moveSlot))
                && ArenaMovePP(i,sArenaResident[i].moveSlot))
                body->moveSlot=sArenaResident[i].moveSlot;
        }
        else if(sArenaResident[i].alive)
        {
            // A voluntary replacement has a short commitment cost. Forced
            // replacements after fainting and first send-outs are exempt.
            body->cooldown=60;body->dashCooldown=60;
        }
        body->sprite = CreateSprite(&template, body->x / Q, body->y / Q, 0);
        body->shadow = CreateSprite(&sShadowTemplate, body->x / Q, body->y / Q + 8, 10);
        gSprites[body->sprite].oam.paletteNum = i;
        gSprites[body->sprite].affineAnimPaused = TRUE;
        gSprites[body->sprite].affineAnimBeginning = FALSE;
        SetOamMatrix(gSprites[body->sprite].oam.matrixNum, body->art ? 0x100 : 0x200, 0, 0, body->art ? 0x100 : 0x200);
        body->drawnFrame = body->drawnDirection = 255;
    }
    sArena.cueSprite = CreateSprite(&sShotTemplate, 120, 32, 0);
    gSprites[sArena.cueSprite].oam.paletteNum = IndexOfSpritePaletteTag(SHOT_TAG + 1);
    gSprites[sArena.cueSprite].invisible = TRUE;
    sArena.aimSprite = CreateSprite(&sAimTemplate, 120, 32, 1);
    gSprites[sArena.aimSprite].invisible = TRUE;
    CreateSidelineTrainers();
    CaptureInit();
    sArena.aiRandom = gBattleMons[1].personality ^ 0xA12E7A11;
    sArena.style = gBattleMons[1].personality % 3;
    sArena.reaction = Clamp(21 - gBattleMons[1].level / 2, 8, 21);
    sArena.aimError = Clamp(12 - gBattleMons[1].level / 4, 3, 12);
    sArena.thinkTimer = 1; sArena.aimTimer = 0; sArena.goalTimer = 0;
    sArena.aiState = AI_REPOSITION;
    sArena.observed.x = sArena.previous.x = 120;
    sArena.observed.y = sArena.previous.y = ARENA_MAX_Y;
    sArena.goal.x = sArena.waypoint.x = 120;
    sArena.goal.y = sArena.waypoint.y = ARENA_MIN_Y;
    memset(&gArenaAiTelemetry, 0, sizeof(gArenaAiTelemetry));
    gArenaAiTelemetry.level = gBattleMons[1].level;
    gArenaAiTelemetry.reaction = sArena.reaction;
    gArenaAiTelemetry.aimError = sArena.aimError;
    gArenaAiTelemetry.style = sArena.style;
    // Prime the initial route while the screen is forced blank. The first
    // playable frame should not pay for graph search plus sprite initialization.
    AiChooseGoal();sArena.thinkTimer=sArena.reaction;
    gArenaAiTelemetry.decisions++;
    sArena.frame = 0;
    sArena.active = TRUE;
    gRealtimeArenaQuietResult = FALSE;
    gArenaIntroTelemetry.elapsed=gMain.vblankCounter1-gArenaIntroTelemetry.started;
    sArena.paused = FALSE;
    sArena.resultTimer = 0;
    sArena.hitstop = sArena.attackBuffer = sArena.dashBuffer = 0;
    gRealtimeArenaTelemetry.active = TRUE;
    gRealtimeArenaTelemetry.paused = FALSE;
    gRealtimeArenaTelemetry.entries++;
    gRealtimeArenaTelemetry.hpBefore = gBattleMons[0].hp;
    gRealtimeArenaTelemetry.expBefore = GetMonData(&gPlayerParty[gBattlerPartyIndexes[0]], MON_DATA_EXP);
    DrawHud();
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_FULL);
    SendoutInit();
    MonVisualScaleApply();
    AnimateSprites();BuildOamBuffer();
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    ShowBg(0);
    ShowBg(1);
    SetVBlankCallback(VBlank_Arena);
    SetMainCallback2(CB2_Arena);
}

static u16 Speed(u8 side)
{
    // Q8 pixels per hardware frame; no render-rate-dependent clock.
    // Compress the high end on a 240px arena, preserving native Speed ordering.
    // 1.0..1.94 pixels/frame instead of reaching 3.23 at high levels.
    u32 speed=gBattleMons[side].speed;
    u8 stage=gBattleMons[side].statStages[STAT_SPEED];
    speed=speed*gStatStageRatios[stage][0]/gStatStageRatios[stage][1];
    speed=StormSpeed(side,256 + Clamp(speed, 1, 120) * 2);
    return (gBattleMons[side].status1&STATUS1_PARALYSIS)?max(64,speed/4):speed;
}

static u8 Facing(s32 dx, s32 dy)
{
    if (Abs(dx) * 2 < Abs(dy)) return dy < 0 ? FACE_UP : FACE_DOWN;
    if (Abs(dy) * 2 < Abs(dx)) return dx < 0 ? FACE_LEFT : FACE_RIGHT;
    return dy < 0 ? (dx < 0 ? 5 : 3) : (dx < 0 ? 7 : 1);
}

static void MoveDelta(u8 side, s32 dx, s32 dy)
{
    struct ArenaBody *body = &sArena.bodies[side];
    struct ArenaBody *other = &sArena.bodies[side ^ 1];
    bool8 ghost = GhostBody(side);
    // A rolling body runs over the other one instead of being held off it, and
    // a body left overlapping (a roll that ended on top of it) may step away.
    bool8 over = Rolling(side) && body->actionLife;
    // A ghost may step into the wall band and through cover; see PhaseWrap.
    s32 margin = ghost ? ARENA_PHASE_MARGIN * Q : 0;
    s32 x, y;
    if(ElementMotion(side,dx,dy))return;
    body->moving = dx || dy;
    if (dx || dy) body->facing = Facing(dx, dy);
    x = Clamp(body->x + dx, ARENA_MIN_X * Q - margin, ARENA_MAX_X * Q + margin);
    y = Clamp(body->y + dy, ARENA_MIN_Y * Q - margin, ARENA_MAX_Y * Q + margin);
    // Axis-separated sliding. Even the fastest dash step is smaller than any
    // solid obstacle; the swept test also prevents corner cutting.
    if ((ghost || ArenaNav_LineClear(body->x / Q, body->y / Q, x / Q, body->y / Q, ARENA_BODY_RADIUS))
        && CoverCanStep(x/Q,body->y/Q)
        && (over || Abs(x - other->x) >= 22 * Q || Abs(body->y - other->y) >= 22 * Q
            || Abs(x - other->x) > Abs(body->x - other->x))) body->x = x;
    else if (dx) gArenaAiTelemetry.wallBlocks[side]++;
    if ((ghost || ArenaNav_LineClear(body->x / Q, body->y / Q, body->x / Q, y / Q, ARENA_BODY_RADIUS))
        && CoverCanStep(body->x/Q,y/Q)
        && (over || Abs(body->x - other->x) >= 22 * Q || Abs(y - other->y) >= 22 * Q
            || Abs(y - other->y) > Abs(body->y - other->y))) body->y = y;
    else if (dy) gArenaAiTelemetry.wallBlocks[side]++;
    if (ghost) PhaseWrap(side, dx, dy);
}

static void Move(u8 side, s32 dx, s32 dy, s32 speed)
{
    if (dx && dy) speed = speed * 181 / 256;
    MoveDelta(side, dx * speed, dy * speed);
}

// Matheus: dash may cross cover or a rival only when its landing is clear.
// Commit the landing immediately, then keep the existing ten-frame dodge and
// recovery. This cannot strand the actor inside a prop on a menu/status handoff.
static bool8 DashLanding(struct ArenaBody *body)
{
    s32 speed=Speed(0)*3,x,y;
    if(body->dashX && body->dashY)speed=speed*181/256;
    x=body->x+body->dashX*speed*10;
    y=body->y+body->dashY*speed*10;
    if(!ArenaNav_CanStand(x/Q,y/Q) || !CoverCanStep(x/Q,y/Q)
        || (Abs(x-sArena.bodies[1].x)<22*Q && Abs(y-sArena.bodies[1].y)<22*Q))return FALSE;
    // Only phase when the old sweep would hit something; open-ground dash
    // retains its familiar continuous movement.
    if(ArenaNav_LineClear(body->x/Q,body->y/Q,x/Q,y/Q,ARENA_BODY_RADIUS)
        && ArenaMoves_SegmentHit(body->x/Q,body->y/Q,x/Q,y/Q,sArena.bodies[1].x/Q,sArena.bodies[1].y/Q,22)==FALSE)return FALSE;
    ArenaFeedback_Dust(body->x/Q,body->y/Q,TRUE);
    body->x=x;body->y=y;body->facing=Facing(body->dashX,body->dashY);
    body->dashX=body->dashY=0;
    ArenaFeedback_Dust(x/Q,y/Q,TRUE);
    return TRUE;
}

static void Fire(u8 side, u8 slot, s32 targetX, s32 targetY)
{
    struct ArenaBody *body = &sArena.bodies[side];
    const struct ArenaMoveProfile *profile;
    u32 i;
    s32 dx, dy, len;
    u8 pp;
    // The selected move is captured at BeginShot. Changing L/R during its
    // animation changes the NEXT attack, never this attack's cost or power.
    if (slot > ARENA_STRUGGLE_SLOT || !ArenaMovePP(side,slot)) return;
    profile = ArenaMoves_Get(ArenaMoveId(side,slot));
    if (!profile) return;
    dx = body->attackX; dy = body->attackY;
    len = max(Abs(dx), Abs(dy)) + min(Abs(dx), Abs(dy)) / 2;
    if (!len) { dx = Q; len = Q; }
    body->attackX = dx * Q / len; body->attackY = dy * Q / len;
    if(ArenaMoves_Beam(profile->move))
    {
        static const s16 dirs[8][2]={{0,256},{181,181},{256,0},{181,-181},{0,-256},{-181,-181},{-256,0},{-181,181}};
        body->attackX=dirs[body->shotFacing][0];body->attackY=dirs[body->shotFacing][1];
        sFlameElementMask[side]=sFlameReach[side]=0;
        gArenaFlameTelemetry[0]++;
    }
    if (profile->kind == ARENA_MOVE_PROJECTILE
        && !(profile->move==MOVE_PSYCHIC
             && PsychicStart(side,targetX,targetY)))
    {
        for (i = 0; i < SHOTS_COUNT; i++) if (!sArena.shots[i].life) break;
        if (i == SHOTS_COUNT) return;
        if(!ArenaTerrain_ReserveProjectile())return;
        sArena.shots[i].sprite = ArenaMoveFx_CreateBolt(profile,body->x/Q,body->y/Q,body->shotFacing,i);
        if (sArena.shots[i].sprite == MAX_SPRITES) return;
        sArena.shots[i].x = body->x; sArena.shots[i].y = body->y;
        sArena.shots[i].vx = dx * profile->speed / len;
        sArena.shots[i].vy = dy * profile->speed / len;
        sArena.shots[i].side = side; sArena.shots[i].move = profile->move;
        sArena.shots[i].life = profile->range * Q / profile->speed;
        sArena.shots[i].direction = body->shotFacing; sArena.shots[i].age = 0;
        sArena.shots[i].elementMask=0;
        // Decorative water stream segments share the head's tiles. They have
        // no hitboxes and can never multiply native damage or PP costs.
        {
            u32 t;
            for(t=0;t<2;t++)
            {
                sArena.shots[i].trail[t]=MAX_SPRITES;
                if(profile->move==MOVE_WATER_GUN||profile->move==MOVE_SURF||profile->move==MOVE_ICY_WIND||profile->move==MOVE_SHOCK_WAVE)
                {
                    u8 sprite=ArenaMoveFx_CreateBolt(profile,body->x/Q,body->y/Q,body->shotFacing,i);
                    sArena.shots[i].trail[t]=sprite;
                    if(sprite!=MAX_SPRITES)gSprites[sprite].invisible=TRUE;
                }
            }
        }
    }
    body->actionLife = profile->active;
    if(profile->move==MOVE_BIDE)sBideDamage[side]=0;
    body->actionAge = 0; body->connected = FALSE; body->terrainMask=0;
    // Pressure applies on commitment, even if the projectile misses or hits
    // cover. Self-targeted buffs do not target the opponent. Clamp the last PP.
    pp = ArenaMovePP(side,slot);
    if (gBattleMons[side ^ 1].ability == ABILITY_PRESSURE
        && GetMoveTarget(profile->move) != TARGET_USER && pp > 1)
        pp--;
    if(slot<MAX_MON_MOVES)
    {
        gBattleMons[side].pp[slot] = --pp;
        SetMonData(side ? &gEnemyParty[gBattlerPartyIndexes[side]] : &gPlayerParty[gBattlerPartyIndexes[side]],
            MON_DATA_PP1 + slot, &pp);
    }
    body->cooldown = profile->recovery + (side ? Clamp(8-gBattleMons[1].level/8,2,8) : 0);
    if (gBattleMons[side].ability == ABILITY_TRUANT) body->cooldown = body->cooldown * 2 + 60; // loafs between actions
    gRealtimeArenaTelemetry.shots[side]++;
    gArenaCombatTelemetry.lastMove[side] = profile->move;
    if(!GetMovePower(profile->move) && body->statusActions<255)body->statusActions++;
    sArena.hudDirty = TRUE;
    PlaySE(profile->move == MOVE_ROLLOUT ? SE_M_TAKE_DOWN
        : profile->move == MOVE_PSYCHIC ? SE_M_PSYBEAM
        : profile->move == MOVE_SHADOW_BALL ? SE_M_PSYBEAM2
        : profile->move == MOVE_DRAGON_CLAW ? SE_M_SCRATCH
        : profile->move == MOVE_CRUNCH ? SE_M_BITE
        : profile->move == MOVE_WATER_GUN || profile->move == MOVE_SURF ? SE_M_BUBBLE_BEAM
        : profile->move == MOVE_ICY_WIND ? SE_M_ICY_WIND
        : profile->move == MOVE_SHOCK_WAVE ? SE_M_THUNDERBOLT
        : profile->move == MOVE_FLAMETHROWER ? SE_M_FLAMETHROWER
        : profile->visual == ARENA_VIS_EMBER ? SE_M_EMBER
        : profile->move == MOVE_ROCK_THROW ? SE_M_ROCK_THROW
        : profile->move == MOVE_BUBBLE ? SE_M_BUBBLE
        : profile->move == MOVE_GUST ? SE_M_GUST
        : profile->move == MOVE_BITE ? SE_M_BITE
        : profile->move == MOVE_LEAF_BLADE ? SE_M_RAZOR_WIND
        : profile->move == MOVE_WING_ATTACK ? SE_M_WING_ATTACK
        : profile->move == MOVE_SCRATCH ? SE_M_SCRATCH
        : profile->kind == ARENA_MOVE_RUSH ? SE_M_SWIFT
        : profile->kind == ARENA_MOVE_CONE ? SE_M_LEER
        : profile->move == MOVE_ABSORB || profile->move == MOVE_MEGA_DRAIN ? SE_M_ABSORB : SE_BALL);
}

static void BeginShot(u8 side, s32 targetX, s32 targetY)
{
    struct ArenaBody *body = &sArena.bodies[side];
    const struct ArenaMoveProfile *profile = ArenaMoves_Get(ArenaMoveId(side,body->moveSlot));
    s32 dx,dy,len;
    if (!profile || body->shotTimer || PsychicBusy(side) || SpecialBusy(side) || !ArenaMovePP(side,body->moveSlot)
        || (gBattleMons[side].status1&(STATUS1_SLEEP|STATUS1_FREEZE))) return;
    if((gBattleMons[side].status1&STATUS1_PARALYSIS) && Random()%4==0)
    {
        body->cooldown=30;
        ArenaFeedback_Impact(side,body->x/Q,body->y/Q,0,ARENA_FEEDBACK_MISS);
        return;
    }
    body->shotSlot = body->moveSlot;
    body->aimX = targetX; body->aimY = targetY;
    body->shotFacing = Facing(targetX - body->x, targetY - body->y);
    dx=targetX-body->x;dy=targetY-body->y;
    len=max(Abs(dx),Abs(dy))+min(Abs(dx),Abs(dy))/2;
    if(!len){dy=-Q;len=Q;}
    body->attackX=dx*Q/len;body->attackY=dy*Q/len;
    body->shotElapsed = 0;
    body->shotTimer = profile->windup + profile->active + 8;
    body->animation = profile->animation;
    body->animClock = 0;
    body->drawnFrame = 255;
    body->rollAngle = 0; body->rollStage = 0; body->rollDistance = 0;
    if (profile->move == MOVE_ROLLOUT) RollBallBegin(side);
}

static void TickPendingShots(void)
{
    u32 side;
    for (side = 0; side < 2; side++)
    {
        struct ArenaBody *body = &sArena.bodies[side];
        const struct ArenaMoveProfile *profile;
        if(gBattleMons[side].status1&(STATUS1_SLEEP|STATUS1_FREEZE))
        {body->shotTimer=body->actionLife=body->dash=0;continue;}
        if (!body->shotTimer) continue;
        profile = ArenaMoves_Get(ArenaMoveId(side,body->shotSlot));
        // Presentation maps the profile's release to the source HitFrame.
        if (body->shotElapsed == profile->windup)
            Fire(side, body->shotSlot, body->aimX, body->aimY);
        body->shotElapsed++;
        body->shotTimer--;
    }
}

static void BufferPlayerActions(void)
{
    struct ArenaBody *body = &sArena.bodies[0];
    s32 dx = (JOY_HELD(DPAD_RIGHT) != 0) - (JOY_HELD(DPAD_LEFT) != 0);
    s32 dy = (JOY_HELD(DPAD_DOWN) != 0) - (JOY_HELD(DPAD_UP) != 0);
    if(sCapture.state==ARENA_CAPTURE_AIM)
    {sArena.attackBuffer=sArena.dashBuffer=0;return;}
    // Brief taps made just before recovery ends survive for six active frames.
    // This also catches input during the two-frame impact stop.
    if (JOY_NEW(A_BUTTON)) sArena.attackBuffer = 6;
    else if (sArena.attackBuffer) sArena.attackBuffer--;
    if (!MATT_AUTO_SIM_COMBAT && JOY_NEW(B_BUTTON) && (dx || dy))
    {
        sArena.dashBuffer = 6;
        sArena.dashRequestX = dx; sArena.dashRequestY = dy;
    }
    else if (sArena.dashBuffer && !MATT_AUTO_SIM_COMBAT) sArena.dashBuffer--;
    if (JOY_NEW(L_BUTTON | R_BUTTON) && JOY_HELD(L_BUTTON|R_BUTTON)!=(L_BUTTON|R_BUTTON))
    {
        u32 i;
        for (i = 1; i <= MAX_MON_MOVES+1; i++)
        {
            u8 slot = (body->moveSlot + (JOY_NEW(L_BUTTON) ? MAX_MON_MOVES+1 - i : i)) % (MAX_MON_MOVES+1);
            if (SupportedMove(ArenaMoveId(0,slot)) && ArenaMovePP(0,slot)) { body->moveSlot = slot; break; }
        }
    }
}

#if MATT_AUTO_SIM_COMBAT
// Matt mod: auto-sim. The player's Pokemon closes to its move's range and
// holds there, steps off walls, and only side-steps shots that would hit.
static s32 PlayerTypeValue(u16 move)
{
    u32 i;
    u8 type = GetMoveType(move);
    const struct BattlePokemon *target = &gBattleMons[1];
    s32 value = 100;
    if (target->ability == ABILITY_LEVITATE && type == TYPE_GROUND) return 0;
    value = value * GetTypeModifier(type, target->types[0]) / 4096;
    if (target->types[1] != target->types[0]) value = value * GetTypeModifier(type, target->types[1]) / 4096;
    if (target->ability == ABILITY_WONDER_GUARD && value <= 100) return 0;
    return value;
}

#if MATT_AUTO_ATTACK
// Rough Gen 3 damage estimate (no random roll, no crits). Physical/special
// is decided by the move's type, as in Gen 3.
static s32 MattEstimateDamage(u16 move)
{
    const struct BattlePokemon *a = &gBattleMons[0], *d = &gBattleMons[1];
    u8 type = GetMoveType(move);
    s32 atk, def, dmg, eff;
    if (move == MOVE_NIGHT_SHADE || move == MOVE_SEISMIC_TOSS) return PlayerTypeValue(move) ? a->level : 0;
    if (!GetMovePower(move)) return 0;
    eff = move == MOVE_STRUGGLE ? 100 : PlayerTypeValue(move);
    if (!eff) return 0;
    if (type < TYPE_MYSTERY) { atk = a->attack; def = d->defense; }
    else { atk = a->spAttack; def = d->spDefense; }
    if (def < 1) def = 1;
    dmg = ((2 * a->level / 5 + 2) * GetMovePower(move) * atk / def) / 50 + 2;
    if (type == a->types[0] || type == a->types[1]) dmg = dmg * 3 / 2;
    return dmg * eff / 100;
}

static bool8 MattIsStatusInflict(u16 move)
{
    u8 e = GetMoveEffect(move);
    return e == EFFECT_NON_VOLATILE_STATUS || e == EFFECT_CONFUSE;
}

static s32 AiTypeValue(u16 move);
static s32 MattEnemyDamage(u16 move)
{
    const struct BattlePokemon *a = &gBattleMons[1], *d = &gBattleMons[0];
    u8 type = GetMoveType(move);
    s32 atk, def, dmg, eff;
    if (move == MOVE_NIGHT_SHADE || move == MOVE_SEISMIC_TOSS) return AiTypeValue(move) ? a->level : 0;
    if (!GetMovePower(move)) return 0;
    eff = AiTypeValue(move);
    if (type < TYPE_MYSTERY) { atk = a->attack; def = d->defense; if (sBarrierLife[0][0]) def *= 2; }
    else { atk = a->spAttack; def = d->spDefense; if (sBarrierLife[0][1]) def *= 2; }
    if (def < 1) def = 1;
    dmg = ((2 * a->level / 5 + 2) * GetMovePower(move) * atk / def) / 50 + 2;
    if (type == a->types[0] || type == a->types[1]) dmg = dmg * 3 / 2;
    return dmg * eff / 100;
}

// Expected damage of an enemy hit predicted to land on the player soon:
// incoming projectiles on a closing course (same projection the dodge AI
// uses) or a melee/rush/cone wind-up within reach. 0 = no threat.
static s32 MattIncomingThreat(void)
{
    struct ArenaBody *me = &sArena.bodies[0], *foe = &sArena.bodies[1];
    s32 best = 0, d;
    u32 i;
    for (i = 0; i < SHOTS_COUNT; i++)
    {
        struct ArenaShot *shot = &sArena.shots[i];
        s32 dx, dy, t, px, py, v;
        if (!shot->life || shot->side != 1) continue;
        v = shot->vx * shot->vx + shot->vy * shot->vy;
        if (!v) continue;
        dx = shot->x - me->x; dy = shot->y - me->y;
        t = -(dx * shot->vx + dy * shot->vy) / v;
        if (t < 0 || t > 24) continue;
        px = (dx + shot->vx * t) / Q; py = (dy + shot->vy * t) / Q;
        if (px * px + py * py > 20 * 20) continue;
        d = MattEnemyDamage(shot->move);
        if (d > best) best = d;
    }
    if (foe->shotTimer)
    {
        u16 move = ArenaMoveId(1, foe->shotSlot);
        const struct ArenaMoveProfile *p = ArenaMoves_Get(move);
        s32 dist = ArenaNav_Distance(me->x/Q, me->y/Q, foe->x/Q, foe->y/Q);
        if (p && p->kind != ARENA_MOVE_PROJECTILE && p->kind != ARENA_MOVE_SELF
            && foe->shotElapsed <= p->windup && dist <= p->range + 16)
        {
            d = MattEnemyDamage(move);
            if (d > best) best = d;
        }
    }
    // Wind-ups are only 2-4 frames, so also predict the next swing: the rival
    // is in reach of its selected move and that move is almost off cooldown.
    if (!foe->shotTimer && foe->cooldown <= 12 && !foe->actionLife)
    {
        u16 move = ArenaMoveId(1, foe->moveSlot);
        const struct ArenaMoveProfile *p = ArenaMoves_Get(move);
        s32 dist = ArenaNav_Distance(me->x/Q, me->y/Q, foe->x/Q, foe->y/Q);
        if (p && p->kind != ARENA_MOVE_SELF && dist <= p->range + 12
            && ArenaNav_LineClear(me->x/Q, me->y/Q, foe->x/Q, foe->y/Q, 2))
        {
            d = MattEnemyDamage(move);
            if (d > best) best = d;
        }
    }
    return best;
}

// Category (0 physical, 1 special) of the enemy's strongest usable move.
static bool8 MattIsHealMove(u16 m)
{return m==MOVE_RECOVER||m==MOVE_SOFT_BOILED||m==MOVE_MILK_DRINK||m==MOVE_SYNTHESIS||m==MOVE_MORNING_SUN||m==MOVE_MOONLIGHT;}
static s32 MattStageSum(u8 side,s8 sign)
{
    u32 i; s32 n=0;
    for(i=STAT_ATK;i<NUM_BATTLE_STATS;i++)
    {s32 d=gBattleMons[side].statStages[i]-DEFAULT_STAT_STAGE; if(d*sign>0)n+=d*sign;}
    return n;
}
static bool8 MattHasDropMove(u8 side)
{
    u32 i;u8 st;bool8 self;
    for(i=0;i<MAX_MON_MOVES;i++)
        if(SupportedMove(ArenaMoveId(side,i))&&ArenaMovePP(side,i)&&RealtimeArena_StatChange(ArenaMoveId(side,i),&st,&self)<0)return TRUE;
    return FALSE;
}
static s32 MattEnemyBestDamage(void)
{
    s32 best=0,d;u32 i;
    for(i=0;i<MAX_MON_MOVES;i++)
        if(SupportedMove(ArenaMoveId(1,i))&&ArenaMovePP(1,i)&&(d=MattEnemyDamage(ArenaMoveId(1,i)))>best)best=d;
    return best;
}

static s8 MattEnemyMainCategory(void)
{
    s32 best = 0, d; u32 i; s8 cat = -1;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        u16 move = ArenaMoveId(1, i);
        if (!SupportedMove(move) || !ArenaMovePP(1, i)) continue;
        d = MattEnemyDamage(move);
        if (d > best) { best = d; cat = GetMoveType(move) < TYPE_MYSTERY ? 0 : 1; }
    }
    return cat;
}

// Returns FALSE when nothing is worth using (catch mode: holds fire).
static bool8 PlayerAutoChooseMove(s32 distance)
{
    struct ArenaBody *body = &sArena.bodies[0];
    const struct BattlePokemon *me = &gBattleMons[0], *foe = &gBattleMons[1];
    s32 best = -100000, score, est, bestEst = 1;
    u32 i;
    u8 bestSlot = 0xFF;
    bool8 healthy = me->hp * 10 >= me->maxHP * 6;
    bool8 foeStatused = (foe->status1 & STATUS1_ANY) != 0;
    bool8 hasAttack = FALSE;
    s32 threat = MattIncomingThreat();
    bool8 canGuard = FALSE;
    s8 foeCat = MattEnemyMainCategory();
    if (sEncore[0] && body->moveSlot < MAX_MON_MOVES && ArenaMovePP(0, body->moveSlot)) return TRUE; // Encore lock
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        u16 move = ArenaMoveId(0, i);
        if (!SupportedMove(move) || !ArenaMovePP(0, i)) continue;
        est = MattEstimateDamage(move);
        if (est) hasAttack = TRUE;
        if (est > bestEst) bestEst = est;
        if (move == MOVE_PROTECT || move == MOVE_DETECT) canGuard = TRUE;
    }
    // Guards must not crowd out attacking: one real attack between guards.
    if ((sMattProtectGap & 0x8000) && hasAttack) canGuard = FALSE;
    if ((sMattProtectGap & 0x7FFF) && threat < me->hp) canGuard = FALSE;
    if (sProtectChain[0] && threat * 2 < me->hp) canGuard = FALSE;
    for (i = 0; i <= MAX_MON_MOVES; i++)
    {
        u16 move = ArenaMoveId(0, i);
        const struct ArenaMoveProfile *p = ArenaMoves_Get(move);
        u8 pp, maxPP;
        if (!p || !SupportedMove(move) || !ArenaMovePP(0, i)) continue;
        if (move == MOVE_STRUGGLE && hasAttack) continue;
        pp = ArenaMovePP(0, i);
        maxPP = i < MAX_MON_MOVES ? CalculatePPWithBonus(move, me->ppBonuses, i) : 1;
        if (GetMovePower(move) || move == MOVE_NIGHT_SHADE || move == MOVE_SEISMIC_TOSS)
        {
            est = MattEstimateDamage(move);
            if (!est) continue;                                  // immune: never
            if (move == MOVE_SNORE && !(me->status1 & STATUS1_SLEEP)) continue;
            if (sMattCatchMode)
            {
                // Weaken without KO: allow for rolls and crits (x1.3).
                if (est * 13 / 10 >= foe->hp) continue;
                score = est * 100 / (foe->hp ? foe->hp : 1);     // biggest safe chunk
            }
            else if (est >= foe->hp)
                score = 1000 + GetMoveAccuracy(move);      // finisher: reliable
            else
                score = est * 100 / bestEst;                     // relative damage 0..100
        }
        else if (move == MOVE_PROTECT || move == MOVE_DETECT)
        {
            if (!canGuard || threat < 1 || threat * 4 < me->hp) continue;
            score = 1100 + threat * 100 / (me->hp ? me->hp : 1);
            score >>= sProtectChain[0];
        }
        else if (move == MOVE_ENDURE)
        {
            // Only vs a predicted KO when a guard is unavailable.
            if (canGuard || threat < me->hp || me->hp <= 1 || ((sMattProtectGap & 0x8000) && hasAttack)) continue;
            score = 1050 >> sProtectChain[0];
        }
        else if (MattIsHealMove(move))
        {
            if (me->hp * 10 > me->maxHP * 4 || threat * 2 >= me->hp) continue;
            score = 950;
        }
        else if (move == MOVE_REST)
        {
            bool8 statused = (me->status1 & STATUS1_ANY) != 0;
            if (me->hp == me->maxHP || threat * 2 >= me->hp) continue;
            if (!(me->hp * 10 <= me->maxHP * 4 || (statused && me->hp * 10 <= me->maxHP * 7))) continue;
            score = MattEnemyBestDamage() * 3 >= me->maxHP ? 30 : 900;
        }
        else if (move == MOVE_HAZE)
        {
            if (MattStageSum(1, 1) < 2 && !MattStageSum(0, -1)) continue;
            score = 95;
        }
        else if (move == MOVE_COUNTER || move == MOVE_MIRROR_COAT)
        {
            if (!sLastHitAge[0] || ((sLastHitSpecial & 1) != (move == MOVE_MIRROR_COAT)) || sLastHit[0] * 2 < bestEst) continue;
            score = sLastHit[0] * 2 >= foe->hp ? 1050 : 90;
        }
        else if (move == MOVE_DESTINY_BOND)
        {
            if (sDestinyLife[0] || me->hp * 4 > me->maxHP || threat < me->hp) continue;
            score = 980;
        }
        else if (move == MOVE_LEECH_SEED)
        {
            if (gBattleMons[1].volatiles.leechSeed || IS_BATTLER_OF_TYPE(1, TYPE_GRASS) || sMattCatchMode) continue;
            score = healthy ? 80 : 40;
        }
        else if (move == MOVE_PERISH_SONG || move == MOVE_SKETCH || move == MOVE_BATON_PASS || move == MOVE_WHIRLWIND
                 || move == MOVE_TRANSFORM || move == MOVE_ENCORE || move == MOVE_MUD_SPORT || move == MOVE_ODOR_SLEUTH
                 || move == MOVE_SNORE || move == MOVE_SAFEGUARD || move == MOVE_MIND_READER || move == MOVE_BELLY_DRUM)
            continue; // situational/inert for the auto-player: never wasted
        else if (move == MOVE_MIST)
        {
            if ((gSideStatuses[0] & SIDE_STATUS_MIST) || !MattHasDropMove(1) || !healthy) continue;
            score = 75;
        }
        else if (move == MOVE_SUBSTITUTE)
        {
            if (sCover[0].hp || me->hp * 4 <= me->maxHP || threat * 2 > me->hp) continue;
            score = healthy ? 80 : 40;
        }
        else if (move == MOVE_REFLECT || move == MOVE_LIGHT_SCREEN)
        {
            u8 k = move == MOVE_LIGHT_SCREEN;
            if (sBarrierLife[0][k] || foeCat != (s8)k || foe->hp * 3 < foe->maxHP) continue;
            score = 90;
        }
        else if (move == MOVE_DOUBLE_TEAM || move == MOVE_SMOKESCREEN)
        {
            if ((move == MOVE_DOUBLE_TEAM ? sDecoys[0].life : sSmoke[0].life) || (sMattStatusUsed & (1 << i) && !threat)) continue;
            score = threat ? 75 : (healthy ? 50 : -1);
            if (score < 0) continue;
        }
        else
        {
            if (sMattStatusUsed & (1 << i)) continue;            // never repeat setup/status
            if (MattIsStatusInflict(move))
            {
                if (foeStatused || foe->volatiles.confusionTurns) continue;
                score = sMattCatchMode ? 400 : (healthy ? 85 : -1);
            }
            else
                score = (healthy && foe->hp * 2 > foe->maxHP && threat * 4 < me->hp) ? 70 : -1; // buffs early, when safe
            if (score < 0) continue;
        }
        score += AiRandom() % 20;                                 // variety
        if (i == body->moveSlot && hasAttack && score < 1000) score -= 12;
        if (maxPP && pp * 4 <= maxPP) score -= 25;                // conserve PP
        if (pp <= 1 && score < 1000) score -= 25;
        if (p->kind != ARENA_MOVE_SELF && distance > p->range) score -= (distance - p->range) / 3;
        if (score > best) { best = score; bestSlot = i; }
    }
    if (bestSlot == 0xFF) return FALSE;
    body->moveSlot = bestSlot;
    return TRUE;
}

// Picks the best ball in the bag and throws it at the target (normal roll).
static bool8 MattAutoThrow(void)
{
    struct ArenaBody *enemy = &sArena.bodies[1];
    static const u16 balls[] = {ITEM_ULTRA_BALL, ITEM_GREAT_BALL, ITEM_POKE_BALL};
    u32 i;
    u16 ball = ITEM_NONE;
    if (IsPlayerPartyAndPokemonStorageFull()) { CaptureMessage(7, 90); return FALSE; }
    for (i = 0; i < ARRAY_COUNT(balls); i++) if (CheckBagHasItem(balls[i], 1)) { ball = balls[i]; break; }
    if (ball == ITEM_NONE && CheckBagHasItem(ITEM_MASTER_BALL, 1)) ball = ITEM_MASTER_BALL;
    if (ball == ITEM_NONE && MATT_INFINITE_MASTER_BALL) ball = ITEM_MASTER_BALL;
    if (ball == ITEM_NONE) { CaptureMessage(6, 90); return FALSE; }
    if (!(ball == ITEM_MASTER_BALL && MATT_INFINITE_MASTER_BALL)) RemoveBagItem(ball, 1);
    sMattBall = ball;
    gLastUsedItem = ball;
    gArenaCaptureTelemetry.throws++;
    if (ball >= ITEM_ULTRA_BALL && gBattleResults.catchAttempts[ball - ITEM_ULTRA_BALL] < 255)
        gBattleResults.catchAttempts[ball - ITEM_ULTRA_BALL]++;
    sCapture.aimX = Clamp(enemy->x / Q + Clamp(sCapture.velocityX * 8 / Q, -16, 16), ARENA_MIN_X, ARENA_MAX_X);
    sCapture.aimY = Clamp(enemy->y / Q + Clamp(sCapture.velocityY * 8 / Q, -16, 16), ARENA_MIN_Y, ARENA_MAX_Y);
    sCapture.startX = 14; sCapture.startY = 108;
    sArena.attackBuffer = sArena.dashBuffer = 0;
    CaptureSetState(ARENA_CAPTURE_THROW);
    CaptureMessage(8, 1);
    PlaySE(SE_BALL_THROW);
    return TRUE;
}
#endif

static void PlayerAutoSteer(s32 *outX, s32 *outY)
{
    struct ArenaBody *body = &sArena.bodies[0];
    struct ArenaBody *foe = &sArena.bodies[1];
    const struct ArenaMoveProfile *p = ArenaMoves_Get(ArenaMoveId(0, body->moveSlot));
    s32 bx = body->x / Q, by = body->y / Q, ex = foe->x / Q, ey = foe->y / Q;
    s32 d, range, gx, gy;
    struct ArenaPoint wp;
    u32 i;
    *outX = *outY = 0;
    // Dodge only real hits: an enemy shot passing within 12px in <=12 ticks.
    if (!body->dashCooldown && !body->actionLife && !body->shotTimer)
    {
        for (i = 0; i < SHOTS_COUNT; i++)
        {
            struct ArenaShot *shot = &sArena.shots[i];
            s32 sx, sy, t, px, py;
            if (!shot->life || shot->side != 1 || (!shot->vx && !shot->vy)) continue;
            sx = shot->x - body->x; sy = shot->y - body->y;
            t = -(sx * shot->vx + sy * shot->vy) / (shot->vx * shot->vx + shot->vy * shot->vy);
            if (t < 1 || t > 16) continue;
            px = (sx + shot->vx * t) / Q; py = (sy + shot->vy * t) / Q;
            if (px * px + py * py > 196) continue;
            gx = -shot->vy; gy = shot->vx;
            if (gx * px + gy * py > 0) { gx = -gx; gy = -gy; }
            // Prefer the side-step that points toward the arena centre.
            if ((bx < ARENA_MIN_X + 16 && gx < 0) || (bx > ARENA_MAX_X - 16 && gx > 0)
             || (by < ARENA_MIN_Y + 16 && gy < 0) || (by > ARENA_MAX_Y - 16 && gy > 0)) { gx = -gx; gy = -gy; }
            sArena.dashRequestX = (gx > Q / 4) - (gx < -Q / 4);
            sArena.dashRequestY = (gy > Q / 4) - (gy < -Q / 4);
            if (sArena.dashRequestX || sArena.dashRequestY) sArena.dashBuffer = 6;
            return;
        }
    }
    if (!p) return;
    range = p->kind == ARENA_MOVE_SELF ? 60 : p->kind == ARENA_MOVE_MELEE ? 18 : p->range - 8;
    if (range < 14) range = 14;
    d = ArenaNav_Distance(bx, by, ex, ey);
    gx = ex; gy = ey;
    // Low HP: back off briefly while recovering from an attack.
    if (gBattleMons[0].hp * 4 < gBattleMons[0].maxHP && body->cooldown && !sMattCatchMode)
        sMattBackoff = 30;
    // Ranged moves keep a little room; melee closes all the way in.
    if (sMattBackoff || (p->kind != ARENA_MOVE_MELEE && p->kind != ARENA_MOVE_RUSH && d < 28))
    {
        s32 len = max(Abs(bx - ex), Abs(by - ey));
        if (sMattBackoff) sMattBackoff--;
        if (!len) len = 1;
        gx = Clamp(bx + (bx - ex) * 24 / len, ARENA_MIN_X + 8, ARENA_MAX_X - 8);
        gy = Clamp(by + (by - ey) * 24 / len, ARENA_MIN_Y + 8, ARENA_MAX_Y - 8);
        if (ArenaNav_NextWaypoint(bx, by, gx, gy, &wp)) { gx = wp.x; gy = wp.y; }
        *outX = (gx > bx + 1) - (gx < bx - 1);
        *outY = (gy > by + 1) - (gy < by - 1);
        return;
    }
    // In range with a clear line: hold position. Otherwise close in.
    if (d <= range && ArenaNav_LineClear(bx, by, ex, ey, 2))
    {
        // Step off walls toward the centre, never kite.
        gx = bx + (bx < ARENA_MIN_X + 10) - (bx > ARENA_MAX_X - 10);
        gy = by + (by < ARENA_MIN_Y + 10) - (by > ARENA_MAX_Y - 10);
        if (gx == bx && gy == by) return;
        *outX = gx - bx; *outY = gy - by;
        return;
    }
    if (ArenaNav_NextWaypoint(bx, by, gx, gy, &wp)) { gx = wp.x; gy = wp.y; }
    *outX = (gx > bx + 1) - (gx < bx - 1);
    *outY = (gy > by + 1) - (gy < by - 1);
}
#endif

#if MATT_AUTO_ITEMS
// Prefer a heal move (saves items) unless HP is critical (<=10%).
static bool8 MattHasHealPP(void)
{
    u32 i;
    if (gBattleMons[0].hp * 10 <= gBattleMons[0].maxHP) return FALSE;
    for (i = 0; i < MAX_MON_MOVES; i++)
        if (ArenaMovePP(0, i) && (MattIsHealMove(ArenaMoveId(0, i)) || ArenaMoveId(0, i) == MOVE_REST)) return TRUE;
    return FALSE;
}
static void MattUseItem(u16 item, u32 heal, u32 cure)
{
    struct BattlePokemon *mon = &gBattleMons[0];
    struct Pokemon *party = &gPlayerParty[gBattlerPartyIndexes[0]];
    static const u8 used[] = _("USED "), bang[] = _("!");
    u8 *s;
    RemoveBagItem(item, 1);
    gArenaItemsUsed++;
    if (heal) { mon->hp = min(mon->maxHP, mon->hp + heal); SetMonData(party, MON_DATA_HP, &mon->hp); }
    if (cure)
    {
        mon->status1 &= ~cure;
        SetMonData(party, MON_DATA_STATUS, &mon->status1);
        sArena.bodies[0].burnClock = 0;
    }
    s = StringCopy(sMattItemMsg, used);
    CopyItemName(item, s);
    StringAppend(sMattItemMsg, bang);
    CaptureMessage(9, 100);
    sArena.hudDirty = TRUE;
    sMattItemCd = 360;
    PlaySE(SE_USE_ITEM);
}

// Player side only, from the real bag, consumed normally.
static void MattAutoItems(void)
{
    struct BattlePokemon *mon = &gBattleMons[0];
    u32 status = mon->status1 & STATUS1_ANY;
    u32 missing;
    if (sMattItemCd) { sMattItemCd--; return; }
    if (!mon->hp || sArena.resultTimer || sCapture.state != ARENA_CAPTURE_IDLE || sMattPrompt == 1) return;
    missing = mon->maxHP - mon->hp;
    if (mon->hp * 10 <= mon->maxHP * 3 && !MattHasHealPP())
    {
        static const struct { u16 item; u16 heal; } pots[] = {
            {ITEM_POTION, 20}, {ITEM_SUPER_POTION, 50}, {ITEM_HYPER_POTION, 200}, {ITEM_MAX_POTION, 0xFFFF}};
        u32 i; s32 pick = -1;
        if (status && CheckBagHasItem(ITEM_FULL_RESTORE, 1))
        { MattUseItem(ITEM_FULL_RESTORE, missing, STATUS1_ANY | STATUS1_TOXIC_COUNTER); return; }
        // Weakest potion that restores at least half the missing HP (or
        // reaches 50%); otherwise the strongest one available.
        for (i = 0; i < ARRAY_COUNT(pots); i++)
        {
            u32 h = min(pots[i].heal, missing);
            if (!CheckBagHasItem(pots[i].item, 1)) continue;
            pick = i;
            if (h * 2 >= missing || mon->hp + h >= mon->maxHP / 2) break;
        }
        if (pick >= 0) { MattUseItem(pots[pick].item, min(pots[pick].heal, missing), 0); return; }
        if (CheckBagHasItem(ITEM_FULL_RESTORE, 1))
        { MattUseItem(ITEM_FULL_RESTORE, missing, STATUS1_ANY | STATUS1_TOXIC_COUNTER); return; }
    }
    if (status)
    {
        u16 item = ITEM_NONE;
        if (status & STATUS1_PSN_ANY) item = ITEM_ANTIDOTE;
        else if (status & STATUS1_PARALYSIS) item = ITEM_PARALYZE_HEAL;
        else if (status & STATUS1_SLEEP) item = ITEM_AWAKENING;
        else if (status & STATUS1_BURN) item = ITEM_BURN_HEAL;
        else if (status & STATUS1_FREEZE) item = ITEM_ICE_HEAL;
        if (item == ITEM_NONE || !CheckBagHasItem(item, 1)) item = CheckBagHasItem(ITEM_FULL_HEAL, 1) ? ITEM_FULL_HEAL : ITEM_NONE;
        if (item != ITEM_NONE) MattUseItem(item, 0, STATUS1_ANY | STATUS1_TOXIC_COUNTER);
    }
}
#endif

#if MATT_AUTO_SWITCH
// x10 type multiplier of one attacking type against a (t1,t2) defender.
static u32 MattTypeMul(u8 atk,u8 t1,u8 t2)
{
    u32 i,m=10;
    m=m*GetTypeModifier(atk,t1)/4096;
    if(t2!=t1)m=m*GetTypeModifier(atk,t2)/4096;
    return m;
}
// Worst x10 multiplier the foe's damaging moves have against (t1,t2).
static u32 MattFoeThreat(u8 t1,u8 t2)
{
    u32 i,b=0,m;
    for(i=0;i<MAX_MON_MOVES;i++)
    {
        u16 mv=gBattleMons[1].moves[i];
        if(!mv||!gBattleMons[1].pp[i]||!GetMovePower(mv))continue;
        if((m=MattTypeMul(GetMoveType(mv),t1,t2))>b)b=m;
    }
    return b;
}
// Best x10 multiplier (STAB x1.5) of a party mon's usable attacks on the foe.
static u32 MattOffense(struct Pokemon *mon,u8 t1,u8 t2)
{
    u32 i,b=0,m;
    for(i=0;i<MAX_MON_MOVES;i++)
    {
        u16 mv=GetMonData(mon,MON_DATA_MOVE1+i);
        if(!mv||!GetMonData(mon,MON_DATA_PP1+i)||!GetMovePower(mv)||!SupportedMove(mv))continue;
        m=MattTypeMul(GetMoveType(mv),gBattleMons[1].types[0],gBattleMons[1].types[1]);
        if(GetMoveType(mv)==t1||GetMoveType(mv)==t2)m=m*3/2;
        if(m>b)b=m;
    }
    return b;
}
static bool8 MattCanStillHeal(void)
{
    u32 i;
    for(i=0;i<MAX_MON_MOVES;i++)
        if(ArenaMovePP(0,i)&&(MattIsHealMove(ArenaMoveId(0,i))||ArenaMoveId(0,i)==MOVE_REST))return TRUE;
#if MATT_AUTO_ITEMS
    if(CheckBagHasItem(ITEM_POTION,1)||CheckBagHasItem(ITEM_SUPER_POTION,1)||CheckBagHasItem(ITEM_HYPER_POTION,1)
        ||CheckBagHasItem(ITEM_MAX_POTION,1)||CheckBagHasItem(ITEM_FULL_RESTORE,1))return TRUE;
#endif
    return FALSE;
}
static bool8 MattTrapped(void)
{
    struct BattlePokemon *me=&gBattleMons[0],*foe=&gBattleMons[1];
    if(me->volatiles.wrapped||me->volatiles.escapePrevention||me->volatiles.root)return TRUE;
    if(!foe->hp)return FALSE;
    if(foe->ability==ABILITY_SHADOW_TAG)return TRUE;
    if(foe->ability==ABILITY_ARENA_TRAP&&!IS_BATTLER_OF_TYPE(0,TYPE_FLYING)&&me->ability!=ABILITY_LEVITATE)return TRUE;
    if(foe->ability==ABILITY_MAGNET_PULL&&IS_BATTLER_OF_TYPE(0,TYPE_STEEL))return TRUE;
    return FALSE;
}
// Returns the party slot to switch to, or PARTY_SIZE. Cheap; runs every 15 frames.
static u8 MattAutoSwitchPick(void)
{
    struct BattlePokemon *me=&gBattleMons[0];
    struct ArenaBody *body=&sArena.bodies[0];
    u32 i,threat,off,lowHp,outmatched;
    s32 best=-9999,score;
    u8 pick=PARTY_SIZE;
    if((sMattSwitch&3)>=2||(sMattSwitch>>2)||!me->hp||!gBattleMons[1].hp)return PARTY_SIZE;
    if(sMattCatchMode||sMattForceCatch||sMattPrompt==1||sCapture.state!=ARENA_CAPTURE_IDLE||sArena.resultTimer||sArena.paused)return PARTY_SIZE;
    if(gBattleTypeFlags&(BATTLE_TYPE_ARENA|BATTLE_TYPE_SAFARI|BATTLE_TYPE_LINK|BATTLE_TYPE_DOUBLE|BATTLE_TYPE_PALACE))return PARTY_SIZE;
    if(ClashBusy()||GuardBusy()||SpecialBusy(0)||SpecialBusy(1)||body->shotTimer||body->actionLife||MattTrapped())return PARTY_SIZE;
    lowHp=me->hp*5<=me->maxHP&&!MattCanStillHeal();
    threat=MattFoeThreat(me->types[0],me->types[1]);
    off=MattOffense(&gPlayerParty[gBattlerPartyIndexes[0]],me->types[0],me->types[1]);
    outmatched=threat>=20&&off<20;
    if(!lowHp&&!outmatched)return PARTY_SIZE;
    for(i=0;i<PARTY_SIZE;i++)
    {
        struct Pokemon *mon=&gPlayerParty[i];
        u16 sp=GetMonData(mon,MON_DATA_SPECIES_OR_EGG),hp=GetMonData(mon,MON_DATA_HP),max=GetMonData(mon,MON_DATA_MAX_HP);
        u8 t1,t2;u32 th,of;
        if(i==gBattlerPartyIndexes[0]||!sp||sp==SPECIES_EGG||!hp||!max)continue;
        if(hp*2<max)continue; // healthy: at least half HP
        t1=gSpeciesInfo[sp].types[0];t2=gSpeciesInfo[sp].types[1];
        th=MattFoeThreat(t1,t2);of=MattOffense(mon,t1,t2);
        // Outmatched switches need a clearly better matchup.
        if(outmatched&&!lowHp&&!(th<=10&&th<threat&&of>=10))continue;
        if(lowHp&&th>=40)continue;
        score=(s32)of*2-(s32)th*3+(s32)(hp*20/max)+(s32)GetMonData(mon,MON_DATA_LEVEL)/2;
        if(score>best){best=score;pick=i;}
    }
    return pick;
}
#endif

static void TickPlayer(void)
{
    struct ArenaBody *body = &sArena.bodies[0];
    s32 dx = (JOY_HELD(DPAD_RIGHT) != 0) - (JOY_HELD(DPAD_LEFT) != 0);
    s32 dy = (JOY_HELD(DPAD_DOWN) != 0) - (JOY_HELD(DPAD_UP) != 0);
#if MATT_AUTO_ITEMS
    MattAutoItems();
#endif
    if (sMattProtectGap & 0x7FFF) sMattProtectGap--;
    if(SpecialBusy(0)||GuardBusy())return;
    if(gBattleMons[0].status1&(STATUS1_SLEEP|STATUS1_FREEZE))
    {body->moving=FALSE;sArena.attackBuffer=sArena.dashBuffer=0;return;}
    if (body->dashCooldown) body->dashCooldown--;
#if MATT_AUTO_SIM_COMBAT
    PlayerAutoSteer(&dx, &dy);
#endif
    if (sArena.dashBuffer && !body->dashCooldown && !body->actionLife)
    {
        body->dash = 10; body->dashCooldown = 75;
        body->dashX = sArena.dashRequestX; body->dashY = sArena.dashRequestY;
        DashLanding(body);
        sArena.dashBuffer = 0;
        if (body->shotTimer && body->shotElapsed <= ArenaMoves_Get(ArenaMoveId(0,body->shotSlot))->windup)
            gArenaCombatTelemetry.cancelled[0]++;
        body->shotTimer = 0;
        gRealtimeArenaTelemetry.dodges++;
        PlaySE(SE_M_DOUBLE_TEAM);
    }
    if (body->dash) { dx = body->dashX; dy = body->dashY; }
    if (!body->actionLife)
        Move(0, dx, dy, Speed(0) * (body->dash ? 3 : body->shotTimer ? 1 : 2) / (body->dash ? 1 : 2));
    if (body->moving && !(sArena.frame % (body->dash ? 3 : 14)))
        ArenaFeedback_Dust(body->x/Q, body->y/Q, body->dash != 0);
    if (body->dash) body->dash--;
    // Recovery follows the committed animation, not the start of its hitbox.
    // Changing slots cannot reset this shared recovery.
    if (body->cooldown && !body->shotTimer && !body->actionLife) body->cooldown--;
#if MATT_AUTO_SIM_COMBAT
    dx = dy = 0; // auto-sim always aims at the rival
#if MATT_AUTO_ATTACK
    if (sMattThrowWait) sMattThrowWait--;
    if (!body->cooldown && !body->dash && !body->shotTimer && sCapture.state == ARENA_CAPTURE_IDLE && sMattPrompt != 1)
    {
        s32 dist = ArenaNav_Distance(body->x/Q, body->y/Q, sArena.bodies[1].x/Q, sArena.bodies[1].y/Q);
        const struct ArenaMoveProfile *ap;
        bool8 canAttack = TRUE;
        if (sMattCatchMode == 2 && (sMattShots >= 3 || gBattleMons[1].hp * 5 <= gBattleMons[1].maxHP
            || gBattleMons[1].level > gBattleMons[0].level))
        {
            // CATCH (100%): a few hits (or none vs a higher level), then a sure throw.
            if (!sMattThrowWait)
            {
                if (IsPlayerPartyAndPokemonStorageFull()) { CaptureMessage(7, 90); sMattCatchMode = 0; }
                else
                {
                    sMattForceCatch = TRUE; sMattBall = ITEM_POKE_BALL; gLastUsedItem = ITEM_POKE_BALL;
                    sCapture.aimX = sArena.bodies[1].x / Q; sCapture.aimY = sArena.bodies[1].y / Q;
                    sCapture.startX = 14; sCapture.startY = 108;
                    sArena.attackBuffer = sArena.dashBuffer = 0;
                    CaptureSetState(ARENA_CAPTURE_THROW); CaptureMessage(8, 1); PlaySE(SE_BALL_THROW);
                    sMattThrowWait = 90;
                }
                return;
            }
            canAttack = FALSE;
        }
        else if (sMattCatchMode == 1 && gBattleMons[1].hp * 5 <= gBattleMons[1].maxHP)
        {
            // Weak enough: stop attacking and throw a ball (normal catch roll).
            if (!sMattThrowWait && MattAutoThrow()) { sMattThrowWait = 90; return; }
            canAttack = FALSE;
        }
        if (canAttack && sArena.frame % 8 == 0) canAttack = PlayerAutoChooseMove(dist) || !sMattCatchMode;
        else if (canAttack && sMattCatchMode) canAttack = PlayerAutoChooseMove(dist);
        ap = ArenaMoves_Get(ArenaMoveId(0, body->moveSlot));
        if (canAttack && ap && ArenaMovePP(0, body->moveSlot)
            && (GetMovePower(ArenaMoveId(0, body->moveSlot)) || ArenaMoveId(0, body->moveSlot) == MOVE_STRUGGLE
                || sArena.frame % 8 == 0 || sMattCatchMode) // support moves only when freshly chosen
            && (ap->kind == ARENA_MOVE_SELF
                || (dist <= ap->range && ArenaNav_LineClear(body->x/Q, body->y/Q, sArena.bodies[1].x/Q, sArena.bodies[1].y/Q, 2))))
        {
            if (!GetMovePower(ArenaMoveId(0, body->moveSlot)) && body->moveSlot < MAX_MON_MOVES)
                sMattStatusUsed |= 1 << body->moveSlot;
            else sMattProtectGap &= 0x7FFF; // attacked: guards allowed again
            sArena.attackBuffer = 1;
            if (sMattCatchMode == 2) sMattShots++;
        }
        else if (sMattCatchMode == 2 && !canAttack) sMattShots = 3; // no safe move left
        else if (sMattCatchMode == 1 && !canAttack && gBattleMons[1].hp * 5 > gBattleMons[1].maxHP && !sMattThrowWait)
        {
            // Every move risks a KO: throw now (normal roll) rather than stall.
            if (MattAutoThrow()) sMattThrowWait = 90;
        }
    }
#endif
#endif
    if (sCapture.state==ARENA_CAPTURE_IDLE && (JOY_HELD(A_BUTTON) || sArena.attackBuffer) && !body->cooldown && !body->dash && !body->shotTimer)
    {
        // A alone aims at the rival. D-pad + A gives explicit directional
        // aim, including destructible cover. This is still the same four moves.
        body->manualAim=dx||dy;
        // Rollout goes where the body is pointing, not at the rival.
        if(!dx&&!dy&&(ArenaMoveId(0,body->moveSlot)==MOVE_ROLLOUT
            ||SmokeBlocks(body->x/Q,body->y/Q,sArena.bodies[1].x/Q,sArena.bodies[1].y/Q)))
        {
            static const s8 facing[8][2]={{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1}};
            dx=facing[body->facing][0];dy=facing[body->facing][1];
            body->manualAim=TRUE;
        }
        BeginShot(0,dx||dy?body->x+dx*200*Q:DecoyAim(1,FALSE),
                    dx||dy?body->y+dy*200*Q:DecoyAim(1,TRUE));
        sArena.attackBuffer = 0;
    }
}

static u16 AiRandom(void)
{
    // Isolated deterministic stream: AI thinking never consumes the native
    // accuracy/critical-hit/damage RNG, nor reads future player input.
    sArena.aiRandom = sArena.aiRandom * 1664525 + 1013904223;
    return sArena.aiRandom >> 16;
}

static void AiRoute(void)
{
    struct ArenaBody *body = &sArena.bodies[1];
    // A ghost opponent walks straight through cover rather than around it.
    if (GhostBody(1)) { sArena.waypoint = sArena.goal; return; }
    if (ArenaNav_NextWaypoint(body->x / Q, body->y / Q, sArena.goal.x, sArena.goal.y, &sArena.waypoint))
        gArenaAiTelemetry.paths++;
    else
    {
        sArena.waypoint.x=body->x/Q;sArena.waypoint.y=body->y/Q;
        sArena.goalTimer=0;
    }
}

static void AiChooseGoal(void)
{
    static const s16 offsets[8][2] =
    {
        {256,0}, {181,181}, {0,256}, {-181,181},
        {-256,0}, {-181,-181}, {0,-256}, {181,-181}
    };
    struct ArenaBody *body = &sArena.bodies[1];
    const struct ArenaMoveProfile *p = ArenaMoves_Get(ArenaMoveId(1,body->moveSlot));
    s32 preferred = p->kind == ARENA_MOVE_MELEE ? 24 : p->kind == ARENA_MOVE_RUSH ? p->range-22
        : p->move == MOVE_HYPER_BEAM ? 100
        : p->move == MOVE_FLAMETHROWER ? 76 : p->kind == ARENA_MOVE_CONE ? 40 : 70+sArena.style*8;
    s32 best = 0x7FFFFFFF;
    u32 i;
    if (p->kind == ARENA_MOVE_PROJECTILE && gBattleMons[1].hp * 3 < gBattleMons[1].maxHP) preferred += 12;
    for (i = 0; i < 8 + ARENA_CORNERS; i++)
    {
        struct ArenaPoint p;
        s32 score;
        // Prefer the eight tactical positions around the observed opponent.
        // Only scan all obstacle corners when those positions are occluded.
        // This bounds routine thinking cost without removing the fallback.
        if(i==8&&best!=0x7FFFFFFF)break;
        if (i < 8)
        {
            p.x = Clamp(sArena.observed.x + offsets[i][0] * preferred / Q, ARENA_MIN_X, ARENA_MAX_X);
            p.y = Clamp(sArena.observed.y + offsets[i][1] * preferred / Q, ARENA_MIN_Y, ARENA_MAX_Y);
        }
        else p = ArenaNav_Corner(i - 8);
        if (!ArenaNav_CanStand(p.x, p.y) || !CoverCanStep(p.x,p.y)
            || !ArenaNav_LineClear(p.x, p.y, sArena.observed.x, sArena.observed.y, 2)) continue;
        score = Abs(ArenaNav_Distance(p.x,p.y,sArena.observed.x,sArena.observed.y) - preferred) * 3
            + ArenaNav_Distance(body->x/Q,body->y/Q,p.x,p.y) + AiRandom() % 24;
        if (score < best) { best = score; sArena.goal = p; }
    }
    sArena.goalTimer = 55;
    AiRoute();
}

static bool8 AiTryEvade(void)
{
    struct ArenaBody *body = &sArena.bodies[1];
    u32 i;
    if(StormEvade())return TRUE;
    // Only visible incoming projectiles at a perception tick. Novices often
    // miss the opportunity; high levels still have a reaction delay.
    for (i = 0; i < SHOTS_COUNT; i++)
    {
        struct ArenaShot *shot = &sArena.shots[i];
        s32 dx, dy, t, px, py, len, sign;
        struct ArenaPoint goal;
        if (!shot->life || shot->side != 0
            || SmokeBlocks(body->x/Q,body->y/Q,shot->x/Q,shot->y/Q)
            || !ArenaNav_LineClear(body->x/Q,body->y/Q,shot->x/Q,shot->y/Q,2)) continue;
        dx = shot->x - body->x; dy = shot->y - body->y;
        t = -(dx * shot->vx + dy * shot->vy) / (shot->vx * shot->vx + shot->vy * shot->vy);
        if (t < 1 || t > 22) continue;
        px = (dx + shot->vx * t) / Q; py = (dy + shot->vy * t) / Q;
        if (px * px + py * py > 256 || AiRandom() % 100 >= min(75, 15 + gBattleMons[1].level)) continue;
        len = max(Abs(shot->vx), Abs(shot->vy));
        for (sign = -1; sign <= 1; sign += 2)
        {
            goal.x = body->x/Q + sign * -shot->vy * 26 / len;
            goal.y = body->y/Q + sign * shot->vx * 26 / len;
            if (ArenaNav_CanStand(goal.x,goal.y)
                && ArenaNav_LineClear(body->x/Q,body->y/Q,goal.x,goal.y,ARENA_BODY_RADIUS))
            {
                sArena.goal = sArena.waypoint = goal;
                sArena.aiState = AI_EVADE;
                sArena.goalTimer = sArena.reaction;
                gArenaAiTelemetry.dodges++;
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void AiBeginAim(void)
{
    struct ArenaBody *body = &sArena.bodies[1];
    s32 travel = ArenaNav_Distance(body->x/Q,body->y/Q,sArena.observed.x,sArena.observed.y) * 2 / 5;
    s32 prediction = min(30+gBattleMons[1].level*2, 80);
    s32 errorX = AiRandom() % (sArena.aimError * 2 + 1) - sArena.aimError;
    s32 errorY = AiRandom() % (sArena.aimError * 2 + 1) - sArena.aimError;
    body->aimX = (sArena.observed.x + errorX
        + (sArena.observed.x - sArena.previous.x) * travel * prediction / (sArena.reaction * 100)) * Q;
    body->aimY = (sArena.observed.y + errorY
        + (sArena.observed.y - sArena.previous.y) * travel * prediction / (sArena.reaction * 100)) * Q;
    sArena.aimTimer = Clamp(20 - gBattleMons[1].level / 6, 12, 20);
    sArena.aiState = AI_AIM;
    // This target is locked for the entire visible wind-up.
}

static s32 AiTypeValue(u16 move)
{
    u32 i;
    u8 type=GetMoveType(move);
    const struct BattlePokemon *target=&gBattleMons[0];
    s32 value=100;
    if(target->ability==ABILITY_LEVITATE && type==TYPE_GROUND)return 0;
    value=value*GetTypeModifier(type,target->types[0])/4096;
    if(target->types[1]!=target->types[0])value=value*GetTypeModifier(type,target->types[1])/4096;
    if(target->ability==ABILITY_WONDER_GUARD && value<=100)return 0;
    if(type==gBattleMons[1].types[0] || type==gBattleMons[1].types[1])value=value*3/2;
    return value;
}

static void AiChooseMove(s32 distance)
{
    u32 i;
    s32 best=-100000;
    bool8 hasPhysical=FALSE,hasAttack=FALSE;
    if(sEncore[1]&&sArena.bodies[1].moveSlot<MAX_MON_MOVES&&ArenaMovePP(1,sArena.bodies[1].moveSlot))return; // Encore lock
    for(i=0;i<=MAX_MON_MOVES;i++)
    {
        u16 move=ArenaMoveId(1,i);
        if(SupportedMove(move) && ArenaMovePP(1,i) && GetMovePower(move)
            && GetMoveType(move)<TYPE_MYSTERY && AiTypeValue(move))hasPhysical=TRUE;
        if(SupportedMove(move) && ArenaMovePP(1,i) && GetMovePower(move))hasAttack=TRUE;
    }
    for(i=0;i<=MAX_MON_MOVES;i++)
    {
        u16 move=ArenaMoveId(1,i);
        const struct ArenaMoveProfile *p=ArenaMoves_Get(move);
        s32 score;
        if(!p||!ArenaMovePP(1,i))continue;
        // Pure estimate from the native type chart. Never call TypeCalc here:
        // its damage globals belong to the actual hit transaction.
        score=GetMovePower(move)*(move==MOVE_STRUGGLE?100:AiTypeValue(move))/50+AiRandom()%25;
        if(move==MOVE_NIGHT_SHADE||move==MOVE_SEISMIC_TOSS)score=AiTypeValue(move)?gBattleMons[1].level*2:-1000;
        if(move==MOVE_SNORE&&!(gBattleMons[1].status1&STATUS1_SLEEP))score=-1000;
        if(move==MOVE_FAKE_OUT&&sArena.frame>240)score-=40; // a lead-off move
        if(move==MOVE_ROLLOUT&&AiTypeValue(move))score+=45; // the roll's ramp is worth more than its listed power
        else if(move!=MOVE_STRUGGLE && GetMovePower(move) && !AiTypeValue(move))score=-1000;
        if((move==MOVE_ABSORB||move==MOVE_MEGA_DRAIN||move==MOVE_GIGA_DRAIN||move==MOVE_LEECH_LIFE) && AiTypeValue(move)
            && gBattleMons[1].hp*2<gBattleMons[1].maxHP)score+=90;
        if(!GetMovePower(move))
        {
            u8 stat=STAT_ATK;bool8 self;
            s8 change=RealtimeArena_StatChange(move,&stat,&self);
            bool8 useful=change && (change>0?gBattleMons[1].statStages[stat]<8:gBattleMons[0].statStages[stat]>4);
            if((move==MOVE_LEER||move==MOVE_TAIL_WHIP||move==MOVE_SCREECH||move==MOVE_HOWL)&&!hasPhysical)useful=FALSE;
            if(move==MOVE_FOCUS_ENERGY)useful=hasPhysical&&!gBattleMons[1].volatiles.focusEnergy;
            if(move==MOVE_DOUBLE_TEAM)useful=!sDecoys[1].life;
            if(move==MOVE_SUBSTITUTE)useful=!sCover[1].hp&&gBattleMons[1].hp>gBattleMons[1].maxHP/2;
            if(move==MOVE_SMOKESCREEN)useful=!sSmoke[1].life&&distance<100;
            if(move==MOVE_TELEPORT)useful=distance<60;
            if(move==MOVE_REFLECT)useful=!sBarrierLife[1][0];
            if(move==MOVE_LIGHT_SCREEN)useful=!sBarrierLife[1][1];
            if(move==MOVE_RAIN_DANCE)useful=!StormActive();
            // One setup action, then pressure. Repeated buffs should not turn
            // early wild encounters into several seconds of waiting around.
            if(hasAttack && sArena.bodies[1].statusActions)useful=FALSE;
            score=useful?85+AiRandom()%25:-1000;
            {
                struct BattlePokemon *me=&gBattleMons[1];
                if(MattIsHealMove(move))
                    score=me->hp*10<=me->maxHP*4?150+AiRandom()%40:-1000;
                else if(move==MOVE_REST)
                    score=me->hp<me->maxHP&&(me->hp*10<=me->maxHP*3||((me->status1&STATUS1_ANY)&&me->hp*2<=me->maxHP))?130+AiRandom()%30:-1000;
                else if(move==MOVE_ENDURE)
                    score=me->hp*5<=me->maxHP&&me->hp>1&&!sProtectChain[1]&&distance<60&&AiRandom()%3==0?140:-1000;
                else if(move==MOVE_HAZE)
                    score=MattStageSum(0,1)>=2||MattStageSum(1,-1)>=2?120+AiRandom()%20:-1000;
                else if(move==MOVE_MIST)
                    score=!(gSideStatuses[1]&SIDE_STATUS_MIST)&&MattHasDropMove(0)&&!sArena.bodies[1].statusActions?90+AiRandom()%20:-1000;
                else if(move==MOVE_THUNDER_WAVE)
                    score=!(gBattleMons[0].status1&STATUS1_ANY)&&!IS_BATTLER_OF_TYPE(0,TYPE_GROUND)&&!sArena.bodies[1].statusActions?110+AiRandom()%20:-1000;
                else if(move==MOVE_SUPERSONIC||move==MOVE_CONFUSE_RAY)
                    score=sArena.bodies[0].cooldown<30&&AiRandom()%3==0?80+AiRandom()%20:-1000;
                else if(move==MOVE_LEECH_SEED)
                    score=!gBattleMons[0].volatiles.leechSeed&&!IS_BATTLER_OF_TYPE(0,TYPE_GRASS)?105+AiRandom()%20:-1000;
                else if(move==MOVE_COUNTER||move==MOVE_MIRROR_COAT)
                    score=sLastHitAge[1]&&sLastHit[1]&&((sLastHitSpecial>>1)&1)==(move==MOVE_MIRROR_COAT)?140+AiRandom()%30:-1000;
                else if(move==MOVE_DESTINY_BOND)
                    score=!sDestinyLife[1]&&me->hp*4<=me->maxHP?125:-1000;
                else if(move==MOVE_PERISH_SONG)
                    score=!sPerish[0]&&!sArena.bodies[1].statusActions&&AiRandom()%4==0?70:-1000;
                else if(move==MOVE_TRANSFORM)
                    score=!me->volatiles.transformed?200:-1000;
                else if(move==MOVE_ENCORE)
                    score=!sEncore[0]&&sArena.bodies[0].moveSlot<MAX_MON_MOVES&&!GetMovePower(ArenaMoveId(0,sArena.bodies[0].moveSlot))?100:-1000;
                else if(move==MOVE_SAFEGUARD)
                    score=!(gSideStatuses[1]&SIDE_STATUS_SAFEGUARD)&&!sArena.bodies[1].statusActions?75:-1000;
                else if(move==MOVE_MIND_READER)
                    score=!gBattleMons[0].volatiles.lockOn&&hasAttack&&AiRandom()%3==0?85:-1000;
                else if(move==MOVE_WHIRLWIND)
                    score=distance<50&&AiRandom()%3==0?80:-1000;
                else if(move==MOVE_BELLY_DRUM)
                    score=me->hp*4>=me->maxHP*3&&hasPhysical&&me->statStages[STAT_ATK]<MAX_STAT_STAGE?120:-1000;
                else if(move==MOVE_ODOR_SLEUTH||move==MOVE_MUD_SPORT||move==MOVE_BATON_PASS||move==MOVE_SKETCH)
                    score=!sArena.bodies[1].statusActions&&AiRandom()%4==0?50:-1000;
            }
        }
        if(p->kind!=ARENA_MOVE_SELF && distance>p->range)score-=(distance-p->range)*2;
        if(ArenaMovePP(1,i)<=2)score-=12;
        if(score>best){best=score;sArena.bodies[1].moveSlot=i;}
    }
}

static void TickEnemy(void)
{
    struct ArenaBody *body = &sArena.bodies[1];
    s32 dx, dy, len;
    body->moving = FALSE;
    if(SpecialBusy(1)||GuardBusy())return;
    if(gBattleMons[1].status1&(STATUS1_SLEEP|STATUS1_FREEZE))return;
    if (body->cooldown && !body->shotTimer && !body->actionLife) body->cooldown--;
    if (sArena.goalTimer) sArena.goalTimer--;
    if (body->shotTimer) return;
    if (sArena.aiState == AI_AIM)
    {
        if (--sArena.aimTimer == 0)
        {
            if (ArenaNav_LineClear(body->x/Q,body->y/Q,body->aimX/Q,body->aimY/Q,2))
                BeginShot(1, body->aimX, body->aimY);
            sArena.aiState = AI_RECOVER;
            sArena.thinkTimer = 1;
        }
        return;
    }
    if (--sArena.thinkTimer == 0)
    {
        s32 distance;
        sArena.thinkTimer = sArena.reaction;
        sArena.previous = sArena.observed;
        if(!SmokeBlocks(body->x/Q,body->y/Q,sArena.bodies[0].x/Q,sArena.bodies[0].y/Q))
        {sArena.observed.x = sArena.bodies[0].x / Q;sArena.observed.y = sArena.bodies[0].y / Q;}
        else gArenaCoverTelemetry[5]++;
        if(sDecoys[0].life && sDecoys[0].age>=24 && !SmokeBlocks(body->x/Q,body->y/Q,sDecoys[0].x,sDecoys[0].y))
        {
            sArena.observed.x=sDecoys[0].x;
            sArena.observed.y=sDecoys[0].y;
            gArenaDecoyTelemetry[4]++;
        }
        gArenaAiTelemetry.decisions++;
        distance = ArenaNav_Distance(body->x/Q,body->y/Q,sArena.observed.x,sArena.observed.y);
        if(!body->cooldown)AiChooseMove(distance);
        if (!AiTryEvade())
        {
            const struct ArenaMoveProfile *p=ArenaMoves_Get(ArenaMoveId(1,body->moveSlot));
            if(p->kind==ARENA_MOVE_SELF && !body->cooldown && ArenaMovePP(1,body->moveSlot))
            {
                if(p->move==MOVE_SUBSTITUTE||p->move==MOVE_SMOKESCREEN)
                    BeginShot(1,sArena.observed.x*Q,sArena.observed.y*Q);
                else BeginShot(1,body->x,body->y-Q);
                sArena.aiState=AI_RECOVER;return;
            }
            if (!body->cooldown && ArenaMovePP(1,body->moveSlot)
                && distance < p->range && distance > 8
                && ArenaNav_LineClear(body->x/Q,body->y/Q,sArena.observed.x,sArena.observed.y,2))
            {
                AiBeginAim(); return;
            }
            sArena.aiState = body->cooldown ? AI_RECOVER : AI_REPOSITION;
            if (!sArena.goalTimer || distance < 38
                || ArenaNav_Distance(body->x/Q,body->y/Q,sArena.goal.x,sArena.goal.y) < 5
                || ArenaNav_Distance(sArena.previous.x,sArena.previous.y,sArena.observed.x,sArena.observed.y) > 18)
                AiChooseGoal();
            else AiRoute();
        }
    }
    if (ArenaNav_Distance(body->x/Q,body->y/Q,sArena.waypoint.x,sArena.waypoint.y) < 3
        && ArenaNav_Distance(body->x/Q,body->y/Q,sArena.goal.x,sArena.goal.y) >= 4) AiRoute();
    dx = sArena.waypoint.x * Q - body->x;
    dy = sArena.waypoint.y * Q - body->y;
    len = max(Abs(dx), Abs(dy)) + min(Abs(dx), Abs(dy)) / 2;
    if (len > Q)
    {
        s32 oldX=body->x,oldY=body->y;
        MoveDelta(1, dx * Speed(1) / len, dy * Speed(1) / len);
        if(oldX==body->x && oldY==body->y)
        {
            if(++sArena.stalledTicks>=24)
            {
                // Moving cover or the other body can invalidate a static
                // route. Replan instead of pushing forever into that point.
                sArena.stalledTicks=0;sArena.goalTimer=0;
                AiChooseGoal();
            }
        }
        else sArena.stalledTicks=0;
    }
}

static bool8 MattSupportMoveEffect(u8 side,u16 move)
{
    struct BattlePokemon *mon=&gBattleMons[side];
    struct Pokemon *party=side?&gEnemyParty[gBattlerPartyIndexes[side]]:&gPlayerParty[gBattlerPartyIndexes[side]];
    struct ArenaBody *body=&sArena.bodies[side];
    u32 heal=0,i;
    switch(move)
    {
    case MOVE_RECOVER: case MOVE_SOFT_BOILED: case MOVE_MILK_DRINK:
        heal=mon->maxHP/2; break;
    case MOVE_SYNTHESIS: case MOVE_MORNING_SUN: case MOVE_MOONLIGHT:
        heal=mon->maxHP/2;
        if(!IsAbilityOnField(ABILITY_CLOUD_NINE)
           &&!IsAbilityOnField(ABILITY_AIR_LOCK))
        {
            if(gBattleWeather&B_WEATHER_SUN)heal=mon->maxHP*2/3;
            else if(gBattleWeather&B_WEATHER_ANY)heal=mon->maxHP/4;
        }
        break;
    case MOVE_REST:
        if(mon->hp==mon->maxHP||mon->ability==ABILITY_INSOMNIA||mon->ability==ABILITY_VITAL_SPIRIT)
        {ArenaFeedback_Impact(side,body->x/Q,body->y/Q,0,ARENA_FEEDBACK_MISS);return TRUE;}
        mon->hp=mon->maxHP;mon->status1=STATUS1_SLEEP_TURN(2);body->burnClock=0;
        SetMonData(party,MON_DATA_HP,&mon->hp);SetMonData(party,MON_DATA_STATUS,&mon->status1);
        sArena.hudDirty=TRUE;PlaySE(SE_M_MORNING_SUN);body->flash=8;
        return TRUE;
    case MOVE_HAZE:
        for(i=0;i<NUM_BATTLE_STATS;i++)gBattleMons[0].statStages[i]=gBattleMons[1].statStages[i]=DEFAULT_STAT_STAGE;
        PlaySE(SE_M_HAZE);ArenaFeedback_Impact(side^1,sArena.bodies[side^1].x/Q,sArena.bodies[side^1].y/Q,0,ARENA_FEEDBACK_STAT);
        return TRUE;
    case MOVE_MIST:
        if(gSideStatuses[side]&SIDE_STATUS_MIST)
        {ArenaFeedback_Impact(side,body->x/Q,body->y/Q,0,ARENA_FEEDBACK_MISS);return TRUE;}
        gSideStatuses[side]|=SIDE_STATUS_MIST;gSideTimers[side].mistTimer=5;
        PlaySE(SE_M_MIST);return TRUE;
    case MOVE_BELLY_DRUM:
        if(mon->hp<=mon->maxHP/2||mon->statStages[STAT_ATK]==MAX_STAT_STAGE)
        {ArenaFeedback_Impact(side,body->x/Q,body->y/Q,0,ARENA_FEEDBACK_MISS);return TRUE;}
        mon->hp-=mon->maxHP/2;mon->statStages[STAT_ATK]=MAX_STAT_STAGE;
        SetMonData(party,MON_DATA_HP,&mon->hp);sArena.hudDirty=TRUE;
        ArenaFeedback_Impact(side,body->x/Q,body->y/Q,STAT_ATK|256,ARENA_FEEDBACK_STAT);PlaySE(SE_M_BELLY_DRUM);
        return TRUE;
    case MOVE_SAFEGUARD:
        if(gSideStatuses[side]&SIDE_STATUS_SAFEGUARD)
        {ArenaFeedback_Impact(side,body->x/Q,body->y/Q,0,ARENA_FEEDBACK_MISS);return TRUE;}
        gSideStatuses[side]|=SIDE_STATUS_SAFEGUARD;gSideTimers[side].safeguardTimer=5;
        PlaySE(SE_M_REFLECT);return TRUE;
    case MOVE_MUD_SPORT:
        gBattleMons[side].volatiles.mudSport=1;gFieldTimers.mudSportTimer=5;PlaySE(SE_M_SAND_ATTACK);return TRUE;
    case MOVE_DESTINY_BOND:
        sDestinyLife[side]=180;PlaySE(SE_M_PSYBEAM);return TRUE;
    case MOVE_PERISH_SONG:
        // Gen III: everyone not Soundproof and not already counting.
        for(i=0;i<2;i++)if(!sPerish[i]&&gBattleMons[i].ability!=ABILITY_SOUNDPROOF)sPerish[i]=4;
        PlaySE(SE_M_PERISH_SONG);return TRUE;
    case MOVE_TRANSFORM:
    {
        struct BattlePokemon *t=&gBattleMons[side^1];
        if(mon->volatiles.transformed)
        {ArenaFeedback_Impact(side,body->x/Q,body->y/Q,0,ARENA_FEEDBACK_MISS);return TRUE;}
        // Native copy minus species/sprite: types, stats, stages, moves (5 PP).
        mon->attack=t->attack;mon->defense=t->defense;mon->speed=t->speed;
        mon->spAttack=t->spAttack;mon->spDefense=t->spDefense;
        mon->types[0]=t->types[0];mon->types[1]=t->types[1];mon->ability=t->ability;
        for(i=0;i<NUM_BATTLE_STATS;i++)mon->statStages[i]=t->statStages[i];
        for(i=0;i<MAX_MON_MOVES;i++){mon->moves[i]=t->moves[i];mon->pp[i]=t->moves[i]?min(5,GetMovePP(t->moves[i])):0;}
        mon->volatiles.transformed=1;body->moveSlot=FirstMove(side,TRUE);
        if(body->moveSlot==ARENA_NO_MOVE)body->moveSlot=ARENA_STRUGGLE_SLOT;
        PlaySE(SE_M_TELEPORT);body->flash=8;sArena.hudDirty=TRUE;return TRUE;
    }
    case MOVE_BATON_PASS: // No party switch in a wild arena: no effect.
        ArenaFeedback_Impact(side,body->x/Q,body->y/Q,0,ARENA_FEEDBACK_MISS);return TRUE;
    default: return FALSE;
    }
    if(mon->hp==mon->maxHP||!heal)
    {ArenaFeedback_Impact(side,body->x/Q,body->y/Q,0,ARENA_FEEDBACK_MISS);return TRUE;}
    mon->hp=min(mon->maxHP,mon->hp+heal);
    SetMonData(party,MON_DATA_HP,&mon->hp);
    gArenaMoveTelemetry.healed[side]+=heal;
    sArena.hudDirty=TRUE;body->flash=8;PlaySE(SE_M_MORNING_SUN);
    return TRUE;
}

static bool8 MattTargetUtility(u8 side,u8 targetSide,u16 move)
{
    struct BattlePokemon *t=&gBattleMons[targetSide];
    struct ArenaBody *target=&sArena.bodies[targetSide];
    bool8 worked=FALSE;
    u8 acc=GetMoveAccuracy(move);
    switch(move)
    {
    case MOVE_THUNDER_WAVE:
        if(!IS_BATTLER_OF_TYPE(targetSide,TYPE_GROUND) && Random()%100<acc)
            worked=RealtimeArena_InflictStatus(side,targetSide,MOVE_EFFECT_PARALYSIS,FALSE);
        if(worked){target->burnClock=0;gArenaStatusTelemetry[targetSide]++;}
        break;
    case MOVE_SUPERSONIC: case MOVE_CONFUSE_RAY:
        // Arena confusion = a short daze (no self-hit roll): cancels the
        // pending action and adds about 1.5s of recovery.
        if(t->ability!=ABILITY_OWN_TEMPO && !(gSideStatuses[targetSide]&SIDE_STATUS_SAFEGUARD) && Random()%100<acc)
        {
            target->shotTimer=0;target->actionLife=0;target->cooldown=max(target->cooldown,90);worked=TRUE;
        }
        break;
    case MOVE_LEECH_SEED:
        if(!IS_BATTLER_OF_TYPE(targetSide,TYPE_GRASS) && !gBattleMons[targetSide].volatiles.leechSeed && Random()%100<acc)
        {gBattleMons[targetSide].volatiles.leechSeed=LEECHSEEDED_BY(side);worked=TRUE;}
        break;
    case MOVE_WHIRLWIND:
        // Wild Gen III Whirlwind ends the battle; the arena blows the target away.
        if(t->ability!=ABILITY_SUCTION_CUPS)
        {
            target->knockX=sArena.bodies[side].attackX*8;target->knockY=sArena.bodies[side].attackY*8;
            target->shotTimer=0;target->actionLife=0;target->cooldown=max(target->cooldown,40);worked=TRUE;
        }
        break;
    case MOVE_ENCORE:
        if(!sEncore[targetSide] && target->moveSlot<MAX_MON_MOVES && ArenaMovePP(targetSide,target->moveSlot))
        {sEncore[targetSide]=60;worked=TRUE;} // ~3 turns in 15-frame units
        break;
    case MOVE_MIND_READER:
        gBattleMons[targetSide].volatiles.lockOn=2;
        gBattleMons[targetSide].volatiles.battlerWithSureHit=side;worked=TRUE;
        break;
    case MOVE_ODOR_SLEUTH:
        t->volatiles.foresight=1;worked=TRUE;
        break;
    case MOVE_SKETCH: // Permanent move copy is not modeled in the arena.
        break;
    default: return FALSE;
    }
    if(side==0)gBattleResults.lastUsedMovePlayer=move;else gBattleResults.lastUsedMoveOpponent=move;
    ArenaFeedback_Impact(targetSide,target->x/Q,target->y/Q,0,worked?ARENA_FEEDBACK_STAT:ARENA_FEEDBACK_MISS);
    if(worked)PlaySE(SE_M_CONFUSE_RAY);
    sArena.hudDirty=TRUE;
    return TRUE;
}

static void ApplyResolvedHit(u8 side,u16 move,s32 damage);
static void ApplyMoveHit(u8 side,u16 move)
{
    const struct ArenaMoveProfile *profile=ArenaMoves_Get(move);
    u8 targetSide=profile->kind==ARENA_MOVE_SELF?side:side^1;
    struct ArenaBody *target=&sArena.bodies[targetSide];
    s32 damage;
    if(move==MOVE_PROTECT||move==MOVE_DETECT)
    {
        // Gen III consecutive use: success 1/2^n after n straight guards.
        if(sProtectChain[side] && (Random()&((1u<<min(sProtectChain[side],3))-1)))
        {sProtectChain[side]=0;ArenaFeedback_Wall(target->x/Q,target->y/Q);return;}
        sProtectChain[side]++;if(!side)sMattProtectGap=240|0x8000;GuardStart(side);return;
    }
    if(move==MOVE_ENDURE)
    {
        // Shares Protect's consecutive-use counter, as in Gen III.
        if(sProtectChain[side] && (Random()&((1u<<min(sProtectChain[side],3))-1)))
        {sProtectChain[side]=0;ArenaFeedback_Wall(target->x/Q,target->y/Q);return;}
        sProtectChain[side]++;sEndureLife[side]=60;PlaySE(SE_M_HARDEN);
        if(!side)sMattProtectGap|=0x8000;
        ArenaFeedback_Impact(side,target->x/Q,target->y/Q,STAT_DEF|256,ARENA_FEEDBACK_STAT);
        return;
    }
    sProtectChain[side]=0;
    if(MattSupportMoveEffect(side,move))return;
    if(GetMovePower(move)&&GuardIntercept(side,move))return;
    if(move==MOVE_SUBSTITUTE){if(!CoverStart(side))ArenaFeedback_Wall(target->x/Q,target->y/Q);return;}
    if(move==MOVE_SMOKESCREEN){SmokeStart(side);return;}
    if(move==MOVE_RAIN_DANCE){StormStart();return;}
    if(move==MOVE_REFLECT||move==MOVE_LIGHT_SCREEN){BarrierStart(side,move);return;}
    if(move!=MOVE_TELEPORT&&move!=MOVE_DOUBLE_TEAM&&SpecialInvulnerable(targetSide))
    {gArenaWarpTossTelemetry[7]++;return;}
    if(move==MOVE_TELEPORT){WarpStart(side);return;}
    if(MattTargetUtility(side,targetSide,move))return;
    if(move==MOVE_DOUBLE_TEAM)
    {
        if(!DecoyStart(side))ArenaFeedback_Impact(side,target->x/Q,target->y/Q,0,ARENA_FEEDBACK_MISS);
        return;
    }
    if(!GetMovePower(move))
    {
        u8 stat=STAT_ATK;bool8 self;
        s8 delta=RealtimeArena_StatChange(move,&stat,&self);
        bool8 worked=RealtimeArena_ResolveStatMove(side,targetSide,move);
        if(worked)gArenaMoveTelemetry.statChanges[side]++;
        ArenaFeedback_Impact(targetSide,target->x/Q,target->y/Q,stat|(delta>0?256:0),
            worked?(move==MOVE_FOCUS_ENERGY?ARENA_FEEDBACK_CRIT:ARENA_FEEDBACK_STAT):ARENA_FEEDBACK_MISS);
        return;
    }
    if(move==MOVE_COUNTER||move==MOVE_MIRROR_COAT)
    {
        bool8 special=move==MOVE_MIRROR_COAT;
        // Gen III: 2x the last hit of the matching category taken recently.
        if(!sLastHitAge[side]||!sLastHit[side]||((sLastHitSpecial>>side)&1)!=special
           ||(special?IS_BATTLER_OF_TYPE(targetSide,TYPE_DARK):IS_BATTLER_OF_TYPE(targetSide,TYPE_GHOST)))
        {ArenaFeedback_Impact(targetSide,target->x/Q,target->y/Q,0,ARENA_FEEDBACK_MISS);return;}
        sLastHitAge[side]=0;
        ApplyResolvedHit(side,move,min(65535,sLastHit[side]*2));
        return;
    }
    if(move==MOVE_SNORE&&!(gBattleMons[side].status1&STATUS1_SLEEP))
    {ArenaFeedback_Impact(targetSide,target->x/Q,target->y/Q,0,ARENA_FEEDBACK_MISS);return;}
    damage=RealtimeArena_ResolveDamage(side,targetSide,move==MOVE_NATURE_POWER?MOVE_SWIFT:move);
    if(move==MOVE_BIDE && damage>0)damage=min(65535,sBideDamage[side]*2);
    // Visual only: super-effective hits get a slightly longer hit-pause and a
    // 2px horizontal shake of the arena layer. Damage is untouched.
    if(damage>0 && (gBattleStruct->moveResultFlags[1]&MOVE_RESULT_SUPER_EFFECTIVE))sSeShake=10;
    ApplyResolvedHit(side,move,damage);
    if(sSeShake==10)sArena.hitstop=max(sArena.hitstop,5);
}

// Also used by Seismic Toss: accuracy is decided at the grab, HP at landing.
static void ApplyResolvedHit(u8 side,u16 move,s32 damage)
{
    u8 targetSide=side^1;
    struct ArenaBody *target=&sArena.bodies[targetSide];
    if(damage>0)
    {
        u16 hp=gBattleMons[targetSide].hp;
        u16 dealt=min(damage,hp);
        if(sEndureLife[targetSide] && dealt>=hp && hp>1)dealt=hp-1;
        if(move!=MOVE_COUNTER&&move!=MOVE_MIRROR_COAT)
        {
            sLastHit[targetSide]=dealt;sLastHitAge[targetSide]=180;
            if(GetMoveType(move)>=TYPE_MYSTERY)sLastHitSpecial|=1<<targetSide;else sLastHitSpecial&=~(1<<targetSide);
        }
        if(target->actionLife && ArenaMoveId(targetSide,target->shotSlot)==MOVE_BIDE)
            sBideDamage[targetSide]=min(32767,sBideDamage[targetSide]+dealt);
        ArenaFeedback_Impact(targetSide,target->x/Q,target->y/Q,dealt,ARENA_FEEDBACK_DAMAGE);
        sArena.hitstop=damage>=8?3:2;
        hp-=dealt;
        gBattleMons[targetSide].hp=hp;
        SetMonData(targetSide?&gEnemyParty[gBattlerPartyIndexes[targetSide]]:
                   &gPlayerParty[gBattlerPartyIndexes[targetSide]],MON_DATA_HP,&hp);
        if(move==MOVE_ABSORB || move==MOVE_MEGA_DRAIN || move==MOVE_GIGA_DRAIN || move==MOVE_LEECH_LIFE)
        {
            u16 healing=min(RealtimeArena_DrainAmount(dealt),gBattleMons[side].maxHP-gBattleMons[side].hp);
            u16 healedHp=gBattleMons[side].hp+healing;
            if(gBattleMons[targetSide].ability==ABILITY_LIQUID_OOZE)
            {
                // Liquid Ooze: the drain hurts instead (native amount).
                healing=min(RealtimeArena_DrainAmount(dealt),gBattleMons[side].hp);
                healedHp=gBattleMons[side].hp-healing;healing=0;
                if(!healedHp){sArena.resultTimer=12;sArena.lastAttacker=targetSide;sArena.lastTarget=side;gArenaResultTelemetry.started=gMain.vblankCounter1;}
            }
            gBattleMons[side].hp=healedHp;
            SetMonData(side?&gEnemyParty[gBattlerPartyIndexes[side]]:
                       &gPlayerParty[gBattlerPartyIndexes[side]],MON_DATA_HP,&healedHp);
            gArenaMoveTelemetry.healed[side]+=healing;
            ArenaFeedback_Drain(side,sArena.bodies[side].x/Q,sArena.bodies[side].y/Q,
                                target->x/Q,target->y/Q,healing);
        }
        target->flash=8;
        if(MoveMakesContact(move) && gBattleMons[side].hp)
        {
            u8 ability=gBattleMons[targetSide].ability;
            u8 effect=ability==ABILITY_STATIC?MOVE_EFFECT_PARALYSIS:
                ability==ABILITY_POISON_POINT?MOVE_EFFECT_POISON:
                ability==ABILITY_FLAME_BODY?MOVE_EFFECT_BURN:0;
            if(effect && Random()%3==0)
                RealtimeArena_InflictStatus(targetSide,side,effect,TRUE);
            if(ability==ABILITY_ROUGH_SKIN)
            {
                u16 hurt=min(gBattleMons[side].hp,max(1,gBattleMons[side].maxHP/16));
                u16 left=gBattleMons[side].hp-hurt;
                gBattleMons[side].hp=left;
                SetMonData(side?&gEnemyParty[gBattlerPartyIndexes[side]]:&gPlayerParty[gBattlerPartyIndexes[side]],MON_DATA_HP,&left);
                ArenaFeedback_Impact(side,sArena.bodies[side].x/Q,sArena.bodies[side].y/Q,hurt,ARENA_FEEDBACK_DAMAGE);
                if(!left){sArena.resultTimer=12;sArena.lastAttacker=targetSide;sArena.lastTarget=side;gArenaResultTelemetry.started=gMain.vblankCounter1;}
            }
            if(ability==ABILITY_EFFECT_SPORE && Random()%10==0)
            {
                do {effect=Random()&3;}while(!effect);
                if(effect==MOVE_EFFECT_BURN)effect=MOVE_EFFECT_PARALYSIS;
                RealtimeArena_InflictStatus(targetSide,side,effect,TRUE);
            }
        }
        if((gBattleMons[targetSide].status1&STATUS1_FREEZE)&&GetMoveType(move)==TYPE_FIRE)
        {
            gBattleMons[targetSide].status1&=~STATUS1_FREEZE;
            SetMonData(targetSide?&gEnemyParty[gBattlerPartyIndexes[targetSide]]:&gPlayerParty[gBattlerPartyIndexes[targetSide]],
                       MON_DATA_STATUS,&gBattleMons[targetSide].status1);
        }
        target->knockX=sArena.bodies[side].attackX*(move==MOVE_QUICK_ATTACK?4:2);
        target->knockY=sArena.bodies[side].attackY*(move==MOVE_QUICK_ATTACK?4:2);
        gRealtimeArenaTelemetry.hits[side]++;
        gRealtimeArenaTelemetry.lastDamage=damage;
        if(hp || move==MOVE_SUPERPOWER || move==MOVE_METEOR_MASH || move==MOVE_STEEL_WING)
        {
            u8 effects=RealtimeArena_ResolveSecondary(side,targetSide,move);
            if(effects&1){gArenaMoveTelemetry.statChanges[side]++;gArenaAdventureTelemetry[0]++;}
            if(effects&4)gArenaAdventureTelemetry[2]++;
            if(effects&8)
            {
                target->burnClock=0;gArenaStatusTelemetry[targetSide]++;
                ArenaFeedback_Impact(targetSide,target->x/Q,target->y/Q,0,ARENA_FEEDBACK_BURN);
            }
            if(effects&2)
            {
                gArenaAdventureTelemetry[1]++;
                // A committed hit is not refunded. Cancel only the pending
                // action and add recovery; already emitted projectiles live on.
                if(target->shotTimer && target->shotElapsed<=ArenaMoves_Get(ArenaMoveId(targetSide,target->shotSlot))->windup)
                    gArenaCombatTelemetry.cancelled[targetSide]++;
                target->shotTimer=0;target->actionLife=0;
                target->cooldown=max(target->cooldown,18);
            }
        }
        sArena.hudDirty=TRUE;
        PlaySE(move==MOVE_ABSORB?SE_M_ABSORB_2:SE_M_COMET_PUNCH);
        if(!hp && sDestinyLife[targetSide] && gBattleMons[side].hp)
        {
            // Destiny Bond: the attacker faints too.
            u16 zero=0;gBattleMons[side].hp=0;sDestinyLife[targetSide]=0;
            SetMonData(side?&gEnemyParty[gBattlerPartyIndexes[side]]:&gPlayerParty[gBattlerPartyIndexes[side]],MON_DATA_HP,&zero);
            sArena.hudDirty=TRUE;
        }
        if(!hp)
        {
            // Let a committed beam finish its visual release after a KO.
            // Damage/PP are already resolved; no further hits or input run.
            sArena.resultTimer=ArenaMoves_Beam(move)?max(12,sArena.bodies[side].actionLife+8):12;
            sArena.lastAttacker=side;sArena.lastTarget=targetSide;
            gArenaResultTelemetry.started=gMain.vblankCounter1;
        }
    }
    else if(move==MOVE_FALSE_SWIPE && !(gBattleStruct->moveResultFlags[1]&MOVE_RESULT_NO_EFFECT))
    {
        // The native damage clamp legitimately returns zero at 1 HP. This
        // contact is not an accuracy miss and must never faint the target.
        gRealtimeArenaTelemetry.hits[side]++;
        gRealtimeArenaTelemetry.lastDamage=0;
        ArenaFeedback_Impact(targetSide,target->x/Q,target->y/Q,0,ARENA_FEEDBACK_DAMAGE);
    }
    else
    {
        gRealtimeArenaTelemetry.misses++;
        ArenaFeedback_Impact(targetSide,target->x/Q,target->y/Q,0,
            (gBattleStruct->moveResultFlags[1]&MOVE_RESULT_DOESNT_AFFECT_FOE)?ARENA_FEEDBACK_IMMUNE:ARENA_FEEDBACK_MISS);
    }
}

#include "arena_psychic.inc"
#include "arena_elements.inc"
#include "arena_storm.inc"
#include "arena_warp_toss.inc"
#include "arena_guard.inc"
#include "arena_flame_clash.inc"

static void DrawPsychicAuras(void)
{
    u32 side,i,n=0;
    for(i=0;i<3;i++)if(sRockShadows[i]!=MAX_SPRITES)gSprites[sRockShadows[i]].invisible=TRUE;
    for(i=0;i<ARENA_OBSTACLES&&n<3;i++)
        if(gArenaPsychicRocks[i].state)
        {
            const struct ArenaPsychicRock *r=&gArenaPsychicRocks[i];
            if(sRockShadows[n]!=MAX_SPRITES)
            {struct Sprite *s=&gSprites[sRockShadows[n]];s->x=r->x/Q;s->y=r->y/Q+8;s->invisible=sArena.paused;}
            n++;
        }
    for(side=0;side<2;side++)
        if(sPsychic[side].active)
            ArenaMoveFx_Psychic(side,sArena.bodies[side].x/Q,sArena.bodies[side].y/Q-8,
                min(23,sPsychic[side].age/4),!sArena.paused);
}

static void HitTerrain(u8 index,const struct ArenaMoveProfile*p,s16 ix,s16 iy)
{
    u32 before=gArenaPhysicsTelemetry.broken;
    ArenaPhysics_Hit(index,ix,iy,p->kind==ARENA_MOVE_PROJECTILE?1:2);
    if(before!=gArenaPhysicsTelemetry.broken)sArena.hitstop=3;
}

static void TickActions(void)
{
    u32 side;
    for(side=0;side<2&&!sArena.resultTimer;side++)
    {
        struct ArenaBody *body=&sArena.bodies[side],*target=&sArena.bodies[side^1];
        const struct ArenaMoveProfile *p;
        s16 oldX=body->x/Q,oldY=body->y/Q;
        bool8 hit=FALSE;
        if(!body->actionLife)continue;
        p=ArenaMoves_Get(ArenaMoveId(side,body->shotSlot));
        if(p->move==MOVE_BIDE)
        {
            body->actionAge++;
            if(!(body->actionAge%24))ArenaFeedback_Dust(oldX,oldY,TRUE);
            if(!--body->actionLife)
            {
                if(sBideDamage[side] && !target->dash
                    && ArenaNav_Distance(oldX,oldY,target->x/Q,target->y/Q)<=p->range
                    && ArenaNav_LineClear(oldX,oldY,target->x/Q,target->y/Q,2))
                    ApplyMoveHit(side,MOVE_BIDE);
                else ArenaFeedback_Impact(side,oldX,oldY,0,ARENA_FEEDBACK_MISS);
            }
            continue;
        }
        if(p->move==MOVE_DIG||p->move==MOVE_FLY)
        {
            if(!body->connected){body->connected=TRUE;DiveStart(side,p->move);}
            continue;
        }
        if(p->move==MOVE_SEISMIC_TOSS)
        {
            if(!body->connected){body->connected=TRUE;TossStart(side);}
            if(body->actionLife){body->actionAge++;body->actionLife--;}
            continue;
        }
        if(p->kind==ARENA_MOVE_SELF)
        {
            if(!body->connected){body->connected=TRUE;ApplyMoveHit(side,p->move);}
        }
        else if(p->kind==ARENA_MOVE_RUSH)
        {
            bool8 rollout=p->move==MOVE_ROLLOUT;
            s32 speed=rollout?RolloutSpeed(body,p):p->speed;
            s32 dx=body->attackX*speed/Q,dy=body->attackY*speed/Q;
            s16 obstacle=ArenaNav_FirstObstacle(oldX,oldY,(body->x+dx)/Q,(body->y+dy)/Q,ARENA_BODY_RADIUS);
            // A ghost's rush passes through cover instead of slamming into it.
            if(obstacle>=0 && !GhostBody(side))
            {
                if(rollout)
                {
                    // A fast roll smashes through cover; a slow one stops at it.
                    u32 before=gArenaPhysicsTelemetry.broken;
                    ArenaPhysics_Hit(obstacle,body->attackX*3,body->attackY*3,2+body->rollStage);
                    if(before!=gArenaPhysicsTelemetry.broken)sArena.hitstop=3;
                    else RolloutEnd(body);
                }
                else
                {
                    body->actionLife=1;
                    HitTerrain(obstacle,p,body->attackX*3,body->attackY*3);
                }
                gArenaMoveTelemetry.rushWalls[side]++;
                ArenaFeedback_Wall(oldX,oldY);PlaySE(SE_WALL_HIT);
            }
            MoveDelta(side,dx,dy);
            if(rollout)
            {
                s32 moved=Abs(body->x/Q-oldX)+Abs(body->y/Q-oldY);
                if(!moved && body->actionLife>1)RolloutEnd(body); // the wall
                body->rollDistance=min(255,body->rollDistance+moved);
                if(RollBallReady(side))body->rollAngle+=moved*sBall[side].step;
                body->rollStage=min(ROLLOUT_MAX_STAGE,body->rollDistance/ROLLOUT_STAGE_PX);
                if(!(body->actionAge%2))ArenaFeedback_Dust(body->x/Q,body->y/Q,TRUE);
            }
            else if(!(body->actionAge%3))ArenaFeedback_Dust(body->x/Q,body->y/Q,TRUE);
            hit=ArenaMoves_SegmentHit(oldX,oldY,body->x/Q,body->y/Q,target->x/Q,target->y/Q,p->radius);
        }
        else if(ArenaMoves_Beam(p->move))
        {
            u32 j;
            u8 reach=min(p->range,min(20+body->actionAge*7,body->actionLife*11));
            s16 obstacle;
            // A sustained, aim-locked cone. Cover stops the jet until it breaks.
            // One PP and one native damage resolution, NOT damage every frame.
            obstacle=ArenaNav_FirstObstacle(oldX,oldY,oldX+body->attackX*reach/Q,oldY+body->attackY*reach/Q,1);
            if(obstacle>=0)
            {
                const struct ArenaRect *r=&gArenaObstacles[obstacle];
                s16 nearX=body->attackX>0?r->left-1:r->right+1;
                s16 nearY=body->attackY>0?r->top-1:r->bottom+1;
                s16 alongX=body->attackX?(nearX-oldX)*Q/body->attackX:0;
                s16 alongY=body->attackY?(nearY-oldY)*Q/body->attackY:0;
                // Eight aim directions let us intersect the AABB analytically;
                // avoid thirty full visibility scans on every GBA frame.
                reach=min(reach,max(4,max(alongX,alongY)));
                if(!(body->terrainMask&(1<<obstacle)))
                {
                    body->terrainMask|=1<<obstacle;
                    ArenaPhysics_Hit(obstacle,body->attackX*4,body->attackY*4,3);
                    ArenaFeedback_Embers(oldX+body->attackX*reach/Q,oldY+body->attackY*reach/Q,body->attackX,body->attackY);
                    gArenaFlameTelemetry[3]++;
                }
            }
            sFlameReach[side]=reach;gArenaFlameTelemetry[1]++;
            if(GetMoveType(p->move)==TYPE_ELECTRIC&&!body->connected)
            {
                u32 along;
                for(along=8;along<=reach;along+=6)
                    if(StormCharge(side,p->move,oldX+body->attackX*along/Q,oldY+body->attackY*along/Q))
                    {body->connected=TRUE;break;}
            }
            gArenaFlameTelemetry[4]=max(gArenaFlameTelemetry[4],reach);
            for(j=0;j<ARENA_OBSTACLES;j++)if(!(body->terrainMask&(1<<j))&&ArenaNav_IsSolid(j))
            {
                const struct ArenaRect *r=&gArenaObstacles[j];
                s16 x=(r->left+r->right)/2,y=(r->top+r->bottom)/2;
                if(ArenaMoves_InCone(x-oldX,y-oldY,body->attackX,body->attackY,reach,p->cone)
                    &&ArenaNav_FirstObstacle(oldX,oldY,x,y,0)==j)
                {
                    body->terrainMask|=1<<j;
                    ArenaPhysics_Hit(j,body->attackX*4,body->attackY*4,3);
                    ArenaFeedback_Embers(x,y,body->attackX,body->attackY);gArenaFlameTelemetry[3]++;
                }
            }
            for(j=0;j<ELEMENT_COUNT;j++)
            {
                struct ArenaSurface *surface=&sSurfaces[j];
                if(surface->kind&&ArenaMoves_InCone(surface->x-oldX,surface->y-oldY,body->attackX,body->attackY,reach+8,p->cone)
                    &&ArenaNav_LineClear(oldX,oldY,surface->x,surface->y,0))
                {
                    struct ArenaShot heat={0};
                    heat.move=(p->move==MOVE_AURORA_BEAM?MOVE_ICY_WIND:p->move==MOVE_HYDRO_PUMP?MOVE_SURF:(p->move==MOVE_FLAMETHROWER||p->move==MOVE_FIRE_BLAST||p->move==MOVE_SACRED_FIRE)?MOVE_FLAMETHROWER:MOVE_NONE);heat.x=surface->x*Q;heat.y=surface->y*Q;
                    heat.elementMask=sFlameElementMask[side];ElementReact(&heat);
                    sFlameElementMask[side]=heat.elementMask;
                }
            }
            if(!(body->actionAge%9))ArenaFeedback_Embers(oldX+body->attackX*reach/Q,oldY+body->attackY*reach/Q-8,body->attackX,body->attackY);
            hit=(ArenaMoves_Beam(p->move)==1||body->actionAge>=8)
                &&ArenaMoves_InCone((target->x-body->x)/Q,(target->y-body->y)/Q,body->attackX,body->attackY,reach,p->cone);
        }
        else if(p->kind==ARENA_MOVE_MELEE||p->kind==ARENA_MOVE_CONE)
        {
            u8 range=p->kind==ARENA_MOVE_CONE?min(p->range,24+body->actionAge*4):p->range;
            u32 i;
            if(p->kind==ARENA_MOVE_MELEE)
                for(i=0;i<ARENA_OBSTACLES;i++)
                {
                    const struct ArenaRect*r=&gArenaObstacles[i];
                    s16 x=Clamp(oldX,r->left,r->right),y=Clamp(oldY,r->top,r->bottom);
                    if(!(body->terrainMask&(1<<i))&&ArenaNav_IsSolid(i)
                        &&ArenaMoves_InCone(x-oldX,y-oldY,body->attackX,body->attackY,range,p->cone)
                        &&ArenaNav_FirstObstacle(oldX,oldY,x,y,0)==i)
                    {
                        body->terrainMask|=1<<i;
                        HitTerrain(i,p,body->attackX*3,body->attackY*3);
                    }
                }
            hit=ArenaMoves_InCone((target->x-body->x)/Q,(target->y-body->y)/Q,
                                  body->attackX,body->attackY,range,p->cone);
        }
        if(!body->connected && GetMovePower(p->move))
        {
            if(p->kind==ARENA_MOVE_RUSH)
            {if(CoverSegment(side,p->move,oldX,oldY,body->x/Q,body->y/Q,p->radius))body->connected=TRUE;}
            else if(p->kind==ARENA_MOVE_CONE||p->kind==ARENA_MOVE_MELEE)
            {
                u8 range=ArenaMoves_Beam(p->move)?sFlameReach[side]:p->kind==ARENA_MOVE_CONE?min(p->range,24+body->actionAge*4):p->range;
                if(CoverCone(side,p,range,hit))body->connected=TRUE;
            }
        }
        if(!body->connected && GetMovePower(p->move) && sDecoys[side^1].life)
        {
            struct ArenaDecoy*d=&sDecoys[side^1];
            bool8 decoyHit=FALSE;
            if(p->kind==ARENA_MOVE_RUSH)
                decoyHit=DecoySegment(side^1,oldX,oldY,body->x/Q,body->y/Q,p->radius);
            else if(d->age>=24 && (p->kind==ARENA_MOVE_CONE||p->kind==ARENA_MOVE_MELEE))
            {
                u8 range=ArenaMoves_Beam(p->move)?sFlameReach[side]:p->kind==ARENA_MOVE_CONE?min(p->range,24+body->actionAge*4):p->range;
                if(ArenaMoves_InCone(d->x-body->x/Q,d->y-body->y/Q,body->attackX,body->attackY,range,p->cone)
                    &&ArenaNav_LineClear(body->x/Q,body->y/Q,d->x,d->y,2)
                    &&(!hit||target->dash||ArenaNav_Distance(oldX,oldY,d->x,d->y)<ArenaNav_Distance(oldX,oldY,target->x/Q,target->y/Q)))
                {DecoyBreak(side^1);decoyHit=TRUE;}
            }
            if(decoyHit)body->connected=TRUE;
        }
        if(hit&&!body->connected&&!target->dash
            &&ArenaNav_LineClear(body->x/Q,body->y/Q,target->x/Q,target->y/Q,2))
        {
            body->connected=TRUE;
            if(p->move==MOVE_FLAMETHROWER)gArenaFlameTelemetry[2]++;
            gArenaRolloutStage=body->rollStage;
            ApplyMoveHit(side,p->move);
            if(p->move==MOVE_ROLLOUT)
            {
                // Rolled over: the rival is flattened where it stands, not
                // shoved ahead of the ball, and the roll carries on past it.
                target->flat=ROLLOUT_FLAT_FRAMES;
                target->knockX=target->knockY=0;
                sArena.hitstop=max(sArena.hitstop,4);
            }
        }
        if(body->actionLife){body->actionAge++;body->actionLife--;}
    }
}

static void DestroyShot(struct ArenaShot *shot)
{
    u32 t;
    DestroySprite(&gSprites[shot->sprite]);
    for(t=0;t<2;t++)
        if(shot->trail[t]!=MAX_SPRITES)DestroySprite(&gSprites[shot->trail[t]]);
    shot->life=0;
}

static void TickShots(void)
{
    u32 i;
    for (i = 0; i < SHOTS_COUNT && !sArena.resultTimer; i++)
    {
        struct ArenaShot *shot = &sArena.shots[i];
        struct ArenaBody *target;
        const struct ArenaMoveProfile *p;
        s16 oldX,oldY;
        s16 obstacle;
        if (!shot->life) continue;
        p=ArenaMoves_Get(shot->move);
        oldX=shot->x/Q;oldY=shot->y/Q;
        target = &sArena.bodies[shot->side ^ 1];
        obstacle=ArenaNav_FirstObstacle(shot->x/Q,shot->y/Q,(shot->x+shot->vx)/Q,(shot->y+shot->vy)/Q,2);
        if (obstacle>=0)
        {
            gArenaAiTelemetry.blockedShots[shot->side]++;
            ArenaFeedback_Wall(shot->x/Q,shot->y/Q);
            PlaySE(SE_WALL_HIT);
            HitTerrain(obstacle,p,shot->vx,shot->vy);
            DestroyShot(shot);
            continue;
        }
        shot->x += shot->vx; shot->y += shot->vy;
        if(ElementReact(shot)){DestroyShot(shot);continue;}
        if(CoverSegment(shot->side,shot->move,oldX,oldY,shot->x/Q,shot->y/Q,p->radius))
        {DestroyShot(shot);continue;}
        if(DecoySegment(shot->side^1,oldX,oldY,shot->x/Q,shot->y/Q,p->radius))
        {DestroyShot(shot);continue;}
        if (ArenaMoves_SegmentHit(oldX,oldY,shot->x/Q,shot->y/Q,target->x/Q,target->y/Q,p->radius) && !target->dash)
        {
            if(SpecialInvulnerable(shot->side^1))gArenaWarpTossTelemetry[7]++;
            else{ApplyMoveHit(shot->side,shot->move);shot->life = 1;}
        }
        if (shot->x < 3 * Q || shot->x > 237 * Q || shot->y < 19 * Q || shot->y > 157 * Q) shot->life = 1;
        if (--shot->life == 0) DestroyShot(shot);
        else
        {
            u32 t;
            ArenaMoveFx_Bolt(shot->sprite,p,shot->x/Q,shot->y/Q,shot->direction,++shot->age);
            for(t=0;t<2;t++)
                if(shot->trail[t]!=MAX_SPRITES)
                {
                    u8 delay=(t+1)*2;
                    gSprites[shot->trail[t]].invisible=shot->age<delay;
                    ArenaMoveFx_Bolt(shot->trail[t],p,(shot->x-shot->vx*delay)/Q,
                        (shot->y-shot->vy*delay)/Q,shot->direction,shot->age);
                }
        }
    }
}

static void TickPhysics(void)
{
    u32 side;
    ArenaPhysics_Update();
    for(side=0;side<2;side++)
    {
        struct ArenaBody *body=&sArena.bodies[side];
        if(SpecialBusy(side))continue;
        if(sArena.lastBlast!=gArenaPhysicsTelemetry.blastSerial)
        {
            s32 dx=body->x/Q-gArenaPhysicsTelemetry.blastX;
            s32 dy=body->y/Q-gArenaPhysicsTelemetry.blastY;
            s32 len=max(1,max(Abs(dx),Abs(dy))+min(Abs(dx),Abs(dy))/2);
            if(dx*dx+dy*dy<46*46){body->knockX=dx*1200/len;body->knockY=dy*1200/len;}
        }
        if(body->knockX||body->knockY)
        {
            MoveDelta(side,body->knockX,body->knockY);
            body->knockX=body->knockX*180/256;body->knockY=body->knockY*180/256;
            if(Abs(body->knockX)<8)body->knockX=0;
            if(Abs(body->knockY)<8)body->knockY=0;
        }
    }
    sArena.lastBlast=gArenaPhysicsTelemetry.blastSerial;
}


static void MattTickItems(void)
{
    u32 side;
    for(side=0;side<2;side++)
    {
        struct BattlePokemon *b=&gBattleMons[side];
        if(sLastHitAge[side])sLastHitAge[side]--;
        if(sDestinyLife[side])sDestinyLife[side]--;
        if(sEncore[side]&&sArena.frame%15==0)sEncore[side]--;
        if(sArena.frame%300==0 && b->hp && !sArena.resultTimer)
        {
            if(gBattleMons[side].volatiles.lockOn)gBattleMons[side].volatiles.lockOn--;
            if((gSideStatuses[side]&SIDE_STATUS_SAFEGUARD)&&!--gSideTimers[side].safeguardTimer)gSideStatuses[side]&=~SIDE_STATUS_SAFEGUARD;
            if(gBattleMons[side].volatiles.leechSeed&&gBattleMons[side^1].hp)
            {
                u8 to=gBattleMons[side].volatiles.leechSeed-1;
                u16 d=min(b->hp,max(1,b->maxHP/8)),h;
                b->hp-=d;SetMonData(side?&gEnemyParty[gBattlerPartyIndexes[side]]:&gPlayerParty[gBattlerPartyIndexes[side]],MON_DATA_HP,&b->hp);
                h=min(gBattleMons[to].maxHP,gBattleMons[to].hp+d);gBattleMons[to].hp=h;
                SetMonData(to?&gEnemyParty[gBattlerPartyIndexes[to]]:&gPlayerParty[gBattlerPartyIndexes[to]],MON_DATA_HP,&h);
                ArenaFeedback_Impact(side,sArena.bodies[side].x/Q,sArena.bodies[side].y/Q,d,ARENA_FEEDBACK_DAMAGE);
                sArena.hudDirty=TRUE;
                if(!b->hp){sArena.resultTimer=12;sArena.lastAttacker=side^1;sArena.lastTarget=side;gArenaResultTelemetry.started=gMain.vblankCounter1;continue;}
            }
            if(sPerish[side]&&!--sPerish[side])
            {
                u16 zero=0;b->hp=0;SetMonData(side?&gEnemyParty[gBattlerPartyIndexes[side]]:&gPlayerParty[gBattlerPartyIndexes[side]],MON_DATA_HP,&zero);
                sArena.hudDirty=TRUE;sArena.resultTimer=12;sArena.lastAttacker=side^1;sArena.lastTarget=side;
                gArenaResultTelemetry.started=gMain.vblankCounter1;PlaySE(SE_M_PERISH_SONG);
                continue;
            }
        }
        if(sEndureLife[side])sEndureLife[side]--;
        // Mist: 5 native "turns", one per 300 active ticks.
        if((gSideStatuses[side]&SIDE_STATUS_MIST) && sArena.frame%300==0 && !--gSideTimers[side].mistTimer)
            gSideStatuses[side]&=~SIDE_STATUS_MIST;
    }
    for(side=0;side<2&&!sArena.resultTimer;side++)
    {
        struct BattlePokemon *mon=&gBattleMons[side];
        struct Pokemon *party=side?&gEnemyParty[gBattlerPartyIndexes[side]]:&gPlayerParty[gBattlerPartyIndexes[side]];
        u8 effect=GetItemHoldEffect(mon->item);
        u32 cure=0;
        if(!mon->hp)continue;
        if(effect==HOLD_EFFECT_LEFTOVERS)
        {
            if(mon->hp<mon->maxHP && ++sMattLeftoversClock[side]>=300)
            {
                sMattLeftoversClock[side]=0;
                mon->hp=min(mon->maxHP,mon->hp+max(1,mon->maxHP/16));
                SetMonData(party,MON_DATA_HP,&mon->hp);sArena.hudDirty=TRUE;
            }
            continue;
        }
        switch(effect)
        {
        case HOLD_EFFECT_CURE_PAR: cure=STATUS1_PARALYSIS; break;
        case HOLD_EFFECT_CURE_SLP: cure=STATUS1_SLEEP; break;
        case HOLD_EFFECT_CURE_PSN: cure=STATUS1_PSN_ANY|STATUS1_TOXIC_COUNTER; break;
        case HOLD_EFFECT_CURE_BRN: cure=STATUS1_BURN; break;
        case HOLD_EFFECT_CURE_FRZ: cure=STATUS1_FREEZE; break;
        case HOLD_EFFECT_CURE_STATUS: cure=STATUS1_ANY|STATUS1_TOXIC_COUNTER; break;
        }
        if(cure && (mon->status1&cure&STATUS1_ANY))
        {
            u16 none=ITEM_NONE;
            gBattleStruct->partyState[side][gBattlerPartyIndexes[side]].usedHeldItem=mon->item;
            mon->status1&=~cure;mon->item=ITEM_NONE;
            SetMonData(party,MON_DATA_STATUS,&mon->status1);
            SetMonData(party,MON_DATA_HELD_ITEM,&none);
            sArena.bodies[side].burnClock=0;sArena.hudDirty=TRUE;
            PlaySE(SE_M_MORNING_SUN);
        }
    }
}

static void TickBurn(void)
{
    u32 side;
    MattTickItems();
    // One native Gen III 1/8-max-HP burn pulse per 300 active gameplay ticks.
    // Pausing, capture animation and hitstop do not advance this clock.
    for(side=0;side<2&&!sArena.resultTimer;side++)
    {
        struct ArenaBody *body=&sArena.bodies[side];
        if((gBattleMons[side].status1&STATUS1_ANY) && gBattleMons[side].hp)
        {
            if(++body->burnClock==300)
            {
                u16 damage;
                // Gen III Shed Skin checks once per residual interval, not
                // every rendered frame. Persist the cure into the real party.
                if(gBattleMons[side].ability==ABILITY_SHED_SKIN && Random()%3==0)
                {
                    gBattleMons[side].status1=0;body->burnClock=0;
                    SetMonData(side?&gEnemyParty[gBattlerPartyIndexes[side]]:&gPlayerParty[gBattlerPartyIndexes[side]],
                               MON_DATA_STATUS,&gBattleMons[side].status1);
                    sArena.hudDirty=TRUE;
                    continue;
                }
                if(gBattleMons[side].status1&(STATUS1_SLEEP|STATUS1_FREEZE))
                {
                    u32 status=gBattleMons[side].status1;
                    u8 sleep=status&STATUS1_SLEEP;
                    if(sleep)
                    {
                        u8 step=gBattleMons[side].ability==ABILITY_EARLY_BIRD?2:1;
                        status=(status&~STATUS1_SLEEP)|(sleep>step?sleep-step:0);
                    }
                    else if(Random()%5==0)status&=~STATUS1_FREEZE;
                    gBattleMons[side].status1=status;
                    SetMonData(side?&gEnemyParty[gBattlerPartyIndexes[side]]:&gPlayerParty[gBattlerPartyIndexes[side]],MON_DATA_STATUS,&status);
                    body->burnClock=0;sArena.hudDirty=TRUE;continue;
                }
                if(!(gBattleMons[side].status1&(STATUS1_BURN|STATUS1_PSN_ANY)))
                {body->burnClock=0;continue;}
                damage=min(gBattleMons[side].hp,max(1,gBattleMons[side].maxHP/8));
                if(gBattleMons[side].status1&STATUS1_TOXIC_POISON)
                {
                    u32 status=gBattleMons[side].status1;
                    u8 turns=min(15,((status&STATUS1_TOXIC_COUNTER)>>8)+1);
                    status=(status&~STATUS1_TOXIC_COUNTER)|STATUS1_TOXIC_TURN(turns);
                    gBattleMons[side].status1=status;
                    SetMonData(side?&gEnemyParty[gBattlerPartyIndexes[side]]:&gPlayerParty[gBattlerPartyIndexes[side]],MON_DATA_STATUS,&status);
                    damage=min(gBattleMons[side].hp,max(1,gBattleMons[side].maxHP/16)*turns);
                }
                body->burnClock=0;gBattleMons[side].hp-=damage;
                SetMonData(side?&gEnemyParty[gBattlerPartyIndexes[side]]:&gPlayerParty[gBattlerPartyIndexes[side]],
                           MON_DATA_HP,&gBattleMons[side].hp);
                gArenaStatusTelemetry[side+2]++;sArena.hudDirty=TRUE;
                ArenaFeedback_Impact(side,body->x/Q,body->y/Q,damage,ARENA_FEEDBACK_DAMAGE);
                body->flash=5;
                if(!gBattleMons[side].hp)
                {
                    sArena.resultTimer=12;sArena.lastAttacker=side^1;sArena.lastTarget=side;
                    gArenaResultTelemetry.started=gMain.vblankCounter1;
                }
            }
        }
        else body->burnClock=0;
    }
}

static void CB2_Arena(void)
{
    u32 i;
    bool8 frozen = FALSE;
    u32 stamp=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228,phaseStamp=stamp,now;
    u32 gap=gMain.vblankCounter1-gArenaFrameTelemetry.lastVBlank;
    if(sSendout.active){gArenaFrameTelemetry.lastVBlank=gMain.vblankCounter1;StormDraw(40);SendoutTick();return;}
    gArenaFrameTelemetry.lastVBlank=gMain.vblankCounter1;
    gArenaFrameTelemetry.updates++;
#if MATT_CATCH_OR_FIGHT
    if (sMattPrompt == 0)
    {
        sMattPrompt = 2;
        if (!(gBattleTypeFlags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_SAFARI | BATTLE_TYPE_PYRAMID | BATTLE_TYPE_LINK)))
        { sMattPrompt = 1; sCapture.hudReady = FALSE; sArena.hudDirty = TRUE; }
    }
    if (sMattPrompt == 1)
    {
        if (JOY_NEW(DPAD_LEFT | DPAD_UP)) { sMattChoice = (sMattChoice + 2) % 3; sCapture.hudReady = FALSE; PlaySE(SE_SELECT); }
        if (JOY_NEW(DPAD_RIGHT | DPAD_DOWN)) { sMattChoice = (sMattChoice + 1) % 3; sCapture.hudReady = FALSE; PlaySE(SE_SELECT); }
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            if (JOY_NEW(B_BUTTON)) sMattChoice = 2;  // B = FIGHT
            sMattCatchMode = sMattChoice == 0 ? 1 : sMattChoice == 1 ? 2 : 0;
            sMattPrompt = 2; sCapture.hudReady = FALSE; sArena.hudDirty = TRUE;
            sArena.attackBuffer = sArena.dashBuffer = 0;
            PlaySE(SE_SELECT);
        }
        CaptureHud();
        return;
    }
#endif
    if(gap>gArenaFrameTelemetry.maxGap)gArenaFrameTelemetry.maxGap=gap;
    if(gap>1)gArenaFrameTelemetry.missedVBlanks+=gap-1;
#if MATT_AUTO_SWITCH
    if(!sArena.paused&&!(sArena.frame%15))
    {
        u8 slot;
        if(sMattSwitch>>2)sMattSwitch-=4;
        else if((slot=MattAutoSwitchPick())<PARTY_SIZE)
        {
            // Native party-switch path, with the slot pre-chosen (no menu).
            sMattSwitch=(sMattSwitch&3)+1+(32<<2);
            gArenaMenuRequest=0x10|slot;ArenaExit(FALSE);return;
        }
    }
#endif
    if (JOY_NEW(SELECT_BUTTON) && !ClashBusy() && !GuardBusy() && !SpecialBusy(0) && !SpecialBusy(1) && !sArena.resultTimer && sCapture.state==ARENA_CAPTURE_IDLE) { gArenaMenuRequest=2; ArenaExit(FALSE); return; }
    if (JOY_NEW(START_BUTTON) && !sArena.resultTimer && sCapture.state==ARENA_CAPTURE_IDLE)
    {
        sArena.paused ^= TRUE;
        if(sArena.paused){sPausePage=3;sPauseChoice=0;}
        DrawStage();
        sArena.hudDirty = TRUE;
    }
    if(!ClashBusy()&&!GuardBusy()&&!SpecialBusy(0)&&!SpecialBusy(1))CaptureInput();
    if(sArena.paused)
    {
        u16 keys=gMain.newKeys&DPAD_ANY;
        u8 slot=keys==DPAD_UP?0:keys==DPAD_RIGHT?1:keys==DPAD_DOWN?2:keys==DPAD_LEFT?3:MAX_MON_MOVES;
        if(sPausePage==3)
        {
            if(JOY_NEW(DPAD_UP|DPAD_DOWN))
            {sPauseChoice=(sPauseChoice+(JOY_NEW(DPAD_DOWN)?1:3))%4;sArena.hudDirty=TRUE;}
            if(JOY_NEW(A_BUTTON))
            {
                if(sPauseChoice<2 && !ClashBusy() && !GuardBusy() && !SpecialBusy(0) && !SpecialBusy(1))
                {gArenaMenuRequest=sPauseChoice==0?2:1;ArenaExit(FALSE);return;}
                if(sPauseChoice==2){sPausePage=0;sMoveMenuReady=FALSE;sArena.hudDirty=TRUE;}
                if(sPauseChoice==3){sArena.paused=FALSE;DrawStage();sArena.hudDirty=TRUE;}
            }
        }
        else if(JOY_NEW(L_BUTTON|R_BUTTON))
        {
            sPausePage=(sPausePage+(JOY_NEW(R_BUTTON)?1:2))%3;
            sMoveMenuReady=FALSE;sArena.hudDirty=TRUE;
        }
        if(JOY_NEW(B_BUTTON))
        {if(sPausePage==3)sArena.paused=FALSE;else sPausePage=3;DrawStage();sArena.hudDirty=TRUE;}
        if(sPausePage>0 && sPausePage<3 && JOY_NEW(A_BUTTON) && !ClashBusy() && !GuardBusy() && !SpecialBusy(0) && !SpecialBusy(1))
        {gArenaMenuRequest=sPausePage;ArenaExit(FALSE);return;}
        if(!sPausePage && slot<MAX_MON_MOVES && SupportedMove(ArenaMoveId(0,slot)))
        {
            sArena.bodies[0].moveSlot=slot;
            sArena.hudDirty=TRUE;
        }
        if(!sPausePage && JOY_NEW(A_BUTTON) && NeedsStruggle(0))
        {sArena.bodies[0].moveSlot=ARENA_STRUGGLE_SLOT;sArena.hudDirty=TRUE;}
        // Menu input never queues an attack or dodge for the resumed battle.
        sArena.attackBuffer=sArena.dashBuffer=0;
    }
    if (!sArena.paused)
    {
        // Exhausting every adapted move must not leave a powerless actor
        // trapped in the arena. Finish committed actions, then let Emerald
        // offer remaining native moves or Struggle with its real PP rules.
        if (!sArena.resultTimer && sCapture.state==ARENA_CAPTURE_IDLE
            && !ClashBusy() && !GuardBusy() && !SpecialBusy(0) && !SpecialBusy(1)
            && !PsychicBusy(0) && !PsychicBusy(1)
            && !sArena.bodies[0].shotTimer && !sArena.bodies[1].shotTimer
            && !sArena.bodies[0].actionLife && !sArena.bodies[1].actionLife
            && (FirstMove(0,TRUE)==ARENA_NO_MOVE || FirstMove(1,TRUE)==ARENA_NO_MOVE))
        {
            bool8 flying=FALSE;
            for(i=0;i<SHOTS_COUNT;i++)if(sArena.shots[i].life)flying=TRUE;
            if(!flying){ArenaExit(FALSE);return;}
        }
        sArena.frame++;
        for(i=0;i<2;i++)
            if(!sArena.bodies[i].shotTimer && !sArena.bodies[i].actionLife
                && !ArenaMovePP(i,sArena.bodies[i].moveSlot))
                sArena.bodies[i].moveSlot=FirstMove(i,TRUE);
        gRealtimeArenaTelemetry.frames++;
        if (sArena.resultTimer)
        {
            SpecialTick();GuardTick();
            for(i=0;i<2;i++)if(sArena.bodies[i].actionLife)
            {
                sArena.bodies[i].actionAge++;sArena.bodies[i].actionLife--;
                sFlameReach[i]=min(sFlameReach[i],sArena.bodies[i].actionLife*11);
            }
            if (--sArena.resultTimer == 0) { ArenaExit(TRUE); return; }
        }
        else
        {
            BufferPlayerActions();
            if(sSeShake)sSeShake--; // drawn by the warp/BG1 pass
            if(ClashBusy()){ClashTick();frozen=TRUE;}
            else if(TossActive()){SpecialTick();TickShots();frozen=TRUE;}
            else if(CaptureLocked() || ((sCapture.state==ARENA_CAPTURE_AIM || sCapture.state==ARENA_CAPTURE_THROW) && sArena.frame%3)) frozen=TRUE;
            else if (sArena.hitstop) { sArena.hitstop--; frozen = TRUE; }
            else
            {
                DecoyTick();
                CoverTick();
                BarrierTick();
                GuardTick();
                SpecialTick();
                TickPhysics();
                now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
                gArenaFrameTelemetry.scanlines[0]=now-phaseStamp;phaseStamp=now;
                if(sCapture.state!=ARENA_CAPTURE_AIM)TickPlayer();
                TickEnemy();
                now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
                gArenaFrameTelemetry.scanlines[1]=now-phaseStamp;phaseStamp=now;
                TickPendingShots(); ClashDetect();
                if(!ClashBusy()){TickActions(); TickShots(); TickPsychic();}
                if(!sArena.resultTimer){ElementsTick();StormTick();}
                TickBurn();
                for (i = 0; i < 2 && !sArena.resultTimer; i++)
                {
                    struct BattlePokemon *mon = &gBattleMons[i];
                    struct ArenaBody *body = &sArena.bodies[i];
                    if (++body->weatherClock >= 300)
                    {
                        u16 damage = 0;
                        body->weatherClock = 0;
                        if (gBattleMons[0].ability == ABILITY_AIR_LOCK || gBattleMons[1].ability == ABILITY_AIR_LOCK
                            || gBattleMons[0].ability == ABILITY_CLOUD_NINE || gBattleMons[1].ability == ABILITY_CLOUD_NINE) continue;
                        if ((gBattleWeather & B_WEATHER_SANDSTORM) && !IS_BATTLER_OF_TYPE(i, TYPE_ROCK)
                            && !IS_BATTLER_OF_TYPE(i, TYPE_GROUND)
                            && !IS_BATTLER_OF_TYPE(i, TYPE_STEEL) && mon->ability != ABILITY_SAND_VEIL)
                            damage = max(1, mon->maxHP / 16);
                        if ((gBattleWeather & B_WEATHER_HAIL) && !IS_BATTLER_OF_TYPE(i, TYPE_ICE))
                            damage = max(1, mon->maxHP / 16);
                        if ((gBattleWeather & B_WEATHER_RAIN) && mon->ability == ABILITY_RAIN_DISH)
                            mon->hp = min(mon->maxHP, mon->hp + max(1, mon->maxHP / 16));
                        damage = min(damage, mon->hp);
                        mon->hp -= damage;
                        SetMonData(i ? &gEnemyParty[gBattlerPartyIndexes[i]] : &gPlayerParty[gBattlerPartyIndexes[i]], MON_DATA_HP, &mon->hp);
                        if (damage) ArenaFeedback_Impact(i, body->x/Q, body->y/Q, damage, ARENA_FEEDBACK_DAMAGE);
                        sArena.hudDirty = TRUE;
                        if (!mon->hp)
                        {
                            sArena.resultTimer=12; sArena.lastAttacker=i^1; sArena.lastTarget=i;
                            gArenaResultTelemetry.started=gMain.vblankCounter1;
                        }
                    }
                }
                // Oran/Sitrus use their original Gen III HP thresholds and
                // values. Consume once, persist to the actual party, and never
                // resurrect a fainted Pokemon. No turn menu is needed.
                for (i = 0; i < 2; i++)
                {
                    struct BattlePokemon *mon = &gBattleMons[i];
                    if (mon->hp && mon->hp <= mon->maxHP / 2
                        && GetItemHoldEffect(mon->item) == HOLD_EFFECT_RESTORE_HP)
                    {
                        u16 item = mon->item;
                        struct Pokemon *party = i ? &gEnemyParty[gBattlerPartyIndexes[i]] : &gPlayerParty[gBattlerPartyIndexes[i]];
                        mon->hp = min(mon->maxHP, mon->hp + GetItemHoldEffectParam(item));
                        gBattleStruct->partyState[i][gBattlerPartyIndexes[i]].usedHeldItem = item;
                        mon->item = ITEM_NONE;
                        SetMonData(party, MON_DATA_HELD_ITEM, &mon->item);
                        SetMonData(party, MON_DATA_HP, &mon->hp);
                        sArena.hudDirty = TRUE;
                        PlaySE(SE_M_MORNING_SUN);
                    }
                }
            }
        }
    }
    CaptureTick();
    if(!sArena.active)return;
    frozen |= CaptureLocked();
    frozen |= sArena.hitstop != 0;
    now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
    gArenaFrameTelemetry.scanlines[2]=now-phaseStamp;phaseStamp=now;
    for (i = 0; i < 2; i++)
    {
        struct ArenaBody *body = &sArena.bodies[i];
        const struct ArenaMoveProfile *profile=ArenaMoves_Get(ArenaMoveId(i,body->shotTimer?body->shotSlot:body->moveSlot));
        struct Sprite *sprite = &gSprites[body->sprite];
        sprite->x = body->x / Q;
        sprite->y = body->y / Q;
        gSprites[body->shadow].x = sprite->x;
        gSprites[body->shadow].y = body->y / Q + 8;
        gSprites[body->shadow].invisible = gBattleMons[i].hp == 0;
        // A ghost inside a wall is drawn semi-transparent (the HUD's blend
        // settings already list OBJ and the stage) with a pale spectral tint.
        gArenaGhostTelemetry.phasing[i] = Phasing(i);
        sprite->oam.objMode = gArenaGhostTelemetry.phasing[i] ? ST_OAM_OBJ_BLEND : ST_OAM_OBJ_NORMAL;
        if (!sArena.paused && !frozen)
            sprite->x2 = i && sArena.aiState == AI_AIM ? ((sArena.frame & 2) ? 1 : -1) : 0;
        if (body->art)
        {
            u8 animation = PsychicBusy(i) ? ARENA_ANIM_SPECIAL : body->shotTimer ? profile->animation : body->moving ? ARENA_ANIM_WALK : ARENA_ANIM_IDLE;
            u8 direction = body->shotTimer || PsychicBusy(i) ? body->shotFacing : body->facing;
            const struct ArenaSpriteAnimation *anim;
            u8 frame;
            u16 tick=body->animClock/Q;
            bool8 holdPose=FALSE;
            if (!RollBallVisible(i)) RollBallHide(i);
            if(TossActive())holdPose=TossPose(i,&animation,&direction);
            anim=&body->art->animations[animation];
            if (body->animation != animation)
            {
                body->animation = animation; body->animClock = 0; body->drawnFrame = 255;
            }
            if(body->shotTimer)
                tick=body->shotElapsed<profile->windup?body->shotElapsed*anim->hitTick/profile->windup:
                    anim->hitTick+(body->shotElapsed-profile->windup)*(anim->totalTicks-anim->hitTick-1)/(profile->active+8);
            if(PsychicBusy(i))tick=anim->hitTick;
            if(holdPose)tick=anim->hitTick;
            sprite->y2=PsychicBusy(i)?-3-Sin(sPsychic[i].age*4,2):0;
            frame = ArenaSprites_Frame(anim,tick);
            sprite->y += 8-body->art->ground[animation*8+direction]*MonVisualScale(i)/256;
            if (!RollBallVisible(i) && (body->drawnFrame != frame || body->drawnDirection != direction))
            {
                ArenaSprites_Decode(anim, direction, frame, sMonFrameTiles[i]);
                ArenaRender_Copy(sMonFrameTiles[i],
                    (u8 *)OBJ_VRAM0 + GetSpriteTileStartByTag(MON_TAG + i * 2) * 32, 2048);
                body->drawnFrame = frame; body->drawnDirection = direction;
                gArenaSpriteTelemetry.uploads[i]++;
            }
            if (!sArena.paused && !sArena.resultTimer && !frozen)
                body->animClock += animation == ARENA_ANIM_WALK ? Clamp(Speed(i) * Q / 220, 128, 512) : Q;
            if (RollBallVisible(i))
            {
                // Curled into a ball: its own frames stand in for the pose.
                RollBallShow(i, sprite);
                if (!sArena.paused && !frozen) RollBallTick(i);
            }
            else if (Rolling(i) && !TossActive())
            {
                // Roll like a ball along the heading, seen from the arena's
                // front-elevated camera: one rotation about the ground axis
                // perpendicular to the travel direction, projected to the
                // screen. Sideways travel turns like a wheel with the top
                // moving toward the heading; up/down travel tumbles forward;
                // diagonals blend the two. Spin-up in place, then flat out.
                s32 ux = body->attackX, uy = body->attackY, ct, st, v, m00, m01, m10, m11, spin, cross, det;
                if (!body->actionLife && body->shotElapsed && !sArena.paused && !frozen) RollBallTick(i);
                if (!sArena.paused && !frozen)
                {
                    u16 rate = body->actionLife ? ROLLOUT_SPIN
                        : ROLLOUT_SPIN * min(body->shotElapsed, profile->windup) / max(1, profile->windup);
                    body->rollAngle += rate;
                    if (!body->actionLife && !(sArena.frame % 6)) ArenaFeedback_Dust(body->x / Q, body->y / Q, FALSE);
                }
                ct = Cos(body->rollAngle >> 8, 256); st = Sin(body->rollAngle >> 8, 256);
                if (ct > -40 && ct < 40) ct = ct < 0 ? -40 : 40;
                v = 256 - ct;
                spin = ux * ROLLOUT_CAM_COS / 256 * st / 256;
                cross = v * ux / 256 * uy / 256 * ROLLOUT_CAM_SIN / 256;
                m00 = ct + v * uy * uy / 65536;
                m01 = -spin - cross;
                m10 = spin - cross;
                m11 = ct + v * (ux * ux / 256) * (ROLLOUT_CAM_SIN * ROLLOUT_CAM_SIN / 256) / 65536;
                det = (m00 * m11 - m01 * m10) / 256;
                if (det > -16 && det < 16) det = det < 0 ? -16 : 16;
                // The hardware wants the inverse map (screen to texture).
                SetOamMatrix(sprite->oam.matrixNum, m11 * 256 / det, -m01 * 256 / det, -m10 * 256 / det, m00 * 256 / det);
                // Keep the pose's centre where it stands: the hardware turns
                // about the canvas centre, so shift by what that moves it.
                {
                    s32 px = sBall[i].pivotX, py = sBall[i].pivotY;
                    sprite->x += px - ((m00 * px + m01 * py) >> 8);
                    sprite->y += py - ((m10 * px + m11 * py) >> 8);
                }
            }
            else if (body->rollAngle)
            {
                body->rollAngle = 0;
                SetOamMatrix(sprite->oam.matrixNum, 0x100, 0, 0, 0x100);
            }
            else if (body->flat && !TossActive())
            {
                // Flattened under a Rollout: wide and low, pressed onto the
                // ground beneath the ball, then springing back up.
                s32 f = body->flat >= ROLLOUT_FLAT_SPRING ? 256 : body->flat * 256 / ROLLOUT_FLAT_SPRING;
                SetOamMatrix(sprite->oam.matrixNum, 65536 / (256 + 72 * f / 256), 0, 0, 65536 / (256 - 150 * f / 256));
                sprite->y += 8 * f / 256;
                if (!sArena.paused && !frozen && !--body->flat)
                    SetOamMatrix(sprite->oam.matrixNum, 0x100, 0, 0, 0x100);
            }
            sprite->subpriority = body->flat ? 1 : 0;
            gArenaSpriteTelemetry.animation[i] = animation;
            gArenaSpriteTelemetry.frame[i] = frame;
            gArenaSpriteTelemetry.direction[i] = direction;
        }
        else
        {
            if (!sArena.paused && !frozen)
                sprite->y2 = body->moving && (sArena.frame & 8) ? -1 : 0;
            sprite->oam.tileNum = GetSpriteTileStartByTag(MON_TAG + i * 2 + (body->facing < 3 || body->facing > 5));
            SetOamMatrix(sprite->oam.matrixNum, body->facing > 0 && body->facing < 4 ? -0x200 : 0x200, 0, 0, 0x200);
        }
        // A short palette flash keeps the original pose visible, unlike
        // alternating invisible frames. Dust now communicates the dash.
        sprite->invisible = sArena.resultTimer && !gBattleMons[i].hp;
        gSprites[body->shadow].invisible = sprite->invisible;
        if (body->flash) BlendPalette(OBJ_PLTT_ID(i), 16, body->flash + 3, RGB_WHITE);
        else BlendPalette(OBJ_PLTT_ID(i), 16, gArenaGhostTelemetry.phasing[i] ? 7 : 0, RGB(22, 26, 31));
        if (body->flash && !sArena.paused && !frozen) body->flash--;
        gRealtimeArenaTelemetry.x[i] = sprite->x;
        gRealtimeArenaTelemetry.y[i] = body->y / Q;
        gRealtimeArenaTelemetry.speed[i] = Speed(i);
        gArenaAiTelemetry.facing[i] = body->facing;
        gArenaCombatTelemetry.cooldown[i] = body->cooldown;
        gArenaCombatTelemetry.windup[i] = body->shotTimer;
        gArenaCombatTelemetry.dashCooldown[i] = body->dashCooldown;
        gArenaCombatTelemetry.pendingSlot[i] = body->shotTimer ? body->shotSlot : 255;
        gArenaMoveTelemetry.kind[i]=profile->kind;
        gArenaMoveTelemetry.phase[i]=body->actionLife?2:body->shotTimer&&body->shotElapsed<=profile->windup?1:body->cooldown?3:0;
        gArenaMoveTelemetry.range[i]=profile->range;
        gArenaMoveTelemetry.active[i]=body->actionLife;
        gArenaMoveTelemetry.actionMove[i]=profile->move;
        ArenaMoveFx_Action(i,profile,body->x/Q,body->y/Q,body->shotFacing,body->actionAge,
            body->actionLife!=0&&profile->move!=MOVE_PROTECT&&profile->move!=MOVE_DETECT&&profile->move!=MOVE_DOUBLE_TEAM&&profile->move!=MOVE_TELEPORT&&profile->move!=MOVE_SEISMIC_TOSS&&profile->move!=MOVE_DIG&&profile->move!=MOVE_FLY&&profile->move!=MOVE_SUBSTITUTE&&profile->move!=MOVE_SMOKESCREEN,sArena.paused);
        ArenaMoveFx_Flame(i,profile->move,body->x/Q,body->y/Q-8,body->shotFacing,ClashBusy()?sFlameClash.age:body->actionAge,sFlameReach[i],
            ArenaMoves_Beam(profile->move)&&body->actionLife&&!sArena.paused&&gBattleMons[i].hp,ClashBusy());
    }
    {
        u8 warmth=0;
        for(i=0;i<2;i++)
        {
            struct ArenaBody *b=&sArena.bodies[i];
            if(b->actionLife&&ArenaMoveId(i,b->shotSlot)==MOVE_FLAMETHROWER)
                warmth=max(warmth,b->actionAge<8?4:b->actionLife<10?1:2);
        }
        // Brief warm light on the scenery only, never the HUD or HP palettes.
        BlendPalette(16,240,sArena.paused?0:warmth,RGB(31,16,4));
    }
    now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
    gArenaFrameTelemetry.scanlines[3]=now-phaseStamp;phaseStamp=now;
    gArenaCombatTelemetry.aimBlocked = !ArenaNav_LineClear(sArena.bodies[0].x/Q,
        sArena.bodies[0].y/Q, DecoyAim(1,FALSE)/Q, DecoyAim(1,TRUE)/Q, 2);
    gArenaCombatTelemetry.aimVisible = !sArena.paused && !sArena.resultTimer
        && sCapture.state==ARENA_CAPTURE_IDLE
        && !(sArena.bodies[0].shotTimer?sArena.bodies[0].manualAim:JOY_HELD(DPAD_ANY))
        && (JOY_HELD(A_BUTTON) || sArena.bodies[0].shotTimer);
    gSprites[sArena.aimSprite].x = DecoyAim(1,FALSE) / Q;
    gSprites[sArena.aimSprite].y = DecoyAim(1,TRUE) / Q;
    gSprites[sArena.aimSprite].invisible = !gArenaCombatTelemetry.aimVisible || ClashBusy();
    gSprites[sArena.aimSprite].oam.paletteNum = IndexOfSpritePaletteTag(SHOT_TAG
        + (gArenaCombatTelemetry.aimBlocked ? 1 : sArena.bodies[0].cooldown ? 2 : 0));
    gSprites[sArena.cueSprite].x = sArena.bodies[1].x / Q;
    gSprites[sArena.cueSprite].y = sArena.bodies[1].y / Q + 14;
    gSprites[sArena.cueSprite].invisible = sArena.aiState != AI_AIM || sArena.resultTimer || ClashBusy();
    DecoyDraw();
    SpecialDraw();
    MonVisualScaleApply();
    CoverDraw();
    BarrierDraw();
    GuardDraw();
    ClashDraw();
    CaptureDraw();
    gArenaAiTelemetry.state = sArena.aiState;
    gArenaAiTelemetry.goalX = sArena.goal.x; gArenaAiTelemetry.goalY = sArena.goal.y;
    gArenaAiTelemetry.waypointX = sArena.waypoint.x; gArenaAiTelemetry.waypointY = sArena.waypoint.y;
    gRealtimeArenaTelemetry.paused = sArena.paused;
    gRealtimeArenaTelemetry.selectedMove = sArena.bodies[0].moveSlot;
    ArenaFeedback_Update(sArena.paused, frozen && !CaptureLocked() && !ClashBusy());
    ArenaTerrain_Draw(sArena.paused,frozen);
    ElementsDraw();
    {
        u32 used=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228-stamp;
        StormDraw(used<90?90-used:0);
    }
    DrawPsychicAuras();
    ArenaPsychicFx_Draw(sArena.paused,frozen);
    now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
    gArenaRenderTelemetry[0]=now-phaseStamp;
    gArenaFeedbackTelemetry.hitstop = sArena.hitstop;
    gArenaFeedbackTelemetry.attackBuffer = sArena.attackBuffer;
    gArenaFeedbackTelemetry.dashBuffer = sArena.dashBuffer;
    if (sArena.hudDirty || sHudRecovery!=RecoveryPips() || JOY_NEW(START_BUTTON | L_BUTTON | R_BUTTON)) DrawHud();
    gArenaRenderTelemetry[1]=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228-now;
    now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
    AnimateSprites();
    BuildOamBuffer();
    // BG0's paused picker must cover actors and attack sprites. Change only
    // this frame's OAM, so normal priorities return automatically on resume.
    for(i=0;i<128;i++)
        if(gMain.oamBuffer[i].priority==0)gMain.oamBuffer[i].priority=1;
    TossHideOam();
    ArenaRender_Ready();
    gArenaRenderTelemetry[2]=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228-now;
    now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
    UpdatePaletteFade();
    gArenaRenderTelemetry[3]=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228-now;
    for(i=0;i<4;i++)if(gArenaRenderTelemetry[i]>gArenaRenderTelemetry[4+i])gArenaRenderTelemetry[4+i]=gArenaRenderTelemetry[i];
    now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
    gArenaFrameTelemetry.scanlines[4]=now-phaseStamp;
    gArenaFrameTelemetry.scanlines[5]=now-stamp;
    for(i=0;i<6;i++)if(gArenaFrameTelemetry.scanlines[i]>gArenaFrameTelemetry.peak[i])
        gArenaFrameTelemetry.peak[i]=gArenaFrameTelemetry.scanlines[i];
}

static void ArenaExit(bool8 fainted)
{
    u32 i;
#if MATT_AUTO_SWITCH
    if(fainted)sMattSwitch&=~3; // a faint refunds the per-battle switch budget
#endif
    for(i=0;i<2;i++)
    {
        sArenaResident[i].alive=gBattleMons[i].hp!=0;
        sArenaResident[i].party=gBattlerPartyIndexes[i];
        sArenaResident[i].personality=gBattleMons[i].personality;
        sArenaResident[i].x=sArena.bodies[i].x;
        sArenaResident[i].y=sArena.bodies[i].y;
        sArenaResident[i].facing=sArena.bodies[i].facing;
        sArenaResident[i].cooldown=max(sArena.bodies[i].cooldown,
            sArena.bodies[i].shotTimer+sArena.bodies[i].actionLife);
        sArenaResident[i].dashCooldown=sArena.bodies[i].dashCooldown;
        sArenaResident[i].burnClock=sArena.bodies[i].burnClock;
        sArenaResident[i].moveSlot=sArena.bodies[i].moveSlot;
        sArenaResident[i].statusActions=sArena.bodies[i].statusActions;
    }
    CaptureReset();
    DecoyHide();
    CoverHide();
    BarrierHide();
    SpecialHide();
    // Finish the ORIGINAL battle scripts behind the arena image. No classic
    // scene reconstruction just to say "fainted" and animate an EXP bar.
    // Interactive choices explicitly restore the native UI if needed.
    RollBallExit();
    if(fainted)
    {
        sArena.active=FALSE;sArena.classic=FALSE;
        gRealtimeArenaTelemetry.active=FALSE;
        gRealtimeArenaTelemetry.exits++;
        gRealtimeArenaTelemetry.lastExitFainted=TRUE;
        gRealtimeArenaRestoringFaint=TRUE;
        gRealtimeArenaQuietResult=TRUE;
        gBattlerAttacker=sArena.lastAttacker;gBattlerTarget=sArena.lastTarget;
        ArenaFeedback_Destroy();
        for(i=0;i<2;i++)
        {
            ArenaMoveFx_Action(i,NULL,0,0,0,0,FALSE,TRUE);
            ArenaMoveFx_Flame(i,MOVE_FLAMETHROWER,0,0,0,0,0,FALSE,FALSE);
        }
        gSprites[sArena.aimSprite].invisible=TRUE;
        gSprites[sArena.cueSprite].invisible=TRUE;
        BuildOamBuffer();ArenaRender_Ready();
        gMain.callback1=sArena.savedCB1;
        RealtimeArena_ResumeBattle(TRUE);
        SetMainCallback2(BattleMainCB2);
        return;
    }
    SetVBlankCallback(NULL);
    ArenaFeedback_Destroy();
    for (i = 0; i < 2; i++)
    {
        struct Sprite *sprite = &gSprites[sArena.bodies[i].sprite];
        FreeOamMatrix(sprite->oam.matrixNum);
        DestroySprite(sprite);
        FreeSpriteTilesByTag(MON_TAG + i * 2);
        FreeSpriteTilesByTag(MON_TAG + i * 2 + 1);
    }
    FreeAllWindowBuffers();
    sArena.active = FALSE;
    // SELECT opens the original party selector in trainer arenas, then
    // returns here after the switch. It never enables a turn-based attack.
    sArena.classic = FALSE;
    gRealtimeArenaTelemetry.active = FALSE;
    gRealtimeArenaTelemetry.exits++;
    gRealtimeArenaTelemetry.lastExitFainted = FALSE;
    gRealtimeArenaRestoringFaint = FALSE;
    gMain.callback1 = sArena.savedCB1;
    gRealtimeArenaQuietIntro = TRUE;
    RealtimeArena_ResumeBattle(FALSE);
    ReshowBattleScreenAfterMenu();
}

bool8 RealtimeArena_RequestDemo(void)
{
    // A new, disposable save only. Never reset an existing player's progress.
    if (gSaveFileStatus != SAVE_STATUS_EMPTY) return FALSE;
    sDemoRequested = TRUE;
    return TRUE;
}

bool8 RealtimeArena_SetupDemo(void)
{
#if !ARENA_LAB
    static const struct {u16 species;u8 level;} team[] = {
        {SPECIES_CHARIZARD,36}, {SPECIES_BLASTOISE,36}, {SPECIES_EEVEE,25},
        {SPECIES_DRAGONITE,30}, {SPECIES_SCIZOR,15}, {SPECIES_BLAZIKEN,20}
    };
    static const struct {u16 species;u8 level;} box[] = {
        {SPECIES_TREECKO,11}, {SPECIES_POOCHYENA,12}, {SPECIES_BULBASAUR,12},
        {SPECIES_SQUIRTLE,15}, {SPECIES_GROVYLE,16}, {SPECIES_SCEPTILE,43}
    };
    u32 i;
#endif
    if (!sDemoRequested) return FALSE;
    sDemoRequested = FALSE;
    StringCopy(gSaveBlock2Ptr->playerName, sDemoName);
    gSaveBlock2Ptr->playerGender = MALE;
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_OLDALE_TOWN);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_OLDALE_TOWN);
    gSaveBlock1Ptr->location.warpId = -1;
    gSaveBlock1Ptr->location.x = 10;
    gSaveBlock1Ptr->location.y = 17;
    gSaveBlock1Ptr->pos.x = 10;
    gSaveBlock1Ptr->pos.y = 17;
    SetLastHealLocationWarp(HEAL_LOCATION_OLDALE_TOWN);
    FlagSet(FLAG_SYS_POKEMON_GET);
    FlagSet(FLAG_SYS_POKEDEX_GET);
    FlagSet(FLAG_ADVENTURE_STARTED);
    FlagSet(FLAG_SYS_B_DASH);
    AddBagItem(ITEM_POKE_BALL,20);
    // Johto: the stock New Bark intro flags are left to the native scripts.
    ArenaJohto_DemoIntroFlags();
#if ARENA_LAB
    CreateMonLegacy(&gPlayerParty[0], SPECIES_TREECKO, 6, 20, TRUE, 0, OT_ID_PLAYER_ID, 0);
    CreateMonLegacy(&gPlayerParty[1], SPECIES_MUDKIP, 5, 20, TRUE, 0, OT_ID_PLAYER_ID, 0);
    gPlayerPartyCount = 2;
#else
    // A native NEW GAME preset, never an imported save or a fixture mailbox.
    // RequestDemo already refuses when any existing save is present.
    FlagSet(FLAG_ARENA_PRACTICE);
    gPlayerPartyCount = ARRAY_COUNT(team);
    for (i = 0; i < ARRAY_COUNT(team); i++)
        CreateMonLegacy(&gPlayerParty[i], team[i].species, team[i].level,
                  20, TRUE, i * 2, OT_ID_PLAYER_ID, 0);
    for (i = 0; i < ARRAY_COUNT(box); i++)
        CreateBoxMonAt(0, i, box[i].species, box[i].level,
                       20, TRUE, i * 2, OT_ID_PLAYER_ID, 0);
#endif
    return TRUE;
}

void RealtimeArena_DemoFieldCallback(void)
{
    FieldCB_WarpExitFadeFromBlack();
}
