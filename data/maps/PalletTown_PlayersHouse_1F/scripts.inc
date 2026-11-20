PalletTown_PlayersHouse_1F_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, PalletTown_PlayersHouse_1F_OnFrame
	.byte 0

@ Roda sozinho assim que o mapa aparece, enquanto VAR_MAP_SCENE_PALLET_MOM for 0
PalletTown_PlayersHouse_1F_OnFrame::
	map_script_2 VAR_MAP_SCENE_PALLET_MOM, 0, PalletTown_PlayersHouse_1F_EventScript_MomGiveRunningShoes
	.2byte 0

@ O jogador chega pela escada em (10,2). A mãe chama, e o jogador desce até (9,4), ao lado dela.
@ (A mãe do FRLG não tem quadros de andar; se ela andasse, deslizaria.)
PalletTown_PlayersHouse_1F_EventScript_MomGiveRunningShoes::
	lockall
	textcolor NPC_TEXT_COLOR_FEMALE
	applymovement LOCALID_MOM, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	playse SE_PIN
	applymovement LOCALID_MOM, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement LOCALID_MOM, Common_Movement_Delay48
	waitmovement 0
	msgbox PalletTown_PlayersHouse_1F_Text_OhSon
	closemessage
	applymovement LOCALID_PLAYER, PalletTown_PlayersHouse_1F_Movement_PlayerApproachMom
	waitmovement 0
	applymovement LOCALID_MOM, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	msgbox PalletTown_PlayersHouse_1F_Text_DeliveringRunningShoes
	textcolor NPC_TEXT_COLOR_NEUTRAL
	playfanfare MUS_OBTAIN_KEY_ITEM
	message PalletTown_PlayersHouse_1F_ReceivedRunningShoesFromMom
	waitmessage
	waitfanfare
	msgbox PalletTown_PlayersHouse_1F_Text_SwitchedShoesWithRunningShoes
	special DisableMsgBoxWalkaway
	signmsg
	msgbox PalletTown_PlayersHouse_1F_Text_ExplainRunningShoes
	normalmsg
	textcolor NPC_TEXT_COLOR_FEMALE
	msgbox PalletTown_PlayersHouse_1F_Text_MomTakeCare
	closemessage
	applymovement LOCALID_MOM, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	setflag FLAG_SYS_B_DASH
	setvar VAR_MAP_SCENE_PALLET_MOM, 1
	releaseall
	end

PalletTown_PlayersHouse_1F_Movement_PlayerApproachMom::
	walk_down
	walk_down
	walk_left
	walk_in_place_faster_left
	step_end

PalletTown_PlayersHouse_1F_EventScript_Mom::
	lock
	faceplayer
	goto_if_set FLAG_BEAT_RIVAL_IN_OAKS_LAB, PalletTown_PlayersHouse_1F_EventScript_MomHeal
	checkplayergender
	call_if_eq VAR_RESULT, MALE, PalletTown_PlayersHouse_1F_EventScript_MomOakLookingForYouMale
	call_if_eq VAR_RESULT, FEMALE, PalletTown_PlayersHouse_1F_EventScript_MomOakLookingForYouFemale
	closemessage
	applymovement LOCALID_MOM, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

PalletTown_PlayersHouse_1F_EventScript_MomOakLookingForYouMale::
	msgbox PalletTown_PlayersHouse_1F_Text_AllBoysLeaveOakLookingForYou
	return

PalletTown_PlayersHouse_1F_EventScript_MomOakLookingForYouFemale::
	msgbox PalletTown_PlayersHouse_1F_Text_AllGirlsLeaveOakLookingForYou
	return

PalletTown_PlayersHouse_1F_EventScript_MomHeal::
	msgbox PalletTown_PlayersHouse_1F_Text_YouShouldTakeQuickRest
	closemessage
	call EventScript_OutOfCenterPartyHeal
	msgbox PalletTown_PlayersHouse_1F_Text_LookingGreatTakeCare
	release
	end

@ Displays special text if interacted with from side or back (which are normally inaccessible)
PalletTown_PlayersHouse_1F_EventScript_TV::
	lockall
	goto_if_eq VAR_FACING, DIR_NORTH, PalletTown_PlayersHouse_1F_EventScript_TVScreen
	msgbox PalletTown_PlayersHouse_1F_Text_OopsWrongSide
	releaseall
	end

PalletTown_PlayersHouse_1F_EventScript_TVScreen::
	checkplayergender
	call_if_eq VAR_RESULT, MALE, PalletTown_PlayersHouse_1F_EventScript_TVScreenMale
	call_if_eq VAR_RESULT, FEMALE, PalletTown_PlayersHouse_1F_EventScript_TVScreenFemale
	releaseall
	end

PalletTown_PlayersHouse_1F_EventScript_TVScreenMale::
	msgbox PalletTown_PlayersHouse_1F_Text_MovieOnTVFourBoysOnRailroad
	return

PalletTown_PlayersHouse_1F_EventScript_TVScreenFemale::
	msgbox PalletTown_PlayersHouse_1F_Text_MovieOnTVGirlOnBrickRoad
	return
