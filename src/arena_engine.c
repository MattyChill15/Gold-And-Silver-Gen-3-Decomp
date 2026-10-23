// Arena <-> pokeemerald-expansion battle engine adapter (Johto port).
// Replaces the Gen 3 scratch-script adapters in native-engine.patch with the
// expansion's own damage, accuracy and status-eligibility functions.
#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "battle_main.h"
#include "battle_script_commands.h"
#include "move.h"
#include "pokemon.h"
#include "random.h"
#include "pokedex.h"
#include "battle_controllers.h"
#include "realtime_arena.h"
#include "constants/abilities.h"
#include "constants/moves.h"
#include "constants/battle_move_effects.h"
#include "constants/hold_effects.h"

extern u8 gArenaRolloutStage;

static void NoteMove(u8 attacker, u16 move)
{
    if (attacker == 0) gBattleResults.lastUsedMovePlayer = move;
    else gBattleResults.lastUsedMoveOpponent = move;
}

static void SetResult(u32 flags)
{
    gBattleStruct->moveResultFlags[0] = gBattleStruct->moveResultFlags[1] = flags;
}

static struct Pokemon *PartyMon(u8 battler)
{
    return battler ? &gEnemyParty[gBattlerPartyIndexes[battler]] : &gPlayerParty[gBattlerPartyIndexes[battler]];
}

static bool8 RollAccuracy(u8 attacker, u8 target, u16 move)
{
    u32 acc;
    if (GetMoveAccuracy(move) == 0) return TRUE;
    acc = GetTotalAccuracy(attacker, target, move, GetBattlerAbility(attacker), GetBattlerAbility(target),
                           GetBattlerHoldEffect(attacker), GetBattlerHoldEffect(target));
    return (Random() % 100) < acc;
}

s32 RealtimeArena_ResolveDamage(u8 attacker, u8 target, u16 move)
{
    struct DamageContext ctx = {0};
    s32 dmg, stage;
    u32 flags = 0;
    gBattlerAttacker = attacker;
    gBattlerTarget = target;
    gCurrentMove = move;
    NoteMove(attacker, move);
    if (!RollAccuracy(attacker, target, move))
    {
        SetResult(MOVE_RESULT_MISSED);
        return 0;
    }
    ctx.battlerAtk = attacker;
    ctx.battlerDef = target;
    ctx.move = ctx.chosenMove = move;
    ctx.moveType = GetBattleMoveType(move);
    ctx.abilities[attacker] = GetBattlerAbility(attacker);
    ctx.abilities[target] = GetBattlerAbility(target);
    ctx.holdEffects[attacker] = GetBattlerHoldEffect(attacker);
    ctx.holdEffects[target] = GetBattlerHoldEffect(target);
    ctx.weather = gBattleWeather;
    ctx.fieldStatuses = gFieldStatuses;
    ctx.randomFactor = TRUE;
    if (move == MOVE_ROLLOUT)
    {
        u32 p = GetMovePower(move) << min(gArenaRolloutStage, 3);
        if (gBattleMons[attacker].volatiles.defenseCurl) p *= 2;
        ctx.fixedBasePower = min(p, 255);
    }
    // Absorbing abilities (Flash Fire, Water Absorb, Volt Absorb...) nullify the hit.
    if (CanAbilityAbsorbMove(&ctx))
    {
        if (ctx.abilities[target] == ABILITY_WATER_ABSORB || ctx.abilities[target] == ABILITY_VOLT_ABSORB)
        {
            u16 hp = min(gBattleMons[target].maxHP, gBattleMons[target].hp + max(1, gBattleMons[target].maxHP / 4));
            gBattleMons[target].hp = hp;
            SetMonData(PartyMon(target), MON_DATA_HP, &hp);
        }
        SetResult(MOVE_RESULT_DOESNT_AFFECT_FOE);
        return 0;
    }
    ctx.typeEffectivenessModifier = CalcTypeEffectivenessMultiplier(&ctx);
    if (ctx.typeEffectivenessModifier == UQ_4_12(0.0))
    {
        SetResult(MOVE_RESULT_DOESNT_AFFECT_FOE);
        return 0;
    }
    stage = CalcCritChanceStage(&ctx);
    ctx.isCrit = stage < 0 ? FALSE : stage >= 3 ? TRUE : (Random() % (stage == 0 ? 24 : stage == 1 ? 8 : 2)) == 0;
    dmg = CalculateMoveDamage(&ctx);
    if (ctx.typeEffectivenessModifier > UQ_4_12(1.0)) flags |= MOVE_RESULT_SUPER_EFFECTIVE;
    else if (ctx.typeEffectivenessModifier < UQ_4_12(1.0)) flags |= MOVE_RESULT_NOT_VERY_EFFECTIVE;
    SetResult(flags);
    return dmg;
}

bool8 RealtimeArena_InflictStatus(u8 attacker, u8 target, u8 effect, bool8 ability)
{
    u32 status;
    if (gBattleMons[target].status1 & STATUS1_ANY) return FALSE;
    switch (effect)
    {
    case MOVE_EFFECT_BURN: status = STATUS1_BURN; break;
    case MOVE_EFFECT_POISON: status = STATUS1_POISON; break;
    case MOVE_EFFECT_TOXIC: status = STATUS1_TOXIC_POISON; break;
    case MOVE_EFFECT_PARALYSIS: status = STATUS1_PARALYSIS; break;
    case MOVE_EFFECT_FREEZE: status = STATUS1_FREEZE; break;
    case MOVE_EFFECT_SLEEP: status = STATUS1_SLEEP_TURN((Random() & 3) + 2); break;
    default: return FALSE;
    }
    if (!CanSetNonVolatileStatus(attacker, target, GetBattlerAbility(attacker), GetBattlerAbility(target), effect, CHECK_TRIGGER))
        return FALSE;
    gBattleMons[target].status1 |= status;
    SetMonData(PartyMon(target), MON_DATA_STATUS, &gBattleMons[target].status1);
    if (!ability && gBattleMons[target].ability == ABILITY_SYNCHRONIZE && !gBattleMons[attacker].status1
        && effect != MOVE_EFFECT_SLEEP && effect != MOVE_EFFECT_FREEZE)
        RealtimeArena_InflictStatus(target, attacker, effect, TRUE);
    return TRUE;
}

static bool8 DropStat(u8 battler, u8 stat, s8 delta, bool8 byFoe)
{
    s8 before = gBattleMons[battler].statStages[stat];
    if (byFoe && delta < 0)
    {
        u16 ab = gBattleMons[battler].ability;
        if (ab == ABILITY_CLEAR_BODY || ab == ABILITY_WHITE_SMOKE || ab == ABILITY_FULL_METAL_BODY) return FALSE;
        if (ab == ABILITY_KEEN_EYE && stat == STAT_ACC) return FALSE;
        if (ab == ABILITY_HYPER_CUTTER && stat == STAT_ATK) return FALSE;
        if (gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_MIST) return FALSE;
    }
    gBattleMons[battler].statStages[stat] = max(MIN_STAT_STAGE, min(MAX_STAT_STAGE, before + delta));
    return gBattleMons[battler].statStages[stat] != before;
}

u8 RealtimeArena_ResolveSecondary(u8 attacker, u8 target, u16 move)
{
    u8 result = 0;
    u32 chance = 0, i;
    enum MoveEffect secondary = 0;
    u8 stat = move == MOVE_AURORA_BEAM ? STAT_ATK : move == MOVE_MUD_SLAP ? STAT_ACC
        : move == MOVE_MUD_SHOT || move == MOVE_BUBBLE || move == MOVE_ROCK_TOMB || move == MOVE_ICY_WIND ? STAT_SPEED
        : move == MOVE_PSYCHIC || move == MOVE_SHADOW_BALL || move == MOVE_CRUNCH ? STAT_SPDEF : STAT_DEF;
    for (i = 0; i < GetMoveAdditionalEffectCount(move); i++)
    {
        const struct AdditionalEffect *e = GetMoveAdditionalEffectById(move, i);
        if (!e->self) { secondary = e->moveEffect; chance = e->chance; break; }
    }
    if (move == MOVE_SUPERPOWER)
    {
        DropStat(attacker, STAT_ATK, -1, FALSE);
        DropStat(attacker, STAT_DEF, -1, FALSE);
    }
    if (move == MOVE_METEOR_MASH && Random() % 100 < 20) DropStat(attacker, STAT_ATK, 1, FALSE);
    if (move == MOVE_STEEL_WING && Random() % 100 < 10) DropStat(attacker, STAT_DEF, 1, FALSE);
    if (!gBattleMons[target].hp) return 0;
    if (gBattleMons[target].ability == ABILITY_SHIELD_DUST) secondary = 0;
    if (secondary && (chance == 0 || Random() % 100 < chance))
    {
        switch (secondary)
        {
        case MOVE_EFFECT_PARALYSIS: case MOVE_EFFECT_FREEZE_OR_FROSTBITE:
            RealtimeArena_InflictStatus(attacker, target, secondary == MOVE_EFFECT_PARALYSIS ? MOVE_EFFECT_PARALYSIS : MOVE_EFFECT_FREEZE, FALSE);
            break;
        case MOVE_EFFECT_BURN:
            if (RealtimeArena_InflictStatus(attacker, target, MOVE_EFFECT_BURN, FALSE)) result |= 8;
            break;
        case MOVE_EFFECT_FLINCH:
            if (gBattleMons[target].ability != ABILITY_INNER_FOCUS) result |= 2;
            break;
        case MOVE_EFFECT_STAT_MINUS:
            if (DropStat(target, stat, -1, TRUE)) result |= 1;
            break;
        default:
            break;
        }
    }
    if (gBattleMons[target].ability == ABILITY_COLOR_CHANGE && move != MOVE_STRUGGLE && GetMovePower(move)
        && !IS_BATTLER_OF_TYPE(target, GetMoveType(move)))
    {
        SET_BATTLER_TYPE(target, GetMoveType(move));
        result |= 4;
    }
    return result;
}

s8 RealtimeArena_StatChange(u16 move, u8 *stat, bool8 *self)
{
    *self = FALSE;
    switch (move)
    {
    case MOVE_LEER: case MOVE_TAIL_WHIP: *stat = STAT_DEF; return -1;
    case MOVE_GROWL: *stat = STAT_ATK; return -1;
    case MOVE_SAND_ATTACK: *stat = STAT_ACC; return -1;
    case MOVE_STRING_SHOT: *stat = STAT_SPEED; return -1;
    case MOVE_SCREECH: *stat = STAT_DEF; return -2;
    }
    *self = TRUE;
    switch (move)
    {
    case MOVE_HARDEN: case MOVE_DEFENSE_CURL: *stat = STAT_DEF; return 1;
    case MOVE_HOWL: *stat = STAT_ATK; return 1;
    case MOVE_GROWTH: *stat = STAT_SPATK; return 1;
    case MOVE_AGILITY: *stat = STAT_SPEED; return 2;
    }
    return 0;
}

bool8 RealtimeArena_ResolveStatMove(u8 attacker, u8 target, u16 move)
{
    u8 stat = STAT_ATK;
    bool8 self, worked;
    s8 delta;
    NoteMove(attacker, move);
    if (move == MOVE_FOCUS_ENERGY)
    {
        worked = !gBattleMons[attacker].volatiles.focusEnergy;
        gBattleMons[attacker].volatiles.focusEnergy = 1;
        return worked;
    }
    delta = RealtimeArena_StatChange(move, &stat, &self);
    if (!delta) return FALSE;
    if (!self && !RollAccuracy(attacker, target, move))
    {
        SetResult(MOVE_RESULT_MISSED);
        return FALSE;
    }
    SetResult(0);
    if (move == MOVE_DEFENSE_CURL) gBattleMons[attacker].volatiles.defenseCurl = 1;
    return DropStat(self ? attacker : target, stat, delta, !self);
}

bool8 RealtimeArena_ResolveLeer(u8 attacker, u8 target)
{
    return RealtimeArena_ResolveStatMove(attacker, target, MOVE_LEER);
}

u16 RealtimeArena_DrainAmount(u16 hpDealt)
{
    if (!hpDealt) return 0;
    return max(1, hpDealt / 2);
}

// Gen 3 catch formula, identical to the Emerald Arena build's helper.
static const u8 sArenaBallBonus[] = {20, 15, 10, 15}; // Ultra, Great, Poke, Safari
u32 CalculateBallCatchOdds(u8 target, u16 item)
{
    u32 odds;
    u8 catchRate = gSpeciesInfo[gBattleMons[target].species].catchRate, multiplier = 10;
    switch (item)
    {
    case ITEM_ULTRA_BALL: multiplier = sArenaBallBonus[0]; break;
    case ITEM_GREAT_BALL: multiplier = sArenaBallBonus[1]; break;
    case ITEM_POKE_BALL: multiplier = sArenaBallBonus[2]; break;
    case ITEM_SAFARI_BALL: multiplier = sArenaBallBonus[3]; break;
    case ITEM_NET_BALL:
        if (IS_BATTLER_OF_TYPE(target, TYPE_WATER) || IS_BATTLER_OF_TYPE(target, TYPE_BUG)) multiplier = 30;
        break;
    case ITEM_NEST_BALL:
        if (gBattleMons[target].level < 40) multiplier = max(10, 40 - gBattleMons[target].level);
        break;
    case ITEM_REPEAT_BALL:
        if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(gBattleMons[target].species), FLAG_GET_CAUGHT)) multiplier = 30;
        break;
    case ITEM_TIMER_BALL:
        multiplier = min(40, gBattleResults.battleTurnCounter + 10);
        break;
    }
    odds = (catchRate * multiplier / 10)
        * (gBattleMons[target].maxHP * 3 - gBattleMons[target].hp * 2) / (3 * gBattleMons[target].maxHP);
    if (gBattleMons[target].status1 & (STATUS1_SLEEP | STATUS1_FREEZE)) odds *= 2;
    if (gBattleMons[target].status1 & (STATUS1_POISON | STATUS1_BURN | STATUS1_PARALYSIS | STATUS1_TOXIC_POISON)) odds = odds * 15 / 10;
    return odds;
}

u8 CalculateBallCatchShakes(u32 odds, u16 item)
{
    u8 shakes;
    if (item == ITEM_MASTER_BALL || odds > 254) return BALL_3_SHAKES_SUCCESS;
    if (!odds) return 0;
    odds = Sqrt(Sqrt(16711680 / odds));
    odds = 1048560 / odds;
    for (shakes = 0; shakes < BALL_3_SHAKES_SUCCESS && Random() < odds; shakes++);
    return shakes;
}
