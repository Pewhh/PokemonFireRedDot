#ifndef GUARD_CONSTANTS_BATTLE_AI_H
#define GUARD_CONSTANTS_BATTLE_AI_H

// battlers
#define AI_TARGET 0
#define AI_USER 1
#define AI_TARGET_PARTNER 2
#define AI_USER_PARTNER 3

// get_type command
#define AI_TYPE1_TARGET 0
#define AI_TYPE1_USER 1
#define AI_TYPE2_TARGET 2
#define AI_TYPE2_USER 3
#define AI_TYPE_MOVE 4

// type effectiveness
#define AI_EFFECTIVENESS_x4     160
#define AI_EFFECTIVENESS_x2     80
#define AI_EFFECTIVENESS_x1     40
#define AI_EFFECTIVENESS_x0_5   20
#define AI_EFFECTIVENESS_x0_25  10
#define AI_EFFECTIVENESS_x0     0

// ai weather
#define AI_WEATHER_SUN       0
#define AI_WEATHER_RAIN      1
#define AI_WEATHER_SANDSTORM 2
#define AI_WEATHER_HAIL      3
#define AI_WEATHER_NONE      4 // HACK: sem clima (Fase 6)

// get_how_powerful_move_is
#define MOVE_POWER_DISCOURAGED  0
#define MOVE_NOT_MOST_POWERFUL  1
#define MOVE_MOST_POWERFUL      2

// script's table id to bit
#define AI_SCRIPT_CHECK_BAD_MOVE (1 << 0)
#define AI_SCRIPT_CHECK_VIABILITY (1 << 1)
#define AI_SCRIPT_TRY_TO_FAINT (1 << 2)
#define AI_SCRIPT_SETUP_FIRST_TURN (1 << 3)
#define AI_SCRIPT_RISKY (1 << 4)
#define AI_SCRIPT_PREFER_STRONGEST_MOVE (1 << 5)
#define AI_SCRIPT_PREFER_BATON_PASS (1 << 6)
#define AI_SCRIPT_DOUBLE_BATTLE (1 << 7)
#define AI_SCRIPT_HP_AWARE (1 << 8)
#define AI_SCRIPT_UNKNOWN (1 << 9)
// Módulos novos (Fase 6), nos índices livres da tabela de scripts
#define AI_SCRIPT_SUICIDE (1 << 10) // Explosion/Selfdestruct só com HP baixo
#define AI_SCRIPT_PRIORITY_AWARE (1 << 11) // golpe de prioridade quando cairia antes de agir
#define AI_SCRIPT_ANTI_SETUP (1 << 12) // Haze/Roar/Encore contra boosts do jogador
#define AI_SCRIPT_SETUP_SWEEPER (1 << 13) // boost só quando aguenta o próximo golpe
#define AI_SCRIPT_STALL (1 << 14) // desgaste: Toxic, Leech Seed, Protect alternado, cura
#define AI_SCRIPT_WEATHER (1 << 15) // abre e renova o clima que favorece o Pokémon
#define AI_SCRIPT_STALL_LIGHT (1 << 16) // desgaste leve: só Toxic e Leech Seed

// Flags novas (não são scripts: os índices 20 e 21 da tabela de scripts são AI_Ret)
#define AI_SCRIPT_SMART_SWITCH (1 << 20) // troca de Pokémon nova (Fase 5)
#define AI_SCRIPT_ACE_LAST     (1 << 21) // guarda o último do time (o ás) para o fim
#define AI_SCRIPT_FREQUENT_SWITCH (1 << 22) // personalidade: troca mais vezes (Fase 6)

// Perfis de IA (Fase 4)
#define AI_PROFILE_NOVICE  1
#define AI_PROFILE_COMMON  2
#define AI_PROFILE_SKILLED 3
#define AI_PROFILE_BOSS    4
#define AI_PROFILE_ELITE   5
// 10 - 28 are not used
#define AI_SCRIPT_ROAMING (1 << 29)
#define AI_SCRIPT_SAFARI (1 << 30)
#define AI_SCRIPT_FIRST_BATTLE (1 << 31)

#endif // GUARD_CONSTANTS_BATTLE_AI_H
