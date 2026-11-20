#include "global.h"
#include "gflib.h"
#include "random.h"
#include "overworld.h"
#include "constants/maps.h"
#include "load_save.h"
#include "item_menu.h"
#include "tm_case.h"
#include "berry_pouch.h"
#include "quest_log.h"
#include "wild_encounter.h"
#include "event_data.h"
#include "mail_data.h"
#include "play_time.h"
#include "money.h"
#include "battle_records.h"
#include "pokemon_size_record.h"
#include "pokemon_storage_system.h"
#include "roamer.h"
#include "item.h"
#include "player_pc.h"
#include "berry.h"
#include "easy_chat.h"
#include "union_room_chat.h"
#include "mystery_gift.h"
#include "renewable_hidden_items.h"
#include "trainer_tower.h"
#include "script.h"
#include "berry_powder.h"
#include "pokemon_jump.h"
#include "event_scripts.h"

// this file's functions
static void ResetMiniGamesResults(void);
static void SetAdventuresNameLocks(void);

// EWRAM vars
EWRAM_DATA bool8 gDifferentSaveFile = FALSE;

void SetTrainerId(u32 trainerId, u8 *dst)
{
    dst[0] = trainerId;
    dst[1] = trainerId >> 8;
    dst[2] = trainerId >> 16;
    dst[3] = trainerId >> 24;
}

void CopyTrainerId(u8 *dst, u8 *src)
{
    s32 i;
    for (i = 0; i < 4; i++)
        dst[i] = src[i];
}

static void InitPlayerTrainerId(void)
{
    u32 trainerId = (Random() << 0x10) | GetGeneratedTrainerIdLower();
    SetTrainerId(trainerId, gSaveBlock2Ptr->playerTrainerId);
}

static void SetDefaultOptions(void)
{
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST; // HACK: jogo novo começa com texto rápido
    gSaveBlock2Ptr->optionsWindowFrameType = 0;
    gSaveBlock2Ptr->optionsSound = OPTIONS_SOUND_MONO;
    gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SHIFT;
    gSaveBlock2Ptr->optionsBattleSceneOff = FALSE;
    gSaveBlock2Ptr->regionMapZoom = FALSE;
    gSaveBlock2Ptr->optionsButtonMode = OPTIONS_BUTTON_MODE_HELP;
}

static void ClearPokedexFlags(void)
{
    memset(&gSaveBlock2Ptr->pokedex.owned, 0, sizeof(gSaveBlock2Ptr->pokedex.owned));
    memset(&gSaveBlock2Ptr->pokedex.seen, 0, sizeof(gSaveBlock2Ptr->pokedex.seen));
}

static void ClearBattleTower(void)
{
    CpuFill32(0, &gSaveBlock2Ptr->battleTower, sizeof(gSaveBlock2Ptr->battleTower));
}

static void WarpToPlayersRoom(void)
{
    SetWarpDestination(MAP_GROUP(MAP_PALLET_TOWN_PLAYERS_HOUSE_2F), MAP_NUM(MAP_PALLET_TOWN_PLAYERS_HOUSE_2F), -1, 6, 6);
    WarpIntoMap();
}

void Sav2_ClearSetDefault(void)
{
    ClearSav2();
    SetDefaultOptions();
}

void ResetMenuAndMonGlobals(void)
{
    gDifferentSaveFile = FALSE;
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    ResetBagCursorPositions();
    ResetTMCaseCursorPos();
    BerryPouch_CursorResetToTop();
    ResetQuestLog();
    SeedWildEncounterRng(Random());
    ResetSpecialVars();
}

void NewGameInitData(void)
{
    // InitEventData() below clears every flag, so keep the difficulty chosen in
    // Oak's speech and set it again at the end. One variable per flag, so that
    // HARDCORE does not turn into EXPERT.
    bool8 hardPrev = FlagGet(FLAG_HARD);
    bool8 hardcorePrev = FlagGet(FLAG_HARDCORE);
    bool8 expertPrev = FlagGet(FLAG_EXPERT);
    u8 rivalName[PLAYER_NAME_LENGTH + 1];
    StringCopy(rivalName, gSaveBlock1Ptr->rivalName);
    gDifferentSaveFile = TRUE;
    gSaveBlock2Ptr->encryptionKey = 0;
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    ClearBattleTower();
    ClearSav1();
    ClearMailData();
    gSaveBlock2Ptr->specialSaveWarpFlags = 0;
    gSaveBlock2Ptr->gcnLinkFlags = 0;
    gSaveBlock2Ptr->unkFlag1 = TRUE;
    gSaveBlock2Ptr->unkFlag2 = FALSE;
    InitPlayerTrainerId();
    PlayTimeCounter_Reset();
    ClearPokedexFlags();
    InitEventData();
    ResetFameChecker();
    SetMoney(&gSaveBlock1Ptr->money, 3000);
    ResetGameStats();
    ClearPlayerLinkBattleRecords();
    InitHeracrossSizeRecord();
    InitMagikarpSizeRecord();
    EnableNationalPokedex_RSE();
    gPlayerPartyCount = 0;
    ZeroPlayerPartyMons();
    ResetPokemonStorageSystem();
    ClearRoamerData();
    gSaveBlock1Ptr->registeredItem = 0;
    memset(gSaveBlock1Ptr->registeredItemsExtra, 0, sizeof(gSaveBlock1Ptr->registeredItemsExtra)); // HACK
    ClearBag();
    NewGameInitPCItems();
    ClearEnigmaBerries();
    InitEasyChatPhrases();
    ResetTrainerFanClub();
    UnionRoomChat_InitializeRegisteredTexts();
    ResetMiniGamesResults();
    ClearMysteryGift();
    SetAllRenewableItemFlags();
    WarpToPlayersRoom();
    RunScriptImmediately(EventScript_ResetAllMapFlags);
    StringCopy(gSaveBlock1Ptr->rivalName, rivalName);
    ResetTrainerTowerResults();
    hardPrev ? FlagSet(FLAG_HARD) : FlagClear(FLAG_HARD);
    hardcorePrev ? FlagSet(FLAG_HARDCORE) : FlagClear(FLAG_HARDCORE);
    expertPrev ? FlagSet(FLAG_EXPERT) : FlagClear(FLAG_EXPERT);
    SetAdventuresNameLocks(); // HACK
}

// HACK: trava dos personagens do Adventures pelo nome escolhido (jogador ou rival)
static const u8 sAdvName_Red[] = _("RED");
static const u8 sAdvName_Blue[] = _("BLUE");
static const u8 sAdvName_Green[] = _("GREEN");
static const u8 sAdvName_Yellow[] = _("YELLOW");

static u8 ToUpperChar(u8 c)
{
    if (c >= CHAR_a && c <= CHAR_z)
        return c - CHAR_a + CHAR_A;
    return c;
}

static bool8 NameEqualsIgnoreCase(const u8 *name, const u8 *target)
{
    u8 i;

    for (i = 0; i <= PLAYER_NAME_LENGTH; i++)
    {
        if (ToUpperChar(name[i]) != target[i])
            return FALSE;
        if (target[i] == EOS)
            return TRUE;
    }
    return FALSE;
}

static void SetAdventuresNameLocks(void)
{
    static const struct { const u8 *name; u16 flag; } sLocks[] = {
        {sAdvName_Red,    FLAG_HIDE_ADV_RED},
        {sAdvName_Blue,   FLAG_HIDE_ADV_BLUE},
        {sAdvName_Green,  FLAG_HIDE_ADV_GREEN},
        {sAdvName_Yellow, FLAG_HIDE_ADV_YELLOW},
    };
    u8 i;

    for (i = 0; i < ARRAY_COUNT(sLocks); i++)
    {
        if (NameEqualsIgnoreCase(gSaveBlock2Ptr->playerName, sLocks[i].name)
         || NameEqualsIgnoreCase(gSaveBlock1Ptr->rivalName, sLocks[i].name))
            FlagSet(sLocks[i].flag);
        else
            FlagClear(sLocks[i].flag);
    }
}

static void ResetMiniGamesResults(void)
{
    CpuFill16(0, &gSaveBlock2Ptr->berryCrush, sizeof(struct BerryCrush));
    SetBerryPowder(&gSaveBlock2Ptr->berryCrush.berryPowderAmount, 0);
    ResetPokemonJumpRecords();
    CpuFill16(0, &gSaveBlock2Ptr->berryPick, sizeof(struct BerryPickingResults));
}
