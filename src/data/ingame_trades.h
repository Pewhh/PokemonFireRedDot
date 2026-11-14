// Both versions' in-game trades are in the ROM.
// sInGameTrades holds the FireRed trades (and the ones shared by both versions).
// sInGameTrades_LeafGreen only lists the trades that are different in LeafGreen;
// GetInGameTrade() in trade_scene.c picks the right one for the active version.
static const struct InGameTrade sInGameTrades[] = {
    [INGAME_TRADE_MR_MIME] = 
    {
        .nickname = _("MIMIEN"),
        .species = SPECIES_MR_MIME,
        .ivs = {20, 15, 17, 24, 23, 22},
        .abilityNum = 0,
        .otId = 1985,
        .conditions = {5, 5, 5, 30, 5},
        .personality = 0x00009cae,
        .heldItem = ITEM_NONE,
        .mailNum = 255,
        .otName = _("REYLEY"),
        .otGender = MALE,
        .sheen = 10,
        .requestedSpecies = SPECIES_ABRA
    }, 
    [INGAME_TRADE_SLOWPOKEPSYDUCK] = 
    {
        .nickname = _("POWLKE"),
        .species = SPECIES_SLOWPOKE,
        .ivs = {21, 25, 22, 18, 18, 17},
        .abilityNum = 0,
        .otId = 36728,
        .conditions = {5, 5, 30, 5, 5},
        .personality = 0x498a2e14,
        .heldItem = ITEM_FAB_MAIL,
        .mailNum = 0,
        .otName = _("DONTAE"),
        .otGender = MALE,
        .sheen = 10,
        .requestedSpecies = SPECIES_POLIWHIRL
    }, 
    [INGAME_TRADE_NIDORAN] = 
    {
        .nickname = _("MS. NIDO"),
        .species = SPECIES_NIDORAN_F,
        .ivs = {22, 18, 25, 19, 15, 22},
        .abilityNum = 0,
        .otId = 63184,
        .conditions = {5, 5, 5, 5, 30},
        .personality = 0x4c970b89,
        .heldItem = ITEM_TINY_MUSHROOM,
        .mailNum = 255,
        .otName = _("SAIGE"),
        .otGender = FEMALE,
        .sheen = 10,
        .requestedSpecies = SPECIES_NIDORAN_M
    }, 
    [INGAME_TRADE_FARFETCHD] = 
    {
        .nickname = _("CH'DING"),
        .species = SPECIES_FARFETCHD,
        .ivs = {20, 25, 21, 24, 15, 20},
        .abilityNum = 0,
        .otId = 8810,
        .conditions = {30, 5, 5, 5, 5},
        .personality = 0x151943d7,
        .heldItem = ITEM_STICK,
        .mailNum = 255,
        .otName = _("ELYSSA"),
        .otGender = MALE,
        .sheen = 10,
        .requestedSpecies = SPECIES_SPEAROW
    }, 
    [INGAME_TRADE_NIDORINOA] = 
    {
        .nickname = _("NINA"),
        .species = SPECIES_NIDORINA,
        .ivs = {22, 25, 18, 19, 22, 15},
        .abilityNum = 0,
        .otId = 13637,
        .conditions = {5, 5, 30, 5, 5},
        .personality = 0x00eeca15,
        .heldItem = ITEM_NONE,
        .mailNum = 255,
        .otName = _("TURNER"),
        .otGender = MALE,
        .sheen = 10,
        .requestedSpecies = SPECIES_NIDORINO
    }, 
    [INGAME_TRADE_LICKITUNG] = 
    {
        .nickname = _("MARC"),
        .species = SPECIES_LICKITUNG,
        .ivs = {24, 19, 21, 15, 23, 21},
        .abilityNum = 0,
        .otId = 1239,
        .conditions = {5, 5, 5, 5, 30},
        .personality = 0x451308ab,
        .heldItem = ITEM_NONE,
        .mailNum = 255,
        .otName = _("HADEN"),
        .otGender = MALE,
        .sheen = 10,
        .requestedSpecies = SPECIES_GOLDUCK
    }, 
    [INGAME_TRADE_ELECTRODE] = 
    {
        .nickname = _("ESPHERE"),
        .species = SPECIES_ELECTRODE,
        .ivs = {19, 16, 18, 25, 25, 19},
        .abilityNum = 1,
        .otId = 50298,
        .conditions = {30, 5, 5, 5, 5},
        .personality = 0x06341016,
        .heldItem = ITEM_NONE,
        .mailNum = 255,
        .otName = _("CLIFTON"),
        .otGender = MALE,
        .sheen = 10,
        .requestedSpecies = SPECIES_RAICHU
    }, 
    [INGAME_TRADE_TANGELA] = 
    {
        .nickname = _("TANGENY"),
        .species = SPECIES_TANGELA,
        .ivs = {22, 17, 25, 16, 23, 20},
        .abilityNum = 0,
        .otId = 60042,
        .conditions = {5, 5, 30, 5, 5},
        .personality = 0x5c77ecfa,
        .heldItem = ITEM_STARDUST,
        .mailNum = 255,
        .otName = _("NORMA"),
        .otGender = FEMALE,
        .sheen = 10,
        .requestedSpecies = SPECIES_VENONAT
    },
    [INGAME_TRADE_SEEL] = 
    {
        .nickname = _("SEELOR"),
        .species = SPECIES_SEEL,
        .ivs = {24, 15, 22, 16, 23, 22},
        .abilityNum = 0,
        .otId = 9853,
        .conditions = {5, 5, 5, 5, 30},
        .personality = 0x482cac89,
        .heldItem = ITEM_NONE,
        .mailNum = 255,
        .otName = _("GARETT"),
        .otGender = MALE,
        .sheen = 10,
        .requestedSpecies = SPECIES_PONYTA
    },
    // Route 12 fishing house. PLACEHOLDER: change species, nickname and OT when decided.
    // The same trade is used in both versions (it is not in sInGameTrades_LeafGreen).
    [INGAME_TRADE_ROUTE12] =
    {
        .nickname = _("PIDGEY"),
        .species = SPECIES_PIDGEY,
        .ivs = {15, 15, 15, 15, 15, 15},
        .abilityNum = 0,
        .otId = 12012,
        .conditions = {5, 5, 5, 5, 5},
        .personality = 0x00001212,
        .heldItem = ITEM_NONE,
        .mailNum = 255,
        .otName = _("TRADER"),
        .otGender = MALE,
        .sheen = 10,
        .requestedSpecies = SPECIES_RATTATA
    }
};

// Entries left out here have .species == SPECIES_NONE, meaning "same as FireRed".
static const struct InGameTrade sInGameTrades_LeafGreen[] = {
    [INGAME_TRADE_SLOWPOKEPSYDUCK] = 
    {
        .nickname = _("DUCKY"),
        .species = SPECIES_PSYDUCK,
        .ivs = {18, 21, 17, 25, 18, 22},
        .abilityNum = 0,
        .otId = 36728,
        .conditions = {5, 5, 30, 5, 5},
        .personality = 0x498a2e1d,
        .heldItem = ITEM_FAB_MAIL,
        .mailNum = 1, // PSYDUCK mail, see sInGameTradeMailMessages
        .otName = _("DONTAE"),
        .otGender = MALE,
        .sheen = 10,
        .requestedSpecies = SPECIES_POLIWHIRL
    },
    [INGAME_TRADE_NIDORAN] = 
    {
        .nickname = _("MR. NIDO"),
        .species = SPECIES_NIDORAN_M,
        .ivs = {19, 25, 18, 22, 22, 15},
        .abilityNum = 0,
        .otId = 63184,
        .conditions = {30, 5, 5, 5, 5},
        .personality = 0x4c970b9e,
        .heldItem = ITEM_TINY_MUSHROOM,
        .mailNum = 255,
        .otName = _("SAIGE"),
        .otGender = FEMALE,
        .sheen = 10,
        .requestedSpecies = SPECIES_NIDORAN_F
    },
    [INGAME_TRADE_NIDORINOA] = 
    {
        .nickname = _("NINO"),
        .species = SPECIES_NIDORINO,
        .ivs = {19, 18, 25, 22, 15, 22},
        .abilityNum = 0,
        .otId = 13637,
        .conditions = {5, 5, 5, 5, 30},
        .personality = 0x00eeca19,
        .heldItem = ITEM_NONE,
        .mailNum = 255,
        .otName = _("TURNER"),
        .otGender = MALE,
        .sheen = 10,
        .requestedSpecies = SPECIES_NIDORINA
    },
    [INGAME_TRADE_LICKITUNG] = 
    {
        .nickname = _("MARC"),
        .species = SPECIES_LICKITUNG,
        .ivs = {24, 19, 21, 15, 23, 21},
        .abilityNum = 0,
        .otId = 1239,
        .conditions = {5, 5, 5, 5, 30},
        .personality = 0x451308ab,
        .heldItem = ITEM_NONE,
        .mailNum = 255,
        .otName = _("HADEN"),
        .otGender = MALE,
        .sheen = 10,
        .requestedSpecies = SPECIES_SLOWBRO
    },
};

// Mail held by the Cerulean trade Pokémon ("THAT'S A HEALTHY ___! BE KIND TO IT").
// Index = .mailNum of the trade.
static const u16 sInGameTradeMailMessages[][10] = {
    // 0: FireRed, SLOWPOKE
    {
        EC_WORD_THAT_S,
        EC_WORD_A,
        EC_WORD_HEALTHY,
        EC_POKEMON(SLOWPOKE),
        EC_WORD_EXCL,
        EC_WORD_BE,
        EC_WORD_KIND,
        EC_WORD_TO,
        EC_WORD_IT
    },
    // 1: LeafGreen, PSYDUCK
    {
        EC_WORD_THAT_S,
        EC_WORD_A,
        EC_WORD_HEALTHY,
        EC_POKEMON(PSYDUCK),
        EC_WORD_EXCL,
        EC_WORD_BE,
        EC_WORD_KIND,
        EC_WORD_TO,
        EC_WORD_IT
    }
};
