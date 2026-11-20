#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "random.h"
#include "util.h"
#include "constants/abilities.h"
#include "constants/item_effects.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/battle_ai.h"
#include "constants/battle_move_effects.h"
#include "constants/opponents.h"
#include "battle_ai_script_commands.h"
#include "event_data.h"

static bool8 HasSuperEffectiveMoveAgainstOpponents(bool8 noRng);
static bool8 FindMonWithFlagsAndSuperEffective(u8 flags, u8 moduloPercent);
static bool8 ShouldUseItem(void);

#define BATTLE_HISTORY (gBattleResources->battleHistory)
static void ModulateByTypeEffectiveness(u8 atkType, u8 defType1, u8 defType2, u8 *var);
static bool8 IsSmartSwitchTrainer(void);
static bool8 SmartShouldSwitch(void);
static u8 SmartGetMonToSwitchInto(void);

static bool8 ShouldSwitchIfPerishSong(void)
{
    if (gStatuses3[gActiveBattler] & STATUS3_PERISH_SONG
     && gDisableStructs[gActiveBattler].perishSongTimer == 0)
    {
        *(gBattleStruct->AI_monToSwitchIntoId + (GetBattlerPosition(gActiveBattler) >> 1)) = PARTY_SIZE;
        BtlController_EmitTwoReturnValues(1, B_ACTION_SWITCH, 0);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static bool8 ShouldSwitchIfWonderGuard(void)
{
    u8 opposingBattler;
    u8 moveFlags;
    s32 i, j;
    u16 move;

    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
        return FALSE;
    if (gBattleMons[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)].ability == ABILITY_WONDER_GUARD)
    {
        // Check if Pokemon has a super effective move.
        for (opposingBattler = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), i = 0; i < MAX_MON_MOVES; ++i)
        {
            move = gBattleMons[gActiveBattler].moves[i];
            if (move == MOVE_NONE)
                continue;
            moveFlags = AI_TypeCalc(move, gBattleMons[opposingBattler].species, gBattleMons[opposingBattler].ability);
            if (moveFlags & MOVE_RESULT_SUPER_EFFECTIVE)
                return FALSE;
        }
        // Find a Pokemon in the party that has a super effective move.
        for (i = 0; i < PARTY_SIZE; ++i)
        {
            if (GetMonData(&gEnemyParty[i], MON_DATA_HP) == 0
             || GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_NONE
             || GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_EGG
             || i == gBattlerPartyIndexes[gActiveBattler])
                continue;
            GetMonData(&gEnemyParty[i], MON_DATA_SPECIES); // Unused return value.
            GetMonData(&gEnemyParty[i], MON_DATA_ABILITY_NUM); // Unused return value.        
            for (opposingBattler = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), j = 0; j < MAX_MON_MOVES; ++j)
            {
                move = GetMonData(&gEnemyParty[i], MON_DATA_MOVE1 + j);
                if (move == MOVE_NONE)
                    continue;
                moveFlags = AI_TypeCalc(move, gBattleMons[opposingBattler].species, gBattleMons[opposingBattler].ability);
                if (moveFlags & MOVE_RESULT_SUPER_EFFECTIVE && Random() % 3 < 2)
                {
                    // We found a mon.
                    *(gBattleStruct->AI_monToSwitchIntoId + (GetBattlerPosition(gActiveBattler) >> 1)) = i;
                    BtlController_EmitTwoReturnValues(1, B_ACTION_SWITCH, 0);
                    return TRUE;
                }
            }
        }
    }
    return FALSE; // There is not a single Pokemon in the party that has a super effective move against a mon with Wonder Guard.
}

static bool8 FindMonThatAbsorbsOpponentsMove(void)
{
    u8 battlerIn1, battlerIn2;
    u8 absorbingTypeAbility;
    u8 absorbingTypeAbility2 = ABILITY_NONE; // HACK: Magma Armor reduz dano de Água a 1/8
    s32 i;

    if ((HasSuperEffectiveMoveAgainstOpponents(TRUE) && Random() % 3) 
    || (gLastLandedMoves[gActiveBattler] == MOVE_NONE))
        return FALSE;
    if (gLastLandedMoves[gActiveBattler] == 0xFFFF
     || gBattleMoves[gLastLandedMoves[gActiveBattler]].power == 0)
        return FALSE;
    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
    {
        battlerIn1 = gActiveBattler;
        if (gAbsentBattlerFlags & gBitTable[GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler)))])
            battlerIn2 = gActiveBattler;
        else
            battlerIn2 = GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler)));
    }
    else
    {
        battlerIn1 = gActiveBattler;
        battlerIn2 = gActiveBattler;
    }
    if (gBattleMoves[gLastLandedMoves[gActiveBattler]].type == TYPE_FIRE)
        absorbingTypeAbility = ABILITY_FLASH_FIRE;
    else if (gBattleMoves[gLastLandedMoves[gActiveBattler]].type == TYPE_WATER)
    {
        absorbingTypeAbility = ABILITY_WATER_ABSORB;
        absorbingTypeAbility2 = ABILITY_MAGMA_ARMOR;
    }
    else if (gBattleMoves[gLastLandedMoves[gActiveBattler]].type == TYPE_ELECTRIC)
        absorbingTypeAbility = ABILITY_VOLT_ABSORB;
    else
        return FALSE;
    if (gBattleMons[gActiveBattler].ability == absorbingTypeAbility
     || (absorbingTypeAbility2 != ABILITY_NONE && gBattleMons[gActiveBattler].ability == absorbingTypeAbility2))
        return FALSE;
    for (i = 0; i < PARTY_SIZE; ++i)
    {
        u16 species;
        u8 monAbility;

        if ((GetMonData(&gEnemyParty[i], MON_DATA_HP) == 0)
         || (GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_NONE)
         || (GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_EGG)
         || (i == gBattlerPartyIndexes[battlerIn1])
         || (i == gBattlerPartyIndexes[battlerIn2])
         || (i == *(gBattleStruct->monToSwitchIntoId + battlerIn1))
         || (i == *(gBattleStruct->monToSwitchIntoId + battlerIn2)))
            continue;
        species = GetMonData(&gEnemyParty[i], MON_DATA_SPECIES);
        if (GetMonData(&gEnemyParty[i], MON_DATA_ABILITY_NUM) != ABILITY_NONE)
            monAbility = gSpeciesInfo[species].abilities[1];
        else
            monAbility = gSpeciesInfo[species].abilities[0];
        if ((absorbingTypeAbility == monAbility
          || (absorbingTypeAbility2 != ABILITY_NONE && absorbingTypeAbility2 == monAbility)) && Random() & 1)
        {
            // we found a mon
            *(gBattleStruct->AI_monToSwitchIntoId + (GetBattlerPosition(gActiveBattler) >> 1)) = i;
            BtlController_EmitTwoReturnValues(1, B_ACTION_SWITCH, 0);
            return TRUE;
        }
    }
    return FALSE;
}

static bool8 ShouldSwitchIfNaturalCure(void)
{
    if (!(gBattleMons[gActiveBattler].status1 & STATUS1_SLEEP)
     || (gBattleMons[gActiveBattler].ability != ABILITY_NATURAL_CURE)
     || (gBattleMons[gActiveBattler].hp < gBattleMons[gActiveBattler].maxHP / 2))
        return FALSE;
    if ((gLastLandedMoves[gActiveBattler] == MOVE_NONE || gLastLandedMoves[gActiveBattler] == 0xFFFF) && Random() & 1)
    {
        *(gBattleStruct->AI_monToSwitchIntoId + (GetBattlerPosition(gActiveBattler) >> 1)) = PARTY_SIZE;
        BtlController_EmitTwoReturnValues(1, B_ACTION_SWITCH, 0);
        return TRUE;
    }
    else if (gBattleMoves[gLastLandedMoves[gActiveBattler]].power == 0 && Random() & 1)
    {
        *(gBattleStruct->AI_monToSwitchIntoId + (GetBattlerPosition(gActiveBattler) >> 1)) = PARTY_SIZE;
        BtlController_EmitTwoReturnValues(1, B_ACTION_SWITCH, 0);
        return TRUE;
    }
    if (FindMonWithFlagsAndSuperEffective(MOVE_RESULT_DOESNT_AFFECT_FOE, 1)
     || FindMonWithFlagsAndSuperEffective(MOVE_RESULT_NOT_VERY_EFFECTIVE, 1))
        return TRUE;
    if (Random() & 1)
    {
        *(gBattleStruct->AI_monToSwitchIntoId + (GetBattlerPosition(gActiveBattler) >> 1)) = PARTY_SIZE;
        BtlController_EmitTwoReturnValues(1, B_ACTION_SWITCH, 0);
        return TRUE;
    }
    return FALSE;
}

static bool8 HasSuperEffectiveMoveAgainstOpponents(bool8 noRng)
{
    u8 opposingBattler;
    s32 i;
    u8 moveFlags;
    u16 move;

    opposingBattler = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
    if (!(gAbsentBattlerFlags & gBitTable[opposingBattler]))
    {
        for (i = 0; i < MAX_MON_MOVES; ++i)
        {
            move = gBattleMons[gActiveBattler].moves[i];
            if (move == MOVE_NONE)
                continue;

            moveFlags = AI_TypeCalc(move, gBattleMons[opposingBattler].species, gBattleMons[opposingBattler].ability);
            if (moveFlags & MOVE_RESULT_SUPER_EFFECTIVE)
            {
                if (noRng || (Random() % 10))
                    return TRUE;
            }
        }
    }
    if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
        return FALSE;
    opposingBattler = GetBattlerAtPosition(BATTLE_PARTNER(B_POSITION_PLAYER_LEFT));
    if (!(gAbsentBattlerFlags & gBitTable[opposingBattler]))
    {
        for (i = 0; i < MAX_MON_MOVES; ++i)
        {
            move = gBattleMons[gActiveBattler].moves[i];
            if (move == MOVE_NONE)
                continue;
            moveFlags = AI_TypeCalc(move, gBattleMons[opposingBattler].species, gBattleMons[opposingBattler].ability);
            if (moveFlags & MOVE_RESULT_SUPER_EFFECTIVE)
            {
                if (noRng)
                    return TRUE;
                if (Random() % 10 != 0)
                    return TRUE;
            }
        }
    }
    return FALSE;
}

static bool8 AreStatsRaised(void)
{
    u8 buffedStatsValue = 0;
    s32 i;

    for (i = 0; i < NUM_BATTLE_STATS; ++i)
    {
        if (gBattleMons[gActiveBattler].statStages[i] > 6)
            buffedStatsValue += gBattleMons[gActiveBattler].statStages[i] - 6;
    }
    return (buffedStatsValue > 3);
}

static bool8 FindMonWithFlagsAndSuperEffective(u8 flags, u8 moduloPercent)
{
    u8 battlerIn1, battlerIn2;
    s32 i, j;
    u16 move;
    u8 moveFlags;

    if (gLastLandedMoves[gActiveBattler] == 0)
        return FALSE;
    if ((gLastLandedMoves[gActiveBattler] == 0xFFFF)
     || (gLastHitBy[gActiveBattler] == 0xFF)
     || (gBattleMoves[gLastLandedMoves[gActiveBattler]].power == 0))
        return FALSE;
    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
    {
        battlerIn1 = gActiveBattler;
        if (gAbsentBattlerFlags & gBitTable[GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler)))])
            battlerIn2 = gActiveBattler;
        else
            battlerIn2 = GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler)));
    }
    else
    {
        battlerIn1 = gActiveBattler;
        battlerIn2 = gActiveBattler;
    }
    for (i = 0; i < PARTY_SIZE; ++i)
    {
        u16 species;
        u8 monAbility;

        if ((GetMonData(&gEnemyParty[i], MON_DATA_HP) == 0)
         || (GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_NONE)
         || (GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_EGG)
         || (i == gBattlerPartyIndexes[battlerIn1])
         || (i == gBattlerPartyIndexes[battlerIn2])
         || (i == *(gBattleStruct->monToSwitchIntoId + battlerIn1))
         || (i == *(gBattleStruct->monToSwitchIntoId + battlerIn2)))
            continue;
        species = GetMonData(&gEnemyParty[i], MON_DATA_SPECIES);
        if (GetMonData(&gEnemyParty[i], MON_DATA_ABILITY_NUM) != ABILITY_NONE)
            monAbility = gSpeciesInfo[species].abilities[1];
        else
            monAbility = gSpeciesInfo[species].abilities[0];
        moveFlags = AI_TypeCalc(gLastLandedMoves[gActiveBattler], species, monAbility);
        if (moveFlags & flags)
        {
            battlerIn1 = gLastHitBy[gActiveBattler];
            for (j = 0; j < MAX_MON_MOVES; ++j)
            {
                move = GetMonData(&gEnemyParty[i], MON_DATA_MOVE1 + j);
                if (move == MOVE_NONE)
                    continue;
                moveFlags = AI_TypeCalc(move, gBattleMons[battlerIn1].species, gBattleMons[battlerIn1].ability);
                if (moveFlags & MOVE_RESULT_SUPER_EFFECTIVE && Random() % moduloPercent == 0)
                {
                    *(gBattleStruct->AI_monToSwitchIntoId + (GetBattlerPosition(gActiveBattler) >> 1)) = i;
                    BtlController_EmitTwoReturnValues(1, B_ACTION_SWITCH, 0);
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}


// ============================================================================
// Troca de Pokémon nova (Fase 5 do projeto de IA). Vale para treinadores com
// AI_SCRIPT_SMART_SWITCH (perfis Chefe e Elite), só em batalhas simples.
// Usa só informação justa: golpes e habilidade do jogador que já apareceram em
// batalha, e o item do jogador só depois de revelado.
// ============================================================================

#define SMART_SWITCH_CHANCE_NOVICE  5
#define SMART_SWITCH_CHANCE_COMMON  10
#define SMART_SWITCH_CHANCE_SKILLED 30
#define SMART_SWITCH_CHANCE_BOSS  50
#define SMART_SWITCH_CHANCE_ELITE 80
#define SMART_SWITCH_MIN_TURNS    3 // turnsOnField conta o turno atual: 3 = já agiu 2 turnos

// Troca de quem acabou de entrar: chance extra para a 1ª, 2ª e 3ª ou mais em sequência
static const u8 sQuickSwitchChance[] = {50, 25, 12};
EWRAM_DATA u8 gAIQuickSwitchStreak = 0; // trocas rápidas seguidas da máquina nesta batalha

void AI_ResetSwitchStreak(void)
{
    gAIQuickSwitchStreak = 0;
}

static bool8 IsSmartSwitchTrainer(void)
{
    if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER)
     || (gBattleTypeFlags & (BATTLE_TYPE_DOUBLE | BATTLE_TYPE_LINK | BATTLE_TYPE_FIRST_BATTLE
                           | BATTLE_TYPE_TRAINER_TOWER | BATTLE_TYPE_BATTLE_TOWER | BATTLE_TYPE_EREADER_TRAINER))
     || gTrainerBattleOpponent_A == TRAINER_SECRET_BASE)
        return FALSE;
    return (GetTrainerAIFlags(gTrainerBattleOpponent_A) & AI_SCRIPT_SMART_SWITCH) != 0;
}

static void AI_PartyMonToBattleMon(struct Pokemon *mon, struct BattlePokemon *dst)
{
    u32 i;
    u16 species = GetMonData(mon, MON_DATA_SPECIES);

    CpuFill16(0, dst, sizeof(*dst));
    dst->species = species;
    dst->attack = GetMonData(mon, MON_DATA_ATK);
    dst->defense = GetMonData(mon, MON_DATA_DEF);
    dst->speed = GetMonData(mon, MON_DATA_SPEED);
    dst->spAttack = GetMonData(mon, MON_DATA_SPATK);
    dst->spDefense = GetMonData(mon, MON_DATA_SPDEF);
    for (i = 0; i < MAX_MON_MOVES; i++)
        dst->moves[i] = GetMonData(mon, MON_DATA_MOVE1 + i);
    for (i = 0; i < NUM_BATTLE_STATS; i++)
        dst->statStages[i] = DEFAULT_STAT_STAGE;
    dst->abilityNum = GetMonData(mon, MON_DATA_ABILITY_NUM);
    dst->ability = GetAbilityBySpecies(species, dst->abilityNum);
    dst->type1 = gSpeciesInfo[species].types[0];
    dst->type2 = gSpeciesInfo[species].types[1];
    dst->hp = GetMonData(mon, MON_DATA_HP);
    dst->maxHP = GetMonData(mon, MON_DATA_MAX_HP);
    dst->level = GetMonData(mon, MON_DATA_LEVEL);
    dst->item = GetMonData(mon, MON_DATA_HELD_ITEM);
    dst->status1 = GetMonData(mon, MON_DATA_STATUS);
}

// Cópia do Pokémon do jogador sem o item, se o item ainda não apareceu em batalha
static void AI_GetFairPlayerMon(u8 player, struct BattlePokemon *dst)
{
    *dst = gBattleMons[player];
    if (BATTLE_HISTORY->itemEffects[GET_BATTLER_SIDE(player)] == 0)
        dst->item = ITEM_NONE;
}

static u8 AI_KnownAbility(u8 battler)
{
    if (GetBattlerSide(battler) == B_SIDE_PLAYER)
        return BATTLE_HISTORY->abilities[GET_BATTLER_SIDE(battler)];
    return gBattleMons[battler].ability;
}

// Multiplicador de tipo x10 (0, 2/5, 10, 20, 40), já com imunidades de habilidade conhecidas
static u8 AI_TypeMultiplierByType(u8 type, struct BattlePokemon *def, u8 defAbility)
{
    u8 mult = 10;

    ModulateByTypeEffectiveness(type, def->type1, def->type2, &mult);
    if ((defAbility == ABILITY_LEVITATE && type == TYPE_GROUND)
     || (defAbility == ABILITY_VOLT_ABSORB && type == TYPE_ELECTRIC)
     || (defAbility == ABILITY_WATER_ABSORB && type == TYPE_WATER)
     || (defAbility == ABILITY_FLASH_FIRE && type == TYPE_FIRE)
     || (defAbility == ABILITY_WONDER_GUARD && mult <= 10))
        mult = 0;
    return mult;
}

static u8 AI_TypeMultiplier(u16 move, struct BattlePokemon *def, u8 defAbility)
{
    return AI_TypeMultiplierByType(gBattleMoves[move].type, def, defAbility);
}

// Dano estimado (sem aleatoriedade nem crítico). power/type = 0 usam os do golpe.
static u32 AI_EstimateDamageEx(struct BattlePokemon *atk, u8 atkId, struct BattlePokemon *def, u8 defId, u8 defAbility, u16 move, u16 power, u8 type)
{
    s32 dmg;
    u8 mult;
    u16 savedPower = gBattleMovePower;
    u8 savedCrit = gCritMultiplier;
    u16 savedMove = gCurrentMove;

    if (move == MOVE_NONE || move == 0xFFFF || (power == 0 && gBattleMoves[move].power <= 1))
        return 0;
    if (power == 0)
        type = gBattleMoves[move].type;
    mult = AI_TypeMultiplierByType(type, def, defAbility);
    if (mult == 0)
        return 0;
    gCritMultiplier = 1;
    gCurrentMove = move; // CalculateBaseDamage olha gCurrentMove (Explosion, Solar Beam)
    dmg = CalculateBaseDamage(atk, def, move, gSideStatuses[GET_BATTLER_SIDE(defId)], power, (power != 0) ? type : 0, atkId, defId);
    gBattleMovePower = savedPower;
    gCritMultiplier = savedCrit;
    gCurrentMove = savedMove;
    if (type == atk->type1 || type == atk->type2)
        dmg = dmg * 15 / 10;
    dmg = dmg * mult / 10;
    if (dmg < 1)
        dmg = 1;
    return dmg;
}

static u32 AI_EstimateDamage(struct BattlePokemon *atk, u8 atkId, struct BattlePokemon *def, u8 defId, u8 defAbility, u16 move)
{
    return AI_EstimateDamageEx(atk, atkId, def, defId, defAbility, move, 0, 0);
}

// Presunção de jogador experiente: o Pokémon do jogador provavelmente tem um golpe
// do próprio tipo, mesmo que ainda não tenha usado. Vale para perfis Experiente ou acima.
// Devolve quantos tipos do jogador ainda não tiveram golpe de dano revelado.
// Poder do golpe presumido sobe com o nível do Pokémon do jogador
static u16 AI_PresumedStabPower(u8 level)
{
    if (level <= 20)
        return 40; // como Water Gun
    if (level <= 35)
        return 60; // como Bubblebeam
    return 80;     // perto de Surf, Flamethrower, Thunderbolt
}

static u8 AI_GetPresumedPlayerTypes(u8 player, struct BattlePokemon *playerMon, u8 *types)
{
    u8 count = 0, t, type;
    u32 i;
    u16 move;

    if (GetTrainerAIProfile(gTrainerBattleOpponent_A) < AI_PROFILE_SKILLED)
        return 0;
    for (t = 0; t < 2; t++)
    {
        type = (t == 0) ? playerMon->type1 : playerMon->type2;
        if (type == TYPE_MYSTERY || (t == 1 && type == playerMon->type1))
            continue;
        for (i = 0; i < 8; i++)
        {
            move = BATTLE_HISTORY->usedMoves[player >> 1][i];
            if (move != MOVE_NONE && move != 0xFFFF && gBattleMoves[move].power > 1 && gBattleMoves[move].type == type)
                break;
        }
        if (i == 8)
            types[count++] = type;
    }
    return count;
}

static u16 AI_EffectiveSpeed(struct BattlePokemon *mon)
{
    u32 speed = mon->speed * gStatStageRatios[mon->statStages[STAT_SPEED]][0] / gStatStageRatios[mon->statStages[STAT_SPEED]][1];
    if (mon->status1 & STATUS1_PARALYSIS)
        speed /= 4;
    return speed;
}

// Nota de um Pokémon (em campo ou no banco) contra o Pokémon do jogador. Maior = melhor.
// TRUE se algum golpe de dano do Pokémon acerta o do jogador (tipo e habilidade conhecida)
static bool8 AI_CanHitPlayer(struct BattlePokemon *mon, struct BattlePokemon *player, u8 playerAbility)
{
    u32 i;
    u16 move;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        move = mon->moves[i];
        if (move != MOVE_NONE && gBattleMoves[move].power > 1 && AI_TypeMultiplier(move, player, playerAbility) != 0)
            return TRUE;
    }
    return FALSE;
}

static s32 AI_ScoreMatchup(struct BattlePokemon *mon, u8 monId, struct BattlePokemon *player, u8 playerId)
{
    s32 score;
    u32 i, bestDmg = 0, worstTaken = 0, dmg;
    u8 playerAbility = AI_KnownAbility(playerId);
    u8 presumedTypes[2], presumedCount;
    u16 move;

    if (mon->hp == 0 || player->hp == 0)
        return -1000;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        dmg = AI_EstimateDamage(mon, monId, player, playerId, playerAbility, mon->moves[i]);
        if (dmg > bestDmg)
            bestDmg = dmg;
    }
    // Só golpes do jogador que já apareceram nesta batalha
    for (i = 0; i < 8; i++)
    {
        move = BATTLE_HISTORY->usedMoves[playerId >> 1][i];
        dmg = AI_EstimateDamage(player, playerId, mon, monId, mon->ability, move);
        if (dmg > worstTaken)
            worstTaken = dmg;
    }
    presumedCount = AI_GetPresumedPlayerTypes(playerId, player, presumedTypes);
    for (i = 0; i < presumedCount; i++)
    {
        dmg = AI_EstimateDamageEx(player, playerId, mon, monId, mon->ability, MOVE_TACKLE, AI_PresumedStabPower(player->level), presumedTypes[i]);
        if (dmg > worstTaken)
            worstTaken = dmg;
    }

    score = (bestDmg >= player->hp) ? 100 : bestDmg * 100 / player->hp;
    if (bestDmg >= player->hp)
        score += (AI_EffectiveSpeed(mon) > AI_EffectiveSpeed(player)) ? 200 : 60;
    if (worstTaken >= mon->hp)
        score -= 150;
    else
        score -= worstTaken * 50 / mon->maxHP;
    return score;
}

// Usado pelo comando de script if_target_can_ko_user (Fase 6).
// TRUE se o Pokémon do jogador derruba o da máquina com um golpe: golpes já revelados
// e, do Experiente para cima, o golpe presumido do tipo dele (poder pelo nível).
bool8 AI_PlayerCanKOBattler(u8 aiBattler, u8 playerBattler)
{
    struct BattlePokemon playerMon, aiMon;
    u8 presumedTypes[2], presumedCount;
    u32 i, dmg;
    u16 move;

    AI_GetFairPlayerMon(playerBattler, &playerMon);
    aiMon = gBattleMons[aiBattler];
    for (i = 0; i < 8; i++)
    {
        move = BATTLE_HISTORY->usedMoves[playerBattler >> 1][i];
        dmg = AI_EstimateDamage(&playerMon, playerBattler, &aiMon, aiBattler, aiMon.ability, move);
        if (dmg >= aiMon.hp)
            return TRUE;
    }
    presumedCount = AI_GetPresumedPlayerTypes(playerBattler, &playerMon, presumedTypes);
    for (i = 0; i < presumedCount; i++)
    {
        dmg = AI_EstimateDamageEx(&playerMon, playerBattler, &aiMon, aiBattler, aiMon.ability, MOVE_TACKLE, AI_PresumedStabPower(playerMon.level), presumedTypes[i]);
        if (dmg >= aiMon.hp)
            return TRUE;
    }
    return FALSE;
}

static u8 AI_GetAceSlot(void)
{
    s32 i;
    for (i = PARTY_SIZE - 1; i >= 0; i--)
    {
        if (GetMonData(&gEnemyParty[i], MON_DATA_SPECIES) != SPECIES_NONE)
            return i;
    }
    return 0;
}

// Melhor substituto pelo AI_ScoreMatchup. O ás (último do time) só entra se for a única opção.
static u8 AI_FindBestSwitchIn(s32 *bestScoreOut, bool8 mustHit)
{
    u8 player = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
    u8 active = gActiveBattler;
    u8 ace = AI_GetAceSlot();
    bool8 keepAce = (GetTrainerAIFlags(gTrainerBattleOpponent_A) & AI_SCRIPT_ACE_LAST) != 0;
    struct BattlePokemon playerMon, cand;
    s32 i, score, bestScore = -10000;
    u8 bestId = PARTY_SIZE, validCount = 0;

    AI_GetFairPlayerMon(player, &playerMon);
    for (i = 0; i < PARTY_SIZE; i++)
    {
        u16 species = GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG);
        if (species == SPECIES_NONE || species == SPECIES_EGG
         || GetMonData(&gEnemyParty[i], MON_DATA_HP) == 0
         || i == gBattlerPartyIndexes[active]
         || i == *(gBattleStruct->monToSwitchIntoId + active))
            continue;
        validCount++;
    }
    for (i = 0; i < PARTY_SIZE; i++)
    {
        u16 species = GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG);
        if (species == SPECIES_NONE || species == SPECIES_EGG
         || GetMonData(&gEnemyParty[i], MON_DATA_HP) == 0
         || i == gBattlerPartyIndexes[active]
         || i == *(gBattleStruct->monToSwitchIntoId + active))
            continue;
        if (keepAce && i == ace && validCount > 1)
            continue;
        AI_PartyMonToBattleMon(&gEnemyParty[i], &cand);
        // O atual não acerta o jogador por tipo: só serve um substituto que acerte
        if (mustHit && !AI_CanHitPlayer(&cand, &playerMon, AI_KnownAbility(player)))
            continue;
        score = AI_ScoreMatchup(&cand, active, &playerMon, player);
        if (score > bestScore)
        {
            bestScore = score;
            bestId = i;
        }
    }
    if (bestScoreOut != NULL)
        *bestScoreOut = bestScore;
    return bestId;
}

static u8 SmartGetMonToSwitchInto(void)
{
    return AI_FindBestSwitchIn(NULL, FALSE);
}

static bool8 SmartShouldSwitch(void)
{
    u8 active = gActiveBattler;
    u8 player = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
    struct BattlePokemon playerMon, activeMon;
    u32 i, dmg, bestDmg = 0;
    u8 bestMult = 0, playerAbility;
    s32 boosts = 0, activeScore, candScore;
    bool8 reason = FALSE, needHealthyBench = FALSE, cantHit, quickSwitch;
    u8 candId;
    u16 move, encored;

    if (gAbsentBattlerFlags & gBitTable[player])
        return FALSE;
    AI_GetFairPlayerMon(player, &playerMon);
    activeMon = gBattleMons[active];
    playerAbility = AI_KnownAbility(player);
    cantHit = !AI_CanHitPlayer(&activeMon, &playerMon, playerAbility);

    // Pokémon que acabou de entrar (ainda não agiu 2 turnos): pode trocar, mas com
    // chance extra que cai a cada troca seguida, como o Protect (50%, 25%, 12%).
    // Exceção: nenhum golpe dele acerta o Pokémon do jogador por causa do tipo.
    quickSwitch = (gDisableStructs[active].turnsOnField < SMART_SWITCH_MIN_TURNS && !cantHit);

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        move = activeMon.moves[i];
        dmg = AI_EstimateDamage(&activeMon, active, &playerMon, player, playerAbility, move);
        if (dmg > bestDmg)
            bestDmg = dmg;
        if (move != MOVE_NONE && gBattleMoves[move].power > 1)
        {
            u8 mult = AI_TypeMultiplier(move, &playerMon, playerAbility);
            if (mult > bestMult)
                bestMult = mult;
        }
    }
    // Pode nocautear agora: fica
    if (bestDmg >= playerMon.hp)
        return FALSE;
    // Já acumulou boost: fica
    for (i = STAT_ATK; i < NUM_BATTLE_STATS; i++)
    {
        if (activeMon.statStages[i] > DEFAULT_STAT_STAGE)
            boosts += activeMon.statStages[i] - DEFAULT_STAT_STAGE;
    }
    if (boosts >= 2)
        return FALSE;

    // 1. Nenhum golpe que cause dano ao Pokémon do jogador
    if (bestDmg == 0)
        reason = TRUE;
    // 2. Preso por Encore num golpe inútil
    encored = gDisableStructs[active].encoredMove;
    if (encored != MOVE_NONE && AI_EstimateDamage(&activeMon, active, &playerMon, player, playerAbility, encored) == 0)
        reason = TRUE;
    // 3. Toxic avançado, Leech Seed, Curse ou ataques muito reduzidos (precisa de banco saudável)
    if (((activeMon.status1 & STATUS1_TOXIC_POISON) && (activeMon.status1 & STATUS1_TOXIC_COUNTER) >= STATUS1_TOXIC_TURN(3))
     || (gStatuses3[active] & STATUS3_LEECHSEED)
     || (activeMon.status2 & STATUS2_CURSED)
     || (activeMon.statStages[STAT_ATK] <= DEFAULT_STAT_STAGE - 2 && activeMon.statStages[STAT_SPATK] <= DEFAULT_STAT_STAGE - 2))
    {
        reason = TRUE;
        needHealthyBench = TRUE;
    }
    // 4. Desvantagem dupla: o jogador já mostrou golpe super efetivo e a IA não tem nem golpe neutro
    if (bestMult < 10)
    {
        for (i = 0; i < 8; i++)
        {
            move = BATTLE_HISTORY->usedMoves[player >> 1][i];
            if (move != MOVE_NONE && move != 0xFFFF && gBattleMoves[move].power > 1
             && AI_TypeMultiplier(move, &activeMon, activeMon.ability) >= 20)
            {
                reason = TRUE;
                break;
            }
        }
        // Golpe do próprio tipo presumido (perfil Experiente ou acima)
        {
            u8 presumedTypes[2];
            u8 n = AI_GetPresumedPlayerTypes(player, &playerMon, presumedTypes);
            for (i = 0; i < n; i++)
            {
                if (AI_TypeMultiplierByType(presumedTypes[i], &activeMon, activeMon.ability) >= 20)
                    reason = TRUE;
            }
        }
    }
    if (!reason)
        return FALSE;

    // Se o atual não acerta por tipo, só troca por quem acerta; se ninguém acerta, fica
    candId = AI_FindBestSwitchIn(&candScore, cantHit);
    if (candId == PARTY_SIZE)
        return FALSE;
    if (needHealthyBench && GetMonData(&gEnemyParty[candId], MON_DATA_HP) <= GetMonData(&gEnemyParty[candId], MON_DATA_MAX_HP) / 2)
        return FALSE;
    // O substituto precisa estar melhor que o atual
    activeScore = AI_ScoreMatchup(&activeMon, active, &playerMon, player);
    if (candScore <= activeScore)
        return FALSE;
    {
        u8 profile = GetTrainerAIProfile(gTrainerBattleOpponent_A);
        u8 chance = (profile >= AI_PROFILE_ELITE) ? SMART_SWITCH_CHANCE_ELITE
                  : (profile >= AI_PROFILE_BOSS) ? SMART_SWITCH_CHANCE_BOSS
                  : (profile >= AI_PROFILE_SKILLED) ? SMART_SWITCH_CHANCE_SKILLED
                  : (profile >= AI_PROFILE_COMMON) ? SMART_SWITCH_CHANCE_COMMON
                  : SMART_SWITCH_CHANCE_NOVICE;
        // Personalidade "troca frequente" (Agatha e rival): +25, no máximo 95%
        if (GetTrainerAIFlags(gTrainerBattleOpponent_A) & AI_SCRIPT_FREQUENT_SWITCH)
            chance = (chance + 25 > 95) ? 95 : chance + 25;
        if (Random() % 100 >= chance)
            return FALSE;
    }

    if (quickSwitch)
    {
        u8 idx = (gAIQuickSwitchStreak > 2) ? 2 : gAIQuickSwitchStreak;
        if (Random() % 100 >= sQuickSwitchChance[idx])
            return FALSE;
        gAIQuickSwitchStreak++;
    }
    else
    {
        gAIQuickSwitchStreak = 0;
    }

    *(gBattleStruct->AI_monToSwitchIntoId + (GetBattlerPosition(active) >> 1)) = candId;
    BtlController_EmitTwoReturnValues(1, B_ACTION_SWITCH, 0);
    return TRUE;
}

static bool8 ShouldSwitch(void)
{
    u8 battlerIn1, battlerIn2;
    s32 i;
    s32 availableToSwitch;

    if ((gBattleMons[gActiveBattler].status2 & (STATUS2_WRAPPED | STATUS2_ESCAPE_PREVENTION))
     || (gStatuses3[gActiveBattler] & STATUS3_ROOTED)
     || AbilityBattleEffects(ABILITYEFFECT_CHECK_OTHER_SIDE, gActiveBattler, ABILITY_SHADOW_TAG, 0, 0)
     || (AbilityBattleEffects(ABILITYEFFECT_CHECK_OTHER_SIDE, gActiveBattler, ABILITY_ARENA_TRAP, 0, 0)
         && !IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_FLYING)
         && gBattleMons[gActiveBattler].ability != ABILITY_LEVITATE)) // FIX: Voador e Levitate não ficam presos
        return FALSE;
    if (AbilityBattleEffects(ABILITYEFFECT_FIELD_SPORT, 0, ABILITY_MAGNET_PULL, 0, 0))
        if ((gBattleMons[gActiveBattler].type1 == TYPE_STEEL) || (gBattleMons[gActiveBattler].type2 == TYPE_STEEL))
            return FALSE;
    availableToSwitch = 0;
    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
    {
        battlerIn1 = gActiveBattler;
        if (gAbsentBattlerFlags & gBitTable[GetBattlerAtPosition(GetBattlerPosition(gActiveBattler) ^ BIT_FLANK)])
            battlerIn2 = gActiveBattler;
        else
            battlerIn2 = GetBattlerAtPosition(GetBattlerPosition(gActiveBattler) ^ BIT_FLANK);
    }
    else
    {
        battlerIn2 = gActiveBattler;
        battlerIn1 = gActiveBattler;
    }
    for (i = 0; i < PARTY_SIZE; ++i)
    {
        if ((GetMonData(&gEnemyParty[i], MON_DATA_HP) == 0)
         || (GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_NONE)
         || (GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_EGG)
         || (i == gBattlerPartyIndexes[battlerIn1])
         || (i == gBattlerPartyIndexes[battlerIn2])
         || (i == *(gBattleStruct->monToSwitchIntoId + battlerIn1))
         || (i == *(gBattleStruct->monToSwitchIntoId + battlerIn2)))
            continue;
        ++availableToSwitch;
    }
    if (!availableToSwitch)
        return FALSE;
    if (IsSmartSwitchTrainer())
    {
        if (ShouldSwitchIfPerishSong() || FindMonThatAbsorbsOpponentsMove())
            return TRUE;
        return SmartShouldSwitch();
    }
    if (ShouldSwitchIfPerishSong()
     || ShouldSwitchIfWonderGuard()
     || FindMonThatAbsorbsOpponentsMove()
     || ShouldSwitchIfNaturalCure())
        return TRUE;
    if (HasSuperEffectiveMoveAgainstOpponents(FALSE)
     || AreStatsRaised())
        return FALSE;
    if (FindMonWithFlagsAndSuperEffective(MOVE_RESULT_DOESNT_AFFECT_FOE, 2)
     || FindMonWithFlagsAndSuperEffective(MOVE_RESULT_NOT_VERY_EFFECTIVE, 3))
        return TRUE;
    return FALSE;
}

void AI_TrySwitchOrUseItem(void)
{
    u8 battlerIn1, battlerIn2;

    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
    {
        if (ShouldSwitch())
        {
            if (*(gBattleStruct->AI_monToSwitchIntoId + (GetBattlerPosition(gActiveBattler) >> 1)) == 6)
            {
                s32 monToSwitchId = GetMostSuitableMonToSwitchInto();
                if (monToSwitchId == 6)
                {
                    if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
                    {
                        battlerIn1 = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
                        battlerIn2 = battlerIn1;
                    }
                    else
                    {
                        battlerIn1 = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
                        battlerIn2 = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
                    }
                    for (monToSwitchId = 0; monToSwitchId < PARTY_SIZE; ++monToSwitchId)
                    {
                        if ((!GetMonData(&gEnemyParty[monToSwitchId], MON_DATA_HP) == 0)
                         && (monToSwitchId != gBattlerPartyIndexes[battlerIn1])
                         && (monToSwitchId != gBattlerPartyIndexes[battlerIn2])
                         && (monToSwitchId != *(gBattleStruct->monToSwitchIntoId + battlerIn1))
                         && (monToSwitchId != *(gBattleStruct->monToSwitchIntoId + battlerIn2)))
                            break;
                    }
                }
                *(gBattleStruct->AI_monToSwitchIntoId + (GetBattlerPosition(gActiveBattler) >> 1)) = monToSwitchId;
            }
            *(gBattleStruct->monToSwitchIntoId + gActiveBattler) = *(gBattleStruct->AI_monToSwitchIntoId + (GetBattlerPosition(gActiveBattler) >> 1));
            return;
        }
        else if (ShouldUseItem())
        {
            return;
        }
    }
    BtlController_EmitTwoReturnValues(1, B_ACTION_USE_MOVE, (gActiveBattler ^ BIT_SIDE) << 8);
}

static void ModulateByTypeEffectiveness(u8 atkType, u8 defType1, u8 defType2, u8 *var)
{
    s32 i = 0;

    while (TYPE_EFFECT_ATK_TYPE(i) != TYPE_ENDTABLE)
    {
        if (TYPE_EFFECT_ATK_TYPE(i) == TYPE_FORESIGHT)
        {
            i += 3;
            continue;
        }
        else if (TYPE_EFFECT_ATK_TYPE(i) == atkType)
        {
            // Check type1.
            if (TYPE_EFFECT_DEF_TYPE(i) == defType1)
                *var = (*var * TYPE_EFFECT_MULTIPLIER(i)) / 10;
            // Check type2.
            if (TYPE_EFFECT_DEF_TYPE(i) == defType2 && defType1 != defType2)
                *var = (*var * TYPE_EFFECT_MULTIPLIER(i)) / 10;
        }
        i += 3;
    }
}

u8 GetMostSuitableMonToSwitchInto(void)
{
    u8 opposingBattler;
    u8 bestDmg;
    u32 bestDmgFallback;
    u16 savedCurrentMove;
    u8 bestMonId;
    u8 battlerIn1, battlerIn2;
    s32 i, j;
    u8 invalidMons;
    u16 move;

    if (*(gBattleStruct->monToSwitchIntoId + gActiveBattler) != PARTY_SIZE)
        return *(gBattleStruct->monToSwitchIntoId + gActiveBattler);
    if (IsSmartSwitchTrainer())
    {
        u8 smartId = SmartGetMonToSwitchInto();
        if (smartId != PARTY_SIZE)
            return smartId;
    }
    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
    {
        battlerIn1 = gActiveBattler;
        if (gAbsentBattlerFlags & gBitTable[GetBattlerAtPosition(GetBattlerPosition(gActiveBattler) ^ BIT_FLANK)])
            battlerIn2 = gActiveBattler;
        else
            battlerIn2 = GetBattlerAtPosition(GetBattlerPosition(gActiveBattler) ^ BIT_FLANK);
        // UB: It considers the opponent only player's side even though it can battle alongside player.
        opposingBattler = Random() & BIT_FLANK;
        if (gAbsentBattlerFlags & gBitTable[opposingBattler])
            opposingBattler ^= BIT_FLANK;
    }
    else
    {
        opposingBattler = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        battlerIn1 = gActiveBattler;
        battlerIn2 = gActiveBattler;
    }
    invalidMons = 0;
    while (invalidMons != 0x3F) // All mons are invalid.
    {
        bestDmg = 0;
        bestMonId = 6;
        // Find the mon whose type is the most suitable offensively.
        for (i = 0; i < PARTY_SIZE; ++i)
        {
            u16 species = GetMonData(&gEnemyParty[i], MON_DATA_SPECIES);
            if (species != SPECIES_NONE
                && GetMonData(&gEnemyParty[i], MON_DATA_HP) != 0
                && !(gBitTable[i] & invalidMons)
                && gBattlerPartyIndexes[battlerIn1] != i
                && gBattlerPartyIndexes[battlerIn2] != i
                && i != *(gBattleStruct->monToSwitchIntoId + battlerIn1)
                && i != *(gBattleStruct->monToSwitchIntoId + battlerIn2))
            {
                u8 type1 = gSpeciesInfo[species].types[0];
                u8 type2 = gSpeciesInfo[species].types[1];
                u8 typeDmg = 10;
                // FIX: argumentos estavam invertidos. O código media o quanto o JOGADOR acerta o candidato
                // e escolhia o maior, ou seja, o Pokémon mais fraco contra o jogador. Agora mede o quanto
                // o tipo do CANDIDATO acerta o jogador, como diz o comentário original.
                ModulateByTypeEffectiveness(type1, gBattleMons[opposingBattler].type1, gBattleMons[opposingBattler].type2, &typeDmg);
                if (type2 != type1)
                    ModulateByTypeEffectiveness(type2, gBattleMons[opposingBattler].type1, gBattleMons[opposingBattler].type2, &typeDmg);
                if (bestDmg < typeDmg)
                {
                    bestDmg = typeDmg;
                    bestMonId = i;
                }
            }
            else
            {
                invalidMons |= gBitTable[i];
            }
        }
        // Ok, we know the mon has the right typing but does it have at least one super effective move?
        if (bestMonId != PARTY_SIZE)
        {
            for (i = 0; i < MAX_MON_MOVES; ++i)
            {
                move = GetMonData(&gEnemyParty[bestMonId], MON_DATA_MOVE1 + i);
                if (move != MOVE_NONE && TypeCalc(move, gActiveBattler, opposingBattler) & MOVE_RESULT_SUPER_EFFECTIVE)
                    break;
            }
            if (i != MAX_MON_MOVES)
                return bestMonId; // Has both the typing and at least one super effective move.

            invalidMons |= gBitTable[bestMonId]; // Sorry buddy, we want something better.
        }
        else
        {
            invalidMons = 0x3F; // No viable mon to switch.
        }
    }
    gDynamicBasePower = 0;
    gBattleStruct->dynamicMoveType = 0;
    gBattleScripting.dmgMultiplier = 1;
    gMoveResultFlags = 0;
    gCritMultiplier = 1;
    bestDmgFallback = 0;
    bestMonId = 6;
    savedCurrentMove = gCurrentMove;
    // If we couldn't find the best mon in terms of typing, find the one that deals most damage.
    for (i = 0; i < PARTY_SIZE; ++i)
    {
        if (((u16)(GetMonData(&gEnemyParty[i], MON_DATA_SPECIES)) == SPECIES_NONE)
         || (GetMonData(&gEnemyParty[i], MON_DATA_HP) == 0)
         || (gBattlerPartyIndexes[battlerIn1] == i)
         || (gBattlerPartyIndexes[battlerIn2] == i)
         || (i == *(gBattleStruct->monToSwitchIntoId + battlerIn1))
         || (i == *(gBattleStruct->monToSwitchIntoId + battlerIn2)))
            continue;
        for (j = 0; j < MAX_MON_MOVES; ++j)
        {
            move = GetMonData(&gEnemyParty[i], MON_DATA_MOVE1 + j);
            gBattleMoveDamage = 0;
            if (move != MOVE_NONE && gBattleMoves[move].power != 1)
            {
                gCurrentMove = move; // FIX: antes calculava o dano do último golpe usado, não deste
                AI_CalcDmg(gActiveBattler, opposingBattler);
                TypeCalc(move, gActiveBattler, opposingBattler);
            }
            if (bestDmgFallback < (u32)gBattleMoveDamage)
            {
                bestDmgFallback = gBattleMoveDamage; // FIX: antes guardado em u8 (dano acima de 255 dava a volta)
                bestMonId = i;
            }
        }
    }
    gCurrentMove = savedCurrentMove;
    return bestMonId;
}

static u8 GetAI_ItemType(u8 itemId, const u8 *itemEffect) // NOTE: should take u16 as item Id argument
{
    if (itemId == ITEM_FULL_RESTORE)
        return AI_ITEM_FULL_RESTORE;
    else if (itemEffect[4] & ITEM4_HEAL_HP)
        return AI_ITEM_HEAL_HP;
    else if (itemEffect[3] & ITEM3_STATUS_ALL)
        return AI_ITEM_CURE_CONDITION;
    else if (itemEffect[0] & (ITEM0_DIRE_HIT | ITEM0_X_ATTACK) || itemEffect[1] != 0 || itemEffect[2] != 0)
        return AI_ITEM_X_STAT;
    else if (itemEffect[3] & ITEM3_GUARD_SPEC)
        return AI_ITEM_GUARD_SPECS;
    else
        return AI_ITEM_NOT_RECOGNIZABLE;
}

// Itens da IA por modo (Fase 6): nos modos em que o jogador não pode usar itens
// na batalha (Hard, Hardcore e Expert), o treinador também não usa. O item que o
// Pokémon segura continua funcionando normalmente.
static bool8 AI_ItemsBlockedByMode(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        return FALSE;
    return FlagGet(FLAG_HARD) || FlagGet(FLAG_HARDCORE) || FlagGet(FLAG_EXPERT);
}

static bool8 ShouldUseItem(void)
{
    s32 i;
    u8 validMons = 0;
    bool8 shouldUse = FALSE;

    if (AI_ItemsBlockedByMode())
        return FALSE;
    for (i = 0; i < PARTY_SIZE; ++i)
        if (GetMonData(&gEnemyParty[i], MON_DATA_HP) != 0
         && GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG) != SPECIES_NONE
         && GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG) != SPECIES_EGG)
            ++validMons;
    for (i = 0; i < MAX_TRAINER_ITEMS; ++i)
    {
        u16 item;
        const u8 *itemEffects;
        u8 paramOffset;
        u8 battlerSide;

        if (i && validMons > (gBattleResources->battleHistory->itemsNo - i) + 1)
            continue;
        item = gBattleResources->battleHistory->trainerItems[i];
        if (item == ITEM_NONE || gItemEffectTable[item - ITEM_POTION] == NULL)
            continue;
        if (item == ITEM_ENIGMA_BERRY)
            itemEffects = gSaveBlock1Ptr->enigmaBerry.itemEffect;
        else
            itemEffects = gItemEffectTable[item - ITEM_POTION];
        *(gBattleStruct->AI_itemType + gActiveBattler / 2) = GetAI_ItemType(item, itemEffects);
        switch (*(gBattleStruct->AI_itemType + gActiveBattler / 2))
        {
        case AI_ITEM_FULL_RESTORE:
            if (gBattleMons[gActiveBattler].hp >= gBattleMons[gActiveBattler].maxHP / 4)
                break;
            if (gBattleMons[gActiveBattler].hp == 0)
                break;
            shouldUse = TRUE;
            break;
        case AI_ITEM_HEAL_HP:
            paramOffset = GetItemEffectParamOffset(item, 4, 4);
            if (paramOffset == 0 || gBattleMons[gActiveBattler].hp == 0)
                break;
            if (gBattleMons[gActiveBattler].hp < gBattleMons[gActiveBattler].maxHP / 4 || gBattleMons[gActiveBattler].maxHP - gBattleMons[gActiveBattler].hp > itemEffects[paramOffset])
                shouldUse = TRUE;
            break;
        case AI_ITEM_CURE_CONDITION:
            *(gBattleStruct->AI_itemFlags + gActiveBattler / 2) = 0;
            if (itemEffects[3] & ITEM3_SLEEP && gBattleMons[gActiveBattler].status1 & STATUS1_SLEEP)
            {
                *(gBattleStruct->AI_itemFlags + gActiveBattler / 2) |= 0x20;
                shouldUse = TRUE;
            }
            if (itemEffects[3] & ITEM3_POISON && (gBattleMons[gActiveBattler].status1 & STATUS1_POISON || gBattleMons[gActiveBattler].status1 & STATUS1_TOXIC_POISON))
            {
                *(gBattleStruct->AI_itemFlags + gActiveBattler / 2) |= 0x10;
                shouldUse = TRUE;
            }
            if (itemEffects[3] & ITEM3_BURN && gBattleMons[gActiveBattler].status1 & STATUS1_BURN)
            {
                *(gBattleStruct->AI_itemFlags + gActiveBattler / 2) |= 0x8;
                shouldUse = TRUE;
            }
            if (itemEffects[3] & ITEM3_FREEZE && gBattleMons[gActiveBattler].status1 & STATUS1_FREEZE)
            {
                *(gBattleStruct->AI_itemFlags + gActiveBattler / 2) |= 0x4;
                shouldUse = TRUE;
            }
            if (itemEffects[3] & ITEM3_PARALYSIS && gBattleMons[gActiveBattler].status1 & STATUS1_PARALYSIS)
            {
                *(gBattleStruct->AI_itemFlags + gActiveBattler / 2) |= 0x2;
                shouldUse = TRUE;
            }
            if (itemEffects[3] & ITEM3_CONFUSION && gBattleMons[gActiveBattler].status2 & STATUS2_CONFUSION)
            {
                *(gBattleStruct->AI_itemFlags + gActiveBattler / 2) |= 0x1;
                shouldUse = TRUE;
            }
            break;
        case AI_ITEM_X_STAT:
            *(gBattleStruct->AI_itemFlags + gActiveBattler / 2) = 0;
            if (!gDisableStructs[gActiveBattler].isFirstTurn)
                break;
            if (itemEffects[0] & ITEM0_X_ATTACK)
                *(gBattleStruct->AI_itemFlags + gActiveBattler / 2) |= 0x1;
            if (itemEffects[1] & ITEM1_X_DEFEND)
                *(gBattleStruct->AI_itemFlags + gActiveBattler / 2) |= 0x2;
            if (itemEffects[1] & ITEM1_X_SPEED)
                *(gBattleStruct->AI_itemFlags + gActiveBattler / 2) |= 0x4;
            if (itemEffects[2] & ITEM2_X_SPATK)
                *(gBattleStruct->AI_itemFlags + gActiveBattler / 2) |= 0x8;
            if (itemEffects[2] & ITEM2_X_ACCURACY)
                *(gBattleStruct->AI_itemFlags + gActiveBattler / 2) |= 0x20;
            if (itemEffects[0] & ITEM0_DIRE_HIT)
                *(gBattleStruct->AI_itemFlags + gActiveBattler / 2) |= 0x80;
            shouldUse = TRUE;
            break;
        case AI_ITEM_GUARD_SPECS:
            battlerSide = GetBattlerSide(gActiveBattler);
            if (gDisableStructs[gActiveBattler].isFirstTurn && gSideTimers[battlerSide].mistTimer == 0)
                shouldUse = TRUE;
            break;
        case AI_ITEM_NOT_RECOGNIZABLE:
            return FALSE;
        }
        if (shouldUse)
        {
            BtlController_EmitTwoReturnValues(1, B_ACTION_USE_ITEM, 0);
            *(gBattleStruct->chosenItem + (gActiveBattler / 2) * 2) = item;
            gBattleResources->battleHistory->trainerItems[i] = 0;
            return shouldUse;
        }
    }
    return FALSE;
}
