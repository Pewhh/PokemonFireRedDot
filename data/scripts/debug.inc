@ HACK: menu de debug do Fire & Leaf Reminded. Só entra na versão de teste
@ (make debug / DEBUG=1); event_scripts.s só inclui este arquivo com DEBUG_MENU.

Debug_EventScript_Main::
	lockall
Debug_EventScript_MainLoop::
	setvar VAR_0x8004, DEBUG_LIST_MAIN
	special Debug_ShowList
	waitstate
	switch VAR_RESULT
	case 0, Debug_EventScript_Warp
	case 1, Debug_EventScript_Progress
	case 2, Debug_EventScript_Pokemon
	case 3, Debug_EventScript_Items
	case 4, Debug_EventScript_Battle
	case 5, Debug_EventScript_NoClip
	case 6, Debug_EventScript_Encounters
	case 7, Debug_EventScript_Sight
	goto Debug_EventScript_Close

Debug_EventScript_Close::
	releaseall
	end

@ ---------------------------------------------------------------------------
@ Teleporte
@ ---------------------------------------------------------------------------
Debug_EventScript_Warp::
	setvar VAR_0x8004, DEBUG_LIST_WARP
	special Debug_ShowList
	waitstate
	switch VAR_RESULT
	case 0, Debug_EventScript_WarpPallet
	case 1, Debug_EventScript_WarpViridian
	case 2, Debug_EventScript_WarpPewter
	case 3, Debug_EventScript_WarpRoute4
	case 4, Debug_EventScript_WarpCerulean
	case 5, Debug_EventScript_WarpVermilion
	case 6, Debug_EventScript_WarpRoute10
	case 7, Debug_EventScript_WarpLavender
	case 8, Debug_EventScript_WarpCeladon
	case 9, Debug_EventScript_WarpFuchsia
	case 10, Debug_EventScript_WarpSaffron
	case 11, Debug_EventScript_WarpCinnabar
	case 12, Debug_EventScript_WarpIndigo
	case 13, Debug_EventScript_WarpOneIsland
	case 14, Debug_EventScript_WarpGymPewter
	case 15, Debug_EventScript_WarpGymCerulean
	case 16, Debug_EventScript_WarpGymVermilion
	case 17, Debug_EventScript_WarpGymCeladon
	case 18, Debug_EventScript_WarpGymFuchsia
	case 19, Debug_EventScript_WarpGymSaffron
	case 20, Debug_EventScript_WarpGymCinnabar
	case 21, Debug_EventScript_WarpGymViridian
	case 22, Debug_EventScript_WarpSevenIsland
	case 23, Debug_EventScript_WarpTrainerTower
	goto Debug_EventScript_MainLoop

Debug_EventScript_WarpPallet::
	warp MAP_PALLET_TOWN_PLAYERS_HOUSE_1F, 0
	waitstate
	end
Debug_EventScript_WarpViridian::
	warp MAP_VIRIDIAN_CITY_POKEMON_CENTER_1F, 0
	waitstate
	end
Debug_EventScript_WarpPewter::
	warp MAP_PEWTER_CITY_POKEMON_CENTER_1F, 0
	waitstate
	end
Debug_EventScript_WarpRoute4::
	warp MAP_ROUTE4_POKEMON_CENTER_1F, 0
	waitstate
	end
Debug_EventScript_WarpCerulean::
	warp MAP_CERULEAN_CITY_POKEMON_CENTER_1F, 0
	waitstate
	end
Debug_EventScript_WarpVermilion::
	warp MAP_VERMILION_CITY_POKEMON_CENTER_1F, 0
	waitstate
	end
Debug_EventScript_WarpRoute10::
	warp MAP_ROUTE10_POKEMON_CENTER_1F, 0
	waitstate
	end
Debug_EventScript_WarpLavender::
	warp MAP_LAVENDER_TOWN_POKEMON_CENTER_1F, 0
	waitstate
	end
Debug_EventScript_WarpCeladon::
	warp MAP_CELADON_CITY_POKEMON_CENTER_1F, 0
	waitstate
	end
Debug_EventScript_WarpFuchsia::
	warp MAP_FUCHSIA_CITY_POKEMON_CENTER_1F, 0
	waitstate
	end
Debug_EventScript_WarpSaffron::
	warp MAP_SAFFRON_CITY_POKEMON_CENTER_1F, 0
	waitstate
	end
Debug_EventScript_WarpCinnabar::
	warp MAP_CINNABAR_ISLAND_POKEMON_CENTER_1F, 0
	waitstate
	end
Debug_EventScript_WarpIndigo::
	warp MAP_INDIGO_PLATEAU_POKEMON_CENTER_1F, 0
	waitstate
	end
Debug_EventScript_WarpOneIsland::
	warp MAP_ONE_ISLAND_POKEMON_CENTER_1F, 0
	waitstate
	end
Debug_EventScript_WarpGymPewter::
	warp MAP_PEWTER_CITY_GYM, 0
	waitstate
	end
Debug_EventScript_WarpGymCerulean::
	warp MAP_CERULEAN_CITY_GYM, 0
	waitstate
	end
Debug_EventScript_WarpGymVermilion::
	warp MAP_VERMILION_CITY_GYM, 0
	waitstate
	end
Debug_EventScript_WarpGymCeladon::
	warp MAP_CELADON_CITY_GYM, 0
	waitstate
	end
Debug_EventScript_WarpGymFuchsia::
	warp MAP_FUCHSIA_CITY_GYM, 0
	waitstate
	end
Debug_EventScript_WarpGymSaffron::
	warp MAP_SAFFRON_CITY_GYM, 0
	waitstate
	end
Debug_EventScript_WarpGymCinnabar::
	warp MAP_CINNABAR_ISLAND_GYM, 0
	waitstate
	end
Debug_EventScript_WarpGymViridian::
	warp MAP_VIRIDIAN_CITY_GYM, 0
	waitstate
	end
Debug_EventScript_WarpSevenIsland::
	warp MAP_SEVEN_ISLAND_POKEMON_CENTER_1F, 0
	waitstate
	end
Debug_EventScript_WarpTrainerTower::
	warp MAP_TRAINER_TOWER_LOBBY, 1 @ porta de entrada
	waitstate
	end

@ ---------------------------------------------------------------------------
@ Progresso
@ ---------------------------------------------------------------------------
Debug_EventScript_Progress::
	setvar VAR_0x8004, DEBUG_LIST_PROGRESS
	special Debug_ShowList
	waitstate
	switch VAR_RESULT
	case 0, Debug_EventScript_Badges
	case 1, Debug_EventScript_Difficulty
	case 2, Debug_EventScript_Events
	goto Debug_EventScript_MainLoop

Debug_EventScript_Badges::
	setvar VAR_0x8004, DEBUG_NUM_BADGES
	special Debug_InputNumber
	waitstate
	goto_if_eq VAR_RESULT, DEBUG_NUM_CANCEL, Debug_EventScript_Progress
	special Debug_SetBadges
	msgbox Debug_Text_Done
	goto Debug_EventScript_Progress

Debug_EventScript_Difficulty::
	setvar VAR_0x8004, DEBUG_LIST_DIFFICULTY
	special Debug_ShowList
	waitstate
	goto_if_eq VAR_RESULT, DEBUG_LIST_CANCEL, Debug_EventScript_Progress
	special Debug_SetDifficulty
	msgbox Debug_Text_Done
	goto Debug_EventScript_Progress

Debug_EventScript_Events::
	setvar VAR_0x8004, DEBUG_LIST_EVENTS
	special Debug_ShowList
	waitstate
	goto_if_eq VAR_RESULT, DEBUG_LIST_CANCEL, Debug_EventScript_Progress
	special Debug_ToggleEvent
	msgbox Debug_Text_State
	goto Debug_EventScript_Events

@ ---------------------------------------------------------------------------
@ Pokémon
@ ---------------------------------------------------------------------------
Debug_EventScript_Pokemon::
	setvar VAR_0x8004, DEBUG_LIST_POKEMON
	special Debug_ShowList
	waitstate
	switch VAR_RESULT
	case 0, Debug_EventScript_GiveMon
	case 1, Debug_EventScript_Heal
	case 2, Debug_EventScript_Hurt
	case 3, Debug_EventScript_Dead
	goto Debug_EventScript_MainLoop

Debug_EventScript_GiveMon::
	setvar VAR_0x8004, DEBUG_NUM_SPECIES
	special Debug_InputNumber
	waitstate
	goto_if_eq VAR_RESULT, DEBUG_NUM_CANCEL, Debug_EventScript_Pokemon
	copyvar VAR_0x8008, VAR_RESULT
	setvar VAR_0x8004, DEBUG_NUM_LEVEL
	special Debug_InputNumber
	waitstate
	goto_if_eq VAR_RESULT, DEBUG_NUM_CANCEL, Debug_EventScript_Pokemon
	copyvar VAR_0x8009, VAR_RESULT
	special Debug_GiveMon
	goto_if_eq VAR_RESULT, 2, Debug_EventScript_NoRoom
	msgbox Debug_Text_Done
	goto Debug_EventScript_Pokemon

Debug_EventScript_Heal::
	special HealPlayerParty
	msgbox Debug_Text_Done
	goto Debug_EventScript_Pokemon

Debug_EventScript_Hurt::
	special Debug_HurtParty
	msgbox Debug_Text_Done
	goto Debug_EventScript_Pokemon

Debug_EventScript_Dead::
	special Debug_ToggleDeadFirst
	msgbox Debug_Text_State
	goto Debug_EventScript_Pokemon

@ ---------------------------------------------------------------------------
@ Itens e dinheiro
@ ---------------------------------------------------------------------------
Debug_EventScript_Items::
	setvar VAR_0x8004, DEBUG_LIST_ITEMS
	special Debug_ShowList
	waitstate
	switch VAR_RESULT
	case 0, Debug_EventScript_GiveItem
	case 1, Debug_EventScript_Money
	goto Debug_EventScript_MainLoop

Debug_EventScript_GiveItem::
	setvar VAR_0x8004, DEBUG_NUM_ITEM
	special Debug_InputNumber
	waitstate
	goto_if_eq VAR_RESULT, DEBUG_NUM_CANCEL, Debug_EventScript_Items
	copyvar VAR_0x8008, VAR_RESULT
	setvar VAR_0x8004, DEBUG_NUM_QUANTITY
	special Debug_InputNumber
	waitstate
	goto_if_eq VAR_RESULT, DEBUG_NUM_CANCEL, Debug_EventScript_Items
	copyvar VAR_0x8009, VAR_RESULT
	special Debug_GiveItem
	goto_if_eq VAR_RESULT, FALSE, Debug_EventScript_NoRoom
	msgbox Debug_Text_Done
	goto Debug_EventScript_Items

Debug_EventScript_Money::
	special Debug_AddMoney
	msgbox Debug_Text_Done
	goto Debug_EventScript_Items

@ ---------------------------------------------------------------------------
@ Batalha
@ ---------------------------------------------------------------------------
Debug_EventScript_Battle::
	setvar VAR_0x8004, DEBUG_LIST_BATTLE
	special Debug_ShowList
	waitstate
	switch VAR_RESULT
	case 0, Debug_EventScript_TrainerBattle
	case 1, Debug_EventScript_WildBattle
	goto Debug_EventScript_MainLoop

Debug_EventScript_TrainerBattle::
	setvar VAR_0x8004, DEBUG_NUM_TRAINER
	special Debug_InputNumber
	waitstate
	goto_if_eq VAR_RESULT, DEBUG_NUM_CANCEL, Debug_EventScript_Battle
	copyvar VAR_0x8008, VAR_RESULT
	special Debug_PrepareTrainerBattle
	dotrainerbattle
	gotopostbattlescript

Debug_EventScript_AfterTrainerBattle::
	releaseall
	end

Debug_EventScript_WildBattle::
	setvar VAR_0x8004, DEBUG_NUM_SPECIES
	special Debug_InputNumber
	waitstate
	goto_if_eq VAR_RESULT, DEBUG_NUM_CANCEL, Debug_EventScript_Battle
	copyvar VAR_0x8008, VAR_RESULT
	setvar VAR_0x8004, DEBUG_NUM_LEVEL
	special Debug_InputNumber
	waitstate
	goto_if_eq VAR_RESULT, DEBUG_NUM_CANCEL, Debug_EventScript_Battle
	copyvar VAR_0x8009, VAR_RESULT
	special Debug_PrepareWildBattle
	dowildbattle
	releaseall
	end

@ ---------------------------------------------------------------------------
@ Interruptores
@ ---------------------------------------------------------------------------
Debug_EventScript_NoClip::
	special Debug_ToggleNoClip
	msgbox Debug_Text_State
	goto Debug_EventScript_MainLoop

Debug_EventScript_Encounters::
	special Debug_ToggleEncounters
	msgbox Debug_Text_State
	goto Debug_EventScript_MainLoop

Debug_EventScript_Sight::
	special Debug_ToggleTrainerSight
	msgbox Debug_Text_State
	goto Debug_EventScript_MainLoop

Debug_EventScript_NoRoom::
	msgbox Debug_Text_NoRoom
	goto Debug_EventScript_MainLoop

Debug_Text_Done::
	.string "Done.$"

Debug_Text_State::
	.string "{STR_VAR_1}.$"

Debug_Text_NoRoom::
	.string "No room.$"

Debug_Text_TrainerDefeat::
	.string "Debug battle over.$"
