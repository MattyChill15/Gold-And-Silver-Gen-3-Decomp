@ M03 Spell of the Unown — P1 scaffold.
@ Locked lines are LINES_M03 B01–B14 in film order (timing audit PASS).
@ Off-meter nicknames for B03–B08 drift from those keys (Lisa was labeled B04,
@ crystal B03, center B05, abduction B06, approach B07, door B08). This file
@ keeps the locked order and puts each mechanic on the beat that speaks it:
@ Lisa battle B03, abduction clearflag B05, center B06, charicific door B08,
@ Brock/Misty fades B09/B10, Entei FORCE_LOSS + HealPlayerParty B11,
@ Charizard overworld stub B12–B13. Does not set FLAG_ARENA_SHOW_SJ7 or seriesBit(7).
@ Speech matches M01: msgbox "SPEAKER:\nbody". Narrator is that msgbox with no namebox.

.macro m03_warp dest, x, y
	releaseall
	warp \dest, \x, \y
	waitstate
	end
.endm

Arena_M03_StartFromSJ6A::
	setflag FLAG_M03_STARTED
	setflag FLAG_M03_HIDE_ENTEI
	setflag FLAG_M03_HIDE_CHARIZARD
	setvar VAR_M03_SCENE, 1
	releaseall
	warp MAP_M03_HALE_MANOR, 15, 18
	waitstate
	end

M03_B01_SpencerStudy::
	lockall
	msgbox M03_B01_T0, MSGBOX_DEFAULT
	msgbox M03_B01_T1, MSGBOX_DEFAULT
	msgbox M03_B01_T2, MSGBOX_DEFAULT
	msgbox M03_B01_T3, MSGBOX_DEFAULT
	setvar VAR_M03_SCENE, 2
	m03_warp MAP_M03_UNOWN_CHAMBER, 24, 7

M03_B02_UnownBox::
	lockall
	msgbox M03_B02_T0, MSGBOX_DEFAULT
	msgbox M03_B02_T1, MSGBOX_DEFAULT
	setvar VAR_0x8004, 1
	setvar VAR_0x8005, 1
	setvar VAR_0x8006, 8
	setvar VAR_0x8007, 3
	special ShakeCamera
	playse SE_M_EARTHQUAKE
	waitse
	fadescreen FADE_TO_WHITE
	fadescreen FADE_FROM_WHITE
	msgbox M03_B02_T2, MSGBOX_DEFAULT
	msgbox M03_B02_T3, MSGBOX_DEFAULT
	setvar VAR_M03_SCENE, 3
	m03_warp MAP_M03_GREENFIELD_APPROACH, 13, 44

@ LINES B03. Off-meter name for this mechanic was M03_B04_LisaChallenge.
M03_B03_LisaChallenge::
	lockall
	msgbox M03_B03_T0, MSGBOX_DEFAULT
	msgbox M03_B03_T1, MSGBOX_DEFAULT
M03_B03_Fight::
	cleartrainerflag TRAINER_M03_LISA
	setflag FLAG_ARENA_NO_WHITEOUT
	trainerbattle_no_intro TRAINER_M03_LISA, ArenaStory_Text_Lose
	clearflag FLAG_ARENA_NO_WHITEOUT
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, M03_B03_Won
	special HealPlayerParty
	goto M03_B03_Fight
M03_B03_Won::
	msgbox M03_B03_T2, MSGBOX_DEFAULT
	msgbox M03_B03_T3, MSGBOX_DEFAULT
	setvar VAR_M03_SCENE, 4
	goto M03_B04_CrystalTown

@ LINES B04. Off-meter name was M03_B03_CrystalCreep.
M03_B04_CrystalTown::
	lockall
	msgbox M03_B04_T0, MSGBOX_DEFAULT
	msgbox M03_B04_T1, MSGBOX_DEFAULT
	msgbox M03_B04_T2, MSGBOX_DEFAULT
	msgbox M03_B04_T3, MSGBOX_DEFAULT
	msgbox M03_B04_T4, MSGBOX_DEFAULT
	setvar VAR_M03_SCENE, 5
	goto M03_B05_EnteiTakesDelia

@ LINES B05. clearflag here so Entei/Molly are visible on later loads.
M03_B05_EnteiTakesDelia::
	lockall
	clearflag FLAG_M03_HIDE_ENTEI
	msgbox M03_B05_T0, MSGBOX_DEFAULT
	msgbox M03_B05_T1, MSGBOX_DEFAULT
	msgbox M03_B05_T2, MSGBOX_DEFAULT
	msgbox M03_B05_T3, MSGBOX_DEFAULT
	setvar VAR_M03_SCENE, 6
	m03_warp MAP_M03_GREENFIELD_CENTER, 7, 5

@ LINES B06. Center news, Charizard cutaway, and Molly's email.
M03_B06_CenterNews::
	lockall
	msgbox M03_B06_T0, MSGBOX_DEFAULT
	msgbox M03_B06_T1, MSGBOX_DEFAULT
	msgbox M03_B06_T2, MSGBOX_DEFAULT
	msgbox M03_B06_T3, MSGBOX_DEFAULT
	setvar VAR_M03_SCENE, 7
	m03_warp MAP_M03_GREENFIELD_APPROACH, 13, 44

@ LINES B07. Off-meter name was M03_B07_TowerBreach.
M03_B07_Approach::
	lockall
	msgbox M03_B07_T0, MSGBOX_DEFAULT
	playse SE_M_FLAMETHROWER
	waitse
	msgbox M03_B07_T1, MSGBOX_DEFAULT
	msgbox M03_B07_T2, MSGBOX_DEFAULT
	msgbox M03_B07_T3, MSGBOX_DEFAULT
	msgbox M03_B07_T4, MSGBOX_DEFAULT
	setvar VAR_M03_SCENE, 8
	m03_warp MAP_M03_CHARICIFIC_VALLEY, 10, 38

@ LINES B08 garden door. Charicific stand-in is the grass-field cutaway.
M03_B08_GardenDoor::
	lockall
	msgbox M03_B08_T0, MSGBOX_DEFAULT
	msgbox M03_B08_T1, MSGBOX_DEFAULT
	msgbox M03_B08_T2, MSGBOX_DEFAULT
	setvar VAR_M03_SCENE, 9
	m03_warp MAP_M03_CRYSTAL_HALL_WATER, 8, 5

M03_B09_BrockCutaway::
	lockall
	msgbox M03_B09_T0, MSGBOX_DEFAULT
	msgbox M03_B09_T1, MSGBOX_DEFAULT
	msgbox M03_B09_T2, MSGBOX_DEFAULT
	fadescreen FADE_TO_BLACK
	setobjectxy LOCALID_PLAYER, 12, 8
	fadescreen FADE_FROM_BLACK
	setvar VAR_M03_SCENE, 10
	goto M03_B10_MistyCutaway

M03_B10_MistyCutaway::
	lockall
	msgbox M03_B10_T0, MSGBOX_DEFAULT
	msgbox M03_B10_T1, MSGBOX_DEFAULT
	msgbox M03_B10_T2, MSGBOX_DEFAULT
	fadescreen FADE_TO_BLACK
	fadescreen FADE_FROM_BLACK
	setvar VAR_M03_SCENE, 11
	m03_warp MAP_M03_CRYSTAL_TOWER_ROOF, 10, 12

M03_B11_EnteiForceLoss::
	lockall
	msgbox M03_B11_T0, MSGBOX_DEFAULT
	msgbox M03_B11_T1, MSGBOX_DEFAULT
	msgbox M03_B11_T2, MSGBOX_DEFAULT
	msgbox M03_B11_T3, MSGBOX_DEFAULT
	cleartrainerflag TRAINER_M03_ENTEI_ILLUSION
	setflag FLAG_ARENA_FORCE_LOSS
	setflag FLAG_ARENA_NO_WHITEOUT
	trainerbattle_no_intro TRAINER_M03_ENTEI_ILLUSION, ArenaStory_Text_Lose
	clearflag FLAG_ARENA_FORCE_LOSS
	clearflag FLAG_ARENA_NO_WHITEOUT
	special HealPlayerParty
	setvar VAR_M03_SCENE, 12
	goto M03_B12_CharizardSave

@ Overworld stub. No guest Charizard inject in P1.
M03_B12_CharizardSave::
	lockall
	clearflag FLAG_M03_HIDE_CHARIZARD
	addobject 3
	playse SE_M_FLAMETHROWER
	waitse
	msgbox M03_B12_T0, MSGBOX_DEFAULT
	msgbox M03_B12_T1, MSGBOX_DEFAULT
	msgbox M03_B12_T2, MSGBOX_DEFAULT
	msgbox M03_B12_T3, MSGBOX_DEFAULT
	setvar VAR_M03_SCENE, 13
	goto M03_B13_ClashAndTruce

@ Overworld clash only. No guest battle in P1.
M03_B13_ClashAndTruce::
	lockall
	playse SE_M_EARTHQUAKE
	waitse
	playse SE_M_FLAMETHROWER
	waitse
	msgbox M03_B13_T0, MSGBOX_DEFAULT
	msgbox M03_B13_T1, MSGBOX_DEFAULT
	msgbox M03_B13_T2, MSGBOX_DEFAULT
	msgbox M03_B13_T3, MSGBOX_DEFAULT
	msgbox M03_B13_T4, MSGBOX_DEFAULT
	setvar VAR_M03_SCENE, 14
	m03_warp MAP_M03_UNOWN_CHAMBER, 24, 7

M03_B14_UnownFarewell::
	lockall
	goto_if_set FLAG_M03_DONE, M03_B14_Leave
	msgbox M03_B14_T0, MSGBOX_DEFAULT
	msgbox M03_B14_T1, MSGBOX_DEFAULT
	msgbox M03_B14_T2, MSGBOX_DEFAULT
	msgbox M03_B14_T3, MSGBOX_DEFAULT
	msgbox M03_B14_T4, MSGBOX_DEFAULT
	msgbox M03_B14_T5, MSGBOX_DEFAULT
	msgbox M03_B14_T6, MSGBOX_DEFAULT
	setflag FLAG_M03_DONE
	setvar VAR_M03_SCENE, 15
M03_B14_Leave::
	setvar VAR_M03_SCENE, 15
	m03_warp MAP_GOLDENROD_CITY, 28, 38

M03_B01_T0:
	.string "NARRATOR:\nGreenfield. The Hale mansion.$"
M03_B01_T1:
	.string "SPENCER:\nAnd Entei ran across the land...$"
M03_B01_T2:
	.string "SPENCER:\nPapa's off to the ruins, Molly.$"
M03_B01_T3:
	.string "NARRATOR:\nHe never came back.$"
M03_B02_T0:
	.string "MOLLY:\nPapa... Mama... me.$"
M03_B02_T1:
	.string "NARRATOR:\nThe Unown answered her wish.$"
M03_B02_T2:
	.string "ENTEI:\nI am here, Molly.$"
M03_B02_T3:
	.string "MOLLY:\nPapa? You came back!$"
M03_B03_T0:
	.string "LISA:\nYou! Trainer! Battle me!$"
M03_B03_T1:
	.string "{PLAYER}:\nYou're on! Pikachu, go!$"
M03_B03_T2:
	.string "LISA:\nNice. Lunch is on me.$"
M03_B03_T3:
	.string "LISA:\nGreenfield's Center is close.$"
M03_B04_T0:
	.string "MISTY:\nThe whole town is crystal!$"
M03_B04_T1:
	.string "REPORTER:\nWe're live outside the Center.$"
M03_B04_T2:
	.string "{PLAYER}:\nMom?! Professor Oak?!$"
M03_B04_T3:
	.string "DELIA:\nSpencer worked with you, right?$"
M03_B04_T4:
	.string "OAK:\nYes. A fine friend, Delia.$"
M03_B05_T0:
	.string "ENTEI:\nYou will be Molly's mama.$"
M03_B05_T1:
	.string "{PLAYER}:\nMom! Pikachu, Thunder Shock!$"
M03_B05_T2:
	.string "{PLAYER}:\nIt didn't even feel it!$"
M03_B05_T3:
	.string "BROCK:\nAsh, wait! Don't run off!$"
M03_B06_T0:
	.string "OAK:\nThe Unown made that Entei.$"
M03_B06_T1:
	.string "SCHUYLER:\nAnd Molly is still inside.$"
M03_B06_T2:
	.string "LIZA:\nCharizard... that's Ash's mom.$"
M03_B06_T3:
	.string "MOLLY:\nGo away! Leave us alone!$"
M03_B07_T0:
	.string "LISA:\nTake my Pokégear. Stay in touch.$"
M03_B07_T1:
	.string "MEOWTH:\nWhy's it always us?!$"
M03_B07_T2:
	.string "{PLAYER}:\nChikorita, Vine Whip! Up we go!$"
M03_B07_T3:
	.string "DELIA:\nAsh...? Is that my Ash?$"
M03_B07_T4:
	.string "OAK:\nAsh, can you hear me? Be safe.$"
M03_B08_T0:
	.string "BROCK:\nCyndaquil, Vulpix! Flamethrower!$"
M03_B08_T1:
	.string "MISTY:\nStaryu, Water Gun! Hold it open!$"
M03_B08_T2:
	.string "{PLAYER}:\nStairs... and a grass field?$"
M03_B09_T0:
	.string "MOLLY:\nI'm all grown up. Battle me!$"
M03_B09_T1:
	.string "BROCK:\nYou two go. I've got this.$"
M03_B09_T2:
	.string "BROCK:\nOnix, hold on! Go, Onix!$"
M03_B10_T0:
	.string "MOLLY:\nWater Pokémon only, okay?$"
M03_B10_T1:
	.string "MISTY:\nFine by me. I'm a water pro!$"
M03_B10_T2:
	.string "MISTY:\nStaryu, it's all on you!$"
M03_B11_T0:
	.string "{PLAYER}:\nMom! We're going home!$"
M03_B11_T1:
	.string "MOLLY:\nNo! She's MY mama now!$"
M03_B11_T2:
	.string "ENTEI:\nYou will not take her.$"
M03_B11_T3:
	.string "{PLAYER}:\nPikachu, keep going! Thunder!$"
M03_B12_T0:
	.string "{PLAYER}:\nCharizard?! You came!$"
M03_B12_T1:
	.string "{PLAYER}:\nCharizard, Flamethrower!$"
M03_B12_T2:
	.string "MOLLY:\nStop it! Stop fighting!$"
M03_B12_T3:
	.string "MOLLY:\nThis isn't what I wanted...$"
M03_B13_T0:
	.string "MOLLY:\nI want it all real again.$"
M03_B13_T1:
	.string "OAK:\nEveryone out! The crystal!$"
M03_B13_T2:
	.string "{PLAYER}:\nPikachu! Charizard! Together!$"
M03_B13_T3:
	.string "MOLLY:\nGo, Entei! Break it!$"
M03_B13_T4:
	.string "ENTEI:\nBe well, Molly.$"
M03_B14_T0:
	.string "NARRATOR:\nThe Unown went home. Papa too.$"
M03_B14_T1:
	.string "MOLLY:\nThank you, Entei.$"
M03_B14_T2:
	.string "{PLAYER}:\nGo home, Charizard. Thanks.$"
M03_B14_T3:
	.string "LISA:\nSee you around, Ash!$"
M03_B14_T4:
	.string "JESSIE:\nWe'll get you next time!$"
M03_B14_T5:
	.string "JAMES:\nNext time, for sure!$"
M03_B14_T6:
	.string "NARRATOR:\nSpencer came home. Mama too.$"
