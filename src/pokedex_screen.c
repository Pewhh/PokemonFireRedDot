#include "global.h"
#include "load_save.h"
#include "gflib.h"
#include "graphics.h"
#include "m4a.h"
#include "scanline_effect.h"
#include "task.h"
#include "new_menu_helpers.h"
#include "event_data.h"
#include "help_system.h"
#include "menu_indicators.h"
#include "overworld.h"
#include "strings.h"
#include "menu.h"
#include "pokedex_screen.h"
#include "data.h"
#include "pokedex.h"
#include "trainer_pokemon_sprites.h"
#include "decompress.h"
#include "constants/songs.h"
#include "constants/sound.h"
#include "pokedex_area_markers.h"
#include "field_specials.h"
#include "battle.h"
#include "battle_main.h"
#include "item.h"
#include "party_menu.h"
#include "pokemon_storage_system.h"
#include "constants/items.h"
#include "constants/party_menu.h"
#include "constants/moves.h"
#include "constants/map_types.h"
#include "pokemon_icon.h"
#include "wild_encounter.h"
#include "region_map.h"

#define TAG_AREA_MARKERS 2001

enum TextMode {
    TEXT_LEFT,
    TEXT_CENTER,
    TEXT_RIGHT
};

struct PokedexScreenData
{
    u8 taskId;
    u8 state;
    u8 data[2];
    u8 areaMarkersTaskId;
    u32 unlockedCategories;
    u32 modeSelectInput;
    u16 modeSelectItemsAbove;
    u16 modeSelectCursorPos;
    u8 modeSelectWindowId;
    u8 selectionIconWindowId;
    u8 dexCountsWindowId;
    u8 modeSelectListMenuId;
    u16 pageSpecies[4];
    u8 categoryMonWindowIds[4];
    u8 categoryMonInfoWindowIds[4];
    u8 category;
    u8 firstPageInCategory;
    u8 lastPageInCategory;
    u8 pageNum;
    u8 numMonsOnPage;
    u8 categoryCursorPosInPage;
    u8 categoryPageSelectionCursorTimer;
    u8 parentOfCategoryMenu;
    u32 characteristicMenuInput;
    u16 kantoOrderMenuItemsAbove;
    u16 kantoOrderMenuCursorPos;
    u16 characteristicOrderMenuItemsAbove;
    u16 characteristicOrderMenuCursorPos;
    u16 nationalOrderMenuItemsAbove;
    u16 nationalOrderMenuCursorPos;
    u8 numericalOrderWindowId;
    u8 orderedListMenuTaskId;
    u8 dexOrderId;
    struct ListMenuItem * listItems;
    u16 orderedDexCount;
    u8 windowIds[0x10];
    u16 dexSpecies;
    u16 * bgBufsMem;
    u8 scrollArrowsTaskId;
    u8 categoryPageCursorTaskId;
    u16 modeSelectCursorPosBak;
    u8 unlockedSeviiAreas;
    u16 numSeenKanto;
    u16 numOwnedKanto;
    u16 numSeenNational;
    u16 numOwnedNational;
};

struct PokedexScreenWindowGfx
{
    const u32 * tiles;
    const u16 * pal;
};

struct PokedexCategoryPage
{
    const u16 * species;
    u8 count;
};

EWRAM_DATA static struct PokedexScreenData * sPokedexScreenData = NULL;

static void Task_PokedexScreen(u8 taskId);
static void DexScreen_InitGfxForTopMenu(void);
static void Task_DexScreen_NumericalOrder(u8 taskId);
static void DexScreen_InitGfxForNumericalOrderList(void);
static void Task_DexScreen_CharacteristicOrder(u8 taskId);
static void DexScreen_CreateCharacteristicListMenu(void);
static u16 DexScreen_CountMonsInOrderedList(u8 orderIdx);
static void DexScreen_InitListMenuForOrderedList(const struct ListMenuTemplate * template, u8 order);
static u8 DexScreen_CreateDexOrderScrollArrows(void);
static void DexScreen_DestroyDexOrderListMenu(u8 order);
static void Task_DexScreen_CategorySubmenu(u8 taskId);
static u8 DexScreen_CreateCategoryMenuScrollArrows(void);
static int DexScreen_InputHandler_GetShoulderInput(void);
static void Task_DexScreen_ShowMonPage(u8 taskId);
static bool32 DexScreen_TryScrollMonsVertical(u8 direction);
static void DexScreen_RemoveWindow(u8 *windowId_p);
static void DexScreen_AddTextPrinterParameterized(u8 windowId, u8 fontId, const u8 *str, u8 x, u8 y, u8 colorIdx);
static void DexScreen_PrintNum3RightAlign(u8 windowId, u8 fontId, u16 num, u8 x, u8 y, u8 colorIdx);
static void DexScreen_PrintMonDexNo(u8 windowId, u8 fontId, u16 species, u8 x, u8 y);
static void DexScreen_PrintMonNoAndName(u8 windowId, u16 species);
#define DEX_HEADER_NAME_Y  5 // HACK: nome afastado da moldura
#define DEX_HEADER_TYPES_Y 4 // HACK: tipos com a mesma folga abaixo do nome
#define DEX_HEADER_ICON_Y  2 // HACK: ícone enquadrado com o nome e os tipos
static u16 DexScreen_GetDexCount(u8 caseId, bool8 whichDex);
static void DexScreen_PrintControlInfo(const u8 *src);
static void DexScreen_DestroyCategoryPageMonIconAndInfoWindows(void);
static bool8 DexScreen_CreateCategoryListGfx(bool8 justRegistered);
static void DexScreen_CreateCategoryPageSelectionCursor(u8 cursorPos);
static void DexScreen_UpdateCategoryPageCursorObject(u8 taskId, u8 cursorPos, u8 numMonsInPage);
static bool8 DexScreen_FlipCategoryPageInDirection(u8 direction);
void DexScreen_DexPageZoomEffectFrame(u8 bg, u8 scale);
static u8 DexScreen_DrawMonDexPage(bool8 justRegistered);
u8 RemoveDexPageWindows(void);
u8 DexScreen_DrawMonAreaPage(void);
static bool8 DexScreen_IsPageUnlocked(u8 category, u8 pageNum);
static bool8 DexScreen_IsCategoryUnlocked(u8 category);
static u8 DexScreen_GetPageLimitsForCategory(u8 category);
static bool8 DexScreen_LookUpCategoryBySpecies(u16 species);
u8 DexScreen_DestroyAreaScreenResources(void);
void DexScreen_CreateCategoryPageSpeciesList(u8 category, u8 pageNum);
static u8 DexScreen_PageNumberToRenderablePages(u16 page);
void DexScreen_InputHandler_StartToCry(void);
static bool8 DexExtra_CanOpen(void);
static void DexExtra_Open(u8 page);
static void DexExtra_Close(void);
static u8 DexExtra_HandleInput(void);
static void DexExtra_ClearBgs(void);
static void DexExtra_CopyAllBgs(void);
static void DexExtra_PrintInfo(u16 species);
static void DexExtra_UpdateArrows(void);
static u8 DexExtra_MovesRows(void);
static EWRAM_DATA u8 sDexExtraOpenPage = 0;
static EWRAM_DATA u8 sDexExtraPendingShow = 0; // HACK: troca de página em andamento
// HACK: AREA LIST (lista de encontros do mapa) pronto mas em espera. 1 = liga no menu da Pokédex.
#define DEX_AREA_LIST_ENABLED 0
#if DEX_AREA_LIST_ENABLED
static void Task_DexScreen_AreaList(u8 taskId);
static const u8 sText_AreaListMenu[] = _("AREA LIST");
#endif
static EWRAM_DATA bool8 sDexFromAreaList = FALSE;
extern const struct Evolution gEvolutionTable[][EVOS_PER_MON];
extern const u16 gEggMoves[];
#define EGG_MOVES_SPECIES_OFFSET 20000
#define EGG_MOVES_TERMINATOR 0xFFFF
static const u8 sText_DexData_Moves[];
static const u8 sText_DexData_NextPrevious[];
void DexScreen_PrintStringWithAlignment(const u8 *str, s32 mode);
static void MoveCursorFunc_DexModeSelect(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void ItemPrintFunc_DexModeSelect(u8 windowId, u32 itemId, u8 y);
static void ItemPrintFunc_OrderedListMenu(u8 windowId, u32 itemId, u8 y);
static void Task_DexScreen_RegisterNonKantoMonBeforeNationalDex(u8 taskId);
static void Task_DexScreen_RegisterMonToPokedex(u8 taskId);

#include "data/pokemon_graphics/footprint_table.h"

const u32 sCategoryMonInfoBgTiles[] = INCBIN_U32("graphics/pokedex/mini_page.4bpp.lz");
const u32 sKantoDexTiles[] = INCBIN_U32("graphics/pokedex/kanto_dex_bgtiles.4bpp.lz");
const u32 sNatDexTiles[] = INCBIN_U32("graphics/pokedex/national_dex_bgtiles.4bpp.lz");
const u16 sKantoDexPalette[0x100] = INCBIN_U16("graphics/pokedex/kanto_dex_bgpals.gbapal");

const u16 sDexScreen_CategoryCursorPals[] = {
    RGB(24, 22, 17), RGB(26, 24, 20),
    RGB(26, 20, 15), RGB(27, 23, 19),
    RGB(28, 18, 15), RGB(28, 22, 19),
    RGB(30, 16, 13), RGB(29, 21, 18),
    RGB(28, 18, 15), RGB(28, 22, 19),
    RGB(26, 20, 15), RGB(27, 23, 19)
};

const u16 sNationalDexPalette[0x100] = INCBIN_U16("graphics/pokedex/national_dex_bgpals.gbapal");
const u32 sTopMenuIconTiles_Cave[] = INCBIN_U32("graphics/pokedex/cat_icon_cave.4bpp.lz");
const u32 sTopMenuIconTiles_Urban[] = INCBIN_U32("graphics/pokedex/cat_icon_urban.4bpp.lz");
const u32 sTopMenuSelectionIconTiles_Cancel[] = INCBIN_U32("graphics/pokedex/cat_icon_cancel.4bpp.lz");
const u32 sTopMenuIconTiles_Forest[] = INCBIN_U32("graphics/pokedex/cat_icon_forest.4bpp.lz");
const u32 sTopMenuIconTiles_Grassland[] = INCBIN_U32("graphics/pokedex/cat_icon_grassland.4bpp.lz");
const u32 sTopMenuIconTiles_QuestionMark[] = INCBIN_U32("graphics/pokedex/cat_icon_qmark.4bpp.lz");
const u32 sTopMenuIconTiles_Mountain[] = INCBIN_U32("graphics/pokedex/cat_icon_mountain.4bpp.lz");
const u32 sTopMenuIconTiles_Rare[] = INCBIN_U32("graphics/pokedex/cat_icon_rare.4bpp.lz");
const u32 sTopMenuIconTiles_Sea[] = INCBIN_U32("graphics/pokedex/cat_icon_sea.4bpp.lz");
const u32 sTopMenuIconTiles_Numerical[] = INCBIN_U32("graphics/pokedex/cat_icon_numerical.4bpp.lz");
const u32 sTopMenuIconTiles_RoughTerrain[] = INCBIN_U32("graphics/pokedex/cat_icon_rough_terrain.4bpp.lz");
const u32 sTopMenuIconTiles_WatersEdge[] = INCBIN_U32("graphics/pokedex/cat_icon_waters_edge.4bpp.lz");
const u32 sTopMenuIconTiles_Type[] = INCBIN_U32("graphics/pokedex/cat_icon_type.4bpp.lz");
const u32 sTopMenuIconTiles_Lightest[] = INCBIN_U32("graphics/pokedex/cat_icon_lightest.4bpp.lz");
const u32 sTopMenuIconTiles_Smallest[] = INCBIN_U32("graphics/pokedex/cat_icon_smallest.4bpp.lz");
const u16 sTopMenuIconPals_Cave[] = INCBIN_U16("graphics/pokedex/cat_icon_cave.gbapal");
const u16 sTopMenuIconPals_Urban[] = INCBIN_U16("graphics/pokedex/cat_icon_urban.gbapal");
const u16 sTopMenuSelectionIconPals_Cancel[] = INCBIN_U16("graphics/pokedex/cat_icon_cancel.gbapal");
const u16 sTopMenuIconPals_Forest[] = INCBIN_U16("graphics/pokedex/cat_icon_forest.gbapal");
const u16 sTopMenuIconPals_Grassland[] = INCBIN_U16("graphics/pokedex/cat_icon_grassland.gbapal");
const u16 sTopMenuIconPals_QuestionMark[] = INCBIN_U16("graphics/pokedex/cat_icon_qmark.gbapal");
const u16 sTopMenuIconPals_Mountain[] = INCBIN_U16("graphics/pokedex/cat_icon_mountain.gbapal");
const u16 sTopMenuIconPals_Rare[] = INCBIN_U16("graphics/pokedex/cat_icon_rare.gbapal");
const u16 sTopMenuIconPals_Sea[] = INCBIN_U16("graphics/pokedex/cat_icon_sea.gbapal");
const u16 sTopMenuIconPals_Numerical[] = INCBIN_U16("graphics/pokedex/cat_icon_numerical.gbapal");
const u16 sTopMenuIconPals_RoughTerrain[] = INCBIN_U16("graphics/pokedex/cat_icon_rough_terrain.gbapal");
const u16 sTopMenuIconPals_WatersEdge[] = INCBIN_U16("graphics/pokedex/cat_icon_waters_edge.gbapal");
const u16 sTopMenuIconPals_Type[] = INCBIN_U16("graphics/pokedex/cat_icon_type.gbapal");
const u16 sTopMenuIconPals_Lightest[] = INCBIN_U16("graphics/pokedex/cat_icon_lightest.gbapal");
const u16 sTopMenuIconPals_Smallest[] = INCBIN_U16("graphics/pokedex/cat_icon_smallest.gbapal");
const u8 sDexScreen_CaughtIcon[] = INCBIN_U8("graphics/pokedex/caught_marker.4bpp");
const u32 sTilemap_AreaMap_Kanto[] = INCBIN_U32("graphics/pokedex/map_kanto.4bpp.lz");
const u32 sTilemap_AreaMap_OneIsland[] = INCBIN_U32("graphics/pokedex/map_one_island.4bpp.lz");
const u32 sTilemap_AreaMap_TwoIsland[] = INCBIN_U32("graphics/pokedex/map_two_island.4bpp.lz");
const u32 sTilemap_AreaMap_ThreeIsland[] = INCBIN_U32("graphics/pokedex/map_three_island.4bpp.lz");
const u32 sTilemap_AreaMap_FourIsland[] = INCBIN_U32("graphics/pokedex/map_four_island.4bpp.lz");
const u32 sTilemap_AreaMap_FiveIsland[] = INCBIN_U32("graphics/pokedex/map_five_island.4bpp.lz");
const u32 sTilemap_AreaMap_SixIsland[] = INCBIN_U32("graphics/pokedex/map_six_island.4bpp.lz");
const u32 sTilemap_AreaMap_SevenIsland[] = INCBIN_U32("graphics/pokedex/map_seven_island.4bpp.lz");
const u16 sBlitTiles_WideEllipse[] = INCBIN_U16("graphics/pokedex/blit_wide_ellipse.4bpp");

#include "data/pokemon/pokedex_orders.h"

static const u8 gExpandedPlaceholder_PokedexDescription[] = _("");

#include "data/pokemon/pokedex_text.h"
#include "data/pokemon/pokedex_entries.h"

static const struct BgTemplate sBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 5,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0x0000
    },
    {
        .bg = 1,
        .charBaseIndex = 2,
        .mapBaseIndex = 4,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0x0000
    },
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 6,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0x0000
    },
    {
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 7,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0x0000
    },
};

static const struct WindowTemplate sWindowTemplates[] = {
    {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 30,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x03c4
    },
    {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 18,
        .width = 30,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x0388
    },
    {
        .bg = 255,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 0,
        .height = 0,
        .paletteNum = 0,
        .baseBlock = 0x0000
    },
};

static const struct PokedexScreenData sDexScreenDataInitialState = {
    .modeSelectItemsAbove = 1,
    .modeSelectWindowId = -1,
    .selectionIconWindowId = -1,
    .dexCountsWindowId = -1,
    .pageSpecies = {-1, -1, -1, -1},
    .categoryMonWindowIds = {-1, -1, -1, -1},
    .categoryMonInfoWindowIds = {-1, -1, -1, -1},
    .numericalOrderWindowId = -1,
    .windowIds = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    .scrollArrowsTaskId = -1,
    .categoryPageCursorTaskId = -1,
};


static const struct WindowTemplate sWindowTemplate_ModeSelect = {
    .bg = 1,
    .tilemapLeft = 1,
    .tilemapTop = 2,
    .width = 20,
    .height = 16,
    .paletteNum = 0,
    .baseBlock = 0x0008
};

static const struct WindowTemplate sWindowTemplate_SelectionIcon = {
    .bg = 1,
    .tilemapLeft = 21,
    .tilemapTop = 11,
    .width = 8,
    .height = 6,
    .paletteNum = 1,
    .baseBlock = 0x0148
};

static const struct WindowTemplate sWindowTemplate_DexCounts = {
    .bg = 1,
    .tilemapLeft = 21,
    .tilemapTop = 2,
    .width = 9,
    .height = 9,
    .paletteNum = 0,
    .baseBlock = 0x0178
};

static const struct ListMenuItem sListMenuItems_KantoDexModeSelect[] = {
    {gText_PokemonList,                  LIST_HEADER},
    {gText_NumericalMode,                DEX_MODE(NUMERICAL_KANTO)},
#if DEX_AREA_LIST_ENABLED
    {sText_AreaListMenu,                 DEX_MODE(AREA_LIST)}, // HACK
#endif
    {gText_PokemonHabitats,              LIST_HEADER},
    {gText_DexCategory_GrasslandPkmn,    DEX_CATEGORY_GRASSLAND},
    {gText_DexCategory_ForestPkmn,       DEX_CATEGORY_FOREST},
    {gText_DexCategory_WatersEdgePkmn,   DEX_CATEGORY_WATERS_EDGE},
    {gText_DexCategory_SeaPkmn,          DEX_CATEGORY_SEA},
    {gText_DexCategory_CavePkmn,         DEX_CATEGORY_CAVE},
    {gText_DexCategory_MountainPkmn,     DEX_CATEGORY_MOUNTAIN},
    {gText_DexCategory_RoughTerrainPkmn, DEX_CATEGORY_ROUGH_TERRAIN},
    {gText_DexCategory_UrbanPkmn,        DEX_CATEGORY_URBAN},
    {gText_DexCategory_RarePkmn,         DEX_CATEGORY_RARE},
    {gText_Search,                       LIST_HEADER},
    {gText_AToZMode,                     DEX_MODE(ATOZ)},
    {gText_TypeMode,                     DEX_MODE(TYPE)},
    {gText_LightestMode,                 DEX_MODE(LIGHTEST)},
    {gText_SmallestMode,                 DEX_MODE(SMALLEST)},
    {gText_PokedexOther,                 LIST_HEADER},
    {gText_ClosePokedex,                 LIST_CANCEL},
};

static const struct ListMenuTemplate sListMenuTemplate_KantoDexModeSelect = {
    .items = sListMenuItems_KantoDexModeSelect,
    .moveCursorFunc = MoveCursorFunc_DexModeSelect,
    .itemPrintFunc = ItemPrintFunc_DexModeSelect,
    .totalItems = NELEMS(sListMenuItems_KantoDexModeSelect),
    .maxShowed = 9,
    .windowId = 0,
    .header_X = 0,
    .item_X = 12,
    .cursor_X = 4,
    .upText_Y = 2,
    .cursorPal = 1,
    .fillValue = 0,
    .cursorShadowPal = 3,
    .lettersSpacing = 1,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0,
};

static const struct ListMenuItem sListMenuItems_NatDexModeSelect[] = {
    {gText_PokemonList,                  LIST_HEADER},
    {gText_NumericalModeKanto,           DEX_MODE(NUMERICAL_KANTO)},
    {gText_NumericalModeNational,        DEX_MODE(NUMERICAL_NATIONAL)},
#if DEX_AREA_LIST_ENABLED
    {sText_AreaListMenu,                 DEX_MODE(AREA_LIST)}, // HACK
#endif
    {gText_PokemonHabitats,              LIST_HEADER},
    {gText_DexCategory_GrasslandPkmn,    DEX_CATEGORY_GRASSLAND},
    {gText_DexCategory_ForestPkmn,       DEX_CATEGORY_FOREST},
    {gText_DexCategory_WatersEdgePkmn,   DEX_CATEGORY_WATERS_EDGE},
    {gText_DexCategory_SeaPkmn,          DEX_CATEGORY_SEA},
    {gText_DexCategory_CavePkmn,         DEX_CATEGORY_CAVE},
    {gText_DexCategory_MountainPkmn,     DEX_CATEGORY_MOUNTAIN},
    {gText_DexCategory_RoughTerrainPkmn, DEX_CATEGORY_ROUGH_TERRAIN},
    {gText_DexCategory_UrbanPkmn,        DEX_CATEGORY_URBAN},
    {gText_DexCategory_RarePkmn,         DEX_CATEGORY_RARE},
    {gText_Search,                       LIST_HEADER},
    {gText_AToZMode,                     DEX_MODE(ATOZ)},
    {gText_TypeMode,                     DEX_MODE(TYPE)},
    {gText_LightestMode,                 DEX_MODE(LIGHTEST)},
    {gText_SmallestMode,                 DEX_MODE(SMALLEST)},
    {gText_PokedexOther,                 LIST_HEADER},
    {gText_ClosePokedex,                 LIST_CANCEL},
};

static const struct ListMenuTemplate sListMenuTemplate_NatDexModeSelect = {
    .items = sListMenuItems_NatDexModeSelect,
    .moveCursorFunc = MoveCursorFunc_DexModeSelect,
    .itemPrintFunc = ItemPrintFunc_DexModeSelect,
    .totalItems = NELEMS(sListMenuItems_NatDexModeSelect),
    .maxShowed = 9,
    .windowId = 0,
    .header_X = 0,
    .item_X = 12,
    .cursor_X = 4,
    .upText_Y = 2,
    .cursorPal = 1,
    .fillValue = 0,
    .cursorShadowPal = 3,
    .lettersSpacing = 1,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0,
};

static const struct ScrollArrowsTemplate sScrollArrowsTemplate_KantoDex = {
    .firstArrowType = 2,
    .firstX = 200,
    .firstY = 19,
    .secondArrowType = 3,
    .secondX = 200,
    .secondY = 141,
    .fullyUpThreshold = 0,
    .fullyDownThreshold = 10 + DEX_AREA_LIST_ENABLED, // HACK: +1 item com o AREA LIST
    .tileTag = 2000,
    .palTag = 0xFFFF,
    .palNum = 1
};

static const struct ScrollArrowsTemplate sScrollArrowsTemplate_NatDex = {
    .firstArrowType = 2,
    .firstX = 200,
    .firstY = 19,
    .secondArrowType = 3,
    .secondX = 200,
    .secondY = 141,
    .fullyUpThreshold = 0,
    .fullyDownThreshold = 11 + DEX_AREA_LIST_ENABLED, // HACK: +1 item com o AREA LIST
    .tileTag = 2000,
    .palTag = 0xFFFF,
    .palNum = 1
};


static const struct PokedexScreenWindowGfx sTopMenuSelectionIconGfxPtrs[] = {
    [DEX_CATEGORY_GRASSLAND] = {
        .tiles = sTopMenuIconTiles_Grassland,
        .pal   = sTopMenuIconPals_Grassland
    },
    [DEX_CATEGORY_FOREST] = {
        .tiles = sTopMenuIconTiles_Forest,
        .pal   = sTopMenuIconPals_Forest
    },
    [DEX_CATEGORY_WATERS_EDGE] = {
        .tiles = sTopMenuIconTiles_WatersEdge,
        .pal   = sTopMenuIconPals_WatersEdge
    },
    [DEX_CATEGORY_SEA] = {
        .tiles = sTopMenuIconTiles_Sea,
        .pal   = sTopMenuIconPals_Sea
    },
    [DEX_CATEGORY_CAVE] = {
        .tiles = sTopMenuIconTiles_Cave,
        .pal   = sTopMenuIconPals_Cave
    },
    [DEX_CATEGORY_MOUNTAIN] = {
        .tiles = sTopMenuIconTiles_Mountain,
        .pal   = sTopMenuIconPals_Mountain
    },
    [DEX_CATEGORY_ROUGH_TERRAIN] = {
        .tiles = sTopMenuIconTiles_RoughTerrain,
        .pal   = sTopMenuIconPals_RoughTerrain
    },
    [DEX_CATEGORY_URBAN] = {
        .tiles = sTopMenuIconTiles_Urban,
        .pal   = sTopMenuIconPals_Urban
    },
    [DEX_CATEGORY_RARE] = {
        .tiles = sTopMenuIconTiles_Rare,
        .pal   = sTopMenuIconPals_Rare
    },
    [DEX_MODE(NUMERICAL_KANTO)] = {
        .tiles = sTopMenuIconTiles_Numerical,
        .pal   = sTopMenuIconPals_Numerical
    },
    [DEX_MODE(ATOZ)] = {
        .tiles = gDexScreen_TopMenuIconTiles_AtoZ,
        .pal   = gDexScreen_TopMenuIconPals_AtoZ
    },
    [DEX_MODE(TYPE)] = {
        .tiles = sTopMenuIconTiles_Type,
        .pal   = sTopMenuIconPals_Type
    },
    [DEX_MODE(LIGHTEST)] = {
        .tiles = sTopMenuIconTiles_Lightest,
        .pal   = sTopMenuIconPals_Lightest
    },
    [DEX_MODE(SMALLEST)] = {
        .tiles = sTopMenuIconTiles_Smallest,
        .pal   = sTopMenuIconPals_Smallest
    },
    [DEX_MODE(NUMERICAL_NATIONAL)] = {
        .tiles = sTopMenuIconTiles_Numerical,
        .pal   = sTopMenuIconPals_Numerical
    },
    [DEX_MODE(AREA_LIST)] = { // HACK: usa o ícone de campo
        .tiles = sTopMenuIconTiles_Grassland,
        .pal   = sTopMenuIconPals_Grassland
    },
};

static const struct WindowTemplate sWindowTemplate_OrderedListMenu = {
    .bg = 1,
    .tilemapLeft = 2,
    .tilemapTop = 2,
    .width = 23,
    .height = 16,
    .paletteNum = 0,
    .baseBlock = 0x0008
};

static const struct ListMenuTemplate sListMenuTemplate_OrderedListMenu = {
    .items = sListMenuItems_KantoDexModeSelect,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = ItemPrintFunc_OrderedListMenu,
    .totalItems = 0,
    .maxShowed = 9,
    .windowId = 0,
    .header_X = 0,
    .item_X = 56,
    .cursor_X = 4,
    .upText_Y = 2,
    .cursorPal = 1,
    .fillValue = 0,
    .cursorShadowPal = 3,
    .lettersSpacing = 1,
    .itemVerticalPadding = 0,
    .scrollMultiple = 1,
    .fontId = FONT_NORMAL,
    .cursorKind = 0,
};

static const struct ListMenuWindowRect sListMenuRects_OrderedList[] = {
    {
        .x = 0,
        .y = 0,
        .width = 5,
        .height = 16,
        .palNum = 0
    }, {
        .x = 5,
        .y = 0,
        .width = 2,
        .height = 16,
        .palNum = 1
    }, {
        .x = 7,
        .y = 0,
        .width = 8,
        .height = 16,
        .palNum = 0
    }, {
        .x = 15,
        .y = 0,
        .width = 8,
        .height = 16,
        .palNum = 2,
    }, {
        .x = 0xFF,
        .y = 0xFF,
        .width = 0xFF,
        .height = 0xFF,
        .palNum = 0xFF
    }
};

static const struct ScrollArrowsTemplate sDexOrderScrollArrowsTemplate = {
    .firstArrowType = 2,
    .firstX = 200,
    .firstY = 19,
    .secondArrowType = 3,
    .secondX = 200,
    .secondY = 141,
    .fullyUpThreshold = 0,
    .fullyDownThreshold = 0,
    .tileTag = 2000,
    .palTag = 0xFFFF,
    .palNum = 1,
};

static const struct WindowTemplate sWindowTemplate_CategoryMonIcon = {
    .bg = 2,
    .tilemapLeft = 0,
    .tilemapTop = 0,
    .width = 8,
    .height = 8,
    .paletteNum = 0,
    .baseBlock = 0x0000
};

static const struct WindowTemplate sWindowTemplate_CategoryMonInfo = {
    .bg = 1,
    .tilemapLeft = 0,
    .tilemapTop = 0,
    .width = 8,
    .height = 5,
    .paletteNum = 0,
    .baseBlock = 0x0000
};

const struct WindowTemplate sWindowTemplate_DexEntry_MonPic = {
    .bg = 1,
    .tilemapLeft = 19,
    .tilemapTop = 3,
    .width = 8,
    .height = 8,
    .paletteNum = 9,
    .baseBlock = 0x01a8
};

const struct WindowTemplate sWindowTemplate_DexEntry_SpeciesStats = {
    .bg = 1,
    .tilemapLeft = 2,
    .tilemapTop = 3,
    .width = 13,
    .height = 8,
    .paletteNum = 0,
    .baseBlock = 0x01e8
};

const struct WindowTemplate sWindowTemplate_DexEntry_FlavorText = {
    .bg = 1,
    .tilemapLeft = 0,
    .tilemapTop = 11,
    .width = 30,
    .height = 7,
    .paletteNum = 0,
    .baseBlock = 0x0250
};

const struct WindowTemplate sWindowTemplate_AreaMap_MonIcon = {
    .bg = 2,
    .tilemapLeft = 1,
    .tilemapTop = 2,
    .width = 4,
    .height = 5, // HACK: uma linha a mais para o ícone descer sem cortar
    .paletteNum = 10,
    .baseBlock = 0x01a8
};

const struct WindowTemplate sWindowTemplate_AreaMap_SpeciesName = {
    .bg = 2,
    .tilemapLeft = 5,
    .tilemapTop = 2,
    .width = 12, // HACK: número e nome na mesma linha (páginas AREA, DATA e MOVES)
    .height = 2, // HACK: 2 linhas bastam (o texto termina no último pixel); com 3 cobria o topo da ONE ISLAND no mapa do AREA
    .paletteNum = 0,
    .baseBlock = 0x01bc
};

const struct WindowTemplate sWindowTemplate_AreaMap_Size = {
    .bg = 2,
    .tilemapLeft = 5,
    .tilemapTop = 6, // HACK: mesma altura do rótulo AREA, centralizado no quadro (sem encostar no ícone)
    .width = 4,
    .height = 2,
    .paletteNum = 0,
    .baseBlock = 0x01e0
};

const struct WindowTemplate sWindowTemplate_AreaMap_Area = {
    .bg = 2,
    .tilemapLeft = 20, // HACK: mais estreito (sobra espaço para o cabeçalho), mesmo centro
    .tilemapTop = 2,
    .width = 6,
    .height = 2,
    .paletteNum = 0,
    .baseBlock = 0x01e8
};

const struct WindowTemplate sWindowTemplate_AreaMap_MonTypes = {
    .bg = 2,
    .tilemapLeft = 5,
    .tilemapTop = 4, // HACK: tipos logo abaixo do número e nome
    .width = 8,
    .height = 2,
    .paletteNum = 11,
    .baseBlock = 0x01f4
};

const struct WindowTemplate sWindowTemplate_AreaMap_Kanto = {
    .bg = 2,
    .tilemapLeft = 17,
    .tilemapTop = 4,
    .width = 12,
    .height = 9,
    .paletteNum = 0,
    .baseBlock = 0x0208
};

static const struct WindowTemplate sWindowTemplate_AreaMap_OneIsland = {
    .bg = 2,
    .tilemapLeft = 13,
    .tilemapTop = 4,
    .width = 4,
    .height = 3,
    .paletteNum = 0,
    .baseBlock = 0x0274
};

static const struct WindowTemplate sWindowTemplate_AreaMap_TwoIsland = {
    .bg = 2,
    .tilemapLeft = 13,
    .tilemapTop = 7,
    .width = 4,
    .height = 3,
    .paletteNum = 0,
    .baseBlock = 0x0280
};

static const struct WindowTemplate sWindowTemplate_AreaMap_ThreeIsland = {
    .bg = 2,
    .tilemapLeft = 13,
    .tilemapTop = 10,
    .width = 4,
    .height = 3,
    .paletteNum = 0,
    .baseBlock = 0x028c
};

static const struct WindowTemplate sWindowTemplate_AreaMap_FourIsland = {
    .bg = 2,
    .tilemapLeft = 13,
    .tilemapTop = 13,
    .width = 4,
    .height = 4,
    .paletteNum = 0,
    .baseBlock = 0x0298
};

static const struct WindowTemplate sWindowTemplate_AreaMap_FiveIsland = {
    .bg = 2,
    .tilemapLeft = 17,
    .tilemapTop = 13,
    .width = 4,
    .height = 4,
    .paletteNum = 0,
    .baseBlock = 0x02a8
};

static const struct WindowTemplate sWindowTemplate_AreaMap_SixIsland = {
    .bg = 2,
    .tilemapLeft = 21,
    .tilemapTop = 13,
    .width = 4,
    .height = 4,
    .paletteNum = 0,
    .baseBlock = 0x02b8
};

static const struct WindowTemplate sWindowTemplate_AreaMap_SevenIsland = {
    .bg = 2,
    .tilemapLeft = 25,
    .tilemapTop = 13,
    .width = 4,
    .height = 4,
    .paletteNum = 0,
    .baseBlock = 0x02c8
};

struct {
    const struct WindowTemplate * window;
    const u32 * tiles;
} const sAreaMapStructs_SeviiIslands[] = {
    {&sWindowTemplate_AreaMap_OneIsland,   sTilemap_AreaMap_OneIsland},
    {&sWindowTemplate_AreaMap_TwoIsland,   sTilemap_AreaMap_TwoIsland},
    {&sWindowTemplate_AreaMap_ThreeIsland, sTilemap_AreaMap_ThreeIsland},
    {&sWindowTemplate_AreaMap_FourIsland,  sTilemap_AreaMap_FourIsland},
    {&sWindowTemplate_AreaMap_FiveIsland,  sTilemap_AreaMap_FiveIsland},
    {&sWindowTemplate_AreaMap_SixIsland,   sTilemap_AreaMap_SixIsland},
    {&sWindowTemplate_AreaMap_SevenIsland, sTilemap_AreaMap_SevenIsland},
};

static const u16 sCategoryPageIconWindowBg[] = INCBIN_U16("graphics/pokedex/page_icon_tilemap.bin");

// Circular window x/y; Rectangular window x/y
static const u8 sPageIconCoords_1Mon[1][4] = {
    {11,  3, 11, 11},
};

static const u8 sPageIconCoords_2Mons[2][4] = {
    { 3,  3, 11,  3},
    {18,  9, 10, 11},
};

static const u8 sPageIconCoords_3Mons[3][4] = {
    { 1,  2,  9,  2},
    {11,  9,  3, 11},
    {21,  3, 21, 11}
};

static const u8 sPageIconCoords_4Mons[4][4] = {
    { 0,  2,  6,  3},
    { 7, 10,  0, 12},
    {15, 10, 22, 11},
    {22,  2, 15,  4}
};

const u8 (*const sCategoryPageIconCoords[])[4] = {
    sPageIconCoords_1Mon,
    sPageIconCoords_2Mons,
    sPageIconCoords_3Mons,
    sPageIconCoords_4Mons,
};

static const u8 *const sDexCategoryNamePtrs[] = {
    gText_DexCategory_GrasslandPkmn,
    gText_DexCategory_ForestPkmn,
    gText_DexCategory_WatersEdgePkmn,
    gText_DexCategory_SeaPkmn,
    gText_DexCategory_CavePkmn,
    gText_DexCategory_MountainPkmn,
    gText_DexCategory_RoughTerrainPkmn,
    gText_DexCategory_UrbanPkmn,
    gText_DexCategory_RarePkmn,
};

const u16 sPalette_Silhouette[] = INCBIN_U16("graphics/pokedex/silhouette_sprite_pal.gbapal");

static const u8 sDexScreenPageTurnColumns[][30] = {
    {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
    { 5, 11, 17, 23, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
    { 2,  5,  8, 11, 14, 17, 20, 23, 26, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
    { 2,  3,  5,  7,  9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
    { 2,  4,  5,  7,  8, 10, 11, 13, 14, 16, 17, 19, 20, 22, 23, 25, 26, 28, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
    { 1,  2,  3,  4,  7,  8,  9, 10, 11, 12, 13, 15, 16, 17, 19, 20, 21, 23, 24, 25, 27, 28, 29, 30, 30, 30, 30, 30, 30, 30},
    { 1,  2,  3,  4,  5,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21, 22, 23, 25, 26, 27, 28, 29, 30, 30, 30, 30},
    { 1,  2,  3,  4,  5,  6,  7,  8,  9, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 30, 30},
    { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30},
    { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29},
};

static const struct ScrollArrowsTemplate sScrollArrowsTemplate_CategoryMenu = {
    .firstArrowType = 0,
    .firstX = 16,
    .firstY = 80,
    .secondArrowType = 1,
    .secondX = 224,
    .secondY = 80,
    .fullyUpThreshold = 0,
    .fullyDownThreshold = 0,
    .tileTag = 2000,
    .palTag = 0xFFFF,
    .palNum = 1,
};

const struct CursorStruct sCursorStruct_CategoryPage = {
    .left = 0,
    .top = 160,
    .rowWidth = 64,
    .rowHeight = 40,
    .tileTag = 2002,
    .palTag = 0xFFFF,
    .palNum = 4,
};

#include "data/pokemon/pokedex_categories.h"

void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void CB2_PokedexScreen(void)
{
    if (!gPaletteFade.active || IsDma3ManagerBusyWithBgCopy())
    {
        RunTasks();
        RunTextPrinters();
        AnimateSprites();
        BuildOamBuffer();
    }
    else
    {
        UpdatePaletteFade();
    }
}

void DexScreen_LoadResources(void)
{
    bool8 natDex;
    u8 taskId;

    natDex = IsNationalPokedexEnabled();
    m4aSoundVSyncOff();
    SetVBlankCallback(NULL);
    ResetPaletteFade();
    ResetSpriteData();
    ResetTasks();
    ScanlineEffect_Stop();
    ResetBgsAndClearDma3BusyFlags(TRUE);
    InitBgsFromTemplates(0, sBgTemplates, NELEMS(sBgTemplates));
    SetBgTilemapBuffer(3, (u16 *)Alloc(BG_SCREEN_SIZE));
    SetBgTilemapBuffer(2, (u16 *)Alloc(BG_SCREEN_SIZE));
    SetBgTilemapBuffer(1, (u16 *)Alloc(BG_SCREEN_SIZE));
    SetBgTilemapBuffer(0, (u16 *)Alloc(BG_SCREEN_SIZE));
    if (natDex)
        DecompressAndLoadBgGfxUsingHeap(3, (void *)sNatDexTiles, BG_SCREEN_SIZE, 0, 0);
    else
        DecompressAndLoadBgGfxUsingHeap(3, (void *)sKantoDexTiles, BG_SCREEN_SIZE, 0, 0);
    InitWindows(sWindowTemplates);
    DeactivateAllTextPrinters();
    m4aSoundVSyncOn();
    SetVBlankCallback(VBlankCB);
    EnableInterrupts(INTR_FLAG_VBLANK);
    taskId = CreateTask(Task_PokedexScreen, 0);
    sPokedexScreenData = Alloc(sizeof(struct PokedexScreenData));
    *sPokedexScreenData = sDexScreenDataInitialState;
    sPokedexScreenData->taskId = taskId;
    sPokedexScreenData->listItems = Alloc(NATIONAL_DEX_COUNT * sizeof(struct ListMenuItem));
    sPokedexScreenData->numSeenNational = DexScreen_GetDexCount(FLAG_GET_SEEN, 1);
    sPokedexScreenData->numOwnedNational = DexScreen_GetDexCount(FLAG_GET_CAUGHT, 1);
    sPokedexScreenData->numSeenKanto = DexScreen_GetDexCount(FLAG_GET_SEEN, 0);
    sPokedexScreenData->numOwnedKanto = DexScreen_GetDexCount(FLAG_GET_CAUGHT, 0);
    SetBGMVolume_SuppressHelpSystemReduction(0x80);
    ChangeBgX(0, 0, 0);
    ChangeBgY(0, 0, 0);
    ChangeBgX(1, 0, 0);
    ChangeBgY(1, 0, 0);
    ChangeBgX(2, 0, 0);
    ChangeBgY(2, 0, 0);
    ChangeBgX(3, 0, 0);
    ChangeBgY(3, 0, 0);
    gPaletteFade.bufferTransferDisabled = TRUE;
    if (natDex)
        LoadPalette(sNationalDexPalette, BG_PLTT_ID(0), sizeof(sNationalDexPalette));
    else
        LoadPalette(sKantoDexPalette, BG_PLTT_ID(0), sizeof(sKantoDexPalette));
    FillBgTilemapBufferRect(3, 0x001, 0,  0, 32, 32, 0);
    FillBgTilemapBufferRect(2, 0x000, 0,  0, 32, 32, 17);
    FillBgTilemapBufferRect(1, 0x000, 0,  0, 32, 32, 17);
    FillBgTilemapBufferRect(0, 0x003, 0,  0, 32,  2, 15);
    FillBgTilemapBufferRect(0, 0x000, 0,  2, 32, 16, 17);
    FillBgTilemapBufferRect(0, 0x003, 0, 18, 32,  2, 15);
}

void CB2_OpenPokedexFromStartMenu(void)
{
    DexScreen_LoadResources();
    ClearGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
    SetMainCallback2(CB2_PokedexScreen);
    SetHelpContext(HELPCONTEXT_POKEDEX);
}

#define FREE_IF_NOT_NULL(ptr0) ({ void *ptr = (ptr0); if (ptr) Free(ptr); })

bool8 DoClosePokedex(void)
{
    switch (gMain.state)
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gMain.state++;
        return FALSE;
    case 1:
        if (!gPaletteFade.active)
            gMain.state = 2;
        else
            UpdatePaletteFade();
        return FALSE;
    case 2:
        FREE_IF_NOT_NULL(sPokedexScreenData->listItems);
        FREE_IF_NOT_NULL(sPokedexScreenData);
        FreeAllWindowBuffers();
        FREE_IF_NOT_NULL(GetBgTilemapBuffer(0));
        FREE_IF_NOT_NULL(GetBgTilemapBuffer(1));
        FREE_IF_NOT_NULL(GetBgTilemapBuffer(2));
        FREE_IF_NOT_NULL(GetBgTilemapBuffer(3));
        BGMVolumeMax_EnableHelpSystemReduction();
        break;
    }
    return TRUE;
}

void CB2_ClosePokedex(void)
{
    if (DoClosePokedex())
    {
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
        if (gPokedexOpenedWithL) // HACK: aberta pelo L no mapa, volta sem o menu Start
        {
            gPokedexOpenedWithL = FALSE;
            SetMainCallback2(CB2_ReturnToFieldFromLPokedex);
        }
        else
        {
            SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
        }
    }
}

static void Task_PokedexScreen(u8 taskId)
{
    int i;
    switch (sPokedexScreenData->state)
    {
    case 0:
        sPokedexScreenData->unlockedCategories = 0;
        for (i = 0; i < 9; i++)
            sPokedexScreenData->unlockedCategories |= (DexScreen_IsCategoryUnlocked(i) << i);
        sPokedexScreenData->state = 2;
        break;
    case 1:
        RemoveScrollIndicatorArrowPair(sPokedexScreenData->scrollArrowsTaskId);
        DexScreen_RemoveWindow(&sPokedexScreenData->modeSelectWindowId);
        DexScreen_RemoveWindow(&sPokedexScreenData->selectionIconWindowId);
        DexScreen_RemoveWindow(&sPokedexScreenData->dexCountsWindowId);
        SetMainCallback2(CB2_ClosePokedex);
        DestroyTask(taskId);
        break;
    case 2:
        DexScreen_InitGfxForTopMenu();
        sPokedexScreenData->state = 3;
        break;
    case 3:
        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(0);
        sPokedexScreenData->state = 4;
        break;
    case 4:
        ShowBg(3);
        ShowBg(2);
        ShowBg(1);
        ShowBg(0);
        if (gPaletteFade.bufferTransferDisabled)
        {
            gPaletteFade.bufferTransferDisabled = FALSE;
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0, RGB_WHITEALPHA);
        }
        else
            BeginNormalPaletteFade(~0x8000, 0, 16, 0, RGB_WHITEALPHA);
        sPokedexScreenData->state = 5;
        break;
    case 5:
        ListMenuGetScrollAndRow(sPokedexScreenData->modeSelectListMenuId, &sPokedexScreenData->modeSelectCursorPosBak, NULL);
        if (IsNationalPokedexEnabled())
            sPokedexScreenData->scrollArrowsTaskId = AddScrollIndicatorArrowPair(&sScrollArrowsTemplate_NatDex, &sPokedexScreenData->modeSelectCursorPosBak);
        else
            sPokedexScreenData->scrollArrowsTaskId = AddScrollIndicatorArrowPair(&sScrollArrowsTemplate_KantoDex, &sPokedexScreenData->modeSelectCursorPosBak);
        sPokedexScreenData->state = 6;
        break;
    case 6:
        sPokedexScreenData->modeSelectInput = ListMenu_ProcessInput(sPokedexScreenData->modeSelectListMenuId);
        ListMenuGetScrollAndRow(sPokedexScreenData->modeSelectListMenuId, &sPokedexScreenData->modeSelectCursorPosBak, NULL);
        if (JOY_NEW(A_BUTTON))
        {
            switch (sPokedexScreenData->modeSelectInput)
            {
            case LIST_CANCEL:
                sPokedexScreenData->state = 1;
                break;
            case DEX_CATEGORY_GRASSLAND:
            case DEX_CATEGORY_FOREST:
            case DEX_CATEGORY_WATERS_EDGE:
            case DEX_CATEGORY_SEA:
            case DEX_CATEGORY_CAVE:
            case DEX_CATEGORY_MOUNTAIN:
            case DEX_CATEGORY_ROUGH_TERRAIN:
            case DEX_CATEGORY_URBAN:
            case DEX_CATEGORY_RARE:
                if (DexScreen_IsCategoryUnlocked(sPokedexScreenData->modeSelectInput))
                {
                    RemoveScrollIndicatorArrowPair(sPokedexScreenData->scrollArrowsTaskId);
                    sPokedexScreenData->category = sPokedexScreenData->modeSelectInput;
                    BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
                    sPokedexScreenData->state = 7;
                }
                break;
            case DEX_MODE(NUMERICAL_KANTO):
            case DEX_MODE(NUMERICAL_NATIONAL):
                RemoveScrollIndicatorArrowPair(sPokedexScreenData->scrollArrowsTaskId);
                sPokedexScreenData->dexOrderId = sPokedexScreenData->modeSelectInput - DEX_CATEGORY_COUNT;
                BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
                sPokedexScreenData->state = 9;
                break;
#if DEX_AREA_LIST_ENABLED
            case DEX_MODE(AREA_LIST): // HACK
                RemoveScrollIndicatorArrowPair(sPokedexScreenData->scrollArrowsTaskId);
                BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
                sPokedexScreenData->state = 10;
                break;
#endif
            case DEX_MODE(ATOZ):
            case DEX_MODE(TYPE):
            case DEX_MODE(LIGHTEST):
            case DEX_MODE(SMALLEST):
                RemoveScrollIndicatorArrowPair(sPokedexScreenData->scrollArrowsTaskId);
                sPokedexScreenData->dexOrderId = sPokedexScreenData->modeSelectInput - DEX_CATEGORY_COUNT;
                sPokedexScreenData->characteristicOrderMenuItemsAbove = sPokedexScreenData->characteristicOrderMenuCursorPos = 0;
                BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
                sPokedexScreenData->state = 8;
                break;
            }
            break;
        }
        if (JOY_NEW(B_BUTTON))
        {
            sPokedexScreenData->state = 1;
        }
        break;
    case 7:
        DestroyListMenuTask(sPokedexScreenData->modeSelectListMenuId, &sPokedexScreenData->modeSelectCursorPos, &sPokedexScreenData->modeSelectItemsAbove);
        FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 32, 20);
        CopyBgTilemapBufferToVram(1);
        DexScreen_RemoveWindow(&sPokedexScreenData->modeSelectWindowId);
        DexScreen_RemoveWindow(&sPokedexScreenData->selectionIconWindowId);
        DexScreen_RemoveWindow(&sPokedexScreenData->dexCountsWindowId);
        sPokedexScreenData->pageNum = 0;
        sPokedexScreenData->categoryCursorPosInPage = 0;
        sPokedexScreenData->parentOfCategoryMenu = 0;
        gTasks[taskId].func = Task_DexScreen_CategorySubmenu;
        sPokedexScreenData->state = 0;
        break;
    case 8:
        DestroyListMenuTask(sPokedexScreenData->modeSelectListMenuId, &sPokedexScreenData->modeSelectCursorPos, &sPokedexScreenData->modeSelectItemsAbove);
        HideBg(1);
        DexScreen_RemoveWindow(&sPokedexScreenData->modeSelectWindowId);
        DexScreen_RemoveWindow(&sPokedexScreenData->selectionIconWindowId);
        DexScreen_RemoveWindow(&sPokedexScreenData->dexCountsWindowId);
        gTasks[taskId].func = Task_DexScreen_CharacteristicOrder;
        sPokedexScreenData->state = 0;
        break;
    case 9:
        DestroyListMenuTask(sPokedexScreenData->modeSelectListMenuId, &sPokedexScreenData->modeSelectCursorPos, &sPokedexScreenData->modeSelectItemsAbove);
        HideBg(1);
        DexScreen_RemoveWindow(&sPokedexScreenData->modeSelectWindowId);
        DexScreen_RemoveWindow(&sPokedexScreenData->selectionIconWindowId);
        DexScreen_RemoveWindow(&sPokedexScreenData->dexCountsWindowId);
        gTasks[taskId].func = Task_DexScreen_NumericalOrder;
        sPokedexScreenData->state = 0;
        break;
#if DEX_AREA_LIST_ENABLED
    case 10: // HACK: AREA LIST
        DestroyListMenuTask(sPokedexScreenData->modeSelectListMenuId, &sPokedexScreenData->modeSelectCursorPos, &sPokedexScreenData->modeSelectItemsAbove);
        FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 32, 20);
        CopyBgTilemapBufferToVram(1);
        HideBg(1);
        DexScreen_RemoveWindow(&sPokedexScreenData->modeSelectWindowId);
        DexScreen_RemoveWindow(&sPokedexScreenData->selectionIconWindowId);
        DexScreen_RemoveWindow(&sPokedexScreenData->dexCountsWindowId);
        gTasks[taskId].func = Task_DexScreen_AreaList;
        sPokedexScreenData->state = 0;
        break;
#endif
    }
}

static void DexScreen_InitGfxForTopMenu(void)
{
    struct ListMenuTemplate listMenuTemplate;
    FillBgTilemapBufferRect(3, 0x00E, 0, 0, 30, 20, 0);
    FillBgTilemapBufferRect(2, 0x000, 0, 0, 30, 20, 17);
    FillBgTilemapBufferRect(1, 0x000, 0, 0, 30, 20, 17);
    sPokedexScreenData->modeSelectWindowId = AddWindow(&sWindowTemplate_ModeSelect);
    sPokedexScreenData->selectionIconWindowId = AddWindow(&sWindowTemplate_SelectionIcon);
    sPokedexScreenData->dexCountsWindowId = AddWindow(&sWindowTemplate_DexCounts);
    if (IsNationalPokedexEnabled())
    {
        listMenuTemplate = sListMenuTemplate_NatDexModeSelect;
        listMenuTemplate.windowId = sPokedexScreenData->modeSelectWindowId;
        sPokedexScreenData->modeSelectListMenuId = ListMenuInit(&listMenuTemplate, sPokedexScreenData->modeSelectCursorPos, sPokedexScreenData->modeSelectItemsAbove);
        FillWindowPixelBuffer(sPokedexScreenData->dexCountsWindowId, PIXEL_FILL(0));
        DexScreen_AddTextPrinterParameterized(sPokedexScreenData->dexCountsWindowId, FONT_SMALL, gText_Seen, 0, 2, 0);
        DexScreen_AddTextPrinterParameterized(sPokedexScreenData->dexCountsWindowId, FONT_SMALL, gText_Kanto, 8, 13, 0);
        DexScreen_PrintNum3RightAlign(sPokedexScreenData->dexCountsWindowId, 0, sPokedexScreenData->numSeenKanto, 52, 13, 2);
        DexScreen_AddTextPrinterParameterized(sPokedexScreenData->dexCountsWindowId, FONT_SMALL, gText_National, 8, 24, 0);
        DexScreen_PrintNum3RightAlign(sPokedexScreenData->dexCountsWindowId, 0, sPokedexScreenData->numSeenNational, 52, 24, 2);
        DexScreen_AddTextPrinterParameterized(sPokedexScreenData->dexCountsWindowId, FONT_SMALL, gText_Owned, 0, 37, 0);
        DexScreen_AddTextPrinterParameterized(sPokedexScreenData->dexCountsWindowId, FONT_SMALL, gText_Kanto, 8, 48, 0);
        DexScreen_PrintNum3RightAlign(sPokedexScreenData->dexCountsWindowId, 0, sPokedexScreenData->numOwnedKanto, 52, 48, 2);
        DexScreen_AddTextPrinterParameterized(sPokedexScreenData->dexCountsWindowId, FONT_SMALL, gText_National, 8, 59, 0);
        DexScreen_PrintNum3RightAlign(sPokedexScreenData->dexCountsWindowId, 0, sPokedexScreenData->numOwnedNational, 52, 59, 2);
    }
    else
    {
        listMenuTemplate = sListMenuTemplate_KantoDexModeSelect;
        listMenuTemplate.windowId = sPokedexScreenData->modeSelectWindowId;
        sPokedexScreenData->modeSelectListMenuId = ListMenuInit(&listMenuTemplate, sPokedexScreenData->modeSelectCursorPos, sPokedexScreenData->modeSelectItemsAbove);
        FillWindowPixelBuffer(sPokedexScreenData->dexCountsWindowId, PIXEL_FILL(0));
        DexScreen_AddTextPrinterParameterized(sPokedexScreenData->dexCountsWindowId, FONT_NORMAL_COPY_1, gText_Seen, 0, 9, 0);
        DexScreen_PrintNum3RightAlign(sPokedexScreenData->dexCountsWindowId, 1, sPokedexScreenData->numSeenKanto, 32, 21, 2);
        DexScreen_AddTextPrinterParameterized(sPokedexScreenData->dexCountsWindowId, FONT_NORMAL_COPY_1, gText_Owned, 0, 37, 0);
        DexScreen_PrintNum3RightAlign(sPokedexScreenData->dexCountsWindowId, 1, sPokedexScreenData->numOwnedKanto, 32, 49, 2);
    }
    FillWindowPixelBuffer(0, PIXEL_FILL(15));
    DexScreen_PrintStringWithAlignment(gText_PokedexTableOfContents, TEXT_CENTER);
    FillWindowPixelBuffer(1, PIXEL_FILL(15));
    DexScreen_PrintControlInfo(gText_PickOK);
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_GFX);
    PutWindowTilemap(1);
    CopyWindowToVram(1, COPYWIN_GFX);
    PutWindowTilemap(sPokedexScreenData->dexCountsWindowId);
    CopyWindowToVram(sPokedexScreenData->dexCountsWindowId, COPYWIN_GFX);
}

static void MoveCursorFunc_DexModeSelect(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
    if (itemIndex == LIST_CANCEL)
    {
        CopyToWindowPixelBuffer(sPokedexScreenData->selectionIconWindowId, sTopMenuSelectionIconTiles_Cancel, 0x000, 0x000);
        LoadPalette(sTopMenuSelectionIconPals_Cancel, BG_PLTT_ID(1), PLTT_SIZE_4BPP);
    }
    else
    {
        CopyToWindowPixelBuffer(sPokedexScreenData->selectionIconWindowId, sTopMenuSelectionIconGfxPtrs[itemIndex].tiles, 0x000, 0x000);
        LoadPalette(sTopMenuSelectionIconGfxPtrs[itemIndex].pal, BG_PLTT_ID(1), PLTT_SIZE_4BPP);
    }
    PutWindowTilemap(sPokedexScreenData->selectionIconWindowId);
    CopyWindowToVram(sPokedexScreenData->selectionIconWindowId, COPYWIN_GFX);
}

static void ItemPrintFunc_DexModeSelect(u8 windowId, u32 itemId, u8 y)
{
    if (itemId >= DEX_CATEGORY_COUNT || sPokedexScreenData->unlockedCategories & (1 << itemId))
        ListMenuOverrideSetColors(TEXT_COLOR_WHITE, TEXT_COLOR_TRANSPARENT, TEXT_COLOR_LIGHT_GRAY);
    else
        ListMenuOverrideSetColors(TEXT_DYNAMIC_COLOR_1, TEXT_COLOR_TRANSPARENT, TEXT_DYNAMIC_COLOR_2);
}

static void Task_DexScreen_NumericalOrder(u8 taskId)
{
    switch (sPokedexScreenData->state)
    {
    case 0:
        ListMenuLoadStdPalAt(BG_PLTT_ID(1), 0);
        ListMenuLoadStdPalAt(BG_PLTT_ID(2), 1);
        sPokedexScreenData->orderedDexCount = DexScreen_CountMonsInOrderedList(sPokedexScreenData->dexOrderId);
        sPokedexScreenData->state = 2;
        break;
    case 1:
        DexScreen_DestroyDexOrderListMenu(sPokedexScreenData->dexOrderId);
        HideBg(1);
        DexScreen_RemoveWindow(&sPokedexScreenData->numericalOrderWindowId);
        gTasks[taskId].func = Task_PokedexScreen;
        sPokedexScreenData->state = 0;
        break;
    case 2:
        DexScreen_InitGfxForNumericalOrderList();
        sPokedexScreenData->state = 3;
        break;
    case 3:
        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(1);
        sPokedexScreenData->state = 4;
        break;
    case 4:
        ShowBg(1);
        BeginNormalPaletteFade(~0x8000, 0, 16, 0, RGB_WHITEALPHA);
        sPokedexScreenData->state = 5;
        break;
    case 5:
        ListMenuGetScrollAndRow(sPokedexScreenData->modeSelectListMenuId, &sPokedexScreenData->modeSelectCursorPosBak, NULL);
        sPokedexScreenData->scrollArrowsTaskId = DexScreen_CreateDexOrderScrollArrows();
        sPokedexScreenData->state = 6;
        break;
    case 6:
        sPokedexScreenData->characteristicMenuInput = ListMenu_ProcessInput(sPokedexScreenData->orderedListMenuTaskId);
        ListMenuGetScrollAndRow(sPokedexScreenData->modeSelectListMenuId, &sPokedexScreenData->modeSelectCursorPosBak, NULL);
        if (JOY_NEW(A_BUTTON))
        {
            if ((sPokedexScreenData->characteristicMenuInput >> 16) & 1)
            {
                sPokedexScreenData->dexSpecies = sPokedexScreenData->characteristicMenuInput;
                RemoveScrollIndicatorArrowPair(sPokedexScreenData->scrollArrowsTaskId);
                BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
                sPokedexScreenData->state = 7;
            }
        }
        else if (JOY_NEW(B_BUTTON))
        {
            RemoveScrollIndicatorArrowPair(sPokedexScreenData->scrollArrowsTaskId);
            BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
            sPokedexScreenData->state = 1;
        }
        break;
    case 7:
        DexScreen_DestroyDexOrderListMenu(sPokedexScreenData->dexOrderId);
        FillBgTilemapBufferRect_Palette0(1, 0x000, 0, 0, 32, 20);
        CopyBgTilemapBufferToVram(1);
        DexScreen_RemoveWindow(&sPokedexScreenData->numericalOrderWindowId);
        gTasks[taskId].func = Task_DexScreen_ShowMonPage;
        sPokedexScreenData->state = 0;
        break;
    }
}

static void DexScreen_InitGfxForNumericalOrderList(void)
{
    struct ListMenuTemplate template;
    FillBgTilemapBufferRect(3, 0x00E, 0, 0, 30, 20, 0);
    FillBgTilemapBufferRect(1, 0x000, 0, 0, 32, 32, 17);
    sPokedexScreenData->numericalOrderWindowId = AddWindow(&sWindowTemplate_OrderedListMenu);
    template = sListMenuTemplate_OrderedListMenu;
    template.items = sPokedexScreenData->listItems;
    template.windowId = sPokedexScreenData->numericalOrderWindowId;
    template.totalItems = sPokedexScreenData->orderedDexCount;
    DexScreen_InitListMenuForOrderedList(&template, sPokedexScreenData->dexOrderId);
    FillWindowPixelBuffer(0, PIXEL_FILL(15));
    DexScreen_PrintStringWithAlignment(gText_PokemonListNoColor, TEXT_CENTER);
    FillWindowPixelBuffer(1, PIXEL_FILL(15));
    DexScreen_PrintControlInfo(gText_PickOKExit);
    CopyWindowToVram(0, COPYWIN_GFX);
    CopyWindowToVram(1, COPYWIN_GFX);
}

static void Task_DexScreen_CharacteristicOrder(u8 taskId)
{
    switch (sPokedexScreenData->state)
    {
    case 0:
        ListMenuLoadStdPalAt(BG_PLTT_ID(1), 0);
        ListMenuLoadStdPalAt(BG_PLTT_ID(2), 1);
        sPokedexScreenData->orderedDexCount = DexScreen_CountMonsInOrderedList(sPokedexScreenData->dexOrderId);
        sPokedexScreenData->state = 2;
        break;
    case 1:
        DexScreen_DestroyDexOrderListMenu(sPokedexScreenData->dexOrderId);
        HideBg(1);
        DexScreen_RemoveWindow(&sPokedexScreenData->numericalOrderWindowId);
        gTasks[taskId].func = Task_PokedexScreen;
        sPokedexScreenData->state = 0;
        break;
    case 2:
        DexScreen_CreateCharacteristicListMenu();
        sPokedexScreenData->state = 3;
        break;
    case 3:
        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(1);
        sPokedexScreenData->state = 4;
        break;
    case 4:
        ShowBg(1);
        BeginNormalPaletteFade(~0x8000, 0, 16, 0, RGB_WHITEALPHA);
        sPokedexScreenData->state = 5;
        break;
    case 5:
        ListMenuGetScrollAndRow(sPokedexScreenData->modeSelectListMenuId, &sPokedexScreenData->modeSelectCursorPosBak, NULL);
        sPokedexScreenData->scrollArrowsTaskId = DexScreen_CreateDexOrderScrollArrows();
        sPokedexScreenData->state = 6;
        break;
    case 6:
        sPokedexScreenData->characteristicMenuInput = ListMenu_ProcessInput(sPokedexScreenData->orderedListMenuTaskId);
        ListMenuGetScrollAndRow(sPokedexScreenData->modeSelectListMenuId, &sPokedexScreenData->modeSelectCursorPosBak, NULL);
        if (JOY_NEW(A_BUTTON))
        {
            if (((sPokedexScreenData->characteristicMenuInput >> 16) & 1) && !DexScreen_LookUpCategoryBySpecies(sPokedexScreenData->characteristicMenuInput))
            {
                RemoveScrollIndicatorArrowPair(sPokedexScreenData->scrollArrowsTaskId);
                BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
                sPokedexScreenData->state = 7;
            }
        }
        else if (JOY_NEW(B_BUTTON))
        {
            RemoveScrollIndicatorArrowPair(sPokedexScreenData->scrollArrowsTaskId);
            BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
            sPokedexScreenData->state = 1;
        }
        break;
    case 7:
        DexScreen_DestroyDexOrderListMenu(sPokedexScreenData->dexOrderId);
        FillBgTilemapBufferRect_Palette0(1, 0x000, 0, 0, 32, 20);
        CopyBgTilemapBufferToVram(1);
        DexScreen_RemoveWindow(&sPokedexScreenData->numericalOrderWindowId);
        sPokedexScreenData->parentOfCategoryMenu = 1;
        gTasks[taskId].func = Task_DexScreen_CategorySubmenu;
        sPokedexScreenData->state = 0;
        break;
    }
}

static void DexScreen_CreateCharacteristicListMenu(void)
{
    struct ListMenuTemplate template;
    FillBgTilemapBufferRect(3, 0x00E, 0, 0, 30, 20, 0);
    FillBgTilemapBufferRect(1, 0x000, 0, 0, 32, 32, 17);
    sPokedexScreenData->numericalOrderWindowId = AddWindow(&sWindowTemplate_OrderedListMenu);
    template = sListMenuTemplate_OrderedListMenu;
    template.items = sPokedexScreenData->listItems;
    template.windowId = sPokedexScreenData->numericalOrderWindowId;
    template.totalItems = sPokedexScreenData->orderedDexCount;
    DexScreen_InitListMenuForOrderedList(&template, sPokedexScreenData->dexOrderId);
    FillWindowPixelBuffer(0, PIXEL_FILL(15));
    DexScreen_PrintStringWithAlignment(gText_SearchNoColor, TEXT_CENTER);
    FillWindowPixelBuffer(1, PIXEL_FILL(15));
    DexScreen_PrintControlInfo(gText_PickOKExit);
    CopyWindowToVram(0, COPYWIN_GFX);
    CopyWindowToVram(1, COPYWIN_GFX);
}

static u16 DexScreen_CountMonsInOrderedList(u8 orderIdx)
{
    s32 max_n = IsNationalPokedexEnabled() ? NATIONAL_DEX_COUNT : KANTO_DEX_COUNT;
    u16 ndex_num;
    u16 ret = NATIONAL_DEX_NONE;
    s32 i;
    bool8 caught;
    bool8 seen;

    switch (orderIdx)
    {
    default:
    case DEX_ORDER_NUMERICAL_KANTO:
        for (i = 0; i < KANTO_DEX_COUNT; i++)
        {
            ndex_num = i + 1;
            seen = DexScreen_GetSetPokedexFlag(ndex_num, FLAG_GET_SEEN, FALSE);
            caught = DexScreen_GetSetPokedexFlag(ndex_num, FLAG_GET_CAUGHT, FALSE);
            if (seen)
            {
                sPokedexScreenData->listItems[i].label = gSpeciesNames[NationalPokedexNumToSpecies(ndex_num)];
                ret = ndex_num;
            }
            else
            {
                sPokedexScreenData->listItems[i].label = gText_5Dashes;
            }
            sPokedexScreenData->listItems[i].index = (caught << 17) + (seen << 16) + NationalPokedexNumToSpecies(ndex_num);
        }
        break;
    case DEX_ORDER_ATOZ:
        for (i = 0; i < NUM_SPECIES - 1; i++)
        {
            ndex_num = gPokedexOrder_Alphabetical[i];
            if (ndex_num <= max_n)
            {
                seen = DexScreen_GetSetPokedexFlag(ndex_num, FLAG_GET_SEEN, FALSE);
                caught = DexScreen_GetSetPokedexFlag(ndex_num, FLAG_GET_CAUGHT, FALSE);
                if (seen)
                {
                    sPokedexScreenData->listItems[ret].label = gSpeciesNames[NationalPokedexNumToSpecies(ndex_num)];
                    sPokedexScreenData->listItems[ret].index = (caught << 17) + (seen << 16) + NationalPokedexNumToSpecies(ndex_num);
                    ret++;
                }
            }
        }
        break;
    case DEX_ORDER_TYPE:
        for (i = 0; i < NUM_SPECIES - 1; i++)
        {
            ndex_num = SpeciesToNationalPokedexNum(gPokedexOrder_Type[i]);
            if (ndex_num <= max_n)
            {
                seen = DexScreen_GetSetPokedexFlag(ndex_num, FLAG_GET_SEEN, FALSE);
                caught = DexScreen_GetSetPokedexFlag(ndex_num, FLAG_GET_CAUGHT, FALSE);
                if (caught)
                {
                    sPokedexScreenData->listItems[ret].label = gSpeciesNames[NationalPokedexNumToSpecies(ndex_num)];
                    sPokedexScreenData->listItems[ret].index = (caught << 17) + (seen << 16) + NationalPokedexNumToSpecies(ndex_num);
                    ret++;
                }
            }
        }
        break;
    case DEX_ORDER_LIGHTEST:
        for (i = 0; i < NATIONAL_DEX_COUNT; i++)
        {
            ndex_num = gPokedexOrder_Weight[i];
            if (ndex_num <= max_n)
            {
                seen = DexScreen_GetSetPokedexFlag(ndex_num, FLAG_GET_SEEN, FALSE);
                caught = DexScreen_GetSetPokedexFlag(ndex_num, FLAG_GET_CAUGHT, FALSE);
                if (caught)
                {
                    sPokedexScreenData->listItems[ret].label = gSpeciesNames[NationalPokedexNumToSpecies(ndex_num)];
                    sPokedexScreenData->listItems[ret].index = (caught << 17) + (seen << 16) + NationalPokedexNumToSpecies(ndex_num);
                    ret++;
                }
            }
        }
        break;
    case DEX_ORDER_SMALLEST:
        for (i = 0; i < NATIONAL_DEX_COUNT; i++)
        {
            ndex_num = gPokedexOrder_Height[i];
            if (ndex_num <= max_n)
            {
                seen = DexScreen_GetSetPokedexFlag(ndex_num, FLAG_GET_SEEN, FALSE);
                caught = DexScreen_GetSetPokedexFlag(ndex_num, FLAG_GET_CAUGHT, FALSE);
                if (caught)
                {
                    sPokedexScreenData->listItems[ret].label = gSpeciesNames[NationalPokedexNumToSpecies(ndex_num)];
                    sPokedexScreenData->listItems[ret].index = (caught << 17) + (seen << 16) + NationalPokedexNumToSpecies(ndex_num);
                    ret++;
                }
            }
        }
        break;
    case DEX_ORDER_NUMERICAL_NATIONAL:
        for (i = 0; i < NATIONAL_DEX_COUNT; i++)
        {
            ndex_num = i + 1;
            seen = DexScreen_GetSetPokedexFlag(ndex_num, FLAG_GET_SEEN, FALSE);
            caught = DexScreen_GetSetPokedexFlag(ndex_num, FLAG_GET_CAUGHT, FALSE);
            if (seen)
            {
                sPokedexScreenData->listItems[i].label = gSpeciesNames[NationalPokedexNumToSpecies(ndex_num)];
                ret = ndex_num;
            }
            else
            {
                sPokedexScreenData->listItems[i].label = gText_5Dashes;
            }
            sPokedexScreenData->listItems[i].index = (caught << 17) + (seen << 16) + NationalPokedexNumToSpecies(ndex_num);
        }
        break;
    }
    return ret;
}

static void DexScreen_InitListMenuForOrderedList(const struct ListMenuTemplate * template, u8 order)
{
    switch (order)
    {
    default:
    case DEX_ORDER_NUMERICAL_KANTO:
        sPokedexScreenData->orderedListMenuTaskId = ListMenuInitInRect(template, sListMenuRects_OrderedList, sPokedexScreenData->kantoOrderMenuCursorPos, sPokedexScreenData->kantoOrderMenuItemsAbove);
        break;
    case DEX_ORDER_ATOZ:
    case DEX_ORDER_TYPE:
    case DEX_ORDER_LIGHTEST:
    case DEX_ORDER_SMALLEST:
        sPokedexScreenData->orderedListMenuTaskId = ListMenuInitInRect(template, sListMenuRects_OrderedList, sPokedexScreenData->characteristicOrderMenuCursorPos, sPokedexScreenData->characteristicOrderMenuItemsAbove);
        break;
    case DEX_ORDER_NUMERICAL_NATIONAL:
        sPokedexScreenData->orderedListMenuTaskId = ListMenuInitInRect(template, sListMenuRects_OrderedList, sPokedexScreenData->nationalOrderMenuCursorPos, sPokedexScreenData->nationalOrderMenuItemsAbove);
        break;
    }
}

static void DexScreen_DestroyDexOrderListMenu(u8 order)
{
    switch (order)
    {
    default:
    case DEX_ORDER_NUMERICAL_KANTO:
        DestroyListMenuTask(sPokedexScreenData->orderedListMenuTaskId, &sPokedexScreenData->kantoOrderMenuCursorPos, &sPokedexScreenData->kantoOrderMenuItemsAbove);
        break;
    case DEX_ORDER_ATOZ:
    case DEX_ORDER_TYPE:
    case DEX_ORDER_LIGHTEST:
    case DEX_ORDER_SMALLEST:
        DestroyListMenuTask(sPokedexScreenData->orderedListMenuTaskId, &sPokedexScreenData->characteristicOrderMenuCursorPos, &sPokedexScreenData->characteristicOrderMenuItemsAbove);
        break;
    case DEX_ORDER_NUMERICAL_NATIONAL:
        DestroyListMenuTask(sPokedexScreenData->orderedListMenuTaskId, &sPokedexScreenData->nationalOrderMenuCursorPos, &sPokedexScreenData->nationalOrderMenuItemsAbove);
        break;
    }
}

static u8 DexScreen_CreateDexOrderScrollArrows(void)
{
    struct ScrollArrowsTemplate template = sDexOrderScrollArrowsTemplate;
    if (sPokedexScreenData->orderedDexCount > sListMenuTemplate_OrderedListMenu.maxShowed)
        template.fullyDownThreshold = sPokedexScreenData->orderedDexCount - sListMenuTemplate_OrderedListMenu.maxShowed;
    else
        template.fullyDownThreshold = 0;
    return AddScrollIndicatorArrowPair(&template, &sPokedexScreenData->modeSelectCursorPosBak);
}

struct PokedexListItem
{
    u16 species;
    bool8 seen:1;
    bool8 caught:1;
};

static void ItemPrintFunc_OrderedListMenu(u8 windowId, u32 itemId, u8 y)
{
    u16 species = itemId;
    bool8 seen = (itemId >> 16) & 1;  // not used but required to match
    bool8 caught = (itemId >> 17) & 1;
    u8 type1;
    DexScreen_PrintMonDexNo(sPokedexScreenData->numericalOrderWindowId, FONT_SMALL, species, 12, y);
    if (caught)
    {
        BlitMenuInfoIcon(sPokedexScreenData->numericalOrderWindowId, MENU_INFO_ICON_CAUGHT, 0x28, y);
        type1 = gSpeciesInfo[species].types[0];
        BlitMenuInfoIcon(sPokedexScreenData->numericalOrderWindowId, type1 + 1, 0x78, y);
        if (type1 != gSpeciesInfo[species].types[1])
            BlitMenuInfoIcon(sPokedexScreenData->numericalOrderWindowId, gSpeciesInfo[species].types[1] + 1, 0x98, y);
    }
}

static void Task_DexScreen_CategorySubmenu(u8 taskId)
{
    int pageFlipCmd;
    u8 *ptr;
    switch (sPokedexScreenData->state)
    {
    case 0:
        HideBg(3);
        HideBg(2);
        HideBg(1);
        DexScreen_GetPageLimitsForCategory(sPokedexScreenData->category);
        if (sPokedexScreenData->pageNum < sPokedexScreenData->firstPageInCategory)
            sPokedexScreenData->pageNum = sPokedexScreenData->firstPageInCategory;
        sPokedexScreenData->state = 2;
        break;
    case 1:
        DexScreen_DestroyCategoryPageMonIconAndInfoWindows();
        HideBg(2);
        HideBg(1);
        switch (sPokedexScreenData->parentOfCategoryMenu)
        {
        case 0:
        default:
            gTasks[taskId].func = Task_PokedexScreen;
            break;
        case 1:
            gTasks[taskId].func = Task_DexScreen_CharacteristicOrder;
            break;
        }
        sPokedexScreenData->state = 0;
        break;
    case 2:
        DexScreen_CreateCategoryListGfx(FALSE);
        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(1);
        DexScreen_CreateCategoryPageSelectionCursor(0xFF);
        sPokedexScreenData->state = 3;
        break;
    case 3:
        BeginNormalPaletteFade(~0x8000, 0, 16, 0, RGB_WHITEALPHA);
        ShowBg(3);
        ShowBg(2);
        ShowBg(1);
        sPokedexScreenData->state = 4;
        break;
    case 4:
        sPokedexScreenData->scrollArrowsTaskId = DexScreen_CreateCategoryMenuScrollArrows();
        sPokedexScreenData->categoryPageCursorTaskId = ListMenuAddCursorObjectInternal(&sCursorStruct_CategoryPage, 0);
        sPokedexScreenData->state = 5;
        break;
    case 5:
        DexScreen_CreateCategoryPageSelectionCursor(sPokedexScreenData->categoryCursorPosInPage);
        DexScreen_UpdateCategoryPageCursorObject(sPokedexScreenData->categoryPageCursorTaskId, sPokedexScreenData->categoryCursorPosInPage, sPokedexScreenData->numMonsOnPage);
        sPokedexScreenData->modeSelectCursorPosBak = sPokedexScreenData->pageNum;
        pageFlipCmd = 0;
        if (JOY_NEW(A_BUTTON) && DexScreen_GetSetPokedexFlag(sPokedexScreenData->pageSpecies[sPokedexScreenData->categoryCursorPosInPage], FLAG_GET_SEEN, TRUE))
        {
            RemoveScrollIndicatorArrowPair(sPokedexScreenData->scrollArrowsTaskId);
            ListMenuRemoveCursorObject(sPokedexScreenData->categoryPageCursorTaskId, 0);
            sPokedexScreenData->state = 12;
            break;
        }
        if (!JOY_HELD(R_BUTTON) && JOY_REPT(DPAD_LEFT))
        {
            if (sPokedexScreenData->categoryCursorPosInPage != 0)
            {
                sPokedexScreenData->categoryCursorPosInPage--;
                PlaySE(SE_SELECT);
                break;
            }
            else
                pageFlipCmd = 1;
        }
        if (!JOY_HELD(R_BUTTON) && JOY_REPT(DPAD_RIGHT))
        {
            if (sPokedexScreenData->categoryCursorPosInPage < sPokedexScreenData->numMonsOnPage - 1)
            {
                sPokedexScreenData->categoryCursorPosInPage++;
                PlaySE(SE_SELECT);
                break;
            }
            else
                pageFlipCmd = 2;
        }
        if (pageFlipCmd == 0)
            pageFlipCmd = DexScreen_InputHandler_GetShoulderInput();
        switch (pageFlipCmd)
        {
        case 0: // No action
            break;
        case 1: // Left
            while (sPokedexScreenData->pageNum > sPokedexScreenData->firstPageInCategory)
            {
                sPokedexScreenData->pageNum--;
                if (DexScreen_IsPageUnlocked(sPokedexScreenData->category, sPokedexScreenData->pageNum))
                {
                    sPokedexScreenData->state = 8;
                    break;
                }
            }
            if (sPokedexScreenData->state != 8)
                sPokedexScreenData->state = 6;
            break;
        case 2: // Right
            while (sPokedexScreenData->pageNum < sPokedexScreenData->lastPageInCategory - 1)
            {
                sPokedexScreenData->pageNum++;
                if (DexScreen_IsPageUnlocked(sPokedexScreenData->category, sPokedexScreenData->pageNum))
                {
                    sPokedexScreenData->state = 10;
                    break;
                }
            }
            if (sPokedexScreenData->state != 10)
                sPokedexScreenData->state = 6;
            break;
        }
        if (JOY_NEW(B_BUTTON))
        {
            sPokedexScreenData->state = 6;
        }
        break;
    case 6:
    case 7:
        RemoveScrollIndicatorArrowPair(sPokedexScreenData->scrollArrowsTaskId);
        ListMenuRemoveCursorObject(sPokedexScreenData->categoryPageCursorTaskId, 0);
        BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
        sPokedexScreenData->state = 1;
        break;
    case 8:
    case 10:
        DexScreen_DestroyCategoryPageMonIconAndInfoWindows();
        DexScreen_CreateCategoryPageSelectionCursor(0xFF);
        ListMenuUpdateCursorObject(sPokedexScreenData->categoryPageCursorTaskId, 0, 0xA0, 0);
        sPokedexScreenData->categoryPageSelectionCursorTimer = 0;
        sPokedexScreenData->data[0] = 0;
        sPokedexScreenData->state++;
        break;
    case 9:
        if (DexScreen_FlipCategoryPageInDirection(0))
        {
            sPokedexScreenData->categoryCursorPosInPage = sPokedexScreenData->numMonsOnPage - 1;
            sPokedexScreenData->state = 5;
        }
        break;
    case 11:
        if (DexScreen_FlipCategoryPageInDirection(1))
        {
            sPokedexScreenData->categoryCursorPosInPage = 0;
            sPokedexScreenData->state = 5;
        }
        break;
    case 12:
        sPokedexScreenData->dexSpecies = sPokedexScreenData->pageSpecies[sPokedexScreenData->categoryCursorPosInPage];
        PlaySE(SE_SELECT);
        sPokedexScreenData->state = 14;
        break;
    case 13:
        RemoveDexPageWindows();
        sPokedexScreenData->state = 4;
        break;
    case 14:
        DexScreen_DrawMonDexPage(FALSE);
        sPokedexScreenData->state = 15;
        break;
    case 15:
        sPokedexScreenData->data[0] = 0;
        sPokedexScreenData->data[1] = 0;
        sPokedexScreenData->state++;
        // fallthrough
    case 16:
        if (sPokedexScreenData->data[1] < 6)
        {
            if (sPokedexScreenData->data[0])
            {
                DexScreen_DexPageZoomEffectFrame(0, sPokedexScreenData->data[1]);
                CopyBgTilemapBufferToVram(0);
                sPokedexScreenData->data[0] = 4;
                sPokedexScreenData->data[1]++;
            }
            else
            {
                sPokedexScreenData->data[0]--;
            }
        }
        else
        {
            FillBgTilemapBufferRect_Palette0(0, 0x000, 0, 2, 30, 16);
            CopyBgTilemapBufferToVram(3);
            CopyBgTilemapBufferToVram(2);
            CopyBgTilemapBufferToVram(1);
            CopyBgTilemapBufferToVram(0);
            PlayCry_NormalNoDucking(sPokedexScreenData->dexSpecies, 0, CRY_VOLUME_RS, CRY_PRIORITY_NORMAL);
            sPokedexScreenData->data[0] = 0;
            sPokedexScreenData->state = 17;
        }
        break;
    case 17:
        if (JOY_NEW(A_BUTTON))
        {
            HideBg(2); // HACK: troca de página sem mostrar tiles pela metade
            HideBg(1);
            RemoveDexPageWindows();
            FillBgTilemapBufferRect_Palette0(1, 0x000, 0, 2, 30, 16);
            CopyBgTilemapBufferToVram(1);
            sPokedexScreenData->state = 21;
        }
        else if (JOY_NEW(B_BUTTON))
        {
            sPokedexScreenData->state = 18;
        }
        else
        {
            DexScreen_InputHandler_StartToCry();
        }
        break;
    case 18:
        DexScreen_CreateCategoryListGfx(FALSE);
        DexScreen_DexPageZoomEffectFrame(0, 6);
        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(0);
        sPokedexScreenData->state = 19;
        break;
    case 19:
        sPokedexScreenData->data[0] = 0;
        sPokedexScreenData->data[1] = 6;
        sPokedexScreenData->state++;
        // fallthrough
    case 20:
        if (sPokedexScreenData->data[1])
        {
            if (sPokedexScreenData->data[0])
            {
                sPokedexScreenData->data[1]--;
                FillBgTilemapBufferRect_Palette0(0, 0x000, 0, 2, 30, 16);
                DexScreen_DexPageZoomEffectFrame(0, sPokedexScreenData->data[1]);
                CopyBgTilemapBufferToVram(0);
                sPokedexScreenData->data[0] = 1;
            }
            else
                sPokedexScreenData->data[0]--;
        }
        else
        {
            FillBgTilemapBufferRect_Palette0(0, 0x000, 0, 2, 30, 16);
            CopyBgTilemapBufferToVram(0);
            sPokedexScreenData->state = 13;
        }
        break;
    case 21:
        DexScreen_DrawMonAreaPage();
        sPokedexScreenData->state = 22;
        break;
    case 22:
        if (IsDma3ManagerBusyWithBgCopy()) // HACK: espera os gráficos chegarem antes do mapa de tiles
            break;
        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(0);
        sPokedexScreenData->state = 28;
        break;
    case 28: // HACK: só mostra quando tudo foi copiado
        if (IsDma3ManagerBusyWithBgCopy())
            break;
        ShowBg(2);
        ShowBg(1);
        sPokedexScreenData->state = 23;
        break;
    case 23:
        if (DexExtra_CanOpen() && JOY_NEW(A_BUTTON | START_BUTTON)) // HACK: A = POKéMON DATA, START = MOVES
        {
            sDexExtraOpenPage = JOY_NEW(A_BUTTON) ? 0 : 1;
            DexExtra_ClearBgs();
            sPokedexScreenData->state = 30;
        }
        else if (JOY_NEW(A_BUTTON))
        {
            FillBgTilemapBufferRect_Palette0(2, 0x000, 0, 2, 30, 16);
            FillBgTilemapBufferRect_Palette0(1, 0x000, 0, 2, 30, 16);
            FillBgTilemapBufferRect_Palette0(0, 0x000, 0, 2, 30, 16);
            CopyBgTilemapBufferToVram(2);
            CopyBgTilemapBufferToVram(1);
            CopyBgTilemapBufferToVram(0);
            sPokedexScreenData->state = 26;
        }
        else if (JOY_NEW(B_BUTTON))
        {
            HideBg(2); // HACK
            HideBg(1);
            FillBgTilemapBufferRect_Palette0(2, 0x000, 0, 2, 30, 16);
            FillBgTilemapBufferRect_Palette0(1, 0x000, 0, 2, 30, 16);
            FillBgTilemapBufferRect_Palette0(0, 0x000, 0, 2, 30, 16);
            CopyBgTilemapBufferToVram(2);
            CopyBgTilemapBufferToVram(1);
            CopyBgTilemapBufferToVram(0);
            sPokedexScreenData->state = 24;
        }
        else
        {
            DexScreen_InputHandler_StartToCry();
        }
        break;
    case 24:
        DexScreen_DestroyAreaScreenResources();
        sPokedexScreenData->state = 25;
        break;
    case 25:
        DexScreen_DrawMonDexPage(FALSE);
        sPokedexScreenData->state = 27;
        break;
    case 27: // HACK: espera os gráficos, copia o mapa de tiles e só então mostra
        if (IsDma3ManagerBusyWithBgCopy())
            break;
        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(0);
        sPokedexScreenData->state = 29;
        break;
    case 29:
        if (IsDma3ManagerBusyWithBgCopy())
            break;
        ShowBg(2);
        ShowBg(1);
        sPokedexScreenData->state = 17;
        break;
    case 26:
        DexScreen_DestroyAreaScreenResources();
        sPokedexScreenData->state = 18;
        break;
    case 30: // HACK: abre POKéMON DATA ou MOVES
        HideBg(2); // HACK: mostra só depois que tudo foi copiado (ver DexExtra_HandleInput)
        HideBg(1);
        DexScreen_DestroyAreaScreenResources();
        DexExtra_Open(sDexExtraOpenPage);
        sDexExtraPendingShow = 1;
        sPokedexScreenData->state = 31;
        break;
    case 31:
        switch (DexExtra_HandleInput())
        {
        case 1: // B: volta para SIZE/AREA
            HideBg(2);
            HideBg(1);
            DexExtra_Close();
            DexExtra_ClearBgs();
            sPokedexScreenData->state = 21;
            break;
        case 2: // A: sai das informações do Pokémon
            DexExtra_Close();
            DexExtra_ClearBgs();
            sPokedexScreenData->state = 18;
            break;
        }
        break;
    }
}

static u8 DexScreen_CreateCategoryMenuScrollArrows(void)
{
    struct ScrollArrowsTemplate template = sScrollArrowsTemplate_CategoryMenu;
    template.fullyUpThreshold = sPokedexScreenData->firstPageInCategory;
    template.fullyDownThreshold = sPokedexScreenData->lastPageInCategory - 1;
    sPokedexScreenData->modeSelectCursorPosBak = sPokedexScreenData->pageNum;
    return AddScrollIndicatorArrowPair(&template, &sPokedexScreenData->modeSelectCursorPosBak);
}

/*
 * Returns 1 to flip pages left, 2 to flip pages right, 0 for no action
 */
static int DexScreen_InputHandler_GetShoulderInput(void)
{
    switch (gSaveBlock2Ptr->optionsButtonMode)
    {
    case OPTIONS_BUTTON_MODE_L_EQUALS_A:
        // Using the JOY_HELD and JOY_NEW macros here does not match!
        if ((gMain.heldKeys & R_BUTTON) && (gMain.newKeys & DPAD_LEFT))
            return 1;
        else if ((gMain.heldKeys & R_BUTTON) && (gMain.newKeys & DPAD_RIGHT))
            return 2;
        else
            return 0;
    case OPTIONS_BUTTON_MODE_LR:
        if (gMain.newKeys & L_BUTTON)
            return 1;
        else if (gMain.newKeys & R_BUTTON)
            return 2;
        else
            return 0;
    case OPTIONS_BUTTON_MODE_HELP:
    default:
        return 0;
    }
}

static void Task_DexScreen_ShowMonPage(u8 taskId)
{
    switch (sPokedexScreenData->state)
    {
    case 0:
        HideBg(3);
        HideBg(2);
        HideBg(1);
        sPokedexScreenData->state = 2;
        break;
    case 1:
        HideBg(2);
        HideBg(1);
#if DEX_AREA_LIST_ENABLED
        if (sDexFromAreaList) // HACK: aberto pelo AREA LIST
        {
            sDexFromAreaList = FALSE;
            gTasks[taskId].func = Task_DexScreen_AreaList;
        }
        else
#endif
        {
            gTasks[taskId].func = Task_DexScreen_NumericalOrder;
        }
        sPokedexScreenData->state = 0;
        break;
    case 2:
        sPokedexScreenData->numMonsOnPage = 1;
        DexScreen_DrawMonDexPage(FALSE);
        sPokedexScreenData->state = 3;
        break;
    case 3:
        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(0);
        PlayCry_NormalNoDucking(sPokedexScreenData->dexSpecies, 0, CRY_VOLUME_RS, CRY_PRIORITY_NORMAL);
        sPokedexScreenData->state = 4;
        break;
    case 4:
        BeginNormalPaletteFade(~0x8000, 0, 16, 0, RGB_WHITEALPHA);
        ShowBg(3);
        ShowBg(2);
        ShowBg(1);
        sPokedexScreenData->state = 5;
        break;
    case 5:
        if (JOY_NEW(A_BUTTON))
        {
            HideBg(2); // HACK: troca de página sem mostrar tiles pela metade
            HideBg(1);
            RemoveDexPageWindows();
            FillBgTilemapBufferRect_Palette0(1, 0x000, 0, 2, 30, 16);
            CopyBgTilemapBufferToVram(1);
            sPokedexScreenData->state = 7;
        }
        else if (JOY_NEW(B_BUTTON))
        {
            RemoveDexPageWindows();
            BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
            sPokedexScreenData->state = 1;
        }
        else if (JOY_NEW(DPAD_UP) && DexScreen_TryScrollMonsVertical(1))
        {
            RemoveDexPageWindows();
            BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
            sPokedexScreenData->state = 6;
        }
        else if (JOY_NEW(DPAD_DOWN) && DexScreen_TryScrollMonsVertical(0))
        {
            RemoveDexPageWindows();
            BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
            sPokedexScreenData->state = 6;
        }
        else
        {
            DexScreen_InputHandler_StartToCry();
        }
        break;
    case 6:
        HideBg(2);
        HideBg(1);
        sPokedexScreenData->dexSpecies = sPokedexScreenData->characteristicMenuInput;
        sPokedexScreenData->state = 2;
        break;
    case 7:
        DexScreen_DrawMonAreaPage();
        sPokedexScreenData->state = 8;
        break;
    case 8:
        if (IsDma3ManagerBusyWithBgCopy()) // HACK
            break;
        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(0);
        sPokedexScreenData->state = 14;
        break;
    case 14: // HACK: só mostra quando tudo foi copiado
        if (IsDma3ManagerBusyWithBgCopy())
            break;
        ShowBg(2);
        ShowBg(1);
        sPokedexScreenData->state = 9;
        break;
    case 9:
        if (DexExtra_CanOpen() && JOY_NEW(A_BUTTON | START_BUTTON)) // HACK: A = POKéMON DATA, START = MOVES
        {
            sDexExtraOpenPage = JOY_NEW(A_BUTTON) ? 0 : 1;
            DexExtra_ClearBgs();
            sPokedexScreenData->state = 30;
        }
        else if (JOY_NEW(A_BUTTON))
        {
            BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
            sPokedexScreenData->state = 12;
        }
        else if (JOY_NEW(B_BUTTON))
        {
            HideBg(2); // HACK
            HideBg(1);
            FillBgTilemapBufferRect_Palette0(2, 0x000, 0, 2, 30, 16);
            FillBgTilemapBufferRect_Palette0(1, 0x000, 0, 2, 30, 16);
            FillBgTilemapBufferRect_Palette0(0, 0x000, 0, 2, 30, 16);
            CopyBgTilemapBufferToVram(2);
            CopyBgTilemapBufferToVram(1);
            CopyBgTilemapBufferToVram(0);
            sPokedexScreenData->state = 10;
        }
        else
        {
            DexScreen_InputHandler_StartToCry();
        }
        break;
    case 10:
        DexScreen_DestroyAreaScreenResources();
        sPokedexScreenData->state = 11;
        break;
    case 11:
        DexScreen_DrawMonDexPage(FALSE);
        sPokedexScreenData->state = 13;
        break;
    case 13: // HACK: espera os gráficos, copia o mapa de tiles e só então mostra
        if (IsDma3ManagerBusyWithBgCopy())
            break;
        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(0);
        sPokedexScreenData->state = 15;
        break;
    case 15:
        if (IsDma3ManagerBusyWithBgCopy())
            break;
        ShowBg(2);
        ShowBg(1);
        sPokedexScreenData->state = 5;
        break;
    case 12:
        DexScreen_DestroyAreaScreenResources();
        FillBgTilemapBufferRect_Palette0(0, 0x000, 0, 2, 30, 16);
        CopyBgTilemapBufferToVram(0);
        sPokedexScreenData->state = 1;
        break;
    case 30: // HACK: abre POKéMON DATA ou MOVES
        HideBg(2); // HACK: mostra só depois que tudo foi copiado (ver DexExtra_HandleInput)
        HideBg(1);
        DexScreen_DestroyAreaScreenResources();
        DexExtra_Open(sDexExtraOpenPage);
        sDexExtraPendingShow = 1;
        sPokedexScreenData->state = 31;
        break;
    case 31:
        switch (DexExtra_HandleInput())
        {
        case 1: // B: volta para SIZE/AREA
            HideBg(2);
            HideBg(1);
            DexExtra_Close();
            DexExtra_ClearBgs();
            sPokedexScreenData->state = 7;
            break;
        case 2: // A: sai das informações do Pokémon
            DexExtra_Close();
            BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
            sPokedexScreenData->state = 32;
            break;
        }
        break;
    case 32:
        FillBgTilemapBufferRect_Palette0(0, 0x000, 0, 2, 30, 16);
        CopyBgTilemapBufferToVram(0);
        sPokedexScreenData->state = 1;
        break;
    }
}

static bool32 DexScreen_TryScrollMonsVertical(u8 direction)
{
    int selectedIndex;
    u16 *itemsAbove_p, *cursorPos_p;

    if (sDexFromAreaList) // HACK: vindo do AREA LIST não há lista para percorrer
        return FALSE;

    switch (sPokedexScreenData->dexOrderId)
    {
    default:
    case DEX_ORDER_NUMERICAL_KANTO:
        cursorPos_p = &sPokedexScreenData->kantoOrderMenuCursorPos;
        itemsAbove_p = &sPokedexScreenData->kantoOrderMenuItemsAbove;
        break;
    case DEX_ORDER_ATOZ:
    case DEX_ORDER_TYPE:
    case DEX_ORDER_LIGHTEST:
    case DEX_ORDER_SMALLEST:
        cursorPos_p = &sPokedexScreenData->characteristicOrderMenuCursorPos;
        itemsAbove_p = &sPokedexScreenData->characteristicOrderMenuItemsAbove;
        break;
    case DEX_ORDER_NUMERICAL_NATIONAL:
        cursorPos_p = &sPokedexScreenData->nationalOrderMenuCursorPos;
        itemsAbove_p = &sPokedexScreenData->nationalOrderMenuItemsAbove;
        break;
    }

    selectedIndex = *cursorPos_p + *itemsAbove_p;
    if (direction) // Seek up
    {
        if (selectedIndex == 0)
            return FALSE;

        selectedIndex--;
        while (selectedIndex >= 0) //Should be while (--selectedIndex >= 0) without the selectedIndex-- in the body or before the while at all, but this is needed to match.
        {
            if ((sPokedexScreenData->listItems[selectedIndex].index >> 16) & 1)
            {
                break;
            }
            selectedIndex--;
        }

        if (selectedIndex < 0)
        {
            return FALSE;
        }
    }
    else // Seek down
    {
        if (selectedIndex == sPokedexScreenData->orderedDexCount - 1)
        {
            return FALSE;
        }

        selectedIndex++;
        while (selectedIndex < sPokedexScreenData->orderedDexCount) //Should be while (++selectedIndex < sPokedexScreenData->orderedDexCount) without the selectedIndex++ in the body or before the while at all, but this is needed to match.
        {
            if ((sPokedexScreenData->listItems[selectedIndex].index >> 16) & 1)
                break;
            selectedIndex++;
        }
        if (selectedIndex >= sPokedexScreenData->orderedDexCount)
        {
            return FALSE;
        }
    }
    sPokedexScreenData->characteristicMenuInput = sPokedexScreenData->listItems[selectedIndex].index;

    if (sPokedexScreenData->orderedDexCount > 9)
    {
        if (selectedIndex < 4)
        {
            *cursorPos_p = 0;
            *itemsAbove_p = selectedIndex;
        }
        else if (selectedIndex >= (sPokedexScreenData->orderedDexCount - 4))
        {
            *cursorPos_p = (sPokedexScreenData->orderedDexCount - 9);
            *itemsAbove_p = selectedIndex + 9 - (sPokedexScreenData->orderedDexCount);
        }
        else
        {
            *cursorPos_p = selectedIndex - 4;
            *itemsAbove_p = 4;
        }
    }
    else
    {
        *cursorPos_p = 0;
        *itemsAbove_p = selectedIndex;
    }
    return TRUE;
}

static void DexScreen_RemoveWindow(u8 *windowId_p)
{
    if (*windowId_p != 0xFF)
    {
        RemoveWindow(*windowId_p);
        *windowId_p = 0xFF;
    }
}

static void DexScreen_AddTextPrinterParameterized(u8 windowId, u8 fontId, const u8 *str, u8 x, u8 y, u8 colorIdx)
{
    u8 textColor[3];
    switch (colorIdx)
    {
    case 0:
        textColor[0] = 0;
        textColor[1] = 1;
        textColor[2] = 3;
        break;
    case 1:
        textColor[0] = 0;
        textColor[1] = 5;
        textColor[2] = 1;
        break;
    case 2:
        textColor[0] = 0;
        textColor[1] = 15;
        textColor[2] = 14;
        break;
    case 3:
        textColor[0] = 0;
        textColor[1] = 11;
        textColor[2] = 1;
        break;
    case 4:
        textColor[0] = 0;
        textColor[1] = 1;
        textColor[2] = 2;
        break;
    case 5: // HACK: títulos das seções em marrom (cor da moldura)
        textColor[0] = 0;
        textColor[1] = 6;
        textColor[2] = 3;
        break;
    case 6: // HACK: aba atual em vermelho
        textColor[0] = 0;
        textColor[1] = 14;
        textColor[2] = 3;
        break;
    case 7: // HACK: abas inativas em cinza
        textColor[0] = 0;
        textColor[1] = 12;
        textColor[2] = 11;
        break;
    }
    AddTextPrinterParameterized4(windowId, fontId, x, y, fontId == FONT_SMALL ? 0 : 1, 0, textColor, -1, str);
}

static void DexScreen_PrintNum3LeadingZeroes(u8 windowId, u8 fontId, u16 num, u8 x, u8 y, u8 colorIdx)
{
    u8 buff[4];
    buff[0] = (num / 100) + CHAR_0;
    buff[1] = ((num %= 100) / 10) + CHAR_0;
    buff[2] = (num % 10) + CHAR_0;
    buff[3] = EOS;
    DexScreen_AddTextPrinterParameterized(windowId, fontId, buff, x, y, colorIdx);
}

static void DexScreen_PrintNum3RightAlign(u8 windowId, u8 fontId, u16 num, u8 x, u8 y, u8 colorIdx)
{
    u8 buff[4];
    int i;
    buff[0] = (num / 100) + CHAR_0;
    buff[1] = ((num %= 100) / 10) + CHAR_0;
    buff[2] = (num % 10) + CHAR_0;
    buff[3] = EOS;
    for (i = 0; i < 3; i++)
    {
        if (buff[i] != CHAR_0)
            break;
        buff[i] = CHAR_SPACE;
    }
    DexScreen_AddTextPrinterParameterized(windowId, fontId, buff, x, y, colorIdx);
}

static u32 DexScreen_GetDefaultPersonality(int species)
{
    switch (species)
    {
    case SPECIES_SPINDA:
        return gSaveBlock2Ptr->pokedex.spindaPersonality;
    case SPECIES_UNOWN:
        return gSaveBlock2Ptr->pokedex.unownPersonality;
    default:
        return 0;
    }
}

static void DexScreen_LoadMonPicInWindow(u8 windowId, u16 species, u16 paletteOffset)
{
    LoadMonPicInWindow(species, SHINY_ODDS, DexScreen_GetDefaultPersonality(species), TRUE, paletteOffset >> 4, windowId);
}

static void DexScreen_PrintMonDexNo(u8 windowId, u8 fontId, u16 species, u8 x, u8 y)
{
    u16 dexNum = SpeciesToNationalPokedexNum(species);
    DexScreen_AddTextPrinterParameterized(windowId, fontId, gText_PokedexNo, x, y, 0);
    DexScreen_PrintNum3LeadingZeroes(windowId, fontId, dexNum, x + 9, y, 0);
}

// HACK: "No.149 DRAGONITE" numa linha só (páginas de área, dados e golpes)
static void DexScreen_PrintMonNoAndName(u8 windowId, u16 species)
{
    u8 str[24];
    u8 *end;

    end = StringCopy(str, gText_PokedexNo);
    end = ConvertIntToDecimalStringN(end, SpeciesToNationalPokedexNum(species), STR_CONV_MODE_LEADING_ZEROS, 3);
    *end++ = CHAR_SPACE;
    StringCopy(end, gSpeciesNames[species]);
    DexScreen_AddTextPrinterParameterized(windowId, FONT_NORMAL, str, 0, DEX_HEADER_NAME_Y, 0);
}

s8 DexScreen_GetSetPokedexFlag(u16 nationalDexNo, u8 caseId, bool8 indexIsSpecies)
{
    u8 index;
    u8 bit;
    u8 mask;
    s8 retVal;

    if (indexIsSpecies)
        nationalDexNo = SpeciesToNationalPokedexNum(nationalDexNo);

    nationalDexNo--;
    index = nationalDexNo / 8;
    bit = nationalDexNo % 8;
    mask = 1 << bit;
    retVal = 0;
    switch (caseId)
    {
    case FLAG_GET_SEEN:
        if (gSaveBlock2Ptr->pokedex.seen[index] & mask)
        {
            // Anticheat
            if ((gSaveBlock2Ptr->pokedex.seen[index] & mask) == (gSaveBlock1Ptr->seen1[index] & mask)
                && (gSaveBlock2Ptr->pokedex.seen[index] & mask) == (gSaveBlock1Ptr->seen2[index] & mask))
                retVal = 1;
        }
        break;
    case FLAG_GET_CAUGHT:
        if (gSaveBlock2Ptr->pokedex.owned[index] & mask)
        {
            // Anticheat
            if ((gSaveBlock2Ptr->pokedex.owned[index] & mask) == (gSaveBlock2Ptr->pokedex.seen[index] & mask)
                && (gSaveBlock2Ptr->pokedex.owned[index] & mask) == (gSaveBlock1Ptr->seen1[index] & mask)
                && (gSaveBlock2Ptr->pokedex.owned[index] & mask) == (gSaveBlock1Ptr->seen2[index] & mask))
                retVal = 1;
        }
        break;
    case FLAG_SET_SEEN:
        gSaveBlock2Ptr->pokedex.seen[index] |= mask;
        // Anticheat
        gSaveBlock1Ptr->seen1[index] |= mask;
        gSaveBlock1Ptr->seen2[index] |= mask;
        break;
    case FLAG_SET_CAUGHT:
        gSaveBlock2Ptr->pokedex.owned[index] |= mask;
        break;
    }
    return retVal;
}

static u16 DexScreen_GetDexCount(u8 caseId, bool8 whichDex)
{
    u16 count = 0;
    u16 i;

    switch (whichDex)
    {
    case 0: // Kanto
        for (i = 0; i < KANTO_DEX_COUNT; i++)
        {
            if (DexScreen_GetSetPokedexFlag(i + 1, caseId, FALSE))
                count++;
        }
        break;
    case 1: // National
        for (i = 0; i < NATIONAL_DEX_COUNT; i++)
        {
            if (DexScreen_GetSetPokedexFlag(i + 1, caseId, FALSE))
                count++;

        }
        break;
    }
    return count;
}

static void DexScreen_PrintControlInfo(const u8 *src)
{
    DexScreen_AddTextPrinterParameterized(1, FONT_SMALL, src, 236 - GetStringWidth(FONT_SMALL, src, 0), 2, 4);
}

bool8 DexScreen_DrawMonPicInCategoryPage(u16 species, u8 slot, u8 numSlots)
{
    struct WindowTemplate template;
    numSlots--;
    CopyToBgTilemapBufferRect_ChangePalette(3, sCategoryPageIconWindowBg, sCategoryPageIconCoords[numSlots][slot][0], sCategoryPageIconCoords[numSlots][slot][1], 8, 8, slot + 5);
    if (sPokedexScreenData->categoryMonWindowIds[slot] == 0xFF)
    {
        template = sWindowTemplate_CategoryMonIcon;
        template.tilemapLeft = sCategoryPageIconCoords[numSlots][slot][0];
        template.tilemapTop = sCategoryPageIconCoords[numSlots][slot][1];
        template.paletteNum = slot + 1;
        template.baseBlock = slot * 64 + 8;
        sPokedexScreenData->categoryMonWindowIds[slot] = AddWindow(&template);
        FillWindowPixelBuffer(sPokedexScreenData->categoryMonWindowIds[slot], PIXEL_FILL(0));
        DexScreen_LoadMonPicInWindow(sPokedexScreenData->categoryMonWindowIds[slot], species, slot * 16 + 16);
        PutWindowTilemap(sPokedexScreenData->categoryMonWindowIds[slot]);
        CopyWindowToVram(sPokedexScreenData->categoryMonWindowIds[slot], COPYWIN_GFX);
    }
    else
        PutWindowTilemap(sPokedexScreenData->categoryMonWindowIds[slot]);

    if (sPokedexScreenData->categoryMonInfoWindowIds[slot] == 0xFF)
    {
        if (species != SPECIES_NONE)
        {
            template = sWindowTemplate_CategoryMonInfo;
            template.tilemapLeft = sCategoryPageIconCoords[numSlots][slot][2];
            template.tilemapTop = sCategoryPageIconCoords[numSlots][slot][3];
            template.baseBlock = slot * 40 + 0x108;
            sPokedexScreenData->categoryMonInfoWindowIds[slot] = AddWindow(&template);
            CopyToWindowPixelBuffer(sPokedexScreenData->categoryMonInfoWindowIds[slot], sCategoryMonInfoBgTiles, 0, 0);
            DexScreen_PrintMonDexNo(sPokedexScreenData->categoryMonInfoWindowIds[slot], FONT_SMALL, species, 12, 0);
            DexScreen_AddTextPrinterParameterized(sPokedexScreenData->categoryMonInfoWindowIds[slot], FONT_NORMAL, gSpeciesNames[species], 2, 13, 0);
            if (DexScreen_GetSetPokedexFlag(species, FLAG_GET_CAUGHT, TRUE))
                BlitBitmapRectToWindow(sPokedexScreenData->categoryMonInfoWindowIds[slot], sDexScreen_CaughtIcon, 0, 0, 8, 8, 2, 3, 8, 8);
            PutWindowTilemap(sPokedexScreenData->categoryMonInfoWindowIds[slot]);
            CopyWindowToVram(sPokedexScreenData->categoryMonInfoWindowIds[slot], COPYWIN_GFX);
        }
    }
    else
        PutWindowTilemap(sPokedexScreenData->categoryMonInfoWindowIds[slot]);

    return TRUE;
}

static void DexScreen_DestroyCategoryPageMonIconAndInfoWindows(void)
{
    int i;
    for (i = 0; i < 4; i++)
    {
        DexScreen_RemoveWindow(&sPokedexScreenData->categoryMonWindowIds[i]);
        DexScreen_RemoveWindow(&sPokedexScreenData->categoryMonInfoWindowIds[i]);
    }
}

static void DexScreen_PrintCategoryPageNumbers(u8 windowId, u16 currentPage, u16 totalPages, u16 x, u16 y)
{
    u8 buffer[30];
    u8 *ptr = StringCopy(buffer, gText_Page);
    ptr = ConvertIntToDecimalStringN(ptr, currentPage, STR_CONV_MODE_RIGHT_ALIGN, 2);
    *ptr++ = CHAR_SLASH;
    ptr = ConvertIntToDecimalStringN(ptr, totalPages, STR_CONV_MODE_RIGHT_ALIGN, 2);
    DexScreen_PrintStringWithAlignment(buffer, TEXT_RIGHT);
}

static bool8 DexScreen_CreateCategoryListGfx(bool8 justRegistered)
{
    FillBgTilemapBufferRect_Palette0(3, 2, 0, 0, 30, 20);
    FillBgTilemapBufferRect_Palette0(2, 0, 0, 0, 32, 20);
    FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 32, 20);
    DexScreen_CreateCategoryPageSpeciesList(sPokedexScreenData->category, sPokedexScreenData->pageNum);
    FillWindowPixelBuffer(0, PIXEL_FILL(15));
    if (justRegistered)
    {
        DexScreen_PrintStringWithAlignment(sDexCategoryNamePtrs[sPokedexScreenData->category], TEXT_CENTER);
    }
    else
    {
        DexScreen_PrintStringWithAlignment(sDexCategoryNamePtrs[sPokedexScreenData->category], TEXT_LEFT);
        DexScreen_PrintCategoryPageNumbers(0, DexScreen_PageNumberToRenderablePages(sPokedexScreenData->pageNum), DexScreen_PageNumberToRenderablePages(sPokedexScreenData->lastPageInCategory - 1), 160, 2);
    }
    CopyWindowToVram(0, COPYWIN_GFX);
    FillWindowPixelBuffer(1, PIXEL_FILL(15));
    if (!justRegistered)
        DexScreen_PrintControlInfo(gText_PickFlipPageCheckCancel);
    CopyWindowToVram(1, COPYWIN_GFX);
    if (sPokedexScreenData->pageSpecies[0] != 0xFFFF)
        DexScreen_DrawMonPicInCategoryPage(sPokedexScreenData->pageSpecies[0], 0, sPokedexScreenData->numMonsOnPage);
    if (sPokedexScreenData->pageSpecies[1] != 0xFFFF)
        DexScreen_DrawMonPicInCategoryPage(sPokedexScreenData->pageSpecies[1], 1, sPokedexScreenData->numMonsOnPage);
    if (sPokedexScreenData->pageSpecies[2] != 0xFFFF)
        DexScreen_DrawMonPicInCategoryPage(sPokedexScreenData->pageSpecies[2], 2, sPokedexScreenData->numMonsOnPage);
    if (sPokedexScreenData->pageSpecies[3] != 0xFFFF)
        DexScreen_DrawMonPicInCategoryPage(sPokedexScreenData->pageSpecies[3], 3, sPokedexScreenData->numMonsOnPage);
    return FALSE;
}

static void DexScreen_CreateCategoryPageSelectionCursor(u8 cursorPos)
{
    int i;
    u32 palIdx;

    if (cursorPos == 0xFF)
    {
        for (i = 0; i < 4; i++)
        {
            LoadPalette(&sDexScreen_CategoryCursorPals[0], PLTT_ID(i) + PLTT_ID(5) + 2 + BG_PLTT_OFFSET, PLTT_SIZEOF(1));
            LoadPalette(&sDexScreen_CategoryCursorPals[1], PLTT_ID(i) + PLTT_ID(5) + 8 + BG_PLTT_OFFSET, PLTT_SIZEOF(1));
        }
        LoadPalette(&sDexScreen_CategoryCursorPals[0], 0x141, PLTT_SIZEOF(1));
        sPokedexScreenData->categoryPageSelectionCursorTimer = 0;
    }
    else
    {
        sPokedexScreenData->categoryPageSelectionCursorTimer++;
        if (sPokedexScreenData->categoryPageSelectionCursorTimer == 16)
            sPokedexScreenData->categoryPageSelectionCursorTimer = 0;
        palIdx = sPokedexScreenData->categoryPageSelectionCursorTimer >> 2;
        for (i = 0; i < 4; i++)
        {
            if (i == cursorPos)
            {
                LoadPalette(&sDexScreen_CategoryCursorPals[2 * palIdx + 2], PLTT_ID(i) + PLTT_ID(5) + 2 + BG_PLTT_OFFSET, PLTT_SIZEOF(1));
                LoadPalette(&sDexScreen_CategoryCursorPals[2 * palIdx + 3], PLTT_ID(i) + PLTT_ID(5) + 8 + BG_PLTT_OFFSET, PLTT_SIZEOF(1));
            }
            else
            {
                LoadPalette(&sDexScreen_CategoryCursorPals[0], PLTT_ID(i) + PLTT_ID(5) + 2 + BG_PLTT_OFFSET, PLTT_SIZEOF(1));
                LoadPalette(&sDexScreen_CategoryCursorPals[1], PLTT_ID(i) + PLTT_ID(5) + 8 + BG_PLTT_OFFSET, PLTT_SIZEOF(1));
            }
        }
        LoadPalette(&sDexScreen_CategoryCursorPals[2 * palIdx + 2], OBJ_PLTT_ID(4) + 1, PLTT_SIZEOF(1));
    }
}

static void DexScreen_UpdateCategoryPageCursorObject(u8 taskId, u8 cursorPos, u8 numMonsInPage)
{
    numMonsInPage--;
    ListMenuUpdateCursorObject(taskId, sCategoryPageIconCoords[numMonsInPage][cursorPos][2] * 8, sCategoryPageIconCoords[numMonsInPage][cursorPos][3] * 8, 0);
}

bool8 DexPage_TileBuffer_CopyCol(const u16 *srcBuf, u8 srcCol, u16 *dstBuf, u8 dstCol)
{
    int i;
    const u16 *src = &srcBuf[srcCol];
    u16 *dst = &dstBuf[dstCol];
    for (i = 0; i < 20; i++)
    {
        *dst = *src;
        dst += 32;
        src += 32;
    }
    return FALSE;
}

bool8 DexPage_TileBuffer_FillCol(u16 tileNo, u16 *tileBuf, u8 x)
{
    int i;
    u16 *dst = &tileBuf[x];
    for (i = 0; i < 20; i++)
    {
        *dst = tileNo;
        dst += 32;
    }
    return FALSE;
}

bool8 DexScreen_TurnCategoryPage_BgEffect(u8 page)
{
    int dstCol;
    int srcCol;
    u16 *bg1buff = GetBgTilemapBuffer(1);
    u16 *bg2buff = GetBgTilemapBuffer(2);
    u16 *bg3buff = GetBgTilemapBuffer(3);
    u16 *bg1mem = sPokedexScreenData->bgBufsMem + 0x800;
    u16 *bg2mem = sPokedexScreenData->bgBufsMem + 0x400;
    u16 *bg3mem = sPokedexScreenData->bgBufsMem + 0x000;
    for (dstCol = 0; dstCol < 30; dstCol++)
    {
        srcCol = sDexScreenPageTurnColumns[page][dstCol];
        if (srcCol == 30)
        {
            DexPage_TileBuffer_FillCol(0x000, bg1buff, dstCol);
            DexPage_TileBuffer_FillCol(0x000, bg2buff, dstCol);
            DexPage_TileBuffer_FillCol(0x00C, bg3buff, dstCol);
        }
        else
        {
            DexPage_TileBuffer_CopyCol(bg1mem, srcCol, bg1buff, dstCol);
            DexPage_TileBuffer_CopyCol(bg2mem, srcCol, bg2buff, dstCol);
            DexPage_TileBuffer_CopyCol(bg3mem, srcCol, bg3buff, dstCol);
        }
    }
    CopyBgTilemapBufferToVram(1);
    CopyBgTilemapBufferToVram(2);
    CopyBgTilemapBufferToVram(3);
    return FALSE;
}

/*
 * Direction = 0: Left; 1: Right
 */
static bool8 DexScreen_FlipCategoryPageInDirection(u8 direction)
{
    u16 color;
    if (IsNationalPokedexEnabled())
        color = sNationalDexPalette[7];
    else
        color = sKantoDexPalette[7];
    switch (sPokedexScreenData->data[0])
    {
    case 0:
        sPokedexScreenData->bgBufsMem = Alloc(3 * BG_SCREEN_SIZE);
        if (direction)
            sPokedexScreenData->data[0] = 6;
        else
            sPokedexScreenData->data[0] = 2;
        break;
    case 1:
        Free(sPokedexScreenData->bgBufsMem);
        return TRUE;
        // Go left
    case 2:
        BeginNormalPaletteFade(0x00007FFF, 0, 0, 16, color);
        sPokedexScreenData->data[0]++;
        break;
    case 3:
        FillBgTilemapBufferRect_Palette0(3, 0x00C, 0, 0, 30, 20);
        FillBgTilemapBufferRect_Palette0(2, 0x000, 0, 0, 32, 20);
        FillBgTilemapBufferRect_Palette0(1, 0x000, 0, 0, 32, 20);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(3);
        sPokedexScreenData->data[0]++;
        break;
    case 4:
        BeginNormalPaletteFade(0x00007FFF, 0, 0, 0, color);
        DexScreen_CreateCategoryListGfx(FALSE);
        CpuFastCopy(GetBgTilemapBuffer(3), &sPokedexScreenData->bgBufsMem[0 * BG_SCREEN_SIZE / 2], BG_SCREEN_SIZE);
        CpuFastCopy(GetBgTilemapBuffer(2), &sPokedexScreenData->bgBufsMem[1 * BG_SCREEN_SIZE / 2], BG_SCREEN_SIZE);
        CpuFastCopy(GetBgTilemapBuffer(1), &sPokedexScreenData->bgBufsMem[2 * BG_SCREEN_SIZE / 2], BG_SCREEN_SIZE);
        FillBgTilemapBufferRect_Palette0(3, 0x00C, 0, 0, 30, 20);
        FillBgTilemapBufferRect_Palette0(2, 0x000, 0, 0, 32, 20);
        FillBgTilemapBufferRect_Palette0(1, 0x000, 0, 0, 32, 20);

        sPokedexScreenData->data[1] = 0;
        sPokedexScreenData->data[0]++;
        PlaySE(SE_BALL_TRAY_ENTER);
        break;
    case 5:
        if (sPokedexScreenData->data[1] < 10)
        {
            DexScreen_TurnCategoryPage_BgEffect(sPokedexScreenData->data[1]);
            sPokedexScreenData->data[1]++;
        }
        else
        {
            sPokedexScreenData->data[0] = 1;
        }
        break;
        // Go right
    case 6:
        CpuFastCopy(GetBgTilemapBuffer(3), &sPokedexScreenData->bgBufsMem[0 * BG_SCREEN_SIZE / 2], BG_SCREEN_SIZE);
        CpuFastCopy(GetBgTilemapBuffer(2), &sPokedexScreenData->bgBufsMem[1 * BG_SCREEN_SIZE / 2], BG_SCREEN_SIZE);
        CpuFastCopy(GetBgTilemapBuffer(1), &sPokedexScreenData->bgBufsMem[2 * BG_SCREEN_SIZE / 2], BG_SCREEN_SIZE);

        sPokedexScreenData->data[1] = 9;
        sPokedexScreenData->data[0]++;
        PlaySE(SE_BALL_TRAY_ENTER);
        break;
    case 7:
        if (sPokedexScreenData->data[1] != 0)
        {
            DexScreen_TurnCategoryPage_BgEffect(sPokedexScreenData->data[1]);
            sPokedexScreenData->data[1]--;
        }
        else
        {
#ifdef BUGFIX
            DexScreen_TurnCategoryPage_BgEffect(0);
#else
            DexScreen_TurnCategoryPage_BgEffect(sPokedexScreenData->data[0]);
#endif
            BeginNormalPaletteFade(0x00007FFF, 0, 16, 16, color);
            sPokedexScreenData->data[0]++;
        }
        break;
    case 8:
        gPaletteFade.bufferTransferDisabled = TRUE;
        DexScreen_CreateCategoryListGfx(FALSE);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(3);
        sPokedexScreenData->data[0]++;
        break;
    case 9:
        gPaletteFade.bufferTransferDisabled = FALSE;
        BeginNormalPaletteFade(0x00007FFF, 0, 16, 0, color);
        sPokedexScreenData->data[0] = 1;
        break;
    }
    return FALSE;
}

// Scale from 0 to 6
void DexScreen_DexPageZoomEffectFrame(u8 bg, u8 scale)
{
    u8 tileLeft, tileTop, width, height;
    s16 left, top, divY;

    if (!sPokedexScreenData->numMonsOnPage)
    {
        tileLeft = sCategoryPageIconCoords[0][0][2];
        tileTop = sCategoryPageIconCoords[0][0][3];
    }
    else
    {
        tileLeft = sCategoryPageIconCoords[sPokedexScreenData->numMonsOnPage - 1][sPokedexScreenData->categoryCursorPosInPage][2];
        tileTop = sCategoryPageIconCoords[sPokedexScreenData->numMonsOnPage - 1][sPokedexScreenData->categoryCursorPosInPage][3];
    }

    width = 6 + (scale * 4);
    height = 3 + (scale * 2);
    if (width >= 28) // Make sure it's not wider than the screen
        width = 28;
    if (height >= 14) // Make sure it's not taller than the screen
        height = 14;

    left = tileLeft - ((scale * 4) / 2);
    top = tileTop - ((scale * 2) / 2);
    if ((left + width + 2) >= 30) // Don't wrap right...
        left -= ((left + width + 2) - 30);
    else if (left < 0) // ... left ...
        left = 0;
    if ((top + height + 2) >= 18) // ... down ...
        top -= ((top + height + 2) - 18);
    else if (top < 2) // or up.
        top = 2;

    divY = (top + 1) + ((height / 2) + 1); // The horizontal divider

    // Top edge
    FillBgTilemapBufferRect_Palette0(bg, 4, left, top, 1, 1);
    FillBgTilemapBufferRect_Palette0(bg, 5, left + 1, top, width, 1);
    FillBgTilemapBufferRect_Palette0(bg, BG_TILE_H_FLIP(4), left + 1 + width, top, 1, 1);

    // Bottom edge
    FillBgTilemapBufferRect_Palette0(bg, 10, left, top + 1 + height, 1, 1);
    FillBgTilemapBufferRect_Palette0(bg, 11, left + 1, top + 1 + height, width, 1);
    FillBgTilemapBufferRect_Palette0(bg, BG_TILE_H_FLIP(10), left + 1 + width, top + 1 + height, 1, 1);

    // Left edge
    FillBgTilemapBufferRect_Palette0(bg, 6, left, top + 1, 1, divY - top - 1);
    FillBgTilemapBufferRect_Palette0(bg, 7, left, divY, 1, 1);
    FillBgTilemapBufferRect_Palette0(bg, 9, left, divY + 1, 1, top + height - divY);

    // Right edge
    FillBgTilemapBufferRect_Palette0(bg, BG_TILE_H_FLIP(6), left + 1 + width, top + 1, 1, divY - top - 1);
    FillBgTilemapBufferRect_Palette0(bg, BG_TILE_H_FLIP(7), left + 1 + width, divY, 1, 1);
    FillBgTilemapBufferRect_Palette0(bg, BG_TILE_H_FLIP(9), left + 1 + width, divY + 1, 1, top + height - divY);

    // Interior
    FillBgTilemapBufferRect_Palette0(bg, 1, left + 1, top + 1, width, divY - top - 1);
    FillBgTilemapBufferRect_Palette0(bg, 8, left + 1, divY, width, 1);
    FillBgTilemapBufferRect_Palette0(bg, 2, left + 1, divY + 1, width, top + height - divY);
}

void DexScreen_PrintMonCategory(u8 windowId, u16 species, u8 x, u8 y)
{
    u8 * categoryName;
    u8 index, categoryStr[12];

    species = SpeciesToNationalPokedexNum(species);

    categoryName = (u8 *)gPokedexEntries[species].categoryName;
    index = 0;
    if (DexScreen_GetSetPokedexFlag(species, FLAG_GET_CAUGHT, FALSE))
    {
#if REVISION == 0
        while ((categoryName[index] != CHAR_SPACE) && (index < 11))
#else
        while ((categoryName[index] != EOS) && (index < 11))
#endif
        {
            categoryStr[index] = categoryName[index];
            index++;
        }
    }
    else
    {
        while (index < 11)
        {
            categoryStr[index] = CHAR_QUESTION_MARK;
            index++;
        }
    }

    categoryStr[index] = EOS;

    DexScreen_AddTextPrinterParameterized(windowId, FONT_SMALL, categoryStr, x, y, 0);
    x += GetStringWidth(FONT_SMALL, categoryStr, 0);
    DexScreen_AddTextPrinterParameterized(windowId, FONT_SMALL, gText_PokedexPokemon, x, y, 0);
}

void DexScreen_PrintMonHeight(u8 windowId, u16 species, u8 x, u8 y)
{
    u16 height;
    u32 inches, feet;
    const u8 *labelText;
    u8 buffer[32];
    u8 i;

    species = SpeciesToNationalPokedexNum(species);
    height = gPokedexEntries[species].height;
    labelText = gText_HT;

    i = 0;
    buffer[i++] = EXT_CTRL_CODE_BEGIN;
    buffer[i++] = EXT_CTRL_CODE_MIN_LETTER_SPACING;
    buffer[i++] = 5;
    buffer[i++] = CHAR_SPACE;

    if (DexScreen_GetSetPokedexFlag(species, FLAG_GET_CAUGHT, FALSE))
    {
        inches = 10000 * height / 254; // actually tenths of inches here
        if (inches % 10 >= 5)
            inches += 10;
        feet = inches / 120;
        inches = (inches - (feet * 120)) / 10;
        if (feet / 10 == 0)
        {
            buffer[i++] = 0;
            buffer[i++] = feet + CHAR_0;
        }
        else
        {
            buffer[i++] = feet / 10 + CHAR_0;
            buffer[i++] = feet % 10 + CHAR_0;
        }
        buffer[i++] = CHAR_SGL_QUOTE_RIGHT;
        buffer[i++] = inches / 10 + CHAR_0;
        buffer[i++] = inches % 10 + CHAR_0;
        buffer[i++] = CHAR_DBL_QUOTE_RIGHT;
        buffer[i++] = EOS;
    }
    else
    {
        buffer[i++] = CHAR_QUESTION_MARK;
        buffer[i++] = CHAR_QUESTION_MARK;
        buffer[i++] = CHAR_SGL_QUOTE_RIGHT;
        buffer[i++] = CHAR_QUESTION_MARK;
        buffer[i++] = CHAR_QUESTION_MARK;
        buffer[i++] = CHAR_DBL_QUOTE_RIGHT;
    }

    buffer[i++] = EOS;
    DexScreen_AddTextPrinterParameterized(windowId, FONT_SMALL, labelText, x, y, 0);
    x += 30;
    DexScreen_AddTextPrinterParameterized(windowId, FONT_SMALL, buffer, x, y, 0);
}

void DexScreen_PrintMonWeight(u8 windowId, u16 species, u8 x, u8 y)
{
    u16 weight;
    u32 lbs;
    bool8 output;
    const u8 * labelText;
    const u8 * lbsText;
    u8 buffer[32];
    u8 i;
    u32 j;

    species = SpeciesToNationalPokedexNum(species);
    weight = gPokedexEntries[species].weight;
    labelText = gText_WT;
    lbsText = gText_Lbs;

    i = 0;
    buffer[i++] = EXT_CTRL_CODE_BEGIN;
    buffer[i++] = EXT_CTRL_CODE_MIN_LETTER_SPACING;
    buffer[i++] = 5;

    if (DexScreen_GetSetPokedexFlag(species, FLAG_GET_CAUGHT, FALSE))
    {
        lbs = (weight * 100000) / 4536; // Convert to hundredths of lb

        // Round up to the nearest 0.1 lb
        if (lbs % 10 >= 5)
            lbs += 10;

        output = FALSE;

        if ((buffer[i] = (lbs / 100000) + CHAR_0) == CHAR_0 && !output)
        {
            buffer[i++] = CHAR_SPACE;
        }
        else
        {
            output = TRUE;
            i++;
        }

        lbs %= 100000;
        if ((buffer[i] = (lbs / 10000) + CHAR_0) == CHAR_0 && !output)
        {
            buffer[i++] = CHAR_SPACE;
        }
        else
        {
            output = TRUE;
            i++;
        }

        lbs %= 10000;
        if ((buffer[i] = (lbs / 1000) + CHAR_0) == CHAR_0 && !output)
        {
            buffer[i++] = CHAR_SPACE;
        }
        else
        {
            output = TRUE;
            i++;
        }

        lbs %= 1000;
        buffer[i++] = (lbs / 100) + CHAR_0;
        lbs %= 100;
        buffer[i++] = CHAR_PERIOD;
        buffer[i++] = (lbs / 10) + CHAR_0;
    }
    else
    {
        buffer[i++] = CHAR_QUESTION_MARK;
        buffer[i++] = CHAR_QUESTION_MARK;
        buffer[i++] = CHAR_QUESTION_MARK;
        buffer[i++] = CHAR_QUESTION_MARK;
        buffer[i++] = CHAR_PERIOD;
        buffer[i++] = CHAR_QUESTION_MARK;
    }
    buffer[i++] = CHAR_SPACE;
    buffer[i++] = EXT_CTRL_CODE_BEGIN;
    buffer[i++] = EXT_CTRL_CODE_MIN_LETTER_SPACING;
    buffer[i++] = 0;

    for (j = 0; j < 33 - i && lbsText[j] != EOS; j++)
        buffer[i + j] = lbsText[j];

    buffer[i + j] = EOS;
    DexScreen_AddTextPrinterParameterized(windowId, FONT_SMALL, labelText, x, y, 0);
    x += 30;
    DexScreen_AddTextPrinterParameterized(windowId, FONT_SMALL, buffer, x, y, 0);
}

void DexScreen_PrintMonFlavorText(u8 windowId, u16 species, u8 x, u8 y)
{
    struct TextPrinterTemplate printerTemplate;
    u16 length;
    s32 xCenter;

    species = SpeciesToNationalPokedexNum(species);

    if (DexScreen_GetSetPokedexFlag(species, FLAG_GET_CAUGHT, FALSE))
    {
        const u8 *description = IsLeafGreenVersion() ? gPokedexEntries[species].descriptionLG : gPokedexEntries[species].description;

        printerTemplate.currentChar = description;
        printerTemplate.windowId = windowId;
        printerTemplate.fontId = FONT_NORMAL;
        printerTemplate.letterSpacing = 1;
        printerTemplate.lineSpacing = 0;
        printerTemplate.unk = 0;
        printerTemplate.fgColor = 1;
        printerTemplate.bgColor = 0;
        printerTemplate.shadowColor = 2;

        length = GetStringWidth(FONT_NORMAL, description, 0);
        xCenter = x + (240 - length) / 2;

        if (xCenter > 0)
            x = xCenter;
        else
            x = 0;

        printerTemplate.x = x;
        printerTemplate.y = y;
        printerTemplate.currentX = x;
        printerTemplate.currentY = y;

        AddTextPrinter(&printerTemplate, TEXT_SKIP_DRAW, NULL);
    }
}

void DexScreen_DrawMonFootprint(u8 windowId, u16 species, u8 x, u8 y)
{
    u16 i, j, unused, tileIdx;
    u8 footprintPixel, footprintTile;
    u8 * buffer;
    u8 * footprint;

    if (!(DexScreen_GetSetPokedexFlag(species, FLAG_GET_CAUGHT, TRUE)))
        return;
    footprint = (u8 *)(gMonFootprintTable[species]);
    buffer = gDecompressionBuffer;
    unused = 0;
    tileIdx = 0;

    // Expand 1bpp to 4bpp
    for (i = 0; i < 32; i++)
    {
        footprintPixel = footprint[i];
        for (j = 0; j < 8 / 2; j++)
        {
            footprintTile = 0;
            if (footprintPixel & (1 << (j * 2)))
                footprintTile |= 0x01;
            if (footprintPixel & (2 << (j * 2)))
                footprintTile |= 0x10;
            buffer[tileIdx] = footprintTile;
            tileIdx++;
        }
    }
    BlitBitmapRectToWindow(windowId, buffer, 0, 0, 16, 16, x, y, 16, 16);
}

static u8 DexScreen_DrawMonDexPage(bool8 justRegistered)
{
    DexScreen_DexPageZoomEffectFrame(3, 6);
    FillBgTilemapBufferRect_Palette0(2, 0, 0, 0, 30, 20);
    FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 30, 20);
    FillBgTilemapBufferRect_Palette0(0, 0, 0, 2, 30, 16);

    sPokedexScreenData->windowIds[0] = AddWindow(&sWindowTemplate_DexEntry_MonPic);
    sPokedexScreenData->windowIds[1] = AddWindow(&sWindowTemplate_DexEntry_SpeciesStats);
    sPokedexScreenData->windowIds[2] = AddWindow(&sWindowTemplate_DexEntry_FlavorText);

    // Mon pic
    FillWindowPixelBuffer(sPokedexScreenData->windowIds[0], PIXEL_FILL(0));
    DexScreen_LoadMonPicInWindow(sPokedexScreenData->windowIds[0], sPokedexScreenData->dexSpecies, 144);
    PutWindowTilemap(sPokedexScreenData->windowIds[0]);
    CopyWindowToVram(sPokedexScreenData->windowIds[0], COPYWIN_GFX);

    // Species stats
    FillWindowPixelBuffer(sPokedexScreenData->windowIds[1], PIXEL_FILL(0));
    DexScreen_PrintMonDexNo(sPokedexScreenData->windowIds[1], FONT_SMALL, sPokedexScreenData->dexSpecies, 0, 8);
    DexScreen_AddTextPrinterParameterized(sPokedexScreenData->windowIds[1], FONT_NORMAL, gSpeciesNames[sPokedexScreenData->dexSpecies], 28, 8, 0);
    DexScreen_PrintMonCategory(sPokedexScreenData->windowIds[1], sPokedexScreenData->dexSpecies, 0, 24);
    DexScreen_PrintMonHeight(sPokedexScreenData->windowIds[1], sPokedexScreenData->dexSpecies, 0, 36);
    DexScreen_PrintMonWeight(sPokedexScreenData->windowIds[1], sPokedexScreenData->dexSpecies, 0, 48);
    DexScreen_DrawMonFootprint(sPokedexScreenData->windowIds[1], sPokedexScreenData->dexSpecies, 88, 40);
    PutWindowTilemap(sPokedexScreenData->windowIds[1]);
    CopyWindowToVram(sPokedexScreenData->windowIds[1], COPYWIN_GFX);

    // Dex entry
    FillWindowPixelBuffer(sPokedexScreenData->windowIds[2], PIXEL_FILL(0));
    DexScreen_PrintMonFlavorText(sPokedexScreenData->windowIds[2], sPokedexScreenData->dexSpecies, 0, 8);
    PutWindowTilemap(sPokedexScreenData->windowIds[2]);
    CopyWindowToVram(sPokedexScreenData->windowIds[2], COPYWIN_GFX);

    // Control info
    FillWindowPixelBuffer(1, PIXEL_FILL(15));
    if (justRegistered == FALSE)
    {
        DexScreen_AddTextPrinterParameterized(1, FONT_SMALL, gText_Cry, 8, 2, 4);
        DexScreen_PrintControlInfo(gText_NextDataCancel);
    }
    else
        // Just registered
        DexScreen_PrintControlInfo(gText_Next);
    PutWindowTilemap(1);
    CopyWindowToVram(1, COPYWIN_GFX);

    return 1;
}

u8 RemoveDexPageWindows(void)
{
    DexScreen_RemoveWindow(&sPokedexScreenData->windowIds[0]);
    DexScreen_RemoveWindow(&sPokedexScreenData->windowIds[1]);
    DexScreen_RemoveWindow(&sPokedexScreenData->windowIds[2]);

    return 0;
}

u8 DexScreen_DrawMonAreaPage(void)
{
    int i;
    u8 width, height;
    bool8 monIsCaught;
    s16 left, top;
    u16 speciesId, species;
    u16 kantoMapVoff;

    species = sPokedexScreenData->dexSpecies;
    speciesId = SpeciesToNationalPokedexNum(species);
    monIsCaught = DexScreen_GetSetPokedexFlag(species, FLAG_GET_CAUGHT, TRUE);
    width = 28;
    height = 14;
    left = 0;
    top = 2;

    FillBgTilemapBufferRect_Palette0(3, 4, left, top, 1, 1);
    FillBgTilemapBufferRect_Palette0(3, BG_TILE_H_FLIP(4), left + 1 + width, top, 1, 1);
    FillBgTilemapBufferRect_Palette0(3, BG_TILE_V_FLIP(4), left, top + 1 + height, 1, 1);
    FillBgTilemapBufferRect_Palette0(3, BG_TILE_H_V_FLIP(4), left + 1 + width, top + 1 + height, 1, 1);
    FillBgTilemapBufferRect_Palette0(3, 5, left + 1, top, width, 1);
    FillBgTilemapBufferRect_Palette0(3, BG_TILE_V_FLIP(5), left + 1, top + 1 + height, width, 1);
    FillBgTilemapBufferRect_Palette0(3, 6, left, top + 1, 1, height);
    FillBgTilemapBufferRect_Palette0(3, BG_TILE_H_FLIP(6), left + 1 + width, top + 1, 1, height);
    FillBgTilemapBufferRect_Palette0(3, 1, left + 1, top + 1, width, height);
    FillBgTilemapBufferRect_Palette0(0, 0, 0, 2, 30, 16);

    // HACK: quadro do SIZE com o mesmo tamanho e altura do mapa do AREA (12x9 tiles)
    width = 10;
    height = 7;
    left = 1;
    top = 8;

    FillBgTilemapBufferRect_Palette0(0, 29, left, top, 1, 1);
    FillBgTilemapBufferRect_Palette0(0, BG_TILE_H_FLIP(29), left + 1 + width, top, 1, 1);
    FillBgTilemapBufferRect_Palette0(0, BG_TILE_V_FLIP(29), left, top + 1 + height, 1, 1);
    FillBgTilemapBufferRect_Palette0(0, BG_TILE_H_V_FLIP(29), left + 1 + width, top + 1 + height, 1, 1);
    FillBgTilemapBufferRect_Palette0(0, 30, left + 1, top, width, 1);
    FillBgTilemapBufferRect_Palette0(0, BG_TILE_V_FLIP(30), left + 1, top + 1 + height, width, 1);
    FillBgTilemapBufferRect_Palette0(0, 31, left, top + 1, 1, height);
    FillBgTilemapBufferRect_Palette0(0, BG_TILE_H_FLIP(31), left + 1 + width, top + 1, 1, height);
    FillBgTilemapBufferRect_Palette0(2, 0, 0, 0, 30, 20);
    FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 30, 20);

    sPokedexScreenData->unlockedSeviiAreas = GetUnlockedSeviiAreas();
    kantoMapVoff = 4;
    // If any of the postgame islands are unlocked, Kanto map needs to be flush with the
    // top of the screen.
    for (i = 3; i < 7; i++)
        if ((sPokedexScreenData->unlockedSeviiAreas >> i) & 1)
            kantoMapVoff = 0;

    sPokedexScreenData->windowIds[0] = AddWindow(&sWindowTemplate_AreaMap_Kanto);
    CopyToWindowPixelBuffer(sPokedexScreenData->windowIds[0], (void *)sTilemap_AreaMap_Kanto, 0, 0);
    SetWindowAttribute(sPokedexScreenData->windowIds[0], WINDOW_TILEMAP_TOP,
                       GetWindowAttribute(sPokedexScreenData->windowIds[0], WINDOW_TILEMAP_TOP) + kantoMapVoff);
    PutWindowTilemap(sPokedexScreenData->windowIds[0]);
    for (i = 0; i < 7; i++)
        if ((sPokedexScreenData->unlockedSeviiAreas >> i) & 1)
        {
            sPokedexScreenData->windowIds[i + 1] = AddWindow(sAreaMapStructs_SeviiIslands[i].window);
            CopyToWindowPixelBuffer(sPokedexScreenData->windowIds[i + 1], sAreaMapStructs_SeviiIslands[i].tiles, 0, 0);
            SetWindowAttribute(sPokedexScreenData->windowIds[i + 1], WINDOW_TILEMAP_TOP, GetWindowAttribute(sPokedexScreenData->windowIds[i + 1], WINDOW_TILEMAP_TOP) + kantoMapVoff);
            PutWindowTilemap(sPokedexScreenData->windowIds[i + 1]);
            CopyWindowToVram(sPokedexScreenData->windowIds[i + 1], COPYWIN_GFX);
        }
    sPokedexScreenData->windowIds[8] = AddWindow(&sWindowTemplate_AreaMap_SpeciesName);
    sPokedexScreenData->windowIds[9] = AddWindow(&sWindowTemplate_AreaMap_Size);
    sPokedexScreenData->windowIds[10] = AddWindow(&sWindowTemplate_AreaMap_Area);
    sPokedexScreenData->windowIds[11] = AddWindow(&sWindowTemplate_AreaMap_MonIcon);
    sPokedexScreenData->windowIds[12] = AddWindow(&sWindowTemplate_AreaMap_MonTypes);

    // Draw the mon icon
    FillWindowPixelBuffer(sPokedexScreenData->windowIds[11], PIXEL_FILL(0));
    ListMenu_LoadMonIconPalette(BG_PLTT_ID(10), species);
    ListMenu_DrawMonIconGraphics(sPokedexScreenData->windowIds[11], species, DexScreen_GetDefaultPersonality(species), 0, DEX_HEADER_ICON_Y);
    PutWindowTilemap(sPokedexScreenData->windowIds[11]);
    CopyWindowToVram(sPokedexScreenData->windowIds[11], COPYWIN_GFX);

    // Print "Size"
    FillWindowPixelBuffer(sPokedexScreenData->windowIds[9], PIXEL_FILL(0));
    {
        s32 strWidth = GetStringWidth(FONT_SMALL, gText_Size, 0);
        DexScreen_AddTextPrinterParameterized(sPokedexScreenData->windowIds[9], FONT_SMALL, gText_Size, (sWindowTemplate_AreaMap_Size.width * 8 - strWidth) / 2, 4, 0);
    }
    PutWindowTilemap(sPokedexScreenData->windowIds[9]);
    CopyWindowToVram(sPokedexScreenData->windowIds[9], COPYWIN_GFX);

    // Print "Area"
    FillWindowPixelBuffer(sPokedexScreenData->windowIds[10], PIXEL_FILL(0));
    {
        s32 strWidth = GetStringWidth(FONT_SMALL, gText_Area, 0);
        DexScreen_AddTextPrinterParameterized(sPokedexScreenData->windowIds[10], FONT_SMALL, gText_Area, (sWindowTemplate_AreaMap_Area.width * 8 - strWidth) / 2, 4, 0);
    }
    SetWindowAttribute(sPokedexScreenData->windowIds[10], WINDOW_TILEMAP_TOP, GetWindowAttribute(sPokedexScreenData->windowIds[10], WINDOW_TILEMAP_TOP) + kantoMapVoff);
    PutWindowTilemap(sPokedexScreenData->windowIds[10]);
    CopyWindowToVram(sPokedexScreenData->windowIds[10], COPYWIN_GFX);

    // Print species name
    FillWindowPixelBuffer(sPokedexScreenData->windowIds[8], PIXEL_FILL(0));
    DexScreen_PrintMonNoAndName(sPokedexScreenData->windowIds[8], species);
    PutWindowTilemap(sPokedexScreenData->windowIds[8]);
    CopyWindowToVram(sPokedexScreenData->windowIds[8], COPYWIN_GFX);

    // Type icons
    FillWindowPixelBuffer(sPokedexScreenData->windowIds[12], PIXEL_FILL(0));
    ListMenuLoadStdPalAt(BG_PLTT_ID(11), 1);

    if (monIsCaught)
    {
        BlitMenuInfoIcon(sPokedexScreenData->windowIds[12], 1 + gSpeciesInfo[species].types[0], 0, DEX_HEADER_TYPES_Y);
        if (gSpeciesInfo[species].types[0] != gSpeciesInfo[species].types[1])
            BlitMenuInfoIcon(sPokedexScreenData->windowIds[12], 1 + gSpeciesInfo[species].types[1], 32, DEX_HEADER_TYPES_Y);
    }
    PutWindowTilemap(sPokedexScreenData->windowIds[12]);
    CopyWindowToVram(sPokedexScreenData->windowIds[12], COPYWIN_GFX);

    // Show size comparison
    ResetAllPicSprites();
    LoadPalette(sPalette_Silhouette, OBJ_PLTT_ID(2), PLTT_SIZE_4BPP);

    if (monIsCaught)
    {
        sPokedexScreenData->windowIds[14] = CreateMonPicSprite_HandleDeoxys(species, SHINY_ODDS, DexScreen_GetDefaultPersonality(species), TRUE, 40, 100, 0, 0xFFFF); // HACK: centralizado no quadro do SIZE
        gSprites[sPokedexScreenData->windowIds[14]].oam.paletteNum = 2;
        gSprites[sPokedexScreenData->windowIds[14]].oam.affineMode = ST_OAM_AFFINE_NORMAL;
        gSprites[sPokedexScreenData->windowIds[14]].oam.matrixNum = 2;
        gSprites[sPokedexScreenData->windowIds[14]].oam.priority = 1;
        gSprites[sPokedexScreenData->windowIds[14]].y2 = gPokedexEntries[speciesId].pokemonOffset;
        SetOamMatrix(2, gPokedexEntries[speciesId].pokemonScale, 0, 0, gPokedexEntries[speciesId].pokemonScale);
        sPokedexScreenData->windowIds[15] = CreateTrainerPicSprite(PlayerGenderToFrontTrainerPicId(gSaveBlock2Ptr->playerGender, TRUE), 1, 80, 100, 0, 0xFFFF);
        gSprites[sPokedexScreenData->windowIds[15]].oam.paletteNum = 2;
        gSprites[sPokedexScreenData->windowIds[15]].oam.affineMode = ST_OAM_AFFINE_NORMAL;
        gSprites[sPokedexScreenData->windowIds[15]].oam.matrixNum = 1;
        gSprites[sPokedexScreenData->windowIds[15]].oam.priority = 1;
        gSprites[sPokedexScreenData->windowIds[15]].y2 = gPokedexEntries[speciesId].trainerOffset;
        SetOamMatrix(1, gPokedexEntries[speciesId].trainerScale, 0, 0, gPokedexEntries[speciesId].trainerScale);
    }
    else
    {
        sPokedexScreenData->windowIds[14] = 0xff;
        sPokedexScreenData->windowIds[15] = 0xff;
    }

    // Create the area markers
    sPokedexScreenData->areaMarkersTaskId = CreatePokedexAreaMarkers(species, TAG_AREA_MARKERS, 3, kantoMapVoff * 8);
    if (GetNumPokedexAreaMarkers(sPokedexScreenData->areaMarkersTaskId) == 0)
    {
        // No markers, display "Area Unknown"
        BlitBitmapRectToWindow(sPokedexScreenData->windowIds[0], (void *)sBlitTiles_WideEllipse, 0, 0, 88, 16, 4, 28, 88, 16);
        {
            s32 strWidth = GetStringWidth(FONT_SMALL, gText_AreaUnknown, 0);
            DexScreen_AddTextPrinterParameterized(sPokedexScreenData->windowIds[0], FONT_SMALL, gText_AreaUnknown, (96 - strWidth) / 2, 29, 0);
        }
    }
    CopyWindowToVram(sPokedexScreenData->windowIds[0], COPYWIN_GFX);

    // Draw the control info
    // HACK: com o Pokémon capturado, START abre MOVES e A vai para POKéMON DATA
    FillWindowPixelBuffer(1, PIXEL_FILL(15));
    if (monIsCaught)
    {
        DexScreen_AddTextPrinterParameterized(1, FONT_SMALL, sText_DexData_Moves, 8, 2, 4);
        DexScreen_PrintControlInfo(sText_DexData_NextPrevious);
    }
    else
    {
        DexScreen_AddTextPrinterParameterized(1, FONT_SMALL, gText_Cry, 8, 2, 4);
        DexScreen_PrintControlInfo(gText_CancelPreviousData);
    }
    PutWindowTilemap(1);
    CopyWindowToVram(1, COPYWIN_GFX);

    return 1;
}


u8 DexScreen_DestroyAreaScreenResources(void)
{
    int i;

    DestroyPokedexAreaMarkers(sPokedexScreenData->areaMarkersTaskId);

    for (i = 0; i < 13; i++)
        DexScreen_RemoveWindow(&sPokedexScreenData->windowIds[i]);
    if (sPokedexScreenData->windowIds[15] != 0xff)
        FreeAndDestroyTrainerPicSprite(sPokedexScreenData->windowIds[15]);
    if (sPokedexScreenData->windowIds[14] != 0xff)
        FreeAndDestroyMonPicSprite(sPokedexScreenData->windowIds[14]);
    return 0;
}

static int DexScreen_CanShowMonInDex(u16 species)
{
    if (IsNationalPokedexEnabled() == TRUE)
        return TRUE;
    if (SpeciesToNationalPokedexNum(species) <= KANTO_DEX_COUNT)
        return TRUE;
    return FALSE;
}

static u8 DexScreen_IsPageUnlocked(u8 categoryNum, u8 pageNum)
{
    int i, count;
    u16 species;

    count = gDexCategories[categoryNum].page[pageNum].count;

    for (i = 0; i < 4; i++)
    {
        if (i < count)
        {
            species = gDexCategories[categoryNum].page[pageNum].species[i];
            if (DexScreen_CanShowMonInDex(species) == TRUE && DexScreen_GetSetPokedexFlag(species, FLAG_GET_SEEN, TRUE))
                return TRUE;
        }
    }
    return FALSE;
}

static bool8 DexScreen_IsCategoryUnlocked(u8 categoryNum)
{
    int i;
    u8 count;

    count = gDexCategories[categoryNum].count;

    for (i = 0; i < count; i++)
        if (DexScreen_IsPageUnlocked(categoryNum, i))
            return 1;

    return 0;
}

void DexScreen_CreateCategoryPageSpeciesList(u8 categoryNum, u8 pageNum)
{
    int i, count;
    u16 species;

    count = gDexCategories[categoryNum].page[pageNum].count;
    sPokedexScreenData->numMonsOnPage = 0;

    for (i = 0; i < 4; i++)
        sPokedexScreenData->pageSpecies[i] = 0xffff;
    for (i = 0; i < count; i++)
    {
        species = gDexCategories[categoryNum].page[pageNum].species[i];
        if (DexScreen_CanShowMonInDex(species) == TRUE && DexScreen_GetSetPokedexFlag(species, FLAG_GET_SEEN, TRUE))
        {
            sPokedexScreenData->pageSpecies[sPokedexScreenData->numMonsOnPage] = gDexCategories[categoryNum].page[pageNum].species[i];
            sPokedexScreenData->numMonsOnPage++;
        }
    }
}

static u8 DexScreen_GetPageLimitsForCategory(u8 category)
{
    int i;
    u8 count, firstPage, lastPage;

    count = gDexCategories[category].count;
    firstPage = 0xff;
    lastPage = 0xff;

    for (i = 0; i < count; i++)
        if (DexScreen_IsPageUnlocked(category, i))
        {
            if (firstPage == 0xff)
                firstPage = i;
            lastPage = i;
        }
    if (lastPage != 0xff)
    {
        sPokedexScreenData->firstPageInCategory = firstPage;
        sPokedexScreenData->lastPageInCategory = lastPage + 1;
        return FALSE;
    }
    else
    {
        sPokedexScreenData->firstPageInCategory = 0;
        sPokedexScreenData->lastPageInCategory = 0;
        return TRUE;
    }
}

static u8 DexScreen_LookUpCategoryBySpecies(u16 species)
{
    int i, j, k, categoryCount, categoryPageCount, posInPage;
    u16 dexSpecies;

    for (i = 0; i < NELEMS(gDexCategories); i++)
    {
        categoryCount = gDexCategories[i].count;
        for (j = 0; j < categoryCount; j++)
        {
            categoryPageCount = gDexCategories[i].page[j].count;
            for (k = 0, posInPage = 0; k < categoryPageCount; k++)
            {
                dexSpecies = gDexCategories[i].page[j].species[k];
                if (species == dexSpecies)
                {
                    sPokedexScreenData->category = i;
                    sPokedexScreenData->pageNum = j;
                    sPokedexScreenData->categoryCursorPosInPage = posInPage;
                    return FALSE;
                }
                if (DexScreen_CanShowMonInDex(dexSpecies) == TRUE && DexScreen_GetSetPokedexFlag(dexSpecies, FLAG_GET_SEEN, TRUE))
                    posInPage++;
            }
        }
    }
    return TRUE;
}

static u8 DexScreen_PageNumberToRenderablePages(u16 page)
{
    int i, count;

    for (i = 0, count = 0; i < page; i++)
        if (DexScreen_IsPageUnlocked(sPokedexScreenData->category, i))
            count++;

    return count + 1;
}

void DexScreen_InputHandler_StartToCry(void)
{
    if (JOY_NEW(START_BUTTON))
        PlayCry_NormalNoDucking(sPokedexScreenData->dexSpecies, 0, CRY_VOLUME_RS, CRY_PRIORITY_NORMAL);
}

u8 DexScreen_RegisterMonToPokedex(u16 species)
{
    DexScreen_GetSetPokedexFlag(species, FLAG_SET_SEEN, TRUE);
    DexScreen_GetSetPokedexFlag(species, FLAG_SET_CAUGHT, TRUE);

    if (!IsNationalPokedexEnabled() && SpeciesToNationalPokedexNum(species) > KANTO_DEX_COUNT)
        return CreateTask(Task_DexScreen_RegisterNonKantoMonBeforeNationalDex, 0);

    DexScreen_LoadResources();
    gTasks[sPokedexScreenData->taskId].func = Task_DexScreen_RegisterMonToPokedex;
    DexScreen_LookUpCategoryBySpecies(species);

    return sPokedexScreenData->taskId;
}

static void Task_DexScreen_RegisterNonKantoMonBeforeNationalDex(u8 taskId)
{
    DestroyTask(taskId);
}

static void Task_DexScreen_RegisterMonToPokedex(u8 taskId)
{
    switch (sPokedexScreenData->state)
    {
    case 0:
        DexScreen_GetPageLimitsForCategory(sPokedexScreenData->category);
        if (sPokedexScreenData->pageNum < sPokedexScreenData->firstPageInCategory)
            sPokedexScreenData->pageNum = sPokedexScreenData->firstPageInCategory;
        sPokedexScreenData->state = 3;
        break;
    case 1:
        RemoveDexPageWindows();
        DexScreen_DestroyCategoryPageMonIconAndInfoWindows();

        gMain.state = 0;
        sPokedexScreenData->state = 2;
        break;
    case 2:
        if (DoClosePokedex())
            DestroyTask(taskId);
        break;
    case 3:
        DexScreen_CreateCategoryListGfx(TRUE);
        PutWindowTilemap(0);
        PutWindowTilemap(1);

        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(0);

        DexScreen_CreateCategoryPageSelectionCursor(0xff);

        sPokedexScreenData->state = 4;
        break;
    case 4:
        gPaletteFade.bufferTransferDisabled = 0;
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, 0xffff);
        ShowBg(3);
        ShowBg(2);
        ShowBg(1);
        ShowBg(0);

        sPokedexScreenData->state = 5;
        break;
    case 5:
        gTasks[taskId].data[0] = 30;
        sPokedexScreenData->categoryPageCursorTaskId = ListMenuAddCursorObjectInternal(&sCursorStruct_CategoryPage, 0);
        sPokedexScreenData->state = 6;
        break;
    case 6:
        DexScreen_CreateCategoryPageSelectionCursor(sPokedexScreenData->categoryCursorPosInPage);
        DexScreen_UpdateCategoryPageCursorObject(sPokedexScreenData->categoryPageCursorTaskId, sPokedexScreenData->categoryCursorPosInPage, sPokedexScreenData->numMonsOnPage);

        if (gTasks[taskId].data[0])
            gTasks[taskId].data[0]--;
        else
        {
            ListMenuRemoveCursorObject(sPokedexScreenData->categoryPageCursorTaskId, 0);
            sPokedexScreenData->state = 7;
        }
        break;
    case 7:
        sPokedexScreenData->dexSpecies = sPokedexScreenData->pageSpecies[sPokedexScreenData->categoryCursorPosInPage];
        sPokedexScreenData->state = 8;
        break;
    case 8:
        DexScreen_DrawMonDexPage(TRUE);
        sPokedexScreenData->state = 9;
        break;
    case 9:
        sPokedexScreenData->data[0] = 0;
        sPokedexScreenData->data[1] = 0;
        sPokedexScreenData->state++;
    case 10:
        if (sPokedexScreenData->data[1] < 6)
        {
            if (sPokedexScreenData->data[0])
            {
                DexScreen_DexPageZoomEffectFrame(0, sPokedexScreenData->data[1]);
                CopyBgTilemapBufferToVram(0);
                sPokedexScreenData->data[0] = 4;
                sPokedexScreenData->data[1]++;
            }
            else
                sPokedexScreenData->data[0]--;
        }
        else
        {
            FillBgTilemapBufferRect_Palette0(0, 0, 0, 2, 30, 16);
            CopyBgTilemapBufferToVram(3);
            CopyBgTilemapBufferToVram(2);
            CopyBgTilemapBufferToVram(1);
            CopyBgTilemapBufferToVram(0);

            PlayCry_NormalNoDucking(sPokedexScreenData->dexSpecies, 0, CRY_VOLUME_RS, CRY_PRIORITY_NORMAL);
            sPokedexScreenData->data[0] = 0;
            sPokedexScreenData->state = 11;
        }
        break;
    case 11:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
            sPokedexScreenData->state = 2;
        break;
    }
}

void DexScreen_PrintStringWithAlignment(const u8 * str, s32 mode)
{
    u32 x;

    switch (mode)
    {
    case TEXT_LEFT:
        x = 8;
        break;
    case TEXT_CENTER:
        x = (u32)(240 - GetStringWidth(FONT_NORMAL, str, 0)) / 2;
        break;
    case TEXT_RIGHT:
    default:
        x = 232 - GetStringWidth(FONT_NORMAL, str, 0);
        break;
    }

    DexScreen_AddTextPrinterParameterized(0, FONT_NORMAL, str, x, 2, 4);
}

// =====================================================================================
// HACK: Pokédex — página POKéMON DATA (stats, habilidades, itens, evolução), tela de
// golpes (LEVEL / TM/HM / TUTOR) e desbloqueios. Tudo é lido das mesmas tabelas do jogo
// (species_info, learnsets, tutor, evolution), então muda sozinho quando o código muda.
// =====================================================================================

#define DEX_EXTRA_DATA  0
#define DEX_EXTRA_MOVES 1

#define DEX_MOVES_TAB_LEVEL 0
#define DEX_MOVES_TAB_TMHM  1
#define DEX_MOVES_TAB_TUTOR 2 // TUTOR/EGG sempre por último
#define DEX_MOVES_TAB_COUNT 3

#define DEX_INFO_X     8   // texto da direita começa em x 144 da tela
#define DEX_INFO_Y     4   // primeira linha na altura dos tipos
#define DEX_INFO_WIDTH (12 * 8 - DEX_INFO_X)
#define DEX_INFO_LINE  11
#define DEX_INFO_SECTION_GAP 5

#define DEX_MOVES_MAX       64
#define DEX_MOVES_ROWS      6

#define DEX_EXTRA_RESULT_NONE 0
#define DEX_EXTRA_RESULT_BACK 1 // volta para a página do SIZE/AREA
#define DEX_EXTRA_RESULT_EXIT 2 // sai das informações do Pokémon

static EWRAM_DATA u8 sDexExtraPage = 0;
static EWRAM_DATA u8 sDexMovesTab = 0;
static EWRAM_DATA bool8 sDexMovesFromData = FALSE; // MOVES aberto pela página DATA
static EWRAM_DATA u16 sDexMovesScroll = 0;
static EWRAM_DATA u16 sDexEvoScroll = 0;
static EWRAM_DATA u16 sDexEvoMaxScroll = 0;
static EWRAM_DATA u8 sDexEvoTopY = 0;
static EWRAM_DATA u8 sDexArrowTasks[1] = {0}; // setas cima/baixo

#define DEX_ARROW_TAG_VERT 2010
#define DEX_ARROW_X_RIGHT  226 // borda direita de dentro da moldura
static EWRAM_DATA u8 sDexMovesCount = 0;
static EWRAM_DATA u8 sDexMovesSplit = 0; // quantos golpes vão na coluna da esquerda
static EWRAM_DATA u16 sDexMovesList[DEX_MOVES_MAX] = {0};
static EWRAM_DATA u8 sDexMovesLabel[DEX_MOVES_MAX] = {0}; // nível ou número do TM/HM
static EWRAM_DATA u8 sDexExtraWindowIds[6] = {0};

static const u8 sText_DexData_Moves[]        = _("{START_BUTTON}MOVES");
static const u8 sText_DexData_NextPrevious[] = _("{A_BUTTON}NEXT DATA {B_BUTTON}PREVIOUS DATA");
static const u8 sText_DexData_TabScroll[]    = _("{DPAD_LEFTRIGHT}TAB {DPAD_UPDOWN}SCROLL");
static const u8 sText_DexData_Ability[]      = _("ABILITY");
static const u8 sText_DexData_Items[]        = _("HELD ITEMS");
static const u8 sText_DexData_Evolution[]    = _("EVOLUTION");
static const u8 sText_DexData_Unknown[]      = _("???");
static const u8 sText_DexData_None[]         = _("NONE");
static const u8 sText_DexData_Lv[]           = _("Lv");
static const u8 sText_DexData_TM[]           = _("TM");
static const u8 sText_DexData_HM[]           = _("HM");
static const u8 sText_DexData_Trade[]        = _("TRADE");
static const u8 sText_DexData_Friendship[]   = _("FRIENDSHIP");
static const u8 sText_DexData_Day[]          = _(" DAY");
static const u8 sText_DexData_Night[]        = _(" NIGHT");
static const u8 sText_DexData_Beauty[]       = _("BEAUTY");
static const u8 sText_DexData_AtkGtDef[]     = _(" ATK>DEF");
static const u8 sText_DexData_AtkEqDef[]     = _(" ATK=DEF");
static const u8 sText_DexData_AtkLtDef[]     = _(" ATK<DEF");
static const u8 sText_DexData_Arrow[]        = _("{RIGHT_ARROW}");
static const u8 sText_DexData_Space[]        = _(" ");
static const u8 sText_DexTab_Level[]         = _("LEVEL");
static const u8 sText_DexTab_TMHM[]          = _("TM/HM");
static const u8 sText_DexTab_Tutor[]         = _("TUTOR/EGG");
static const u8 *const sDexMovesTabNames[DEX_MOVES_TAB_COUNT] = {sText_DexTab_Level, sText_DexTab_TMHM, sText_DexTab_Tutor};
static const u8 sDexStatLabel_HP[]  = _("HP");
static const u8 sDexStatLabel_Atk[] = _("Atk");
static const u8 sDexStatLabel_Def[] = _("Def");
static const u8 sDexStatLabel_SpA[] = _("SpA");
static const u8 sDexStatLabel_SpD[] = _("SpD");
static const u8 sDexStatLabel_Spe[] = _("Spe");
static const u8 *const sDexStatLabels[NUM_STATS] = {
    sDexStatLabel_HP, sDexStatLabel_Atk, sDexStatLabel_Def, sDexStatLabel_SpA, sDexStatLabel_SpD, sDexStatLabel_Spe
};

// Barras: 0 transparente, 1 fundo da barra, 2 a 7 = faixas de stat
static const u16 sDexStatBarPalette[16] = {
    RGB(0, 0, 0), RGB(27, 26, 24),
    RGB(28, 10, 8),  // 1-29   vermelho
    RGB(30, 16, 6),  // 30-59  laranja
    RGB(30, 24, 6),  // 60-89  amarelo
    RGB(15, 25, 10), // 90-119 verde
    RGB(6, 15, 6),   // 120-149 verde-escuro
    RGB(6, 14, 28),  // 150+   azul
};
#define DEX_STAT_BAR_PAL 12
#define DEX_STAT_BAR_MAX 255 // escala única: barra cheia = 255 (maior stat possível)
#define DEX_STAT_BAR_W_MAX (11 * 8)

// Página dividida em duas metades: stats à esquerda, informações à direita (a partir da linha dos tipos)
static const struct WindowTemplate sWindowTemplate_DexData_Stats = {
    .bg = 2, .tilemapLeft = 1, .tilemapTop = 7, .width = 5, .height = 10, .paletteNum = 0, .baseBlock = 0x0208
};
// Barras terminam onde termina o nome mais comprido do jogo (calculado na hora)
static const struct WindowTemplate sWindowTemplate_DexData_Bars = {
    .bg = 2, .tilemapLeft = 6, .tilemapTop = 7, .width = 11, .height = 10, .paletteNum = DEX_STAT_BAR_PAL, .baseBlock = 0x023A
};
// Habilidade, itens e evolução: alinhados à esquerda, a partir da linha dos tipos, depois das barras
static const struct WindowTemplate sWindowTemplate_DexData_Info = {
    .bg = 2, .tilemapLeft = 17, .tilemapTop = 4, .width = 12, .height = 13, .paletteNum = 0, .baseBlock = 0x02A8
};
// Abas na linha dos tipos, à direita deles
static const struct WindowTemplate sWindowTemplate_DexMoves_Tabs = {
    .bg = 2, .tilemapLeft = 13, .tilemapTop = 4, .width = 16, .height = 2, .paletteNum = 0, .baseBlock = 0x0208
};
static const struct WindowTemplate sWindowTemplate_DexMoves_List = {
    .bg = 2, .tilemapLeft = 1, .tilemapTop = 7, .width = 28, .height = 10, .paletteNum = 0, .baseBlock = 0x0228
};

// ---- desbloqueios (habilidade e item, 2 bits por espécie) ----
static bool8 DexUnlock_Get(const u8 *flags, u16 species, u8 slot)
{
    u16 bit = species * 2 + slot;
    return (flags[bit >> 3] >> (bit & 7)) & 1;
}

static void DexUnlock_Set(u8 *flags, u16 species, u8 slot)
{
    u16 bit = species * 2 + slot;
    flags[bit >> 3] |= 1 << (bit & 7);
}

static void DexUnlock_AbilitySlot(u16 species, u8 slot)
{
    if (species == SPECIES_NONE || species >= NUM_SPECIES || slot > 1)
        return;
    DexUnlock_Set(gSaveBlock2Ptr->dexAbilityUnlocks, species, slot);
}

static void DexUnlock_Ability(u16 species, u8 ability)
{
    u8 i;

    if (species == SPECIES_NONE || species >= NUM_SPECIES || ability == ABILITY_NONE)
        return;
    for (i = 0; i < 2; i++)
        if (gSpeciesInfo[species].abilities[i] == ability)
            DexUnlock_Set(gSaveBlock2Ptr->dexAbilityUnlocks, species, i);
}

static void DexUnlock_Item(u16 species, u16 item)
{
    if (species == SPECIES_NONE || species >= NUM_SPECIES || item == ITEM_NONE)
        return;
    if (gSpeciesInfo[species].itemCommon == item)
        DexUnlock_Set(gSaveBlock2Ptr->dexItemUnlocks, species, 0);
    if (gSpeciesInfo[species].itemRare == item)
        DexUnlock_Set(gSaveBlock2Ptr->dexItemUnlocks, species, 1);
}

void DexUnlock_FromMon(struct Pokemon *mon)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);

    if (species == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG, NULL))
        return;
    DexUnlock_AbilitySlot(species, GetMonData(mon, MON_DATA_ABILITY_NUM, NULL));
    DexUnlock_Item(species, GetMonData(mon, MON_DATA_HELD_ITEM, NULL));
}

static void DexUnlock_FromBoxMon(struct BoxPokemon *boxMon)
{
    u16 species = GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL);

    if (species == SPECIES_NONE || GetBoxMonData(boxMon, MON_DATA_IS_EGG, NULL))
        return;
    DexUnlock_AbilitySlot(species, GetBoxMonData(boxMon, MON_DATA_ABILITY_NUM, NULL));
    DexUnlock_Item(species, GetBoxMonData(boxMon, MON_DATA_HELD_ITEM, NULL));
}

// Pokémon que você tem agora (time e PC) também liberam a habilidade e o item que seguram
static void DexUnlock_ScanOwned(u16 species)
{
    u8 i, j;

    for (i = 0; i < PARTY_SIZE; i++)
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) == species)
            DexUnlock_FromMon(&gPlayerParty[i]);
    for (i = 0; i < TOTAL_BOXES_COUNT; i++)
        for (j = 0; j < IN_BOX_COUNT; j++)
            if (GetBoxMonDataAt(i, j, MON_DATA_SPECIES) == species)
                DexUnlock_FromBoxMon(GetBoxedMonPtr(i, j));
}

void DexUnlock_AbilitySeenInBattle(u8 ability)
{
    u8 i;

    for (i = 0; i < gBattlersCount; i++)
        if (gBattleMons[i].ability == ability)
            DexUnlock_Ability(gBattleMons[i].species, ability);
}

// Só o Pokémon envolvido na mensagem: alvo (Thief, Covet, Knock Off) ou quem usou o item (berry, Leftovers)
void DexUnlock_ItemSeenInBattle(u16 item)
{
    u8 battlers[3], i;

    battlers[0] = gBattlerTarget;
    battlers[1] = gBattleScripting.battler;
    battlers[2] = gPotentialItemEffectBattler;
    for (i = 0; i < NELEMS(battlers); i++)
        if (battlers[i] < gBattlersCount)
            DexUnlock_Item(gBattleMons[battlers[i]].species, item);
}

// ---- desenho ----
static bool8 DexExtra_CanOpen(void)
{
    return DexScreen_GetSetPokedexFlag(sPokedexScreenData->dexSpecies, FLAG_GET_CAUGHT, TRUE);
}

static void DexExtra_DrawFrame(void)
{
    u8 width = 28, height = 14;
    s16 left = 0, top = 2;

    FillBgTilemapBufferRect_Palette0(3, 4, left, top, 1, 1);
    FillBgTilemapBufferRect_Palette0(3, BG_TILE_H_FLIP(4), left + 1 + width, top, 1, 1);
    FillBgTilemapBufferRect_Palette0(3, BG_TILE_V_FLIP(4), left, top + 1 + height, 1, 1);
    FillBgTilemapBufferRect_Palette0(3, BG_TILE_H_V_FLIP(4), left + 1 + width, top + 1 + height, 1, 1);
    FillBgTilemapBufferRect_Palette0(3, 5, left + 1, top, width, 1);
    FillBgTilemapBufferRect_Palette0(3, BG_TILE_V_FLIP(5), left + 1, top + 1 + height, width, 1);
    FillBgTilemapBufferRect_Palette0(3, 6, left, top + 1, 1, height);
    FillBgTilemapBufferRect_Palette0(3, BG_TILE_H_FLIP(6), left + 1 + width, top + 1, 1, height);
    FillBgTilemapBufferRect_Palette0(3, 1, left + 1, top + 1, width, height);
    FillBgTilemapBufferRect_Palette0(0, 0, 0, 2, 30, 16);
    FillBgTilemapBufferRect_Palette0(2, 0, 0, 0, 30, 20);
    FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 30, 20);
}

// Mesmo cabeçalho da página do AREA: ícone, número e nome em cima, tipos embaixo
static void DexExtra_DrawHeader(u16 species)
{
    u8 *ids = sDexExtraWindowIds;

    ids[0] = AddWindow(&sWindowTemplate_AreaMap_MonIcon);
    ids[1] = AddWindow(&sWindowTemplate_AreaMap_SpeciesName);
    ids[2] = AddWindow(&sWindowTemplate_AreaMap_MonTypes);

    FillWindowPixelBuffer(ids[0], PIXEL_FILL(0));
    ListMenu_LoadMonIconPalette(BG_PLTT_ID(10), species);
    ListMenu_DrawMonIconGraphics(ids[0], species, DexScreen_GetDefaultPersonality(species), 0, DEX_HEADER_ICON_Y);
    PutWindowTilemap(ids[0]);
    CopyWindowToVram(ids[0], COPYWIN_GFX);

    FillWindowPixelBuffer(ids[1], PIXEL_FILL(0));
    DexScreen_PrintMonNoAndName(ids[1], species);
    PutWindowTilemap(ids[1]);
    CopyWindowToVram(ids[1], COPYWIN_GFX);

    FillWindowPixelBuffer(ids[2], PIXEL_FILL(0));
    ListMenuLoadStdPalAt(BG_PLTT_ID(11), 1);
    BlitMenuInfoIcon(ids[2], 1 + gSpeciesInfo[species].types[0], 0, DEX_HEADER_TYPES_Y);
    if (gSpeciesInfo[species].types[0] != gSpeciesInfo[species].types[1])
        BlitMenuInfoIcon(ids[2], 1 + gSpeciesInfo[species].types[1], 32, DEX_HEADER_TYPES_Y);
    PutWindowTilemap(ids[2]);
    CopyWindowToVram(ids[2], COPYWIN_GFX);
}

static void DexExtra_DrawFooter(const u8 *left, const u8 *right)
{
    FillWindowPixelBuffer(1, PIXEL_FILL(15));
    if (left != NULL)
        DexScreen_AddTextPrinterParameterized(1, FONT_SMALL, left, 8, 2, 4);
    DexScreen_PrintControlInfo(right);
    PutWindowTilemap(1);
    CopyWindowToVram(1, COPYWIN_GFX);
}

static u8 DexExtra_StatBarColor(u8 stat)
{
    if (stat < 30)  return 2;
    if (stat < 60)  return 3;
    if (stat < 90)  return 4;
    if (stat < 120) return 5;
    if (stat < 150) return 6;
    return 7;
}

// Monta "Lv30→DRAGONAIR" para uma evolução
static void DexExtra_AppendEvoMethod(u8 *dst, const struct Evolution *evo)
{
    u8 num[8];

    switch (evo->method)
    {
    case EVO_LEVEL:
    case EVO_LEVEL_SILCOON:
    case EVO_LEVEL_CASCOON:
    case EVO_LEVEL_NINJASK:
    case EVO_LEVEL_SHEDINJA:
    case EVO_LEVEL_ATK_GT_DEF:
    case EVO_LEVEL_ATK_EQ_DEF:
    case EVO_LEVEL_ATK_LT_DEF:
        StringAppend(dst, sText_DexData_Lv);
        ConvertIntToDecimalStringN(num, evo->param, STR_CONV_MODE_LEFT_ALIGN, 3);
        StringAppend(dst, num);
        if (evo->method == EVO_LEVEL_ATK_GT_DEF)
            StringAppend(dst, sText_DexData_AtkGtDef);
        else if (evo->method == EVO_LEVEL_ATK_EQ_DEF)
            StringAppend(dst, sText_DexData_AtkEqDef);
        else if (evo->method == EVO_LEVEL_ATK_LT_DEF)
            StringAppend(dst, sText_DexData_AtkLtDef);
        break;
    case EVO_ITEM:
        StringAppend(dst, ItemId_GetName(evo->param));
        break;
    case EVO_TRADE:
        StringAppend(dst, sText_DexData_Trade);
        break;
    case EVO_TRADE_ITEM:
        StringAppend(dst, sText_DexData_Trade);
        StringAppend(dst, sText_DexData_Space);
        StringAppend(dst, ItemId_GetName(evo->param));
        break;
    case EVO_FRIENDSHIP:
    case EVO_FRIENDSHIP_ESPEON:
    case EVO_FRIENDSHIP_UMBREON:
        StringAppend(dst, sText_DexData_Friendship);
        if (evo->method == EVO_FRIENDSHIP_ESPEON)
            StringAppend(dst, sText_DexData_Day);
        else if (evo->method == EVO_FRIENDSHIP_UMBREON)
            StringAppend(dst, sText_DexData_Night);
        break;
    case EVO_BEAUTY:
        StringAppend(dst, sText_DexData_Beauty);
        break;
    }
    StringAppend(dst, sText_DexData_Arrow);
    StringAppend(dst, gSpeciesNames[evo->targetSpecies]);
}

// Só o próximo estágio; cada caminho aparece depois de você ver/ter a evolução
// Todas as próximas evoluções, uma por linha (Bulbasaur: IVYSAUR e VENUSAUR).
// Cada uma só aparece depois de vista; a seguinte só aparece se a anterior apareceu.
static bool8 DexExtra_AppendEvoChain(u8 *dst, u16 species, u8 depth, bool8 *hasEvos)
{
    u8 i;
    bool8 any = FALSE;

    if (depth > 3)
        return FALSE;
    for (i = 0; i < EVOS_PER_MON; i++)
    {
        const struct Evolution *evo = &gEvolutionTable[species][i];
        u32 len;

        if (evo->method == 0 || evo->targetSpecies == SPECIES_NONE)
            continue;
        *hasEvos = TRUE;
        if (!DexScreen_GetSetPokedexFlag(evo->targetSpecies, FLAG_GET_SEEN, TRUE))
            continue;
        if (dst[0] != EOS)
        {
            len = StringLength(dst);
            dst[len] = CHAR_NEWLINE;
            dst[len + 1] = EOS;
        }
        DexExtra_AppendEvoMethod(dst, evo);
        any = TRUE;
        DexExtra_AppendEvoChain(dst, evo->targetSpecies, depth + 1, hasEvos);
    }
    return any;
}

static void DexExtra_BuildEvolutionText(u8 *dst, u16 species)
{
    bool8 hasEvos = FALSE;

    dst[0] = EOS;
    if (!DexExtra_AppendEvoChain(dst, species, 0, &hasEvos))
        StringCopy(dst, hasEvos ? sText_DexData_Unknown : sText_DexData_None);
}

// Quebra cada linha para caber na largura. Pode quebrar num espaço ou logo depois da seta
// (THUNDERSTONE→ / JOLTEON). As quebras de linha do texto original são mantidas.
static void DexExtra_Wrap(u8 *dst, const u8 *src, s32 maxWidth)
{
    u8 line[80], test[80], word[40];
    u32 wordLen, len;
    bool8 spaceBefore = FALSE, nextSpace;

    dst[0] = EOS;
    line[0] = EOS;
    while (TRUE)
    {
        wordLen = 0;
        while (*src != EOS && *src != CHAR_SPACE && *src != CHAR_NEWLINE && wordLen < sizeof(word) - 2)
        {
            word[wordLen++] = *src++;
            if (word[wordLen - 1] == CHAR_RIGHT_ARROW)
                break;
        }
        word[wordLen] = EOS;
        nextSpace = (*src == CHAR_SPACE);
        if (wordLen != 0)
        {
            StringCopy(test, line);
            if (line[0] != EOS && spaceBefore)
            {
                len = StringLength(test);
                test[len] = CHAR_SPACE;
                test[len + 1] = EOS;
            }
            StringAppend(test, word);
            if (line[0] != EOS && GetStringWidth(FONT_SMALL, test, 0) > maxWidth)
            {
                StringAppend(dst, line);
                len = StringLength(dst);
                dst[len] = CHAR_NEWLINE;
                dst[len + 1] = EOS;
                StringCopy(line, word);
            }
            else
            {
                StringCopy(line, test);
            }
        }
        if (*src == EOS)
            break;
        if (*src == CHAR_NEWLINE)
        {
            StringAppend(dst, line);
            len = StringLength(dst);
            dst[len] = CHAR_NEWLINE;
            dst[len + 1] = EOS;
            line[0] = EOS;
            src++;
            spaceBefore = FALSE;
            continue;
        }
        if (*src == CHAR_SPACE)
            src++;
        spaceBefore = nextSpace;
    }
    StringAppend(dst, line);
}

// Texto da coluna da direita, alinhado à esquerda
static void DexExtra_PrintInfoLine(u8 windowId, const u8 *str, u8 y)
{
    DexScreen_AddTextPrinterParameterized(windowId, FONT_SMALL, str, DEX_INFO_X, y, 0);
}

static void DexExtra_PrintInfo(u16 species)
{
    u8 *ids = sDexExtraWindowIds;
    u8 i, y, visible, total;
    u8 text[200], wrapped[220], line[80];
    const u8 *src;
    u16 items[2];

    // Metade direita: habilidade, itens e evolução, centralizados
    FillWindowPixelBuffer(ids[5], PIXEL_FILL(0));
    y = DEX_INFO_Y;
    DexScreen_AddTextPrinterParameterized(ids[5], FONT_SMALL, sText_DexData_Ability, DEX_INFO_X, y, 5); // título em marrom
    y += DEX_INFO_LINE;
    for (i = 0; i < 2; i++)
    {
        u8 ability = gSpeciesInfo[species].abilities[i];

        if (ability == ABILITY_NONE || (i == 1 && ability == gSpeciesInfo[species].abilities[0]))
            continue;
        DexExtra_PrintInfoLine(ids[5],
            DexUnlock_Get(gSaveBlock2Ptr->dexAbilityUnlocks, species, i) ? gAbilityNames[ability] : sText_DexData_Unknown, y);
        y += DEX_INFO_LINE;
    }
    y += DEX_INFO_SECTION_GAP; // espaço entre as seções
    DexScreen_AddTextPrinterParameterized(ids[5], FONT_SMALL, sText_DexData_Items, DEX_INFO_X, y, 5); // título em marrom
    y += DEX_INFO_LINE;
    items[0] = gSpeciesInfo[species].itemCommon;
    items[1] = gSpeciesInfo[species].itemRare;
    if (items[0] == ITEM_NONE && items[1] == ITEM_NONE)
    {
        DexExtra_PrintInfoLine(ids[5], sText_DexData_None, y);
        y += DEX_INFO_LINE;
    }
    for (i = 0; i < 2; i++)
    {
        if (items[i] == ITEM_NONE || (i == 1 && items[1] == items[0]))
            continue;
        DexExtra_PrintInfoLine(ids[5],
            DexUnlock_Get(gSaveBlock2Ptr->dexItemUnlocks, species, i) ? ItemId_GetName(items[i]) : sText_DexData_Unknown, y);
        y += DEX_INFO_LINE;
    }
    y += DEX_INFO_SECTION_GAP; // espaço entre as seções
    DexScreen_AddTextPrinterParameterized(ids[5], FONT_SMALL, sText_DexData_Evolution, DEX_INFO_X, y, 5); // título em marrom
    y += DEX_INFO_LINE;
    DexExtra_BuildEvolutionText(text, species);
    DexExtra_Wrap(wrapped, text, DEX_INFO_WIDTH);
    // uma linha por vez, cada uma centralizada; o que não couber rola com cima/baixo
    sDexEvoTopY = y;
    visible = (sWindowTemplate_DexData_Info.height * 8 - y) / DEX_INFO_LINE;
    total = 0;
    src = wrapped;
    while (*src != EOS)
    {
        i = 0;
        while (*src != EOS && *src != CHAR_NEWLINE && i < sizeof(line) - 1)
            line[i++] = *src++;
        line[i] = EOS;
        if (*src == CHAR_NEWLINE)
            src++;
        if (total >= sDexEvoScroll && total < sDexEvoScroll + visible)
        {
            DexExtra_PrintInfoLine(ids[5], line, y);
            y += DEX_INFO_LINE;
        }
        total++;
    }
    sDexEvoMaxScroll = total > visible ? total - visible : 0;
    PutWindowTilemap(ids[5]);
    CopyWindowToVram(ids[5], COPYWIN_GFX);

}

static u8 DexExtra_GetStatBarWidth(void)
{
    u8 str[24];
    u8 *end;
    u16 s;
    s32 w, maxName = 0;

    for (s = 1; s < NUM_SPECIES; s++)
    {
        w = GetStringWidth(FONT_NORMAL, gSpeciesNames[s], 0);
        if (w > maxName)
            maxName = w;
    }
    end = StringCopy(str, gText_PokedexNo);
    end = ConvertIntToDecimalStringN(end, 1, STR_CONV_MODE_LEADING_ZEROS, 3);
    *end++ = CHAR_SPACE;
    *end = EOS;
    // cabeçalho começa no x 40; as barras, no x 48
    w = 40 + GetStringWidth(FONT_NORMAL, str, 0) + maxName - 48;
    if (w > DEX_STAT_BAR_W_MAX)
        w = DEX_STAT_BAR_W_MAX;
    return w & ~1;
}

static void DexExtra_DrawDataPage(u16 species)
{
    u8 *ids = sDexExtraWindowIds;
    u8 i, y, stat, len, barW = DexExtra_GetStatBarWidth();
    const u8 *stats = &gSpeciesInfo[species].baseHP;

    DexUnlock_ScanOwned(species);
    DexExtra_DrawFrame();
    DexExtra_DrawHeader(species);

    ids[3] = AddWindow(&sWindowTemplate_DexData_Stats);
    ids[4] = AddWindow(&sWindowTemplate_DexData_Bars);
    ids[5] = AddWindow(&sWindowTemplate_DexData_Info);

    // Metade esquerda: stats base (ordem do species_info: HP, Atk, Def, Spe, SpA, SpD)
    LoadPalette(sDexStatBarPalette, BG_PLTT_ID(DEX_STAT_BAR_PAL), sizeof(sDexStatBarPalette));
    FillWindowPixelBuffer(ids[3], PIXEL_FILL(0));
    FillWindowPixelBuffer(ids[4], PIXEL_FILL(0));
    for (i = 0; i < NUM_STATS; i++)
    {
        static const u8 sOrder[NUM_STATS] = {0, 1, 2, 4, 5, 3};

        stat = stats[sOrder[i]];
        y = 2 + i * 12;
        DexScreen_AddTextPrinterParameterized(ids[3], FONT_SMALL, sDexStatLabels[i], 0, y, 0);
        DexScreen_PrintNum3RightAlign(ids[3], FONT_SMALL, stat, 22, y, 0);
        len = (stat >= DEX_STAT_BAR_MAX ? DEX_STAT_BAR_MAX : stat) * barW / DEX_STAT_BAR_MAX;
        len &= ~1; // largura par: evita o pixel da cor vizinha no fim da barra
        if (len < 2)
            len = 2;
        FillWindowPixelRect(ids[4], PIXEL_FILL(1), 0, y + 4, barW, 6);
        FillWindowPixelRect(ids[4], PIXEL_FILL(DexExtra_StatBarColor(stat)), 0, y + 4, len, 6);
    }
    PutWindowTilemap(ids[3]);
    CopyWindowToVram(ids[3], COPYWIN_GFX);
    PutWindowTilemap(ids[4]);
    CopyWindowToVram(ids[4], COPYWIN_GFX);

    DexExtra_PrintInfo(species);
    DexExtra_UpdateArrows();

    DexExtra_DrawFooter(sText_DexData_Moves, gText_CancelPreviousData);
}

// Golpes de ovo ficam na forma básica: sobe a linha evolutiva até achá-la
static u16 DexExtra_GetEggSpecies(u16 species)
{
    u16 s, depth;
    u8 j;
    bool8 found;

    for (depth = 0; depth < 3; depth++)
    {
        found = FALSE;
        for (s = 1; s < NUM_SPECIES && !found; s++)
            for (j = 0; j < EVOS_PER_MON; j++)
                if (gEvolutionTable[s][j].targetSpecies == species)
                {
                    species = s;
                    found = TRUE;
                    break;
                }
        if (!found)
            break;
    }
    return species;
}

static void DexExtra_AddMove(u16 move, u8 label)
{
    if (sDexMovesCount >= DEX_MOVES_MAX || move == MOVE_NONE)
        return;
    sDexMovesList[sDexMovesCount] = move;
    sDexMovesLabel[sDexMovesCount] = label;
    sDexMovesCount++;
}

static void DexExtra_BuildMovesList(u16 species)
{
    const u16 *learnset;
    u16 i, eggSpecies;

    sDexMovesCount = 0;
    sDexMovesSplit = 0;
    switch (sDexMovesTab)
    {
    case DEX_MOVES_TAB_LEVEL:
        learnset = GetSpeciesLevelUpLearnset(species);
        for (i = 0; learnset != NULL && learnset[i] != LEVEL_UP_END; i++)
            DexExtra_AddMove(learnset[i] & LEVEL_UP_MOVE_ID, (learnset[i] & LEVEL_UP_MOVE_LV) >> 9);
        break;
    case DEX_MOVES_TAB_TMHM:
        // TMs na coluna da esquerda, HMs na da direita
        for (i = 0; i < NUM_TECHNICAL_MACHINES; i++)
            if (CanSpeciesLearnTMHM(species, i))
                DexExtra_AddMove(ItemIdToBattleMoveId(ITEM_TM01 + i), i);
        sDexMovesSplit = sDexMovesCount;
        for (i = NUM_TECHNICAL_MACHINES; i < NUM_TECHNICAL_MACHINES + NUM_HIDDEN_MACHINES; i++)
            if (CanSpeciesLearnTMHM(species, i))
                DexExtra_AddMove(ItemIdToBattleMoveId(ITEM_TM01 + i), i);
        return;
    case DEX_MOVES_TAB_TUTOR:
        // Tutores na coluna da esquerda, golpes de ovo na da direita (igual TM/HM)
        for (i = 0; i < TUTOR_MOVE_HYDRO_CANNON + 1; i++)
            if (CanSpeciesLearnTutorMoveForDex(species, i))
                DexExtra_AddMove(GetTutorMoveForDex(i), i);
        sDexMovesSplit = sDexMovesCount;
        eggSpecies = DexExtra_GetEggSpecies(species);
        for (i = 0; gEggMoves[i] != EGG_MOVES_TERMINATOR; i++)
        {
            if (gEggMoves[i] != eggSpecies + EGG_MOVES_SPECIES_OFFSET)
                continue;
            for (i++; gEggMoves[i] != EGG_MOVES_TERMINATOR && gEggMoves[i] < EGG_MOVES_SPECIES_OFFSET; i++)
                DexExtra_AddMove(gEggMoves[i], 0);
            break;
        }
        return;
    }
    // Lista em ordem de cima para baixo: enche a coluna da esquerda e continua na da direita
    if (sDexMovesCount <= DEX_MOVES_ROWS * 2)
        sDexMovesSplit = sDexMovesCount < DEX_MOVES_ROWS ? sDexMovesCount : DEX_MOVES_ROWS;
    else
        sDexMovesSplit = (sDexMovesCount + 1) / 2;
}

static u8 DexExtra_MovesRows(void)
{
    u8 left = sDexMovesSplit, right = sDexMovesCount - sDexMovesSplit;

    return left > right ? left : right;
}

static void DexExtra_PrintMovesTabs(void)
{
    u8 windowId = sDexExtraWindowIds[3];
    u8 i, total = 0, gap, x;

    for (i = 0; i < DEX_MOVES_TAB_COUNT; i++)
        total += GetStringWidth(FONT_SMALL, sDexMovesTabNames[i], 0);
    gap = 6;
    x = 20; // janela começa no x 104; abas no x 124, alinhadas com a coluna da direita dos golpes

    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
    for (i = 0; i < DEX_MOVES_TAB_COUNT; i++)
    {
        u8 w = GetStringWidth(FONT_SMALL, sDexMovesTabNames[i], 0);

        // aba atual em vermelho, as outras em cinza (igual ao OPTION)
        DexScreen_AddTextPrinterParameterized(windowId, FONT_SMALL, sDexMovesTabNames[i], x, DEX_INFO_Y, i == sDexMovesTab ? 6 : 7);
        x += w + gap;
    }
    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void DexExtra_PrintMoveEntry(u8 windowId, u8 idx, u8 x, u8 y)
{
    u8 label[8];

    switch (sDexMovesTab)
    {
    case DEX_MOVES_TAB_LEVEL:
        StringCopy(label, sText_DexData_Lv);
        ConvertIntToDecimalStringN(label + StringLength(label), sDexMovesLabel[idx], STR_CONV_MODE_LEFT_ALIGN, 3);
        break;
    case DEX_MOVES_TAB_TMHM:
        if (sDexMovesLabel[idx] < NUM_TECHNICAL_MACHINES)
        {
            StringCopy(label, sText_DexData_TM);
            ConvertIntToDecimalStringN(label + 2, sDexMovesLabel[idx] + 1, STR_CONV_MODE_LEADING_ZEROS, 2);
        }
        else
        {
            StringCopy(label, sText_DexData_HM);
            ConvertIntToDecimalStringN(label + 2, sDexMovesLabel[idx] - NUM_TECHNICAL_MACHINES + 1, STR_CONV_MODE_LEADING_ZEROS, 2);
        }
        break;
    default:
        DexScreen_AddTextPrinterParameterized(windowId, FONT_SMALL, gMoveNames[sDexMovesList[idx]], x, y, 0);
        return;
    }
    DexScreen_AddTextPrinterParameterized(windowId, FONT_SMALL, label, x, y, 0);
    DexScreen_AddTextPrinterParameterized(windowId, FONT_SMALL, gMoveNames[sDexMovesList[idx]], x + 26, y, 0);
}

static void DexExtra_PrintMovesList(void)
{
    u8 windowId = sDexExtraWindowIds[4];
    u8 row, idx;

    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
    if (sDexMovesCount == 0)
        DexScreen_AddTextPrinterParameterized(windowId, FONT_SMALL, sText_DexData_None, 4, 2, 0);
    for (row = 0; row < DEX_MOVES_ROWS; row++)
    {
        idx = sDexMovesScroll + row;
        if (idx < sDexMovesSplit)
            DexExtra_PrintMoveEntry(windowId, idx, 4, 2 + row * 13);
        idx = sDexMovesSplit + sDexMovesScroll + row;
        if (idx < sDexMovesCount)
            DexExtra_PrintMoveEntry(windowId, idx, 116, 2 + row * 13);
    }
    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void DexExtra_DrawMovesPage(u16 species)
{
    u8 *ids = sDexExtraWindowIds;

    DexExtra_DrawFrame();
    DexExtra_DrawHeader(species);
    ids[3] = AddWindow(&sWindowTemplate_DexMoves_Tabs);
    ids[4] = AddWindow(&sWindowTemplate_DexMoves_List);
    ids[5] = 0xFF;
    sDexMovesScroll = 0;
    DexExtra_BuildMovesList(species);
    DexExtra_PrintMovesTabs();
    DexExtra_PrintMovesList();
    DexExtra_DrawFooter(sText_DexData_TabScroll, gText_CancelPreviousData);
}

static void DexExtra_RemoveArrows(void)
{
    u8 i;

    for (i = 0; i < NELEMS(sDexArrowTasks); i++)
    {
        if (sDexArrowTasks[i] != TASK_NONE)
            RemoveScrollIndicatorArrowPair(sDexArrowTasks[i]);
        sDexArrowTasks[i] = TASK_NONE;
    }
}

// Setas: cima/baixo quando dá para rolar, esquerda/direita ao lado das abas
static void DexExtra_UpdateArrows(void)
{
    u8 rows, maxScroll;

    DexExtra_RemoveArrows();
    if (sDexExtraPage == DEX_EXTRA_MOVES)
    {
        rows = DexExtra_MovesRows();
        maxScroll = rows > DEX_MOVES_ROWS ? rows - DEX_MOVES_ROWS : 0;
        if (maxScroll != 0)
            sDexArrowTasks[0] = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_UP, DEX_ARROW_X_RIGHT, 64, 128,
                                                                          maxScroll, DEX_ARROW_TAG_VERT, DEX_ARROW_TAG_VERT, &sDexMovesScroll);
    }
    else if (sDexEvoMaxScroll != 0)
    {
        sDexArrowTasks[0] = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_UP, DEX_ARROW_X_RIGHT, 32 + sDexEvoTopY + 6, 128,
                                                                      sDexEvoMaxScroll, DEX_ARROW_TAG_VERT, DEX_ARROW_TAG_VERT, &sDexEvoScroll);
    }

}

static void DexExtra_Open(u8 page)
{
    u8 i;

    for (i = 0; i < NELEMS(sDexExtraWindowIds); i++)
        sDexExtraWindowIds[i] = 0xFF;
    for (i = 0; i < NELEMS(sDexArrowTasks); i++)
        sDexArrowTasks[i] = TASK_NONE; // já foram removidas no Close
    sDexExtraPage = page;
    sDexMovesFromData = FALSE;
    sDexEvoScroll = 0;
    if (page == DEX_EXTRA_MOVES)
    {
        sDexMovesTab = DEX_MOVES_TAB_LEVEL;
        DexExtra_DrawMovesPage(sPokedexScreenData->dexSpecies);
        DexExtra_UpdateArrows();
    }
    else
    {
        DexExtra_DrawDataPage(sPokedexScreenData->dexSpecies);
    }
}

static void DexExtra_Close(void)
{
    u8 i;

    DexExtra_RemoveArrows();
    for (i = 0; i < NELEMS(sDexExtraWindowIds); i++)
        DexScreen_RemoveWindow(&sDexExtraWindowIds[i]);
}

static void DexExtra_SwitchPage(u8 page)
{
    HideBg(2);
    HideBg(1);
    DexExtra_Close();
    DexExtra_ClearBgs();
    DexExtra_Open(page);
    sDexExtraPendingShow = 1;
}

static u8 DexExtra_HandleInput(void)
{
    u8 maxScroll;

    // Troca de página: 1) espera os gráficos e copia os mapas de tiles; 2) espera e mostra
    if (sDexExtraPendingShow != 0)
    {
        if (IsDma3ManagerBusyWithBgCopy())
            return DEX_EXTRA_RESULT_NONE;
        if (sDexExtraPendingShow == 1)
        {
            DexExtra_CopyAllBgs();
            sDexExtraPendingShow = 2;
        }
        else
        {
            ShowBg(2);
            ShowBg(1);
            sDexExtraPendingShow = 0;
        }
        return DEX_EXTRA_RESULT_NONE;
    }

    if (JOY_NEW(A_BUTTON))
        return DEX_EXTRA_RESULT_EXIT;
    if (JOY_NEW(B_BUTTON))
    {
        if (sDexExtraPage == DEX_EXTRA_MOVES && sDexMovesFromData)
        {
            sDexMovesFromData = FALSE;
            DexExtra_SwitchPage(DEX_EXTRA_DATA);
            return DEX_EXTRA_RESULT_NONE;
        }
        return DEX_EXTRA_RESULT_BACK;
    }
    if (sDexExtraPage != DEX_EXTRA_MOVES)
    {
        if (JOY_NEW(START_BUTTON))
        {
            PlaySE(SE_SELECT);
            DexExtra_SwitchPage(DEX_EXTRA_MOVES);
            sDexMovesFromData = TRUE;
        }
        else if (JOY_REPT(DPAD_UP) && sDexEvoScroll > 0)
        {
            PlaySE(SE_SELECT);
            sDexEvoScroll--;
            DexExtra_PrintInfo(sPokedexScreenData->dexSpecies);
        }
        else if (JOY_REPT(DPAD_DOWN) && sDexEvoScroll < sDexEvoMaxScroll)
        {
            PlaySE(SE_SELECT);
            sDexEvoScroll++;
            DexExtra_PrintInfo(sPokedexScreenData->dexSpecies);
        }
        return DEX_EXTRA_RESULT_NONE;
    }

    if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        if (JOY_NEW(DPAD_LEFT))
            sDexMovesTab = (sDexMovesTab == 0) ? DEX_MOVES_TAB_COUNT - 1 : sDexMovesTab - 1;
        else
            sDexMovesTab = (sDexMovesTab + 1) % DEX_MOVES_TAB_COUNT;
        PlaySE(SE_SELECT);
        sDexMovesScroll = 0;
        DexExtra_BuildMovesList(sPokedexScreenData->dexSpecies);
        DexExtra_PrintMovesTabs();
        DexExtra_PrintMovesList();
        DexExtra_UpdateArrows();
        CopyBgTilemapBufferToVram(2);
    }
    else if (JOY_REPT(DPAD_UP | DPAD_DOWN))
    {
        u8 rows = DexExtra_MovesRows();

        maxScroll = rows > DEX_MOVES_ROWS ? rows - DEX_MOVES_ROWS : 0;
        if (JOY_REPT(DPAD_UP) && sDexMovesScroll > 0)
            sDexMovesScroll--;
        else if (JOY_REPT(DPAD_DOWN) && sDexMovesScroll < maxScroll)
            sDexMovesScroll++;
        else
            return DEX_EXTRA_RESULT_NONE;
        PlaySE(SE_SELECT);
        DexExtra_PrintMovesList();
    }
    return DEX_EXTRA_RESULT_NONE;
}

static void DexExtra_ClearBgs(void)
{
    FillBgTilemapBufferRect_Palette0(2, 0x000, 0, 2, 30, 16);
    FillBgTilemapBufferRect_Palette0(1, 0x000, 0, 2, 30, 16);
    FillBgTilemapBufferRect_Palette0(0, 0x000, 0, 2, 30, 16);
    CopyBgTilemapBufferToVram(2);
    CopyBgTilemapBufferToVram(1);
    CopyBgTilemapBufferToVram(0);
}

static void DexExtra_CopyAllBgs(void)
{
    CopyBgTilemapBufferToVram(3);
    CopyBgTilemapBufferToVram(2);
    CopyBgTilemapBufferToVram(1);
    CopyBgTilemapBufferToVram(0);
}

#if DEX_AREA_LIST_ENABLED
// =====================================================================================
// HACK: AREA LIST — Pokémon selvagens do mapa atual, lidos da tabela de encontros da
// versão ativa (wild_encounters.json). Abas só para os métodos que o mapa tem; na pesca,
// escolhe a vara. Capturado = ícone colorido; visto = silhueta; não visto = "?".
// =====================================================================================

#define AREA_TAB_LAND       0
#define AREA_TAB_SURF       1
#define AREA_TAB_FISHING    2
#define AREA_TAB_ROCK_SMASH 3
#define AREA_TAB_COUNT      4

#define AREA_FOCUS_GRID 0
#define AREA_FOCUS_TABS 1
#define AREA_FOCUS_RODS 2

#define AREA_MAX_MONS   10
#define AREA_COLUMNS    5
#define AREA_CELL_W     42
#define AREA_CELL_H     30
#define AREA_GRID_X     16  // x absoluto da primeira coluna
#define AREA_WIN_X      8   // janela principal começa no x 8 / y 16 da tela
#define AREA_WIN_Y      16
#define AREA_SILHOUETTE_TAG 0xD7B0

struct AreaListMon
{
    u16 species;
    u8 minLevel;
    u8 maxLevel;
    u8 rate;
};

static EWRAM_DATA u8 sAreaTab = 0;          // posição na lista de abas disponíveis
static EWRAM_DATA u8 sAreaRod = 0;          // 0 OLD, 1 GOOD, 2 SUPER
static EWRAM_DATA u8 sAreaCursor = 0;
static EWRAM_DATA u8 sAreaFocus = 0;
static EWRAM_DATA u8 sAreaNumTabs = 0;
static EWRAM_DATA u8 sAreaTabs[AREA_TAB_COUNT] = {0};
static EWRAM_DATA u8 sAreaNumMons = 0;
static EWRAM_DATA struct AreaListMon sAreaMons[AREA_MAX_MONS] = {0};
static EWRAM_DATA u8 sAreaIconSpriteIds[AREA_MAX_MONS] = {0};
static EWRAM_DATA u8 sAreaWindowId = 0;
static EWRAM_DATA u8 sAreaCaught = 0;
static EWRAM_DATA u8 sAreaSeen = 0;
static EWRAM_DATA u8 sAreaTotal = 0;
static EWRAM_DATA const struct WildPokemonHeader *sAreaHeader = NULL;

static const u8 sText_AreaList_Title[]    = _("AREA LIST");
static const u8 sText_AreaList_NoMons[]   = _("There are no wild POKéMON here.");
static const u8 sText_AreaList_Controls[] = _("{A_BUTTON}DATA {B_BUTTON}CANCEL");
static const u8 sText_AreaList_Move[]     = _("{DPAD_ANY}MOVE");
static const u8 sText_AreaList_Caught[]   = _("CAUGHT ");
static const u8 sText_AreaList_Seen[]     = _("SEEN ");
static const u8 sText_AreaList_Unknown[]  = _("-----");
static const u8 sText_AreaList_QMark[]    = _("?");
static const u8 sText_AreaList_Percent[]  = _("%");
static const u8 sText_AreaList_Dash[]     = _("-");
static const u8 sText_AreaTab_Grass[]     = _("GRASS");
static const u8 sText_AreaTab_Cave[]      = _("CAVE");
static const u8 sText_AreaTab_Land[]      = _("LAND");
static const u8 sText_AreaTab_Surf[]      = _("SURF");
static const u8 sText_AreaTab_Fishing[]   = _("FISHING");
static const u8 sText_AreaTab_RockSmash[] = _("ROCK SMASH");
static const u8 sText_AreaRod_Old[]       = _("OLD ROD");
static const u8 sText_AreaRod_Good[]      = _("GOOD ROD");
static const u8 sText_AreaRod_Super[]     = _("SUPER ROD");
static const u8 *const sAreaRodNames[3] = {sText_AreaRod_Old, sText_AreaRod_Good, sText_AreaRod_Super};

// Chance de cada posição da tabela (iguais às de wild_encounter.c)
static const u8 sAreaLandRates[LAND_WILD_COUNT] = {20, 20, 10, 10, 10, 10, 5, 5, 4, 4, 1, 1};
static const u8 sAreaWaterRates[WATER_WILD_COUNT] = {60, 30, 5, 4, 1};
static const u8 sAreaFishRates[FISH_WILD_COUNT] = {70, 30, 60, 20, 20, 40, 40, 15, 4, 1};
static const u8 sAreaRodFirstSlot[3] = {0, 2, 5};
static const u8 sAreaRodNumSlots[3] = {2, 3, 5};

static const struct WindowTemplate sWindowTemplate_AreaList = {
    .bg = 2, .tilemapLeft = 1, .tilemapTop = 2, .width = 28, .height = 15, .paletteNum = 0, .baseBlock = 0x01A8
};

static const u8 *AreaList_GetTabName(u8 tab)
{
    switch (tab)
    {
    case AREA_TAB_LAND:
        if (gMapHeader.mapType == MAP_TYPE_UNDERGROUND)
            return sText_AreaTab_Cave;
        if (gMapHeader.mapType == MAP_TYPE_INDOOR)
            return sText_AreaTab_Land;
        return sText_AreaTab_Grass;
    case AREA_TAB_SURF:
        return sText_AreaTab_Surf;
    case AREA_TAB_FISHING:
        return sText_AreaTab_Fishing;
    default:
        return sText_AreaTab_RockSmash;
    }
}

static void AreaList_AddSlots(struct AreaListMon *list, u8 *count, const struct WildPokemon *mons, const u8 *rates, u8 first, u8 num)
{
    u8 i, j;

    for (i = first; i < first + num; i++)
    {
        if (mons[i].species == SPECIES_NONE)
            continue;
        for (j = 0; j < *count; j++)
            if (list[j].species == mons[i].species)
                break;
        if (j == *count)
        {
            if (*count >= AREA_MAX_MONS)
                continue;
            list[j].species = mons[i].species;
            list[j].minLevel = mons[i].minLevel;
            list[j].maxLevel = mons[i].maxLevel;
            list[j].rate = 0;
            (*count)++;
        }
        list[j].rate += rates[i];
        if (mons[i].minLevel < list[j].minLevel)
            list[j].minLevel = mons[i].minLevel;
        if (mons[i].maxLevel > list[j].maxLevel)
            list[j].maxLevel = mons[i].maxLevel;
    }
}

// Lista de uma aba (e vara, na pesca)
static void AreaList_BuildList(u8 tab, u8 rod, struct AreaListMon *list, u8 *count)
{
    *count = 0;
    if (sAreaHeader == NULL)
        return;
    switch (tab)
    {
    case AREA_TAB_LAND:
        if (sAreaHeader->landMonsInfo != NULL)
            AreaList_AddSlots(list, count, sAreaHeader->landMonsInfo->wildPokemon, sAreaLandRates, 0, LAND_WILD_COUNT);
        break;
    case AREA_TAB_SURF:
        if (sAreaHeader->waterMonsInfo != NULL)
            AreaList_AddSlots(list, count, sAreaHeader->waterMonsInfo->wildPokemon, sAreaWaterRates, 0, WATER_WILD_COUNT);
        break;
    case AREA_TAB_FISHING:
        if (sAreaHeader->fishingMonsInfo != NULL)
            AreaList_AddSlots(list, count, sAreaHeader->fishingMonsInfo->wildPokemon, sAreaFishRates, sAreaRodFirstSlot[rod], sAreaRodNumSlots[rod]);
        break;
    case AREA_TAB_ROCK_SMASH:
        if (sAreaHeader->rockSmashMonsInfo != NULL)
            AreaList_AddSlots(list, count, sAreaHeader->rockSmashMonsInfo->wildPokemon, sAreaWaterRates, 0, ROCK_WILD_COUNT);
        break;
    }
}

// Abas disponíveis e contadores do mapa inteiro (todas as abas e varas juntas)
static void AreaList_Setup(void)
{
    struct AreaListMon all[AREA_MAX_MONS * 2], tmp[AREA_MAX_MONS];
    u8 numAll = 0, n, i, j, tab, rod;

    sAreaHeader = GetCurrentMapWildMonHeaderForDex();
    sAreaNumTabs = 0;
    if (sAreaHeader != NULL)
    {
        if (sAreaHeader->landMonsInfo != NULL)
            sAreaTabs[sAreaNumTabs++] = AREA_TAB_LAND;
        if (sAreaHeader->waterMonsInfo != NULL)
            sAreaTabs[sAreaNumTabs++] = AREA_TAB_SURF;
        if (sAreaHeader->fishingMonsInfo != NULL)
            sAreaTabs[sAreaNumTabs++] = AREA_TAB_FISHING;
        if (sAreaHeader->rockSmashMonsInfo != NULL)
            sAreaTabs[sAreaNumTabs++] = AREA_TAB_ROCK_SMASH;
    }
    if (sAreaTab >= sAreaNumTabs)
        sAreaTab = 0;

    for (i = 0; i < sAreaNumTabs; i++)
    {
        tab = sAreaTabs[i];
        for (rod = 0; rod < (tab == AREA_TAB_FISHING ? 3 : 1); rod++)
        {
            AreaList_BuildList(tab, rod, tmp, &n);
            for (j = 0; j < n; j++)
            {
                u8 k;
                for (k = 0; k < numAll; k++)
                    if (all[k].species == tmp[j].species)
                        break;
                if (k == numAll && numAll < NELEMS(all))
                    all[numAll++] = tmp[j];
            }
        }
    }
    sAreaTotal = numAll;
    sAreaCaught = sAreaSeen = 0;
    for (i = 0; i < numAll; i++)
    {
        if (DexScreen_GetSetPokedexFlag(all[i].species, FLAG_GET_SEEN, TRUE))
            sAreaSeen++;
        if (DexScreen_GetSetPokedexFlag(all[i].species, FLAG_GET_CAUGHT, TRUE))
            sAreaCaught++;
    }
}

static u8 AreaList_CurrentTab(void)
{
    return sAreaNumTabs != 0 ? sAreaTabs[sAreaTab] : AREA_TAB_LAND;
}

static bool8 AreaList_IsFishing(void)
{
    return sAreaNumTabs != 0 && AreaList_CurrentTab() == AREA_TAB_FISHING;
}

static u8 AreaList_GridTop(void)
{
    return AreaList_IsFishing() ? 76 : 62;
}

static void AreaList_CellPos(u8 index, s16 *cx, s16 *top)
{
    *cx = AREA_GRID_X + (index % AREA_COLUMNS) * AREA_CELL_W + AREA_CELL_W / 2;
    *top = AreaList_GridTop() + (index / AREA_COLUMNS) * AREA_CELL_H;
}

// Contorno de 1 px (coordenadas da tela)
static void AreaList_DrawBox(s16 x, s16 y, s16 w, s16 h)
{
    x -= AREA_WIN_X;
    y -= AREA_WIN_Y;
    FillWindowPixelRect(sAreaWindowId, PIXEL_FILL(1), x, y, w, 1);
    FillWindowPixelRect(sAreaWindowId, PIXEL_FILL(1), x, y + h - 1, w, 1);
    FillWindowPixelRect(sAreaWindowId, PIXEL_FILL(1), x, y, 1, h);
    FillWindowPixelRect(sAreaWindowId, PIXEL_FILL(1), x + w - 1, y, 1, h);
}

static void AreaList_PrintAbs(u8 font, const u8 *str, s16 x, s16 y)
{
    DexScreen_AddTextPrinterParameterized(sAreaWindowId, font, str, x - AREA_WIN_X, y - AREA_WIN_Y, 0);
}

static void AreaList_DestroyIcons(void)
{
    u8 i;

    for (i = 0; i < AREA_MAX_MONS; i++)
    {
        if (sAreaIconSpriteIds[i] != SPRITE_NONE)
            DestroyMonIcon(&gSprites[sAreaIconSpriteIds[i]]);
        sAreaIconSpriteIds[i] = SPRITE_NONE;
    }
}

static void AreaList_CreateIcons(void)
{
    u8 i, silhouette = IndexOfSpritePaletteTag(AREA_SILHOUETTE_TAG);
    s16 cx, top;

    AreaList_DestroyIcons();
    for (i = 0; i < sAreaNumMons; i++)
    {
        u16 species = sAreaMons[i].species;

        if (!DexScreen_GetSetPokedexFlag(species, FLAG_GET_SEEN, TRUE))
            continue; // não visto: só o "?"
        AreaList_CellPos(i, &cx, &top);
        sAreaIconSpriteIds[i] = CreateMonIcon(species, SpriteCallbackDummy, cx, top + 14, 1, DexScreen_GetDefaultPersonality(species), FALSE);
        if (sAreaIconSpriteIds[i] == MAX_SPRITES)
        {
            sAreaIconSpriteIds[i] = SPRITE_NONE;
            continue;
        }
        gSprites[sAreaIconSpriteIds[i]].oam.priority = 1;
        if (!DexScreen_GetSetPokedexFlag(species, FLAG_GET_CAUGHT, TRUE) && silhouette != 0xFF)
            gSprites[sAreaIconSpriteIds[i]].oam.paletteNum = silhouette; // visto: silhueta
    }
}

// O ícone selecionado se mexe; os outros ficam parados
static void AreaList_UpdateIconAnims(void)
{
    u8 i;

    for (i = 0; i < AREA_MAX_MONS; i++)
    {
        if (sAreaIconSpriteIds[i] == SPRITE_NONE)
            continue;
        gSprites[sAreaIconSpriteIds[i]].callback = (sAreaFocus == AREA_FOCUS_GRID && i == sAreaCursor) ? SpriteCB_MonIcon : SpriteCallbackDummy;
    }
}

static void AreaList_Draw(void)
{
    u8 str[48], num[8], i;
    u8 *end;
    s16 x, cx, top, w;

    FillWindowPixelBuffer(sAreaWindowId, PIXEL_FILL(0));

    // Nome do mapa e contadores
    GetMapName(str, gMapHeader.regionMapSectionId, 0);
    AreaList_PrintAbs(FONT_NORMAL, str, 12, 20);

    if (sAreaNumTabs == 0)
    {
        w = GetStringWidth(FONT_SMALL, sText_AreaList_NoMons, 0);
        AreaList_PrintAbs(FONT_SMALL, sText_AreaList_NoMons, 120 - w / 2, 80);
        PutWindowTilemap(sAreaWindowId);
        CopyWindowToVram(sAreaWindowId, COPYWIN_FULL);
        return;
    }

    end = StringCopy(str, sText_AreaList_Caught);
    end = ConvertIntToDecimalStringN(end, sAreaCaught, STR_CONV_MODE_LEFT_ALIGN, 2);
    *end++ = CHAR_SLASH;
    ConvertIntToDecimalStringN(end, sAreaTotal, STR_CONV_MODE_LEFT_ALIGN, 2);
    AreaList_PrintAbs(FONT_SMALL, str, 226 - GetStringWidth(FONT_SMALL, str, 0), 20);
    end = StringCopy(str, sText_AreaList_Seen);
    end = ConvertIntToDecimalStringN(end, sAreaSeen, STR_CONV_MODE_LEFT_ALIGN, 2);
    *end++ = CHAR_SLASH;
    ConvertIntToDecimalStringN(end, sAreaTotal, STR_CONV_MODE_LEFT_ALIGN, 2);
    AreaList_PrintAbs(FONT_SMALL, str, 226 - GetStringWidth(FONT_SMALL, str, 0), 31);

    // Abas (só as que o mapa tem)
    x = 12;
    for (i = 0; i < sAreaNumTabs; i++)
    {
        const u8 *name = AreaList_GetTabName(sAreaTabs[i]);

        w = GetStringWidth(FONT_SMALL, name, 0);
        AreaList_PrintAbs(FONT_SMALL, name, x, 42);
        if (i == sAreaTab)
        {
            FillWindowPixelRect(sAreaWindowId, PIXEL_FILL(1), x - AREA_WIN_X, 54 - AREA_WIN_Y, w, 1);
            if (sAreaFocus == AREA_FOCUS_TABS)
                AreaList_DrawBox(x - 3, 41, w + 6, 16);
        }
        x += w + 12;
    }

    // Varas (só na pesca)
    if (AreaList_IsFishing())
    {
        x = 12;
        for (i = 0; i < 3; i++)
        {
            w = GetStringWidth(FONT_SMALL, sAreaRodNames[i], 0);
            AreaList_PrintAbs(FONT_SMALL, sAreaRodNames[i], x, 58);
            if (i == sAreaRod)
            {
                FillWindowPixelRect(sAreaWindowId, PIXEL_FILL(1), x - AREA_WIN_X, 70 - AREA_WIN_Y, w, 1);
                if (sAreaFocus == AREA_FOCUS_RODS)
                    AreaList_DrawBox(x - 3, 57, w + 6, 16);
            }
            x += w + 12;
        }
    }

    // Grade: "?" nos não vistos; contorno no selecionado
    for (i = 0; i < sAreaNumMons; i++)
    {
        AreaList_CellPos(i, &cx, &top);
        if (!DexScreen_GetSetPokedexFlag(sAreaMons[i].species, FLAG_GET_SEEN, TRUE))
            AreaList_PrintAbs(FONT_NORMAL, sText_AreaList_QMark, cx - 3, top + 7);
        if (sAreaFocus == AREA_FOCUS_GRID && i == sAreaCursor)
            AreaList_DrawBox(cx - AREA_CELL_W / 2 + 2, top, AREA_CELL_W - 4, AREA_CELL_H - 1);
    }

    // Linha de informação do selecionado: nome, níveis e chance
    if (sAreaNumMons != 0 && sAreaFocus == AREA_FOCUS_GRID)
    {
        struct AreaListMon *mon = &sAreaMons[sAreaCursor];

        if (DexScreen_GetSetPokedexFlag(mon->species, FLAG_GET_SEEN, TRUE))
        {
            AreaList_PrintAbs(FONT_SMALL, gSpeciesNames[mon->species], 12, 122);
            end = StringCopy(str, sText_DexData_Lv);
            end = ConvertIntToDecimalStringN(end, mon->minLevel, STR_CONV_MODE_LEFT_ALIGN, 3);
            if (mon->maxLevel != mon->minLevel)
            {
                end = StringCopy(end, sText_AreaList_Dash);
                end = ConvertIntToDecimalStringN(end, mon->maxLevel, STR_CONV_MODE_LEFT_ALIGN, 3);
            }
            AreaList_PrintAbs(FONT_SMALL, str, 92, 122);
            end = ConvertIntToDecimalStringN(num, mon->rate, STR_CONV_MODE_LEFT_ALIGN, 3);
            StringCopy(end, sText_AreaList_Percent);
            AreaList_PrintAbs(FONT_SMALL, num, 150, 122);
        }
        else
        {
            AreaList_PrintAbs(FONT_SMALL, sText_AreaList_Unknown, 12, 122);
        }
    }

    PutWindowTilemap(sAreaWindowId);
    CopyWindowToVram(sAreaWindowId, COPYWIN_FULL);
    AreaList_UpdateIconAnims();
}

static void AreaList_LoadList(void)
{
    AreaList_BuildList(AreaList_CurrentTab(), sAreaRod, sAreaMons, &sAreaNumMons);
    if (sAreaCursor >= sAreaNumMons)
        sAreaCursor = 0;
    AreaList_CreateIcons();
}

static void AreaList_Open(void)
{
    struct SpritePalette pal = {sPalette_Silhouette, AREA_SILHOUETTE_TAG};
    u8 i;

    for (i = 0; i < AREA_MAX_MONS; i++)
        sAreaIconSpriteIds[i] = SPRITE_NONE;
    DexExtra_DrawFrame();
    FillWindowPixelBuffer(0, PIXEL_FILL(15));
    DexScreen_PrintStringWithAlignment(sText_AreaList_Title, TEXT_CENTER);
    CopyWindowToVram(0, COPYWIN_GFX);
    FillWindowPixelBuffer(1, PIXEL_FILL(15));
    DexScreen_AddTextPrinterParameterized(1, FONT_SMALL, sText_AreaList_Move, 8, 2, 4);
    DexScreen_PrintControlInfo(sText_AreaList_Controls);
    PutWindowTilemap(1);
    CopyWindowToVram(1, COPYWIN_GFX);

    LoadMonIconPalettes();
    LoadSpritePalette(&pal);
    sAreaWindowId = AddWindow(&sWindowTemplate_AreaList);
    AreaList_Setup();
    if (sAreaNumTabs == 0 || sAreaFocus > AREA_FOCUS_RODS)
        sAreaFocus = AREA_FOCUS_GRID;
    AreaList_LoadList();
    AreaList_Draw();
}

static void AreaList_Close(void)
{
    AreaList_DestroyIcons();
    FreeMonIconPalettes();
    FreeSpritePaletteByTag(AREA_SILHOUETTE_TAG);
    DexScreen_RemoveWindow(&sAreaWindowId);
    FillBgTilemapBufferRect_Palette0(2, 0x000, 0, 0, 30, 20);
    CopyBgTilemapBufferToVram(2);
}

// Retorna TRUE quando deve abrir a página do Pokémon selecionado
static bool8 AreaList_HandleInput(void)
{
    u8 row, col, rows;
    bool8 redraw = FALSE, reload = FALSE;

    if (sAreaNumTabs == 0)
        return FALSE;

    switch (sAreaFocus)
    {
    case AREA_FOCUS_GRID:
        row = sAreaCursor / AREA_COLUMNS;
        col = sAreaCursor % AREA_COLUMNS;
        rows = (sAreaNumMons + AREA_COLUMNS - 1) / AREA_COLUMNS;
        if (JOY_NEW(A_BUTTON) && sAreaNumMons != 0)
        {
            if (DexScreen_GetSetPokedexFlag(sAreaMons[sAreaCursor].species, FLAG_GET_SEEN, TRUE))
                return TRUE;
            PlaySE(SE_FAILURE);
        }
        else if (JOY_REPT(DPAD_LEFT) && col > 0)
        {
            sAreaCursor--;
            redraw = TRUE;
        }
        else if (JOY_REPT(DPAD_RIGHT) && col < AREA_COLUMNS - 1 && sAreaCursor + 1 < sAreaNumMons)
        {
            sAreaCursor++;
            redraw = TRUE;
        }
        else if (JOY_REPT(DPAD_DOWN) && row + 1 < rows)
        {
            sAreaCursor += AREA_COLUMNS;
            if (sAreaCursor >= sAreaNumMons)
                sAreaCursor = sAreaNumMons - 1;
            redraw = TRUE;
        }
        else if (JOY_REPT(DPAD_UP))
        {
            if (row > 0)
                sAreaCursor -= AREA_COLUMNS;
            else
                sAreaFocus = AreaList_IsFishing() ? AREA_FOCUS_RODS : AREA_FOCUS_TABS;
            redraw = TRUE;
        }
        break;
    case AREA_FOCUS_TABS:
        if (JOY_REPT(DPAD_LEFT | DPAD_RIGHT) && sAreaNumTabs > 1)
        {
            if (JOY_REPT(DPAD_LEFT))
                sAreaTab = (sAreaTab == 0) ? sAreaNumTabs - 1 : sAreaTab - 1;
            else
                sAreaTab = (sAreaTab + 1) % sAreaNumTabs;
            sAreaRod = 0;
            sAreaCursor = 0;
            reload = TRUE;
        }
        else if (JOY_NEW(A_BUTTON) || JOY_REPT(DPAD_DOWN))
        {
            sAreaFocus = AreaList_IsFishing() ? AREA_FOCUS_RODS : AREA_FOCUS_GRID;
            redraw = TRUE;
        }
        break;
    case AREA_FOCUS_RODS:
        if (JOY_REPT(DPAD_LEFT | DPAD_RIGHT))
        {
            if (JOY_REPT(DPAD_LEFT))
                sAreaRod = (sAreaRod == 0) ? 2 : sAreaRod - 1;
            else
                sAreaRod = (sAreaRod + 1) % 3;
            sAreaCursor = 0;
            reload = TRUE;
        }
        else if (JOY_REPT(DPAD_UP))
        {
            sAreaFocus = AREA_FOCUS_TABS;
            redraw = TRUE;
        }
        else if (JOY_NEW(A_BUTTON) || JOY_REPT(DPAD_DOWN))
        {
            sAreaFocus = AREA_FOCUS_GRID;
            redraw = TRUE;
        }
        break;
    }
    if (reload)
        AreaList_LoadList();
    if (reload || redraw)
    {
        PlaySE(SE_SELECT);
        AreaList_Draw();
    }
    return FALSE;
}

static void Task_DexScreen_AreaList(u8 taskId)
{
    switch (sPokedexScreenData->state)
    {
    case 0:
        AreaList_Open();
        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(0);
        sPokedexScreenData->state = 1;
        break;
    case 1:
        ShowBg(3);
        ShowBg(2);
        ShowBg(0);
        HideBg(1);
        BeginNormalPaletteFade(~0x8000, 0, 16, 0, RGB_WHITEALPHA);
        sPokedexScreenData->state = 2;
        break;
    case 2:
        if (JOY_NEW(B_BUTTON))
        {
            BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
            sPokedexScreenData->state = 3;
        }
        else if (AreaList_HandleInput())
        {
            sPokedexScreenData->dexSpecies = sAreaMons[sAreaCursor].species;
            BeginNormalPaletteFade(~0x8000, 0, 0, 16, RGB_WHITEALPHA);
            sPokedexScreenData->state = 4;
        }
        break;
    case 3: // volta para o menu da Pokédex
        AreaList_Close();
        sAreaFocus = AREA_FOCUS_GRID;
        sAreaCursor = 0;
        sAreaTab = 0;
        sAreaRod = 0;
        gTasks[taskId].func = Task_PokedexScreen;
        sPokedexScreenData->state = 0;
        break;
    case 4: // abre a página do Pokémon; ao sair dela, volta para cá
        AreaList_Close();
        sDexFromAreaList = TRUE;
        gTasks[taskId].func = Task_DexScreen_ShowMonPage;
        sPokedexScreenData->state = 0;
        break;
    }
}
#endif // DEX_AREA_LIST_ENABLED
