#include "global.h"
#include "gflib.h"
#include "scanline_effect.h"
#include "text_window_graphics.h"
#include "menu.h"
#include "task.h"
#include "overworld.h"
#include "help_system.h"
#include "text_window.h"
#include "strings.h"
#include "field_fadetransition.h"
#include "gba/m4a_internal.h"
#include "constants/flags.h"
#include "event_data.h"
#include "menu_indicators.h"
#include "list_menu.h"
#include "load_save.h"
#include "main_menu.h"
#include "safari_zone.h"
#include "link.h"
#include "start_menu.h"

// can't include the one in menu_helpers.h since Task_OptionMenu needs bool32 for matching
bool32 IsActiveOverworldLinkBusy(void);

// Menu items
enum
{
    MENUITEM_VERSION = 0, // HACK: troca FIRERED/LEAFGREEN (no jogo, salva e reinicia)
    MENUITEM_TEXTSPEED,
    MENUITEM_BATTLESCENE,
    MENUITEM_BATTLESTYLE,
    MENUITEM_SOUND,
    MENUITEM_BUTTONMODE,
    MENUITEM_FRAMETYPE,
    MENUITEM_WORLDSPEED, // HACK
    MENUITEM_AUTORUN,    // HACK
    MENUITEM_CANCEL,
    MENUITEM_COUNT
};

// HACK: o menu passou a rolar. Cabem 7 linhas na janela; as demais aparecem rolando.
#define OPTIONS_VISIBLE_ROWS 7

static const u8 sText_WorldSpeed[]   = _("WORLD SPEED");
static const u8 sText_WorldSpeed1x[] = _("1x");
static const u8 sText_WorldSpeed2x[] = _("2x");
static const u8 sText_AutoRun[]      = _("AUTO RUN");
static const u8 sText_Off[]          = _("OFF");
static const u8 sText_On[]           = _("ON");

// HACK: layout dos valores — todas as escolhas na linha, a atual em vermelho
#define OPTION_VALUE_X     88
#define OPTION_VALUE_WIDTH 116
static EWRAM_DATA bool8 sOptionsFromMainMenu = FALSE;


// Window Ids
enum
{
    WIN_TEXT_OPTION,
    WIN_OPTIONS
};

// RAM symbols
struct OptionMenu
{
    /*0x00*/ u16 option[MENUITEM_COUNT];
    /*0x0E*/ u16 cursorPos;
    /*0x10*/ u8 loadState;
    /*0x11*/ u8 state;
    /*0x12*/ u8 loadPaletteState;
    u16 scroll;       // HACK: primeira linha visível
    u8 arrowTaskId;   // HACK: setas de rolagem
    u8 firstItem;     // HACK: 0 com VERSION (tela inicial), 1 sem
    u16 arrowOffset;  // HACK: rolagem contada a partir do primeiro item, para as setas
};

static EWRAM_DATA struct OptionMenu *sOptionMenuPtr = NULL;

//Function Declarataions
static void CB2_InitOptionMenu(void);
static void VBlankCB_OptionMenu(void);
static void OptionMenu_InitCallbacks(void);
static void OptionMenu_SetVBlankCallback(void);
static void CB2_OptionMenu(void);
static void SetOptionMenuTask(void);
static void InitOptionMenuBg(void);
static void OptionMenu_PickSwitchCancel(void);
static void OptionMenu_ResetSpriteData(void);
static bool8 LoadOptionMenuPalette(void);
static void Task_OptionMenu(u8 taskId);
static u8 OptionMenu_ProcessInput(void);
static void BufferOptionMenuString(u8 selection);
static void CloseAndSaveOptionMenu(u8 taskId);
static void PrintOptionMenuHeader(void);
static void DrawOptionMenuBg(void);
static void LoadOptionMenuItemNames(void);
static void UpdateSettingSelectionDisplay(u16 selection);
static bool8 UpdateOptionMenuScroll(void);
static void RedrawVisibleOptionRows(void);

// Data Definitions
static const struct WindowTemplate sOptionMenuWinTemplates[] =
{
    {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 3,
        .width = 26,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 7,
        .width = 26,
        .height = 12,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    {
        .bg = 2,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 30,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x16e
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sOptionMenuBgTemplates[] =
{
   {
       .bg = 1,
       .charBaseIndex = 1,
       .mapBaseIndex = 30,
       .screenSize = 0,
       .paletteMode = 0,
       .priority = 0,
       .baseTile = 0
   },
   {
       .bg = 0,
       .charBaseIndex = 1,
       .mapBaseIndex = 31,
       .screenSize = 0,
       .paletteMode = 0,
       .priority = 1,
       .baseTile = 0
   },
   {
       .bg = 2,
       .charBaseIndex = 1,
       .mapBaseIndex = 29,
       .screenSize = 0,
       .paletteMode = 0,
       .priority = 2,
       .baseTile = 0
   },
};

static const u16 sOptionMenuPalette[] = INCBIN_U16("graphics/misc/option_menu.gbapal");
static const u16 sOptionMenuItemCounts[MENUITEM_COUNT] = {2, 3, 2, 2, 2, 3, 10, 2, 2, 0}; // HACK: texto MID, FAST, INSTANT (sem SLOW)

static const u8 *const sOptionMenuItemsNames[MENUITEM_COUNT] =
{
    [MENUITEM_VERSION]     = gText_Version,
    [MENUITEM_TEXTSPEED]   = gText_TextSpeed,
    [MENUITEM_BATTLESCENE] = gText_BattleScene,
    [MENUITEM_BATTLESTYLE] = gText_BattleStyle,
    [MENUITEM_SOUND]       = gText_Sound,
    [MENUITEM_BUTTONMODE]  = gText_ButtonMode,
    [MENUITEM_FRAMETYPE]   = gText_Frame,
    [MENUITEM_WORLDSPEED]  = sText_WorldSpeed,
    [MENUITEM_AUTORUN]     = sText_AutoRun,
    [MENUITEM_CANCEL]      = gText_OptionMenuCancel,
};

static const u8 *const sTextSpeedOptions[] =
{
    // HACK: SLOW removido. Posição no menu = velocidade - 1 (MID=1, FAST=2, INSTANT=3)
    gText_TextSpeedMid, 
    gText_TextSpeedFast,
    gText_TextSpeedInstant
};

static const u8 *const sBattleSceneOptions[] =
{
    gText_BattleSceneOn, 
    gText_BattleSceneOff
};

static const u8 *const sBattleStyleOptions[] =
{
    gText_BattleStyleShift,
    gText_BattleStyleSet
};

static const u8 *const sBattleStyleOptions2[] =
{
    gText_BattleStyleSet,
    gText_BattleStyleSet
};

static const u8 *const sWorldSpeedOptions[] =
{
    sText_WorldSpeed1x,
    sText_WorldSpeed2x
};

static const u8 *const sVersionOptions[] =
{
    gText_VersionFireRed,
    gText_VersionLeafGreen
};

static const u8 *const sBattleStyleOptionsHard[] =
{
    gText_BattleStyleSet
};

static const u8 *const sOffOnOptions[] =
{
    sText_Off,
    sText_On
};

static const u8 *const sSoundOptions[] =
{
    gText_SoundMono, 
    gText_SoundStereo
};

static const u8 *const sButtonTypeOptions[] =
{
    gText_ButtonTypeHelp,
	gText_ButtonTypeLR,
	gText_ButtonTypeLEqualsA
};

static const u8 sOptionMenuPickSwitchCancelTextColor[] = {TEXT_DYNAMIC_COLOR_6, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY};
static const u8 sOptionMenuTextColor[] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_LIGHT_RED, TEXT_COLOR_RED};
static const u8 sOptionMenuUnselectedTextColor[] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};

// Functions
static void CB2_InitOptionMenu(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_OptionMenu(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static bool8 CanChangeVersionInGame(void)
{
    return !GetSafariZoneFlag() && InUnionRoom() != TRUE && IsUpdateLinkStateCBActive() != TRUE;
}

static void OptionMenu_Open(void)
{
    u8 i;

    sOptionMenuPtr = AllocZeroed(sizeof(struct OptionMenu));
    sOptionMenuPtr->loadState = 0;
    sOptionMenuPtr->loadPaletteState = 0;
    sOptionMenuPtr->state = 0;
    // VERSION some onde não dá para salvar (SAFARI ZONE, link, UNION ROOM)
    sOptionMenuPtr->firstItem = (sOptionsFromMainMenu || CanChangeVersionInGame()) ? MENUITEM_VERSION : MENUITEM_TEXTSPEED;
    sOptionMenuPtr->cursorPos = sOptionMenuPtr->firstItem;
    sOptionMenuPtr->option[MENUITEM_VERSION] = IsLeafGreenVersion() ? 1 : 0;
    sOptionMenuPtr->option[MENUITEM_TEXTSPEED] = (gSaveBlock2Ptr->optionsTextSpeed > OPTIONS_TEXT_SPEED_SLOW) ? gSaveBlock2Ptr->optionsTextSpeed - 1 : 0;
    sOptionMenuPtr->option[MENUITEM_BATTLESCENE] = gSaveBlock2Ptr->optionsBattleSceneOff;
    sOptionMenuPtr->option[MENUITEM_BATTLESTYLE] = gSaveBlock2Ptr->optionsBattleStyle;
    sOptionMenuPtr->option[MENUITEM_SOUND] = gSaveBlock2Ptr->optionsSound;
    sOptionMenuPtr->option[MENUITEM_BUTTONMODE] = gSaveBlock2Ptr->optionsButtonMode;
    sOptionMenuPtr->option[MENUITEM_FRAMETYPE] = gSaveBlock2Ptr->optionsWindowFrameType;
    sOptionMenuPtr->option[MENUITEM_WORLDSPEED] = gSaveBlock2Ptr->optionsWorldSpeed;
    sOptionMenuPtr->option[MENUITEM_AUTORUN] = gSaveBlock2Ptr->optionsAutoRun;
    sOptionMenuPtr->scroll = sOptionMenuPtr->firstItem;
    sOptionMenuPtr->arrowOffset = 0;

    for (i = 0; i < MENUITEM_COUNT - 1; i++)
    {
        if (sOptionMenuPtr->option[i] > (sOptionMenuItemCounts[i]) - 1)
            sOptionMenuPtr->option[i] = 0;
    }
    if (!sOptionsFromMainMenu)
        SetHelpContext(HELPCONTEXT_OPTIONS);
    SetMainCallback2(CB2_OptionMenu);
}

void CB2_OptionsMenuFromStartMenu(void)
{
    if (gMain.savedCallback == NULL)
        gMain.savedCallback = CB2_ReturnToFieldWithOpenMenu;
    sOptionsFromMainMenu = FALSE;
    OptionMenu_Open();
}

// HACK: OPTION na tela de CONTINUE / NEW GAME (no lugar do KEY SYSTEM SETTINGS), com a troca de versão
void CB2_OptionsMenuFromMainMenu(void)
{
    gMain.savedCallback = CB2_InitMainMenu;
    sOptionsFromMainMenu = TRUE;
    OptionMenu_Open();
}

static void OptionMenu_InitCallbacks(void)
{
    SetVBlankCallback(NULL);
    SetHBlankCallback(NULL);
}

static void OptionMenu_SetVBlankCallback(void)
{
    SetVBlankCallback(VBlankCB_OptionMenu);
}

static void CB2_OptionMenu(void)
{
    u8 i, state;
    state = sOptionMenuPtr->state;
    switch (state)
    {
    case 0:
        OptionMenu_InitCallbacks();
        break;
    case 1:
        InitOptionMenuBg();
        break;
    case 2:
        OptionMenu_ResetSpriteData();
        break;
    case 3:
        if (LoadOptionMenuPalette() != TRUE)
            return;
        break;
    case 4:
        PrintOptionMenuHeader();
        break;
    case 5:
        DrawOptionMenuBg();
        break;
    case 6:
        LoadOptionMenuItemNames();
        break;
    case 7:
        for (i = 0; i < MENUITEM_COUNT; i++)
            BufferOptionMenuString(i);
        break;
    case 8:
        UpdateSettingSelectionDisplay(sOptionMenuPtr->cursorPos);
        break;
    case 9:
        OptionMenu_PickSwitchCancel();
        break;
    default:
        SetOptionMenuTask();
		break;
    }
    sOptionMenuPtr->state++;
}

static void SetOptionMenuTask(void)
{
    CreateTask(Task_OptionMenu, 0);
    // HACK: setas de rolagem (aparecem quando há linhas escondidas acima/abaixo)
    sOptionMenuPtr->arrowTaskId = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_UP, 120, 52, 156, MENUITEM_COUNT - sOptionMenuPtr->firstItem - OPTIONS_VISIBLE_ROWS, 110, 110, &sOptionMenuPtr->arrowOffset);
    SetMainCallback2(CB2_InitOptionMenu);
}

static void InitOptionMenuBg(void)
{
    void *dest = (void *)VRAM;
    DmaClearLarge16(3, dest, VRAM_SIZE, 0x1000);    
    DmaClear32(3, (void *)OAM, OAM_SIZE);
    DmaClear16(3, (void *)PLTT, PLTT_SIZE);    
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sOptionMenuBgTemplates, NELEMS(sOptionMenuBgTemplates));
    ChangeBgX(0, 0, 0);
    ChangeBgY(0, 0, 0);
    ChangeBgX(1, 0, 0);
    ChangeBgY(1, 0, 0);
    ChangeBgX(2, 0, 0);
    ChangeBgY(2, 0, 0);
    ChangeBgX(3, 0, 0);
    ChangeBgY(3, 0, 0);
    InitWindows(sOptionMenuWinTemplates);
    DeactivateAllTextPrinters();
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_BLEND | BLDCNT_EFFECT_LIGHTEN);
    SetGpuReg(REG_OFFSET_BLDY, BLDCNT_TGT1_BG1);
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_BG2 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR); // HACK: OBJ para as setas
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
};

static void OptionMenu_PickSwitchCancel(void)
{
    s32 x;
    x = 0xE4 - GetStringWidth(FONT_SMALL, gText_PickSwitchCancel, 0);
    FillWindowPixelBuffer(2, PIXEL_FILL(15)); 
    AddTextPrinterParameterized3(2, FONT_SMALL, x, 0, sOptionMenuPickSwitchCancelTextColor, 0, gText_PickSwitchCancel);
    PutWindowTilemap(2);
    CopyWindowToVram(2, COPYWIN_FULL);
}

static void OptionMenu_ResetSpriteData(void)
{
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
    ResetTasks();
    ScanlineEffect_Stop();
}

static bool8 LoadOptionMenuPalette(void)
{
    switch (sOptionMenuPtr->loadPaletteState)
    {
    case 0:
        LoadBgTiles(1, GetUserWindowGraphics(sOptionMenuPtr->option[MENUITEM_FRAMETYPE])->tiles, 0x120, 0x1AA);
        break;
    case 1:
        LoadPalette(GetUserWindowGraphics(sOptionMenuPtr->option[MENUITEM_FRAMETYPE])->palette, BG_PLTT_ID(2), PLTT_SIZE_4BPP);
        break;
    case 2:
        LoadPalette(sOptionMenuPalette, BG_PLTT_ID(1), sizeof(sOptionMenuPalette));
        LoadPalette(GetTextWindowPalette(2), BG_PLTT_ID(15), PLTT_SIZE_4BPP);
        break;
    case 3:
        LoadStdWindowGfxOnBg(1, 0x1B3, BG_PLTT_ID(3));
        break;
    default:
        return TRUE;
    }
    sOptionMenuPtr->loadPaletteState++;
    return FALSE;
}

static void Task_OptionMenu(u8 taskId)
{
    switch (sOptionMenuPtr->loadState)
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        OptionMenu_SetVBlankCallback();
        sOptionMenuPtr->loadState++;
        break;
    case 1:
        if (gPaletteFade.active)
            return;
        sOptionMenuPtr->loadState++;
        break;
    case 2:
        if (((bool32)IsActiveOverworldLinkBusy()) == TRUE)
            break;
        switch (OptionMenu_ProcessInput())
        {
        case 0:
            break;
        case 1:
            sOptionMenuPtr->loadState++;
            break;
        case 2:
            LoadBgTiles(1, GetUserWindowGraphics(sOptionMenuPtr->option[MENUITEM_FRAMETYPE])->tiles, 0x120, 0x1AA);
            LoadPalette(GetUserWindowGraphics(sOptionMenuPtr->option[MENUITEM_FRAMETYPE])->palette, BG_PLTT_ID(2), PLTT_SIZE_4BPP);
            BufferOptionMenuString(sOptionMenuPtr->cursorPos);
            break;
        case 3:
            UpdateSettingSelectionDisplay(sOptionMenuPtr->cursorPos);
            break;
        case 4:
            BufferOptionMenuString(sOptionMenuPtr->cursorPos);
            break;
        case 5: // HACK: rolou, redesenha as linhas visíveis
            RedrawVisibleOptionRows();
            UpdateSettingSelectionDisplay(sOptionMenuPtr->cursorPos);
            break;
        }
        break;
    case 3:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
        sOptionMenuPtr->loadState++;
        break;
    case 4:
        if (gPaletteFade.active)
            return;
        sOptionMenuPtr->loadState++;
        break;
    case 5:
        CloseAndSaveOptionMenu(taskId);
        break;
    }
}

static u8 OptionMenu_ProcessInput(void)
{ 
    u16 current;
    u16 *curr;
    if (JOY_REPT(DPAD_RIGHT))
    {
        current = sOptionMenuPtr->option[(sOptionMenuPtr->cursorPos)];
        if (current == (sOptionMenuItemCounts[sOptionMenuPtr->cursorPos] - 1))
            sOptionMenuPtr->option[sOptionMenuPtr->cursorPos] = 0;
        else
            sOptionMenuPtr->option[sOptionMenuPtr->cursorPos] = current + 1;
        if (sOptionMenuPtr->cursorPos == MENUITEM_FRAMETYPE)
            return 2;
        else
            return 4;
    }
    else if (JOY_REPT(DPAD_LEFT))
    {
        curr = &sOptionMenuPtr->option[sOptionMenuPtr->cursorPos];
        if (*curr == 0)
            *curr = sOptionMenuItemCounts[sOptionMenuPtr->cursorPos] - 1;
        else
            --*curr;
        
        if (sOptionMenuPtr->cursorPos == MENUITEM_FRAMETYPE)
            return 2;
        else
            return 4;
    }
    else if (JOY_REPT(DPAD_UP))
    {
        if (sOptionMenuPtr->cursorPos == sOptionMenuPtr->firstItem)
            sOptionMenuPtr->cursorPos = MENUITEM_CANCEL;
        else
            sOptionMenuPtr->cursorPos = sOptionMenuPtr->cursorPos - 1;
        return UpdateOptionMenuScroll() ? 5 : 3;
    }
    else if (JOY_REPT(DPAD_DOWN))
    {
        if (sOptionMenuPtr->cursorPos == MENUITEM_CANCEL)
            sOptionMenuPtr->cursorPos = sOptionMenuPtr->firstItem;
        else
            sOptionMenuPtr->cursorPos = sOptionMenuPtr->cursorPos + 1;
        return UpdateOptionMenuScroll() ? 5 : 3;
    }
    else if (JOY_NEW(B_BUTTON) || JOY_NEW(A_BUTTON))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// HACK: desenha todas as escolhas da linha; a atual em vermelho, as outras em cinza.
// A primeira fica à esquerda, a última encostada à direita e as do meio distribuídas.
static void PrintOptionChoices(u8 y, const u8 *const *choices, u8 count, u8 selected)
{
    u8 i;
    s32 widths[4], total = 0, gap = 0, x = OPTION_VALUE_X;

    for (i = 0; i < count; i++)
    {
        widths[i] = GetStringWidth(FONT_NORMAL, choices[i], 0);
        total += widths[i];
    }
    if (count > 1)
        gap = (OPTION_VALUE_WIDTH - total) / (count - 1);
    for (i = 0; i < count; i++)
    {
        if (i == count - 1 && count > 1)
            x = OPTION_VALUE_X + OPTION_VALUE_WIDTH - widths[i];
        AddTextPrinterParameterized3(WIN_OPTIONS, FONT_NORMAL, x, y,
                                     i == selected ? sOptionMenuTextColor : sOptionMenuUnselectedTextColor, -1, choices[i]);
        x += widths[i] + gap;
    }
}

static void BufferOptionMenuString(u8 selection)
{
    u8 str[20];
    u8 buf[12];
    u8 y;
    u16 value;
    const u8 *single[1];

    // HACK: só desenha se a linha estiver visível; a posição vem da linha na tela
    if (selection < sOptionMenuPtr->scroll || selection >= sOptionMenuPtr->scroll + OPTIONS_VISIBLE_ROWS)
        return;
    value = sOptionMenuPtr->option[selection];
    y = ((GetFontAttribute(FONT_NORMAL, FONTATTR_MAX_LETTER_HEIGHT) - 1) * (selection - sOptionMenuPtr->scroll)) + 2;
    FillWindowPixelRect(WIN_OPTIONS, 1, OPTION_VALUE_X, y, OPTION_VALUE_WIDTH, GetFontAttribute(FONT_NORMAL, FONTATTR_MAX_LETTER_HEIGHT));

    switch (selection)
    {
    case MENUITEM_VERSION:
        PrintOptionChoices(y, sVersionOptions, 2, value);
        break;
    case MENUITEM_TEXTSPEED:
        PrintOptionChoices(y, sTextSpeedOptions, 3, value);
        break;
    case MENUITEM_BATTLESCENE:
        PrintOptionChoices(y, sBattleSceneOptions, 2, value);
        break;
    case MENUITEM_BATTLESTYLE:
        // Nas dificuldades difíceis o estilo é sempre SET
        if (FlagGet(FLAG_HARD) || FlagGet(FLAG_HARDCORE) || FlagGet(FLAG_EXPERT))
            PrintOptionChoices(y, sBattleStyleOptionsHard, 1, 0);
        else
            PrintOptionChoices(y, sBattleStyleOptions, 2, value);
        break;
    case MENUITEM_SOUND:
        PrintOptionChoices(y, sSoundOptions, 2, value);
        break;
    case MENUITEM_BUTTONMODE:
        PrintOptionChoices(y, sButtonTypeOptions, 3, value);
        break;
    case MENUITEM_FRAMETYPE:
        // 10 molduras não cabem na linha: mostra só a atual
        StringCopy(str, gText_FrameType);
        ConvertIntToDecimalStringN(buf, value + 1, 1, 2);
        StringAppendN(str, buf, 3);
        single[0] = str;
        PrintOptionChoices(y, single, 1, 0);
        break;
    case MENUITEM_WORLDSPEED:
        PrintOptionChoices(y, sWorldSpeedOptions, 2, value);
        break;
    case MENUITEM_AUTORUN:
        PrintOptionChoices(y, sOffOnOptions, 2, value);
        break;
    default:
        break;
    }
    PutWindowTilemap(WIN_OPTIONS);
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_FULL);
}

static void CloseAndSaveOptionMenu(u8 taskId)
{
    if (sOptionsFromMainMenu)
    {
        SetActiveVersion(sOptionMenuPtr->option[MENUITEM_VERSION]); // vai para o save no próximo SAVE
    }
    else
    {
        gFieldCallback = FieldCB_DefaultWarpExit;
        // No jogo, a versão só muda depois de salvar e reiniciar: volta ao mapa e pergunta
        if (sOptionMenuPtr->option[MENUITEM_VERSION] != (IsLeafGreenVersion() ? 1 : 0))
        {
            gPendingVersionChange = sOptionMenuPtr->option[MENUITEM_VERSION] + 1;
            gMain.savedCallback = CB2_ReturnToFieldForVersionChange;
        }
    }
    sOptionsFromMainMenu = FALSE;
    SetMainCallback2(gMain.savedCallback);
    FreeAllWindowBuffers();
    gSaveBlock2Ptr->optionsTextSpeed = sOptionMenuPtr->option[MENUITEM_TEXTSPEED] + 1;
    gSaveBlock2Ptr->optionsBattleSceneOff = sOptionMenuPtr->option[MENUITEM_BATTLESCENE];
    gSaveBlock2Ptr->optionsBattleStyle = sOptionMenuPtr->option[MENUITEM_BATTLESTYLE];
    gSaveBlock2Ptr->optionsSound = sOptionMenuPtr->option[MENUITEM_SOUND];
    gSaveBlock2Ptr->optionsButtonMode = sOptionMenuPtr->option[MENUITEM_BUTTONMODE];
    gSaveBlock2Ptr->optionsWindowFrameType = sOptionMenuPtr->option[MENUITEM_FRAMETYPE];
    gSaveBlock2Ptr->optionsWorldSpeed = sOptionMenuPtr->option[MENUITEM_WORLDSPEED];
    gSaveBlock2Ptr->optionsAutoRun = sOptionMenuPtr->option[MENUITEM_AUTORUN];
    RemoveScrollIndicatorArrowPair(sOptionMenuPtr->arrowTaskId);
    SetPokemonCryStereo(gSaveBlock2Ptr->optionsSound);
    FREE_AND_SET_NULL(sOptionMenuPtr);
    DestroyTask(taskId);
}

static void PrintOptionMenuHeader(void)
{
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_TEXT_OPTION, FONT_NORMAL, gText_Option, 8, 1, TEXT_SKIP_DRAW, NULL);
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_FULL);
}

static void DrawOptionMenuBg(void)
{
    u8 h;
    h = 2;
    
    FillBgTilemapBufferRect(1, 0x1B3, 1, 2, 1, 1, 3);
    FillBgTilemapBufferRect(1, 0x1B4, 2, 2, 0x1B, 1, 3);
    FillBgTilemapBufferRect(1, 0x1B5, 0x1C, 2, 1, 1, 3);
    FillBgTilemapBufferRect(1, 0x1B6, 1, 3, 1, h, 3);
    FillBgTilemapBufferRect(1, 0x1B8, 0x1C, 3, 1, h, 3);
    FillBgTilemapBufferRect(1, 0x1B9, 1, 5, 1, 1, 3);
    FillBgTilemapBufferRect(1, 0x1BA, 2, 5, 0x1B, 1, 3);
    FillBgTilemapBufferRect(1, 0x1BB, 0x1C, 5, 1, 1, 3);
    FillBgTilemapBufferRect(1, 0x1AA, 1, 6, 1, 1, h);
    FillBgTilemapBufferRect(1, 0x1AB, 2, 6, 0x1A, 1, h);
    FillBgTilemapBufferRect(1, 0x1AC, 0x1C, 6, 1, 1, h);
    FillBgTilemapBufferRect(1, 0x1AD, 1, 7, 1, 0x10, h);
    FillBgTilemapBufferRect(1, 0x1AF, 0x1C, 7, 1, 0x10, h);
    FillBgTilemapBufferRect(1, 0x1B0, 1, 0x13, 1, 1, h);
    FillBgTilemapBufferRect(1, 0x1B1, 2, 0x13, 0x1A, 1, h);
    FillBgTilemapBufferRect(1, 0x1B2, 0x1C, 0x13, 1, 1, h);
    CopyBgTilemapBufferToVram(1);
}

static void LoadOptionMenuItemNames(void)
{
    u8 i;
    
    FillWindowPixelBuffer(1, PIXEL_FILL(1));
    // HACK: só as linhas visíveis (o menu rola)
    for (i = 0; i < OPTIONS_VISIBLE_ROWS && sOptionMenuPtr->scroll + i < MENUITEM_COUNT; i++)
    {
        AddTextPrinterParameterized(WIN_OPTIONS, FONT_NORMAL, sOptionMenuItemsNames[sOptionMenuPtr->scroll + i], 8, (u8)((i * (GetFontAttribute(FONT_NORMAL, FONTATTR_MAX_LETTER_HEIGHT))) + 2) - i, TEXT_SKIP_DRAW, NULL);
    }
}

// HACK: mantém o cursor dentro das linhas visíveis. Retorna TRUE se rolou.
static bool8 UpdateOptionMenuScroll(void)
{
    u16 cursor = sOptionMenuPtr->cursorPos;
    u16 oldScroll = sOptionMenuPtr->scroll;

    if (cursor < sOptionMenuPtr->scroll)
        sOptionMenuPtr->scroll = cursor;
    else if (cursor >= sOptionMenuPtr->scroll + OPTIONS_VISIBLE_ROWS)
        sOptionMenuPtr->scroll = cursor - OPTIONS_VISIBLE_ROWS + 1;
    sOptionMenuPtr->arrowOffset = sOptionMenuPtr->scroll - sOptionMenuPtr->firstItem;
    return sOptionMenuPtr->scroll != oldScroll;
}

static void RedrawVisibleOptionRows(void)
{
    u8 i;

    LoadOptionMenuItemNames();
    for (i = 0; i < MENUITEM_COUNT; i++)
        BufferOptionMenuString(i);
}

static void UpdateSettingSelectionDisplay(u16 selection)
{
    u16 maxLetterHeight, y;
    
    maxLetterHeight = GetFontAttribute(FONT_NORMAL, FONTATTR_MAX_LETTER_HEIGHT);
    y = (selection - sOptionMenuPtr->scroll) * (maxLetterHeight - 1) + 0x3A; // HACK: linha na tela
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(y, y + maxLetterHeight));
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(0x10, 0xE0));
}
