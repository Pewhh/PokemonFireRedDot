PewterCity_Museum_1F_MapScripts::
	.byte 0

PewterCity_Museum_1F_EventScript_Scientist1::
	lock
	faceplayer
	goto_if_eq VAR_FACING, DIR_WEST, PewterCity_Museum_1F_EventScript_Scientist1BehindCounter
	goto_if_eq VAR_FACING, DIR_SOUTH, PewterCity_Museum_1F_EventScript_Scientist1BehindCounter
	goto_if_eq VAR_FACING, DIR_NORTH, PewterCity_Museum_1F_EventScript_Scientist1BehindCounter
	msgbox PewterCity_Museum_1F_Text_PleaseEnjoyYourself
	release
	end

PewterCity_Museum_1F_EventScript_Scientist1BehindCounter::
	msgbox PewterCity_Museum_1F_Text_DoYouKnowWhatAmberIs, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, PewterCity_Museum_1F_EventScript_AmberHasGeneticMatter
	goto_if_eq VAR_RESULT, NO, PewterCity_Museum_1F_EventScript_ExplainAmber
	end

PewterCity_Museum_1F_EventScript_AmberHasGeneticMatter::
	msgbox PewterCity_Museum_1F_Text_AmberContainsGeneticMatter
	applymovement LOCALID_MUSEUM_SCIENTIST1, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

PewterCity_Museum_1F_EventScript_ExplainAmber::
	msgbox PewterCity_Museum_1F_Text_AmberIsFossilizedSap
	applymovement LOCALID_MUSEUM_SCIENTIST1, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

PewterCity_Museum_1F_EventScript_EntranceTriggerLeft::
	lockall
	setvar VAR_TEMP_1, 0
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	goto PewterCity_Museum_1F_EventScript_EntranceTrigger
	end

PewterCity_Museum_1F_EventScript_EntranceTriggerMid::
	lockall
	setvar VAR_TEMP_1, 1
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	goto PewterCity_Museum_1F_EventScript_EntranceTrigger
	end

PewterCity_Museum_1F_EventScript_EntranceTriggerRight::
	lockall
	setvar VAR_TEMP_1, 2
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	goto PewterCity_Museum_1F_EventScript_EntranceTrigger
	end

PewterCity_Museum_1F_EventScript_EntranceTrigger::
	textcolor NPC_TEXT_COLOR_MALE
	showmoneybox 0, 0
	msgbox PewterCity_Museum_1F_Text_Its50YForChildsTicket, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, PewterCity_Museum_1F_EventScript_TryPayForTicket
	msgbox PewterCity_Museum_1F_Text_ComeAgain
	closemessage
	hidemoneybox
	applymovement LOCALID_PLAYER, PewterCity_Museum_1F_Movement_ForcePlayerExit
	waitmovement 0
	releaseall
	end

PewterCity_Museum_1F_EventScript_TryPayForTicket::
	checkmoney 50
	goto_if_eq VAR_RESULT, FALSE, PewterCity_Museum_1F_EventScript_NotEnoughMoney
	closemessage
	call_if_eq VAR_TEMP_1, 0, PewterCity_Museum_1F_EventScript_PlayerApproachCounterLeft
	call_if_eq VAR_TEMP_1, 1, PewterCity_Museum_1F_EventScript_PlayerApproachCounterMid
	call_if_eq VAR_TEMP_1, 2, PewterCity_Museum_1F_EventScript_PlayerApproachCounterRight
	playse SE_SHOP
	removemoney 50
	updatemoneybox
	waitse
	msgbox PewterCity_Museum_1F_Text_Right50YThankYou
	setvar VAR_MAP_SCENE_PEWTER_CITY_MUSEUM_1F, 1
	hidemoneybox
	releaseall
	end

PewterCity_Museum_1F_EventScript_PlayerApproachCounterLeft::
	applymovement LOCALID_PLAYER, PewterCity_Museum_1F_Movement_ApproachCounterLeft
	waitmovement 0
	return

PewterCity_Museum_1F_EventScript_PlayerApproachCounterMid::
	applymovement LOCALID_PLAYER, PewterCity_Museum_1F_Movement_ApproachCounterMid
	waitmovement 0
	return

PewterCity_Museum_1F_EventScript_PlayerApproachCounterRight::
	applymovement LOCALID_PLAYER, PewterCity_Museum_1F_Movement_ApproachCounterRight
	waitmovement 0
	return

PewterCity_Museum_1F_EventScript_NotEnoughMoney::
	msgbox PewterCity_Museum_1F_Text_DontHaveEnoughMoney
	closemessage
	hidemoneybox
	applymovement LOCALID_PLAYER, PewterCity_Museum_1F_Movement_ForcePlayerExit
	waitmovement 0
	releaseall
	end

PewterCity_Museum_1F_Movement_ForcePlayerExit::
	walk_down
	step_end

PewterCity_Museum_1F_Movement_ApproachCounterLeft::
	walk_right
	walk_right
	delay_8
	walk_in_place_right
	step_end

PewterCity_Museum_1F_Movement_ApproachCounterMid::
	walk_right
	delay_8
	walk_in_place_right
	step_end

PewterCity_Museum_1F_Movement_ApproachCounterRight::
	delay_8
	walk_in_place_right
	step_end

@ Revives the Mt. Moon fossil (HELIX or DOME) at level 25, same flow as the Cinnabar lab
@ (level 40): show the fossil, hand it over, leave the museum, come back for the Pokemon.
@ The fossil leaves the bag when handed over, so it can only be revived in one place.
@ The OLD AMBER is Cinnabar only.
PewterCity_Museum_1F_EventScript_Scientist2::
	lock
	faceplayer
	goto_if_eq VAR_PEWTER_MUSEUM_REVIVE_STATE, 2, PewterCity_Museum_1F_EventScript_GiveRevivedMon
	goto_if_eq VAR_PEWTER_MUSEUM_REVIVE_STATE, 1, PewterCity_Museum_1F_EventScript_FossilStillReviving
	checkitem ITEM_HELIX_FOSSIL
	goto_if_eq VAR_RESULT, TRUE, PewterCity_Museum_1F_EventScript_ChooseFossilHelix
	checkitem ITEM_DOME_FOSSIL
	goto_if_eq VAR_RESULT, TRUE, PewterCity_Museum_1F_EventScript_ChooseFossilDome
	@ Mt. Moon fossil already revived (here or in Cinnabar)
	goto_if_set FLAG_REVIVED_HELIX, PewterCity_Museum_1F_EventScript_AlreadyRevived
	goto_if_set FLAG_REVIVED_DOME, PewterCity_Museum_1F_EventScript_AlreadyRevived
	checkitem ITEM_OLD_AMBER
	goto_if_eq VAR_RESULT, TRUE, PewterCity_Museum_1F_EventScript_AmberHereNo
	msgbox PewterCity_Museum_1F_Text_NoFossilMtMoonHere
	release
	end

PewterCity_Museum_1F_EventScript_ChooseFossilHelix::
	message PewterCity_Museum_1F_Text_DoYouHaveAFossilForUs
	waitmessage
	multichoice 0, 0, MULTICHOICE_HELIX, FALSE
	switch VAR_RESULT
	case 0, PewterCity_Museum_1F_EventScript_ShowHelixFossil
	case 1, PewterCity_Museum_1F_EventScript_Decline
	case 127, PewterCity_Museum_1F_EventScript_Decline
	end

PewterCity_Museum_1F_EventScript_ChooseFossilDome::
	message PewterCity_Museum_1F_Text_DoYouHaveAFossilForUs
	waitmessage
	multichoice 0, 0, MULTICHOICE_DOME, FALSE
	switch VAR_RESULT
	case 0, PewterCity_Museum_1F_EventScript_ShowDomeFossil
	case 1, PewterCity_Museum_1F_EventScript_Decline
	case 127, PewterCity_Museum_1F_EventScript_Decline
	end

PewterCity_Museum_1F_EventScript_ShowHelixFossil::
	bufferspeciesname STR_VAR_1, SPECIES_OMANYTE
	bufferitemname STR_VAR_2, ITEM_HELIX_FOSSIL
	msgbox PewterCity_Museum_1F_Text_ThatFossilIsOfMonMakeItLiveAgain, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PewterCity_Museum_1F_EventScript_Decline
	msgbox PewterCity_Museum_1F_Text_HandedFossilToScientist
	removeitem ITEM_HELIX_FOSSIL
	msgbox PewterCity_Museum_1F_Text_TakesTimeLookAroundTown
	setvar VAR_PEWTER_MUSEUM_REVIVE_STATE, 1
	setvar VAR_PEWTER_MUSEUM_WHICH_FOSSIL, ITEM_HELIX_FOSSIL
	release
	end

PewterCity_Museum_1F_EventScript_ShowDomeFossil::
	bufferspeciesname STR_VAR_1, SPECIES_KABUTO
	bufferitemname STR_VAR_2, ITEM_DOME_FOSSIL
	msgbox PewterCity_Museum_1F_Text_ThatFossilIsOfMonMakeItLiveAgain, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PewterCity_Museum_1F_EventScript_Decline
	msgbox PewterCity_Museum_1F_Text_HandedFossilToScientist
	removeitem ITEM_DOME_FOSSIL
	msgbox PewterCity_Museum_1F_Text_TakesTimeLookAroundTown
	setvar VAR_PEWTER_MUSEUM_REVIVE_STATE, 1
	setvar VAR_PEWTER_MUSEUM_WHICH_FOSSIL, ITEM_DOME_FOSSIL
	release
	end

PewterCity_Museum_1F_EventScript_FossilStillReviving::
	msgbox PewterCity_Museum_1F_Text_TakesTimeLookAroundTown
	release
	end

PewterCity_Museum_1F_EventScript_GiveRevivedMon::
	goto_if_eq VAR_PEWTER_MUSEUM_WHICH_FOSSIL, ITEM_HELIX_FOSSIL, PewterCity_Museum_1F_EventScript_GiveOmanyte
	goto_if_eq VAR_PEWTER_MUSEUM_WHICH_FOSSIL, ITEM_DOME_FOSSIL, PewterCity_Museum_1F_EventScript_GiveKabuto
	release
	end

@ No room in party or PC: the Pokemon keeps waiting here (state stays 2), like in Cinnabar
PewterCity_Museum_1F_EventScript_GiveOmanyte::
	setvar VAR_TEMP_1, SPECIES_OMANYTE
	bufferspeciesname STR_VAR_1, SPECIES_OMANYTE
	msgbox PewterCity_Museum_1F_Text_FossilMonBackToLife
	givemon SPECIES_OMANYTE, 25
	goto_if_eq VAR_RESULT, 2, EventScript_NoMoreRoomForPokemon
	setflag FLAG_REVIVED_HELIX @ Cinnabar uses this flag too
	setvar VAR_PEWTER_MUSEUM_REVIVE_STATE, 0
	goto_if_eq VAR_RESULT, 0, PewterCity_Museum_1F_EventScript_NicknameMonParty
	goto_if_eq VAR_RESULT, 1, PewterCity_Museum_1F_EventScript_NicknameMonPC
	release
	end

PewterCity_Museum_1F_EventScript_GiveKabuto::
	setvar VAR_TEMP_1, SPECIES_KABUTO
	bufferspeciesname STR_VAR_1, SPECIES_KABUTO
	msgbox PewterCity_Museum_1F_Text_FossilMonBackToLife
	givemon SPECIES_KABUTO, 25
	goto_if_eq VAR_RESULT, 2, EventScript_NoMoreRoomForPokemon
	setflag FLAG_REVIVED_DOME
	setvar VAR_PEWTER_MUSEUM_REVIVE_STATE, 0
	goto_if_eq VAR_RESULT, 0, PewterCity_Museum_1F_EventScript_NicknameMonParty
	goto_if_eq VAR_RESULT, 1, PewterCity_Museum_1F_EventScript_NicknameMonPC
	release
	end

PewterCity_Museum_1F_EventScript_NicknameMonParty::
	textcolor NPC_TEXT_COLOR_NEUTRAL
	playfanfare MUS_LEVEL_UP
	message PewterCity_Museum_1F_Text_ReceivedMonFromMuseum
	waitmessage
	waitfanfare
	msgbox Text_GiveNicknameToThisMon, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PewterCity_Museum_1F_EventScript_End
	call EventScript_GetGiftMonPartySlot
	call EventScript_ChangePokemonNickname
	goto PewterCity_Museum_1F_EventScript_End
	end

PewterCity_Museum_1F_EventScript_NicknameMonPC::
	textcolor NPC_TEXT_COLOR_NEUTRAL
	playfanfare MUS_LEVEL_UP
	message PewterCity_Museum_1F_Text_ReceivedMonFromMuseum
	waitmessage
	waitfanfare
	msgbox Text_GiveNicknameToThisMon, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PewterCity_Museum_1F_EventScript_SentPC
	call EventScript_NameReceivedBoxMon
	goto PewterCity_Museum_1F_EventScript_SentPC
	end

PewterCity_Museum_1F_EventScript_SentPC::
	call EventScript_TransferredToPC
	goto PewterCity_Museum_1F_EventScript_End
	end

PewterCity_Museum_1F_EventScript_Decline::
	msgbox PewterCity_Museum_1F_Text_ComeBackIfYouChangeYourMind
	release
	end

PewterCity_Museum_1F_EventScript_AmberHereNo::
	msgbox PewterCity_Museum_1F_Text_AmberIsCinnabar
	release
	end

PewterCity_Museum_1F_EventScript_AlreadyRevived::
	msgbox PewterCity_Museum_1F_Text_WeHaveTwoFossilsOnExhibit
	release
	end

PewterCity_Museum_1F_EventScript_End::
	release
	end

PewterCity_Museum_1F_EventScript_OldMan::
	msgbox PewterCity_Museum_1F_Text_ShouldBeGratefulForLongLife, MSGBOX_NPC
	end

PewterCity_Museum_1F_EventScript_OldAmberScientist::
	lock
	faceplayer
	goto_if_set FLAG_GOT_OLD_AMBER, PewterCity_Museum_1F_EventScript_AlreadyGotOldAmber
	msgbox PewterCity_Museum_1F_Text_WantYouToGetAmberExamined
	checkitemspace ITEM_OLD_AMBER
	goto_if_eq VAR_RESULT, FALSE, PewterCity_Museum_1F_EventScript_NoRoomForOldAmber
	setflag FLAG_GOT_OLD_AMBER
	removeobject LOCALID_OLD_AMBER
	giveitem_msg PewterCity_Museum_1F_Text_ReceivedOldAmberFromMan, ITEM_OLD_AMBER, 1, MUS_OBTAIN_KEY_ITEM
	release
	end

PewterCity_Museum_1F_EventScript_NoRoomForOldAmber::
	msgbox PewterCity_Museum_1F_Text_DontHaveSpaceForThis
	release
	end

PewterCity_Museum_1F_EventScript_AlreadyGotOldAmber::
	msgbox PewterCity_Museum_1F_Text_GetOldAmberChecked
	release
	end

PewterCity_Museum_1F_EventScript_OldAmber::
	msgbox PewterCity_Museum_1F_Text_BeautifulPieceOfAmber, MSGBOX_NPC
	end

PewterCity_Museum_1F_EventScript_AerodactylFossil::
	lockall
	setvar VAR_0x8004, SPECIES_AERODACTYL
	setvar VAR_0x8005, 10
	setvar VAR_0x8006, 3
	special OpenMuseumFossilPic
	msgbox PewterCity_Museum_1F_Text_AerodactylFossil
	special CloseMuseumFossilPic
	releaseall
	end

PewterCity_Museum_1F_EventScript_KabutopsFossil::
	lockall
	setvar VAR_0x8004, SPECIES_KABUTOPS
	setvar VAR_0x8005, 10
	setvar VAR_0x8006, 3
	special OpenMuseumFossilPic
	msgbox PewterCity_Museum_1F_Text_KabutopsFossil
	special CloseMuseumFossilPic
	releaseall
	end
