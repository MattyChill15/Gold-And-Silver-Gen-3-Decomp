@ Arena series: gym leader rematches after the champion is beaten.
Arena_EventScript_RematchDeclined::
	msgbox Arena_Text_RematchLater
	release
	end

Arena_Text_RematchAsk::
	.string "You're the CHAMPION now.\n"
	.string "Want a rematch? My team got tougher.$"

Arena_Text_RematchWon::
	.string "What power! Come back anytime\n"
	.string "for another rematch.$"

Arena_Text_RematchLater::
	.string "Any time you want a rematch,\n"
	.string "come see me.$"

@ Matt: end-of-part handoff (Johto P1 Morty / P2 Lance / P3 Red). Set VAR_0x8005 = part first.
Arena_EventScript_PartEnd::
	special ArenaSeries_EndPart
	goto_if_eq VAR_RESULT, 0xFF, Arena_EventScript_PartEndSkip
	goto_if_eq VAR_0x8005, 1, Arena_EventScript_PartEnd1
	goto_if_eq VAR_0x8005, 2, Arena_EventScript_PartEnd2
	goto_if_eq VAR_0x8005, 3, Arena_EventScript_PartEnd3
Arena_EventScript_PartEndSkip::
	return

Arena_EventScript_PartEnd1::
	goto_if_eq VAR_RESULT, 1, Arena_EventScript_PartEnd1Chief
	goto_if_eq VAR_RESULT, 2, Arena_EventScript_PartEnd1Show
	msgbox Arena_Text_PartEnd1Classic
	closemessage
	return
Arena_EventScript_PartEnd1Chief::
	msgbox Arena_Text_PartEnd1Chief
	closemessage
	return
Arena_EventScript_PartEnd1Show::
	msgbox Arena_Text_PartEnd1Show
	closemessage
	return
Arena_Text_PartEnd1Classic:
	.string "PART 1 COMPLETE\p"
	.string "The Fog Badge shines.\nEcruteak's ghosts fall quiet.\p"
	.string "Part 2 picks up here:\nOlivine and the lighthouse wait.$"
Arena_Text_PartEnd1Chief:
	.string "PART 1 COMPLETE\p"
	.string "The Fog Badge shines.\nEcruteak's ghosts fall quiet.\p"
	.string "A grey-coat letter arrives:\n“Good work. Keep moving. -Chief”\pPart 2 picks up here:\nOlivine and the lighthouse wait.$"
Arena_Text_PartEnd1Show:
	.string "PART 1 COMPLETE\p"
	.string "The Fog Badge shines.\nEcruteak's ghosts fall quiet.\p"
	.string "That's a wrap on this episode!\nStay tuned for the next one...\pPart 2 picks up here:\nOlivine and the lighthouse wait.$"

Arena_EventScript_PartEnd2::
	goto_if_eq VAR_RESULT, 1, Arena_EventScript_PartEnd2Chief
	goto_if_eq VAR_RESULT, 2, Arena_EventScript_PartEnd2Show
	msgbox Arena_Text_PartEnd2Classic
	closemessage
	return
Arena_EventScript_PartEnd2Chief::
	msgbox Arena_Text_PartEnd2Chief
	closemessage
	return
Arena_EventScript_PartEnd2Show::
	msgbox Arena_Text_PartEnd2Show
	closemessage
	return
Arena_Text_PartEnd2Classic:
	.string "PART 2 COMPLETE\p"
	.string "Johto's champion is crowned!\p"
	.string "Part 3 picks up here:\nthe S.S. Aqua sails for Kanto.$"
Arena_Text_PartEnd2Chief:
	.string "PART 2 COMPLETE\p"
	.string "Johto's champion is crowned!\p"
	.string "A grey-coat letter arrives:\n“Good work. Keep moving. -Chief”\pPart 3 picks up here:\nthe S.S. Aqua sails for Kanto.$"
Arena_Text_PartEnd2Show:
	.string "PART 2 COMPLETE\p"
	.string "Johto's champion is crowned!\p"
	.string "That's a wrap on this episode!\nStay tuned for the next one...\pPart 3 picks up here:\nthe S.S. Aqua sails for Kanto.$"

Arena_EventScript_PartEnd3::
	goto_if_eq VAR_RESULT, 1, Arena_EventScript_PartEnd3Chief
	goto_if_eq VAR_RESULT, 2, Arena_EventScript_PartEnd3Show
	msgbox Arena_Text_PartEnd3Classic
	closemessage
	return
Arena_EventScript_PartEnd3Chief::
	msgbox Arena_Text_PartEnd3Chief
	closemessage
	return
Arena_EventScript_PartEnd3Show::
	msgbox Arena_Text_PartEnd3Show
	closemessage
	return
Arena_Text_PartEnd3Classic:
	.string "CHAPTER 2 COMPLETE\p"
	.string "Red has fallen on Mt. Silver.\nJohto's story is told.\p"
	.string "Next chapter: Hoenn awaits.\nCarry this save into Emerald.$"
Arena_Text_PartEnd3Chief:
	.string "CHAPTER 2 COMPLETE\p"
	.string "Red has fallen on Mt. Silver.\nJohto's story is told.\p"
	.string "A grey-coat letter arrives:\n“Good work. Keep moving. -Chief”\pNext chapter: Hoenn awaits.\nCarry this save into Emerald.$"
Arena_Text_PartEnd3Show:
	.string "CHAPTER 2 COMPLETE\p"
	.string "Red has fallen on Mt. Silver.\nJohto's story is told.\p"
	.string "That's a wrap on this episode!\nStay tuned for the next one...\pNext chapter: Hoenn awaits.\nCarry this save into Emerald.$"
