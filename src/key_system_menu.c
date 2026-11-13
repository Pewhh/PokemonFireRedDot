#include "global.h"
#include "gflib.h"
#include "scanline_effect.h"
#include "text_window_graphics.h"
#include "menu.h"
#include "task.h"
#include "text_window.h"
#include "strings.h"
#include "main_menu.h"
#include "load_save.h"
#include "key_system_menu.h"

// Key System menu, opened from the main menu (CONTINUE / NEW GAME screen).
// Lets the player switch between FireRed and LeafGreen on the same save file.
// This is option_menu.c cut down to a single setting.
//
// Closing the menu only changes the version in RAM (gSaveBlock2Ptr->activeVersion).
// It reaches the save file the next time the player saves in-game.

// Menu items
enum
{
    MENUITEM_VERSION = 0,
    MENUITEM_EXIT,
    MENUITEM_COUNT
};

// Window Ids
enum
{
    WIN_TEXT_HEADER,
    WIN_OPTIONS,
    WIN_CONTROLS
};

// RAM symbols
struct KeySystemMenu
{
    u16 option[MENUITEM_COUNT];
    u16 cursorPos;
    u8 loadState;
    u8 state;
    u8 loadPaletteState;
};

static EWRAM_DATA struct KeySystemMenu *sKeySystemMenuPtr = NULL;

// Function Declarations
static void CB2_KeySystemMenu_Run(void);
static void VBlankCB_KeySystemMenu(void);
static void CB2_KeySystemMenu_Load(void);
static void InitKeySystemMenuBg(void);
static void KeySystemMenu_PrintControls(void);
static void KeySystemMenu_ResetSpriteData(void);
static bool8 LoadKeySystemMenuPalette(void);
static void Task_KeySystemMenu(u8 taskId);
static u8 KeySystemMenu_ProcessInput(void);
static void BufferKeySystemMenuString(u8 selection);
static void CloseKeySystemMenu(u8 taskId);
static void PrintKeySystemMenuHeader(void);
static void DrawKeySystemMenuBg(void);
static void LoadKeySystemMenuItemNames(void);
static void UpdateSettingSelectionDisplay(u16 selection);

// Data Definitions
static const struct WindowTemplate sKeySystemMenuWinTemplates[] =
{
    [WIN_TEXT_HEADER] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 3,
        .width = 26,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    [WIN_OPTIONS] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 7,
        .width = 26,
        .height = 12,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    [WIN_CONTROLS] = {
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

static const struct BgTemplate sKeySystemMenuBgTemplates[] =
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

static const u16 sKeySystemMenuPalette[] = INCBIN_U16("graphics/misc/option_menu.gbapal");

// How many choices each item has (EXIT has none)
static const u16 sKeySystemMenuItemCounts[MENUITEM_COUNT] = {2, 0};

static const u8 *const sKeySystemMenuItemsNames[MENUITEM_COUNT] =
{
    [MENUITEM_VERSION] = gText_Version,
    [MENUITEM_EXIT]    = gText_KeySystemExit,
};

// Index matches gSaveBlock2Ptr->activeVersion: 0 = FireRed, 1 = LeafGreen
static const u8 *const sVersionOptions[] =
{
    gText_VersionFireRed,
    gText_VersionLeafGreen
};

static const u8 sKeySystemMenuControlsTextColor[] = {TEXT_DYNAMIC_COLOR_6, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY};
static const u8 sKeySystemMenuTextColor[] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_LIGHT_RED, TEXT_COLOR_RED};

// Functions
static void CB2_KeySystemMenu_Run(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_KeySystemMenu(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// Entry point. Called by the main menu with the screen already faded to black.
void CB2_KeySystemMenuFromContinueScreen(void)
{
    sKeySystemMenuPtr = AllocZeroed(sizeof(struct KeySystemMenu));
    sKeySystemMenuPtr->loadState = 0;
    sKeySystemMenuPtr->loadPaletteState = 0;
    sKeySystemMenuPtr->state = 0;
    sKeySystemMenuPtr->cursorPos = 0;
    sKeySystemMenuPtr->option[MENUITEM_VERSION] = IsLeafGreenVersion() ? 1 : 0;
    SetMainCallback2(CB2_KeySystemMenu_Load);
}

// Loads the screen one step per frame, then hands over to Task_KeySystemMenu
static void CB2_KeySystemMenu_Load(void)
{
    u8 i, state;
    state = sKeySystemMenuPtr->state;
    switch (state)
    {
    case 0:
        SetVBlankCallback(NULL);
        SetHBlankCallback(NULL);
        break;
    case 1:
        InitKeySystemMenuBg();
        break;
    case 2:
        KeySystemMenu_ResetSpriteData();
        break;
    case 3:
        if (LoadKeySystemMenuPalette() != TRUE)
            return;
        break;
    case 4:
        PrintKeySystemMenuHeader();
        break;
    case 5:
        DrawKeySystemMenuBg();
        break;
    case 6:
        LoadKeySystemMenuItemNames();
        break;
    case 7:
        for (i = 0; i < MENUITEM_COUNT; i++)
            BufferKeySystemMenuString(i);
        break;
    case 8:
        UpdateSettingSelectionDisplay(sKeySystemMenuPtr->cursorPos);
        break;
    case 9:
        KeySystemMenu_PrintControls();
        break;
    default:
        CreateTask(Task_KeySystemMenu, 0);
        SetMainCallback2(CB2_KeySystemMenu_Run);
        break;
    }
    sKeySystemMenuPtr->state++;
}

static void InitKeySystemMenuBg(void)
{
    void *dest = (void *)VRAM;
    DmaClearLarge16(3, dest, VRAM_SIZE, 0x1000);
    DmaClear32(3, (void *)OAM, OAM_SIZE);
    DmaClear16(3, (void *)PLTT, PLTT_SIZE);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sKeySystemMenuBgTemplates, NELEMS(sKeySystemMenuBgTemplates));
    ChangeBgX(0, 0, 0);
    ChangeBgY(0, 0, 0);
    ChangeBgX(1, 0, 0);
    ChangeBgY(1, 0, 0);
    ChangeBgX(2, 0, 0);
    ChangeBgY(2, 0, 0);
    ChangeBgX(3, 0, 0);
    ChangeBgY(3, 0, 0);
    InitWindows(sKeySystemMenuWinTemplates);
    DeactivateAllTextPrinters();
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_BLEND | BLDCNT_EFFECT_LIGHTEN);
    SetGpuReg(REG_OFFSET_BLDY, BLDCNT_TGT1_BG1);
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_BG2 | WINOUT_WIN01_CLR);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
}

static void KeySystemMenu_PrintControls(void)
{
    s32 x;
    x = 0xE4 - GetStringWidth(FONT_SMALL, gText_PickSwitchCancel, 0);
    FillWindowPixelBuffer(WIN_CONTROLS, PIXEL_FILL(15));
    AddTextPrinterParameterized3(WIN_CONTROLS, FONT_SMALL, x, 0, sKeySystemMenuControlsTextColor, 0, gText_PickSwitchCancel);
    PutWindowTilemap(WIN_CONTROLS);
    CopyWindowToVram(WIN_CONTROLS, COPYWIN_FULL);
}

static void KeySystemMenu_ResetSpriteData(void)
{
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
    ResetTasks();
    ScanlineEffect_Stop();
}

static bool8 LoadKeySystemMenuPalette(void)
{
    switch (sKeySystemMenuPtr->loadPaletteState)
    {
    case 0:
        LoadBgTiles(1, GetUserWindowGraphics(gSaveBlock2Ptr->optionsWindowFrameType)->tiles, 0x120, 0x1AA);
        break;
    case 1:
        LoadPalette(GetUserWindowGraphics(gSaveBlock2Ptr->optionsWindowFrameType)->palette, BG_PLTT_ID(2), PLTT_SIZE_4BPP);
        break;
    case 2:
        LoadPalette(sKeySystemMenuPalette, BG_PLTT_ID(1), sizeof(sKeySystemMenuPalette));
        LoadPalette(GetTextWindowPalette(2), BG_PLTT_ID(15), PLTT_SIZE_4BPP);
        break;
    case 3:
        LoadStdWindowGfxOnBg(1, 0x1B3, BG_PLTT_ID(3));
        break;
    default:
        return TRUE;
    }
    sKeySystemMenuPtr->loadPaletteState++;
    return FALSE;
}

static void Task_KeySystemMenu(u8 taskId)
{
    switch (sKeySystemMenuPtr->loadState)
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB_KeySystemMenu);
        sKeySystemMenuPtr->loadState++;
        break;
    case 1:
        if (gPaletteFade.active)
            return;
        sKeySystemMenuPtr->loadState++;
        break;
    case 2:
        switch (KeySystemMenu_ProcessInput())
        {
        case 0: // nothing happened
            break;
        case 1: // close
            sKeySystemMenuPtr->loadState++;
            break;
        case 3: // cursor moved
            UpdateSettingSelectionDisplay(sKeySystemMenuPtr->cursorPos);
            break;
        case 4: // setting changed
            BufferKeySystemMenuString(sKeySystemMenuPtr->cursorPos);
            break;
        }
        break;
    case 3:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
        sKeySystemMenuPtr->loadState++;
        break;
    case 4:
        if (gPaletteFade.active)
            return;
        sKeySystemMenuPtr->loadState++;
        break;
    case 5:
        CloseKeySystemMenu(taskId);
        break;
    }
}

static u8 KeySystemMenu_ProcessInput(void)
{
    u16 *curr = &sKeySystemMenuPtr->option[sKeySystemMenuPtr->cursorPos];
    u16 count = sKeySystemMenuItemCounts[sKeySystemMenuPtr->cursorPos];

    if (JOY_REPT(DPAD_RIGHT))
    {
        if (count == 0)
            return 0;
        if (*curr == count - 1)
            *curr = 0;
        else
            ++*curr;
        return 4;
    }
    else if (JOY_REPT(DPAD_LEFT))
    {
        if (count == 0)
            return 0;
        if (*curr == 0)
            *curr = count - 1;
        else
            --*curr;
        return 4;
    }
    else if (JOY_REPT(DPAD_UP))
    {
        if (sKeySystemMenuPtr->cursorPos == 0)
            sKeySystemMenuPtr->cursorPos = MENUITEM_COUNT - 1;
        else
            sKeySystemMenuPtr->cursorPos--;
        return 3;
    }
    else if (JOY_REPT(DPAD_DOWN))
    {
        if (sKeySystemMenuPtr->cursorPos == MENUITEM_COUNT - 1)
            sKeySystemMenuPtr->cursorPos = 0;
        else
            sKeySystemMenuPtr->cursorPos++;
        return 3;
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

static void BufferKeySystemMenuString(u8 selection)
{
    u8 dst[3];
    u8 x, y;

    memcpy(dst, sKeySystemMenuTextColor, 3);
    x = 0x82;
    y = ((GetFontAttribute(FONT_NORMAL, FONTATTR_MAX_LETTER_HEIGHT) - 1) * selection) + 2;
    FillWindowPixelRect(WIN_OPTIONS, 1, x, y, 0x46, GetFontAttribute(FONT_NORMAL, FONTATTR_MAX_LETTER_HEIGHT));

    switch (selection)
    {
    case MENUITEM_VERSION:
        AddTextPrinterParameterized3(WIN_OPTIONS, FONT_NORMAL, x, y, dst, -1, sVersionOptions[sKeySystemMenuPtr->option[selection]]);
        break;
    default:
        break;
    }
    PutWindowTilemap(WIN_OPTIONS);
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_FULL);
}

static void CloseKeySystemMenu(u8 taskId)
{
    SetMainCallback2(CB2_InitMainMenu); // back to the CONTINUE / NEW GAME screen
    FreeAllWindowBuffers();
    SetActiveVersion(sKeySystemMenuPtr->option[MENUITEM_VERSION]);
    FREE_AND_SET_NULL(sKeySystemMenuPtr);
    DestroyTask(taskId);
}

static void PrintKeySystemMenuHeader(void)
{
    FillWindowPixelBuffer(WIN_TEXT_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_TEXT_HEADER, FONT_NORMAL, gText_KeySystemSettings, 8, 1, TEXT_SKIP_DRAW, NULL);
    PutWindowTilemap(WIN_TEXT_HEADER);
    CopyWindowToVram(WIN_TEXT_HEADER, COPYWIN_FULL);
}

static void DrawKeySystemMenuBg(void)
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

static void LoadKeySystemMenuItemNames(void)
{
    u8 i;

    FillWindowPixelBuffer(WIN_OPTIONS, PIXEL_FILL(1));
    for (i = 0; i < MENUITEM_COUNT; i++)
    {
        AddTextPrinterParameterized(WIN_OPTIONS, FONT_NORMAL, sKeySystemMenuItemsNames[i], 8, (u8)((i * (GetFontAttribute(FONT_NORMAL, FONTATTR_MAX_LETTER_HEIGHT))) + 2) - i, TEXT_SKIP_DRAW, NULL);
    }
}

static void UpdateSettingSelectionDisplay(u16 selection)
{
    u16 maxLetterHeight, y;

    maxLetterHeight = GetFontAttribute(FONT_NORMAL, FONTATTR_MAX_LETTER_HEIGHT);
    y = selection * (maxLetterHeight - 1) + 0x3A;
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(y, y + maxLetterHeight));
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(0x10, 0xE0));
}
