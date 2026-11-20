// HACK: menu de debug do Fire & Leaf Reminded.
// Só é compilado na versão de teste (make debug / DEBUG=1). A ROM final não tem nada disso.
#include "global.h"

#if DEBUG_MENU
#include "gflib.h"
#include "battle.h"
#include "data.h"
#include "debug.h"
#include "event_data.h"
#include "item.h"
#include "list_menu.h"
#include "menu.h"
#include "money.h"
#include "new_menu_helpers.h"
#include "pokemon.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "task.h"
#include "text_window.h"
#include "constants/battle.h"
#include "constants/debug.h"
#include "constants/items.h"
#include "constants/opponents.h"
#include "constants/songs.h"

extern const u8 Debug_EventScript_Main[];

// EWRAM_DATA: variáveis globais com valor inicial fora da EWRAM caem na seção .data,
// que o ld_script do pokefirered descarta (era o erro do make debug).
EWRAM_DATA bool8 gDebugNoClip = FALSE;
EWRAM_DATA bool8 gDebugNoEncounters = FALSE;
EWRAM_DATA bool8 gDebugNoTrainerSight = FALSE;

static EWRAM_DATA u16 sListCursor[DEBUG_LIST_COUNT] = {0};
static EWRAM_DATA u16 sListScroll[DEBUG_LIST_COUNT] = {0};

// ---------------------------------------------------------------------------
// Textos das listas
// ---------------------------------------------------------------------------
static const u8 sText_Warp[]        = _("WARP");
static const u8 sText_Progress[]    = _("PROGRESS");
static const u8 sText_Pokemon[]     = _("POKéMON");
static const u8 sText_Items[]       = _("ITEMS & MONEY");
static const u8 sText_Battle[]      = _("BATTLE");
static const u8 sText_NoClip[]      = _("WALK THROUGH WALLS");
static const u8 sText_Encounters[]  = _("WILD ENCOUNTERS");
static const u8 sText_Sight[]       = _("TRAINER SIGHT");
static const u8 sText_Close[]       = _("CLOSE");

static const u8 sText_Pallet[]      = _("PALLET (HOME)");
static const u8 sText_Viridian[]    = _("VIRIDIAN CITY");
static const u8 sText_Pewter[]      = _("PEWTER CITY");
static const u8 sText_Route4[]      = _("ROUTE 4");
static const u8 sText_Cerulean[]    = _("CERULEAN CITY");
static const u8 sText_Vermilion[]   = _("VERMILION CITY");
static const u8 sText_Route10[]     = _("ROUTE 10");
static const u8 sText_Lavender[]    = _("LAVENDER TOWN");
static const u8 sText_Celadon[]     = _("CELADON CITY");
static const u8 sText_Fuchsia[]     = _("FUCHSIA CITY");
static const u8 sText_Saffron[]     = _("SAFFRON CITY");
static const u8 sText_Cinnabar[]    = _("CINNABAR ISLAND");
static const u8 sText_Indigo[]      = _("INDIGO PLATEAU");
static const u8 sText_OneIsland[]   = _("ONE ISLAND");
static const u8 sText_GymPewter[]   = _("GYM: BROCK");
static const u8 sText_GymCerulean[] = _("GYM: MISTY");
static const u8 sText_GymVermilion[]= _("GYM: LT. SURGE");
static const u8 sText_GymCeladon[]  = _("GYM: ERIKA");
static const u8 sText_GymFuchsia[]  = _("GYM: KOGA");
static const u8 sText_GymSaffron[]  = _("GYM: SABRINA");
static const u8 sText_GymCinnabar[] = _("GYM: BLAINE");
static const u8 sText_GymViridian[] = _("GYM: GIOVANNI");
static const u8 sText_SevenIsland[] = _("SEVEN ISLAND");
static const u8 sText_TrainerTower[]= _("TRAINER TOWER");

static const u8 sText_Badges[]      = _("BADGES");
static const u8 sText_Difficulty[]  = _("DIFFICULTY");
static const u8 sText_Events[]      = _("EVENTS");

static const u8 sText_Normal[]      = _("NORMAL");
static const u8 sText_Hard[]        = _("HARD");
static const u8 sText_Hardcore[]    = _("HARDCORE");
static const u8 sText_Expert[]      = _("EXPERT");

static const u8 sText_EvPokedex[]   = _("POKéDEX");
static const u8 sText_EvNatDex[]    = _("NATIONAL DEX");
static const u8 sText_EvShoes[]     = _("RUNNING SHOES");
static const u8 sText_EvMom[]       = _("MOM SCENE DONE");
static const u8 sText_EvSurge[]     = _("LT. SURGE BEATEN");
static const u8 sText_EvGameClear[] = _("ELITE FOUR BEATEN");
static const u8 sText_EvChampion[]  = _("CHAMPION REMATCH");
static const u8 sText_EvSevii[]     = _("SEVII 4-7");

static const u8 sText_GiveMon[]     = _("GIVE POKéMON");
static const u8 sText_Heal[]        = _("HEAL PARTY");
static const u8 sText_Hurt[]        = _("HURT PARTY");
static const u8 sText_Dead[]        = _("DEAD: 1ST POKéMON");

static const u8 sText_GiveItem[]    = _("GIVE ITEM");
static const u8 sText_Money[]       = _("MONEY +100000");

static const u8 sText_Trainer[]     = _("TRAINER BATTLE");
static const u8 sText_Wild[]        = _("WILD BATTLE");

static const u8 sText_Space[]       = _(" ");
static const u8 sText_On[]          = _("ON");
static const u8 sText_Off[]         = _("OFF");

static const struct ListMenuItem sList_Main[] = {
    {sText_Warp, 0}, {sText_Progress, 1}, {sText_Pokemon, 2}, {sText_Items, 3}, {sText_Battle, 4},
    {sText_NoClip, 5}, {sText_Encounters, 6}, {sText_Sight, 7}, {sText_Close, 8},
};
static const struct ListMenuItem sList_Warp[] = {
    {sText_Pallet, 0}, {sText_Viridian, 1}, {sText_Pewter, 2}, {sText_Route4, 3}, {sText_Cerulean, 4},
    {sText_Vermilion, 5}, {sText_Route10, 6}, {sText_Lavender, 7}, {sText_Celadon, 8}, {sText_Fuchsia, 9},
    {sText_Saffron, 10}, {sText_Cinnabar, 11}, {sText_Indigo, 12}, {sText_OneIsland, 13},
    {sText_GymPewter, 14}, {sText_GymCerulean, 15}, {sText_GymVermilion, 16}, {sText_GymCeladon, 17},
    {sText_GymFuchsia, 18}, {sText_GymSaffron, 19}, {sText_GymCinnabar, 20}, {sText_GymViridian, 21},
    {sText_SevenIsland, 22}, {sText_TrainerTower, 23},
};
static const struct ListMenuItem sList_Progress[] = {
    {sText_Badges, 0}, {sText_Difficulty, 1}, {sText_Events, 2},
};
static const struct ListMenuItem sList_Difficulty[] = {
    {sText_Normal, 0}, {sText_Hard, 1}, {sText_Hardcore, 2}, {sText_Expert, 3},
};
static const struct ListMenuItem sList_Events[] = {
    {sText_EvPokedex, 0}, {sText_EvNatDex, 1}, {sText_EvShoes, 2}, {sText_EvMom, 3},
    {sText_EvSurge, 4}, {sText_EvGameClear, 5}, {sText_EvChampion, 6},
    {sText_EvSevii, 7},
};
static const struct ListMenuItem sList_Pokemon[] = {
    {sText_GiveMon, 0}, {sText_Heal, 1}, {sText_Hurt, 2}, {sText_Dead, 3},
};
static const struct ListMenuItem sList_Items[] = {
    {sText_GiveItem, 0}, {sText_Money, 1},
};
static const struct ListMenuItem sList_Battle[] = {
    {sText_Trainer, 0}, {sText_Wild, 1},
};

static const struct { const struct ListMenuItem *items; u8 count; } sLists[DEBUG_LIST_COUNT] = {
    [DEBUG_LIST_MAIN]       = {sList_Main,       NELEMS(sList_Main)},
    [DEBUG_LIST_WARP]       = {sList_Warp,       NELEMS(sList_Warp)},
    [DEBUG_LIST_PROGRESS]   = {sList_Progress,   NELEMS(sList_Progress)},
    [DEBUG_LIST_DIFFICULTY] = {sList_Difficulty, NELEMS(sList_Difficulty)},
    [DEBUG_LIST_EVENTS]     = {sList_Events,     NELEMS(sList_Events)},
    [DEBUG_LIST_POKEMON]    = {sList_Pokemon,    NELEMS(sList_Pokemon)},
    [DEBUG_LIST_ITEMS]      = {sList_Items,      NELEMS(sList_Items)},
    [DEBUG_LIST_BATTLE]     = {sList_Battle,     NELEMS(sList_Battle)},
};

#define DEBUG_LIST_MAX_SHOWN 8
#define DEBUG_WINDOW_BASE    0x008

// ---------------------------------------------------------------------------
// Abrir o menu (chamado pelo menu Start)
// ---------------------------------------------------------------------------
void Debug_OpenMenu(void)
{
    ScriptContext_SetupScript(Debug_EventScript_Main);
}

// ---------------------------------------------------------------------------
// Lista (special Debug_ShowList: lista em VAR_0x8004, resultado em VAR_RESULT)
// ---------------------------------------------------------------------------
#define tListId     data[0]
#define tWindowId   data[1]
#define tListTaskId data[2]

static void Task_DebugList_HandleInput(u8 taskId);

void Debug_ShowList(void)
{
    struct ListMenuTemplate template;
    struct WindowTemplate winTemplate;
    u8 taskId, listId = gSpecialVar_0x8004;
    u8 shown = sLists[listId].count < DEBUG_LIST_MAX_SHOWN ? sLists[listId].count : DEBUG_LIST_MAX_SHOWN;

    taskId = CreateTask(Task_DebugList_HandleInput, 80);
    gTasks[taskId].tListId = listId;

    winTemplate = SetWindowTemplateFields(0, 1, 1, 14, shown * 2, 15, DEBUG_WINDOW_BASE);
    gTasks[taskId].tWindowId = AddWindow(&winTemplate);
    SetStdWindowBorderStyle(gTasks[taskId].tWindowId, FALSE);

    template.items = sLists[listId].items;
    template.moveCursorFunc = NULL;
    template.itemPrintFunc = NULL;
    template.totalItems = sLists[listId].count;
    template.maxShowed = shown;
    template.windowId = gTasks[taskId].tWindowId;
    template.header_X = 0;
    template.item_X = 8;
    template.cursor_X = 0;
    template.upText_Y = 0;
    template.cursorPal = 2;
    template.fillValue = 1;
    template.cursorShadowPal = 3;
    template.lettersSpacing = 1;
    template.itemVerticalPadding = 0;
    template.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    template.fontId = FONT_NORMAL;
    template.cursorKind = 0;

    if (sListCursor[listId] >= template.totalItems)
        sListCursor[listId] = sListScroll[listId] = 0;
    gTasks[taskId].tListTaskId = ListMenuInit(&template, sListScroll[listId], sListCursor[listId]);
    PutWindowTilemap(gTasks[taskId].tWindowId);
    CopyWindowToVram(gTasks[taskId].tWindowId, COPYWIN_FULL);
}

static void DebugList_Close(u8 taskId)
{
    u8 listId = gTasks[taskId].tListId;

    DestroyListMenuTask(gTasks[taskId].tListTaskId, &sListScroll[listId], &sListCursor[listId]);
    ClearStdWindowAndFrameToTransparent(gTasks[taskId].tWindowId, TRUE);
    RemoveWindow(gTasks[taskId].tWindowId);
    DestroyTask(taskId);
    ScriptContext_Enable();
}

static void Task_DebugList_HandleInput(u8 taskId)
{
    s32 input = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);

    if (input == LIST_NOTHING_CHOSEN)
        return;
    PlaySE(SE_SELECT);
    gSpecialVar_Result = (input == LIST_CANCEL) ? DEBUG_LIST_CANCEL : input;
    DebugList_Close(taskId);
}

#undef tListId
#undef tWindowId
#undef tListTaskId

// ---------------------------------------------------------------------------
// Número (special Debug_InputNumber: tipo em VAR_0x8004, resultado em VAR_RESULT)
// Direcional: esquerda/direita ±1, cima/baixo ±10, L/R ±100. A confirma, B cancela.
// ---------------------------------------------------------------------------
#define tType     data[0]
#define tWindowId data[1]
#define tValue    data[2]
#define tMin      data[3]
#define tMax      data[4]

static const u8 sText_NumberHelp[] = _("{DPAD_LEFTRIGHT}1  {DPAD_UPDOWN}10  L/R 100");

static void Task_DebugNumber_HandleInput(u8 taskId);

static void DebugNumber_Print(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    u8 windowId = task->tWindowId;
    u16 value = task->tValue;

    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    ConvertIntToDecimalStringN(gStringVar1, value, STR_CONV_MODE_LEFT_ALIGN, 4);
    AddTextPrinterParameterized(windowId, FONT_NORMAL, gStringVar1, 0, 1, TEXT_SKIP_DRAW, NULL);
    switch (task->tType)
    {
    case DEBUG_NUM_SPECIES:
        AddTextPrinterParameterized(windowId, FONT_NORMAL, gSpeciesNames[value], 40, 1, TEXT_SKIP_DRAW, NULL);
        break;
    case DEBUG_NUM_ITEM:
        AddTextPrinterParameterized(windowId, FONT_NORMAL, ItemId_GetName(value), 40, 1, TEXT_SKIP_DRAW, NULL);
        break;
    case DEBUG_NUM_TRAINER:
        StringCopy(gStringVar2, gTrainerClassNames[gTrainers[value].trainerClass]);
        StringAppend(gStringVar2, sText_Space);
        StringAppend(gStringVar2, gTrainers[value].trainerName);
        AddTextPrinterParameterized(windowId, FONT_NORMAL, gStringVar2, 40, 1, TEXT_SKIP_DRAW, NULL);
        break;
    }
    AddTextPrinterParameterized(windowId, FONT_SMALL, sText_NumberHelp, 0, 17, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(windowId, COPYWIN_FULL);
}

void Debug_InputNumber(void)
{
    struct WindowTemplate winTemplate;
    u8 taskId = CreateTask(Task_DebugNumber_HandleInput, 80);
    struct Task *task = &gTasks[taskId];

    task->tType = gSpecialVar_0x8004;
    switch (task->tType)
    {
    case DEBUG_NUM_BADGES:   task->tMin = 0; task->tMax = NUM_BADGES;          task->tValue = 0;  break;
    case DEBUG_NUM_SPECIES:  task->tMin = 1; task->tMax = NUM_SPECIES - 1;     task->tValue = 1;  break;
    case DEBUG_NUM_LEVEL:    task->tMin = 1; task->tMax = MAX_LEVEL;           task->tValue = 50; break;
    case DEBUG_NUM_ITEM:     task->tMin = 1; task->tMax = ITEMS_COUNT - 1;     task->tValue = 1;  break;
    case DEBUG_NUM_QUANTITY: task->tMin = 1; task->tMax = 99;                  task->tValue = 1;  break;
    default:                 task->tMin = 1; task->tMax = NUM_TRAINERS - 1;    task->tValue = 1;  break;
    }

    winTemplate = SetWindowTemplateFields(0, 1, 1, 22, 4, 15, DEBUG_WINDOW_BASE);
    task->tWindowId = AddWindow(&winTemplate);
    SetStdWindowBorderStyle(task->tWindowId, FALSE);
    PutWindowTilemap(task->tWindowId);
    DebugNumber_Print(taskId);
}

static void Task_DebugNumber_HandleInput(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    s32 value = task->tValue;
    s32 delta = 0;

    if (JOY_NEW(A_BUTTON) || JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        gSpecialVar_Result = JOY_NEW(A_BUTTON) ? task->tValue : DEBUG_NUM_CANCEL;
        ClearStdWindowAndFrameToTransparent(task->tWindowId, TRUE);
        RemoveWindow(task->tWindowId);
        DestroyTask(taskId);
        ScriptContext_Enable();
        return;
    }
    if (JOY_REPT(DPAD_RIGHT))      delta = 1;
    else if (JOY_REPT(DPAD_LEFT))  delta = -1;
    else if (JOY_REPT(DPAD_UP))    delta = 10;
    else if (JOY_REPT(DPAD_DOWN))  delta = -10;
    else if (JOY_REPT(R_BUTTON))   delta = 100;
    else if (JOY_REPT(L_BUTTON))   delta = -100;
    if (delta == 0)
        return;

    value += delta;
    if (value > task->tMax)
        value = (delta == 1) ? task->tMin : task->tMax;
    if (value < task->tMin)
        value = (delta == -1) ? task->tMax : task->tMin;
    task->tValue = value;
    PlaySE(SE_SELECT);
    DebugNumber_Print(taskId);
}

#undef tType
#undef tWindowId
#undef tValue
#undef tMin
#undef tMax

// ---------------------------------------------------------------------------
// Ações
// ---------------------------------------------------------------------------
static void SetOnOffString(bool8 on)
{
    StringCopy(gStringVar1, on ? sText_On : sText_Off);
}

void Debug_ToggleNoClip(void)
{
    gDebugNoClip ^= TRUE;
    SetOnOffString(gDebugNoClip);
}

void Debug_ToggleEncounters(void)
{
    gDebugNoEncounters ^= TRUE;
    SetOnOffString(!gDebugNoEncounters);
}

void Debug_ToggleTrainerSight(void)
{
    gDebugNoTrainerSight ^= TRUE;
    SetOnOffString(!gDebugNoTrainerSight);
}

// VAR_RESULT = quantidade de insígnias (0 a 8)
void Debug_SetBadges(void)
{
    u32 i;

    for (i = 0; i < NUM_BADGES; i++)
    {
        if (i < gSpecialVar_Result)
            FlagSet(FLAG_BADGE01_GET + i);
        else
            FlagClear(FLAG_BADGE01_GET + i);
    }
}

// VAR_RESULT = 0 Normal, 1 Hard, 2 Hardcore, 3 Expert (mesma regra da escolha com o Oak)
void Debug_SetDifficulty(void)
{
    u16 mode = gSpecialVar_Result;

    if (mode >= 1) FlagSet(FLAG_HARD);     else FlagClear(FLAG_HARD);
    if (mode >= 2) FlagSet(FLAG_HARDCORE); else FlagClear(FLAG_HARDCORE);
    if (mode >= 3) FlagSet(FLAG_EXPERT);   else FlagClear(FLAG_EXPERT);
}

static bool8 ToggleFlag(u16 flag)
{
    if (FlagGet(flag))
        FlagClear(flag);
    else
        FlagSet(flag);
    return FlagGet(flag);
}

// VAR_RESULT = evento da lista DEBUG_LIST_EVENTS. Deixa ON/OFF em STR_VAR_1.
void Debug_ToggleEvent(void)
{
    bool8 on = FALSE;

    switch (gSpecialVar_Result)
    {
    case 0: on = ToggleFlag(FLAG_SYS_POKEDEX_GET); break;
    case 1: EnableNationalPokedex(); on = TRUE; break; // só liga
    case 2: on = ToggleFlag(FLAG_SYS_B_DASH); break;
    case 3:
        on = (VarGet(VAR_MAP_SCENE_PALLET_MOM) == 0);
        VarSet(VAR_MAP_SCENE_PALLET_MOM, on ? 1 : 0);
        break;
    case 4: on = ToggleFlag(FLAG_DEFEATED_LT_SURGE); break;
    case 5: on = ToggleFlag(FLAG_SYS_GAME_CLEAR); break;
    case 6: on = ToggleFlag(FLAG_BEAT_CHAMPION_REMATCH); break;
    case 7: // só liga: missão do CELIO concluída (barco vai às ilhas 4-7, mapa com as páginas 4-7)
        VarSet(VAR_MAP_SCENE_ONE_ISLAND_POKEMON_CENTER_1F, 5);
        FlagSet(FLAG_SYS_SEVII_MAP_123);
        FlagSet(FLAG_SYS_SEVII_MAP_4567);
        RemoveBagItem(ITEM_TRI_PASS, 1);
        RemoveBagItem(ITEM_RUBY, 1);
        if (!CheckBagHasItem(ITEM_RAINBOW_PASS, 1))
            AddBagItem(ITEM_RAINBOW_PASS, 1);
        on = TRUE;
        break;
    }
    SetOnOffString(on);
}

// VAR_0x8008 = espécie, VAR_0x8009 = nível. VAR_RESULT = 0 time, 1 PC, 2 sem espaço.
void Debug_GiveMon(void)
{
    gSpecialVar_Result = ScriptGiveMon(gSpecialVar_0x8008, gSpecialVar_0x8009, ITEM_NONE, 0, 0, 0);
}

// Para testar a enfermeira: HP 1, envenenado e sem PP no 1º golpe (mortos ficam como estão)
void Debug_HurtParty(void)
{
    u32 i;
    u16 hp = 1;
    u32 status = STATUS1_POISON;
    u8 pp = 0;

    for (i = 0; i < gPlayerPartyCount; i++)
    {
        struct Pokemon *mon = &gPlayerParty[i];
        if (GetMonData(mon, MON_DATA_IS_EGG) || GetMonData(mon, MON_DATA_HP) == 0)
            continue;
        SetMonData(mon, MON_DATA_HP, &hp);
        SetMonData(mon, MON_DATA_STATUS, &status);
        SetMonData(mon, MON_DATA_PP1, &pp);
    }
}

// Marca/desmarca o 1º Pokémon do time como morto (Hardcore/Expert). Deixa ON/OFF em STR_VAR_1.
void Debug_ToggleDeadFirst(void)
{
    struct Pokemon *mon = &gPlayerParty[0];
    bool8 dead = !GetMonData(mon, MON_DATA_DEAD);
    u16 hp = dead ? 0 : GetMonData(mon, MON_DATA_MAX_HP);

    SetMonData(mon, MON_DATA_DEAD, &dead);
    SetMonData(mon, MON_DATA_HP, &hp);
    SetOnOffString(dead);
}

// VAR_0x8008 = item, VAR_0x8009 = quantidade. VAR_RESULT = TRUE se coube.
void Debug_GiveItem(void)
{
    gSpecialVar_Result = AddBagItem(gSpecialVar_0x8008, gSpecialVar_0x8009);
}

void Debug_AddMoney(void)
{
    AddMoney(&gSaveBlock1Ptr->money, 100000);
}

// VAR_0x8008 = espécie, VAR_0x8009 = nível
void Debug_PrepareWildBattle(void)
{
    CreateScriptedWildMon(gSpecialVar_0x8008, gSpecialVar_0x8009, ITEM_NONE);
}

#endif // DEBUG_MENU
