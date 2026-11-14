@ Fala do líder sobre o que a insígnia libera, pela POSIÇÃO em que ele foi vencido
@ (4º, 5º ou 6º). Vale para Erika, Koga e Sabrina, em qualquer ordem.
@ Chamar logo depois do setflag da insígnia: call EventScript_ExplainBadgeByPosition
EventScript_ExplainBadgeByPosition::
	specialvar VAR_RESULT, GetBadgeCountForScript
	goto_if_eq VAR_RESULT, 4, EventScript_ExplainBadgePosition4
	goto_if_eq VAR_RESULT, 5, EventScript_ExplainBadgePosition5
	goto_if_eq VAR_RESULT, 6, EventScript_ExplainBadgePosition6
	return

EventScript_ExplainBadgePosition4:
	msgbox Text_BadgePosition4
	return

EventScript_ExplainBadgePosition5:
	msgbox Text_BadgePosition5
	return

EventScript_ExplainBadgePosition6:
	msgbox Text_BadgePosition6
	return

Text_BadgePosition4::
	.string "With four BADGES, POKéMON up to\n"
	.string "Lv. 50 will obey you.\p"
	.string "You can also use STRENGTH\n"
	.string "outside of battle now.$"

Text_BadgePosition5::
	.string "With five BADGES, you can use\n"
	.string "ROCK SMASH outside of battle.$"

Text_BadgePosition6::
	.string "With six BADGES, POKéMON up to\n"
	.string "Lv. 70 will obey you.\p"
	.string "You can also SURF outside of\n"
	.string "battle now.$"
