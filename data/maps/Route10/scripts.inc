Route10_MapScripts::
	.byte 0

Route10_EventScript_Unused::
	end

Route10_EventScript_NorthRockTunnelSign::
	msgbox Route10_Text_RockTunnelDetourToLavender, MSGBOX_SIGN
	end

Route10_EventScript_SouthRockTunnelSign::
	msgbox Route10_Text_RockTunnel, MSGBOX_SIGN
	end

Route10_EventScript_PowerPlantSign::
	msgbox Route10_Text_PowerPlant, MSGBOX_SIGN
	end

@ Bloqueia a entrada norte do ROCK TUNNEL até o jogador vencer o LT. SURGE
@ (o NPC some com FLAG_DEFEATED_LT_SURGE). Garante Brock, Misty e Surge como os 3 primeiros.
Route10_EventScript_RockTunnelWorker::
	msgbox Route10_Text_RockTunnelWorker, MSGBOX_NPC
	end
