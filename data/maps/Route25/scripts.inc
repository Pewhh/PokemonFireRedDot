Route25_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route25_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, Route25_OnFrame
	.byte 0

@ HACK: o BILL só aparece fora da casa durante o convite para a batalha
@ (VAR_BILL_BATTLE = 4 é a volta da batalha no meio do convite: aí ele continua lá)
Route25_OnTransition::
	goto_if_eq VAR_BILL_BATTLE, 4, Route25_EventScript_KeepBill
	setflag FLAG_HIDE_ROUTE25_BILL
Route25_EventScript_KeepBill::
	end

Route25_OnFrame::
	map_script_2 VAR_BILL_BATTLE, 1, Route25_EventScript_BillChallenge
	map_script_2 VAR_BILL_BATTLE, 4, Route25_EventScript_BillCleanup
	.2byte 0

@ Só acontece se o jogo for desligado no meio da batalha do convite
Route25_EventScript_BillCleanup::
	lockall
	removeobject LOCALID_ROUTE25_BILL
	setflag FLAG_HIDE_ROUTE25_BILL
	setvar VAR_BILL_BATTLE, 2
	releaseall
	end

@ Ao sair da casa depois de receber o S.S. TICKET, o BILL sai atrás e chama para uma batalha
Route25_EventScript_BillChallenge::
	lockall
	textcolor NPC_TEXT_COLOR_MALE
	opendoor 51, 4
	waitdooranim
	clearflag FLAG_HIDE_ROUTE25_BILL
	addobject LOCALID_ROUTE25_BILL
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox Route25_Text_BillHowAboutABattle, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Route25_EventScript_BillDeclined
	msgbox Route25_Text_BillLetsGo
	closemessage
	setvar VAR_BILL_BATTLE, 4 @ mantém o BILL na porta quando o mapa recarrega depois da batalha
	trainerbattle_earlyrival TRAINER_BILL_1, RIVAL_BATTLE_HEAL_AFTER, Route25_Text_BillDefeat, Route25_Text_BillVictory
	goto_if_eq VAR_RESULT, FALSE, Route25_EventScript_BillPlayerWon @ FALSE = jogador venceu
	msgbox Route25_Text_BillBattleMeAnytime
	setvar VAR_BILL_BATTLE, 2
	goto Route25_EventScript_BillGoBackInside
	end

Route25_EventScript_BillPlayerWon::
	msgbox Route25_Text_BillTrainMore
	setvar VAR_BILL_BATTLE, 3
	goto Route25_EventScript_BillGoBackInside
	end

Route25_EventScript_BillDeclined::
	msgbox Route25_Text_BillDeclined
	setvar VAR_BILL_BATTLE, 2
	goto Route25_EventScript_BillGoBackInside
	end

Route25_EventScript_BillGoBackInside::
	closemessage
	applymovement LOCALID_ROUTE25_BILL, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	removeobject LOCALID_ROUTE25_BILL
	setflag FLAG_HIDE_ROUTE25_BILL
	closedoor 51, 4
	waitdooranim
	releaseall
	end

Route25_EventScript_SeaCottageSign::
	msgbox Route25_Text_SeaCottageSign, MSGBOX_SIGN
	end

Route25_EventScript_Man::
	msgbox Route25_Text_AreYouHereAlone, MSGBOX_NPC
	end
