@ M04 Celebi: Voice of the Forest — P1 scaffold.
@ Locked lines are LINES_M04 PK10 + B01–B17. B11 is heal/play, then sketch,
@ then Metapod. B17 is the dub ending. Oak is two msgboxes (Matt t746s3551):
@ "True friends last." then "You'll meet again."
@ Speech matches M01/M03: msgbox "SPEAKER:\nbody". Narrator is that msgbox
@ with no namebox. Does not set seriesBit(8), FLAG_ARENA_SHOW_SJ10*, or any P2 flag.
@ VAR_M04_SCENE aliases VAR_M03_SCENE. M03 ON_FRAME reads 1..14 only; M04 uses 16..33
@ and leaves 34 so a finished movie does not replay. 15 stays M03's exited value.
@ Celebi/Suicune are overworld stubs on FLAG_M04_HIDE_*. Charmeleon has no fifth
@ flag (0x920 is the daily band), so the forest object reuses FLAG_M04_HIDE_SUICUNE
@ and the flag is set again before the lake warp. No guest party inject.

.macro m04_warp dest, x, y
	releaseall
	warp \dest, \x, \y
	waitstate
	end
.endm

@ Hooked from ArenaStory_SJ9A_30a_end, which is .if MATT_PART == 1.
Arena_M04_StartFromSJ9A::
	setflag FLAG_M04_STARTED
	setflag FLAG_M04_HIDE_CELEBI
	setflag FLAG_M04_HIDE_SUICUNE
	setvar VAR_M04_SCENE, 16
	releaseall
	warp MAP_M04_ARBORVILLE_PAST, 22, 61
	waitstate
	end

@ PK10. Cries and an emote only. No human line was drafted.
M04_PK10_PikaBoo::
	lockall
	playmoncry SPECIES_PIKACHU, CRY_MODE_NORMAL
	waitmoncry
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_ExclamationMark
	waitmovement 0
	playmoncry SPECIES_PIKACHU, CRY_MODE_NORMAL
	waitmoncry
	setvar VAR_M04_SCENE, 17
	goto M04_B01_Prologue

M04_B01_Prologue::
	lockall
	msgbox M04_B01_T0, MSGBOX_DEFAULT
	msgbox M04_B01_T1, MSGBOX_DEFAULT
	msgbox M04_B01_T2, MSGBOX_DEFAULT
	setvar VAR_M04_SCENE, 18
	goto M04_B02_DarkBall

M04_B02_DarkBall::
	lockall
	msgbox M04_B02_T0, MSGBOX_DEFAULT
	fadescreen FADE_TO_BLACK
	fadescreen FADE_FROM_BLACK
	msgbox M04_B02_T1, MSGBOX_DEFAULT
	setvar VAR_M04_SCENE, 19
	m04_warp MAP_M04_FERRY, 28, 8

@ Winnable. NO_WHITEOUT so a loss stays in the movie and retries, same as M03 Lisa.
M04_B03_Dundee::
	lockall
	msgbox M04_B03_T0, MSGBOX_DEFAULT
M04_B03_Fight::
	cleartrainerflag TRAINER_M04_DUNDEE
	setflag FLAG_ARENA_NO_WHITEOUT
	trainerbattle_no_intro TRAINER_M04_DUNDEE, ArenaStory_Text_Lose
	clearflag FLAG_ARENA_NO_WHITEOUT
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, M04_B03_Won
	special HealPlayerParty
	goto M04_B03_Fight
M04_B03_Won::
	msgbox M04_B03_T1, MSGBOX_DEFAULT
	msgbox M04_B03_T2, MSGBOX_DEFAULT
	msgbox M04_B03_T3, MSGBOX_DEFAULT
	setvar VAR_M04_SCENE, 20
	goto M04_B04_Suicune

@ Suicune overworld flash. Object 1 on MAP_M04_FERRY.
M04_B04_Suicune::
	lockall
	msgbox M04_B04_T0, MSGBOX_DEFAULT
	clearflag FLAG_M04_HIDE_SUICUNE
	addobject 1
	playmoncry SPECIES_SUICUNE, CRY_MODE_ENCOUNTER
	waitmoncry
	msgbox M04_B04_T1, MSGBOX_DEFAULT
	fadescreen FADE_TO_WHITE
	setflag FLAG_M04_HIDE_SUICUNE
	removeobject 1
	fadescreen FADE_FROM_WHITE
	msgbox M04_B04_T2, MSGBOX_DEFAULT
	setvar VAR_M04_SCENE, 21
	m04_warp MAP_M04_ARBORVILLE, 35, 22

M04_B05_Blimp::
	lockall
	msgbox M04_B05_T0, MSGBOX_DEFAULT
	msgbox M04_B05_T1, MSGBOX_DEFAULT
	msgbox M04_B05_T2, MSGBOX_DEFAULT
	fadescreen FADE_TO_BLACK
	fadescreen FADE_FROM_BLACK
	setvar VAR_M04_SCENE, 22
	goto M04_B06_Shrine

M04_B06_Shrine::
	lockall
	msgbox M04_B06_T0, MSGBOX_DEFAULT
	msgbox M04_B06_T1, MSGBOX_DEFAULT
	msgbox M04_B06_T2, MSGBOX_DEFAULT
	setvar VAR_M04_SCENE, 23
	m04_warp MAP_M04_TOWA_HOUSE, 5, 5

M04_B07_Sketchbook::
	lockall
	msgbox M04_B07_T0, MSGBOX_DEFAULT
	msgbox M04_B07_T1, MSGBOX_DEFAULT
	msgbox M04_B07_T2, MSGBOX_DEFAULT
	msgbox M04_B07_T3, MSGBOX_DEFAULT
	msgbox M04_B07_T4, MSGBOX_DEFAULT
	setvar VAR_M04_SCENE, 24
	m04_warp MAP_M04_FOREST, 22, 61

@ Object 1 is Celebi. Stays visible through the lake until B14.
M04_B08_Celebi::
	lockall
	clearflag FLAG_M04_HIDE_CELEBI
	addobject 1
	playmoncry SPECIES_CELEBI, CRY_MODE_NORMAL
	waitmoncry
	msgbox M04_B08_T0, MSGBOX_DEFAULT
	msgbox M04_B08_T1, MSGBOX_DEFAULT
	msgbox M04_B08_T2, MSGBOX_DEFAULT
	msgbox M04_B08_T3, MSGBOX_DEFAULT
	setvar VAR_M04_SCENE, 25
	goto M04_B09_Marauder

M04_B09_Marauder::
	lockall
	msgbox M04_B09_T0, MSGBOX_DEFAULT
	msgbox M04_B09_T1, MSGBOX_DEFAULT
	msgbox M04_B09_T2, MSGBOX_DEFAULT
	setvar VAR_M04_SCENE, 26
	goto M04_B10_Bayleef

@ Winnable Scizor/Sneasel. Charmeleon is object 2, gated on HIDE_SUICUNE (no 5th flag).
@ Text and a cry only — no guest party inject. Flag is set again before the lake.
M04_B10_Bayleef::
	lockall
	msgbox M04_B10_T0, MSGBOX_DEFAULT
	msgbox M04_B10_T1, MSGBOX_DEFAULT
	clearflag FLAG_M04_HIDE_SUICUNE
	addobject 2
	playmoncry SPECIES_CHARMELEON, CRY_MODE_NORMAL
	waitmoncry
	playse SE_M_HEADBUTT
	waitse
	msgbox M04_B10_T2, MSGBOX_DEFAULT
M04_B10_Fight::
	cleartrainerflag TRAINER_M04_MARAUDER_FOREST
	setflag FLAG_ARENA_NO_WHITEOUT
	trainerbattle_no_intro TRAINER_M04_MARAUDER_FOREST, ArenaStory_Text_Lose
	clearflag FLAG_ARENA_NO_WHITEOUT
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, M04_B10_Won
	special HealPlayerParty
	goto M04_B10_Fight
M04_B10_Won::
	msgbox M04_B10_T3, MSGBOX_DEFAULT
	setflag FLAG_M04_HIDE_SUICUNE
	removeobject 2
	setvar VAR_M04_SCENE, 27
	m04_warp MAP_M04_LAKE_OF_LIFE, 37, 36

@ Film order: heal/play, sketch, then Metapod.
M04_B11_Lake::
	lockall
	msgbox M04_B11_T0, MSGBOX_DEFAULT
	playmoncry SPECIES_CELEBI, CRY_MODE_NORMAL
	waitmoncry
	msgbox M04_B11_T1, MSGBOX_DEFAULT
	msgbox M04_B11_T2, MSGBOX_DEFAULT
	playmoncry SPECIES_METAPOD, CRY_MODE_NORMAL
	waitmoncry
	msgbox M04_B11_T3, MSGBOX_DEFAULT
	setvar VAR_M04_SCENE, 28
	goto M04_B12_Mech

M04_B12_Mech::
	lockall
	msgbox M04_B12_T0, MSGBOX_DEFAULT
	msgbox M04_B12_T1, MSGBOX_DEFAULT
	msgbox M04_B12_T2, MSGBOX_DEFAULT
	msgbox M04_B12_T3, MSGBOX_DEFAULT
	setvar VAR_M04_SCENE, 29
	goto M04_B13_Onix

@ Object 2 is Suicune. FORCE_LOSS + NO_WHITEOUT, then clear both and heal.
M04_B13_Onix::
	lockall
	clearflag FLAG_M04_HIDE_SUICUNE
	addobject 2
	playmoncry SPECIES_SUICUNE, CRY_MODE_ENCOUNTER
	waitmoncry
	msgbox M04_B13_T0, MSGBOX_DEFAULT
	msgbox M04_B13_T1, MSGBOX_DEFAULT
	playse SE_M_EARTHQUAKE
	waitse
	msgbox M04_B13_T2, MSGBOX_DEFAULT
	cleartrainerflag TRAINER_M04_DARK_TYRANITAR
	setflag FLAG_ARENA_FORCE_LOSS
	setflag FLAG_ARENA_NO_WHITEOUT
	trainerbattle_no_intro TRAINER_M04_DARK_TYRANITAR, ArenaStory_Text_Lose
	clearflag FLAG_ARENA_FORCE_LOSS
	clearflag FLAG_ARENA_NO_WHITEOUT
	special HealPlayerParty
	setflag FLAG_M04_HIDE_SUICUNE
	removeobject 2
	setvar VAR_M04_SCENE, 30
	goto M04_B14_DarkBall

@ Scripted. No battle. Celebi (object 1) hides again.
M04_B14_DarkBall::
	lockall
	msgbox M04_B14_T0, MSGBOX_DEFAULT
	msgbox M04_B14_T1, MSGBOX_DEFAULT
	msgbox M04_B14_T2, MSGBOX_DEFAULT
	setflag FLAG_M04_HIDE_CELEBI
	removeobject 1
	setvar VAR_M04_SCENE, 31
	goto M04_B15_Revive

M04_B15_Revive::
	lockall
	msgbox M04_B15_T0, MSGBOX_DEFAULT
	msgbox M04_B15_T1, MSGBOX_DEFAULT
	msgbox M04_B15_T2, MSGBOX_DEFAULT
	setvar VAR_M04_SCENE, 32
	goto M04_B16_Unmask

M04_B16_Unmask::
	lockall
	msgbox M04_B16_T0, MSGBOX_DEFAULT
	playse SE_M_THUNDERBOLT
	waitse
	msgbox M04_B16_T1, MSGBOX_DEFAULT
	msgbox M04_B16_T2, MSGBOX_DEFAULT
	setvar VAR_M04_SCENE, 33
	m04_warp MAP_M04_TOWA_HOUSE, 5, 5

M04_B17_Epilogue::
	lockall
	goto_if_set FLAG_M04_DONE, M04_B17_Leave
	msgbox M04_B17_T0, MSGBOX_DEFAULT
	msgbox M04_B17_T1, MSGBOX_DEFAULT
	msgbox M04_B17_T2, MSGBOX_DEFAULT
	msgbox M04_B17_T3, MSGBOX_DEFAULT
	msgbox M04_B17_T4, MSGBOX_DEFAULT
	msgbox M04_B17_T5, MSGBOX_DEFAULT
	msgbox M04_B17_T6, MSGBOX_DEFAULT
	setflag FLAG_M04_DONE
	setvar VAR_M04_SCENE, 34
M04_B17_Leave::
	setvar VAR_M04_SCENE, 34
	m04_warp MAP_VIOLET_CITY_POKEMON_CENTER, 7, 7

M04_B01_T0:
	.string "TOWA:\nTake this bread. Be careful.$"
M04_B01_T1:
	.string "SAM:\nCelebi! Hang on, I've got you!$"
M04_B01_T2:
	.string "NARRATOR:\nCelebi carried them away.$"
M04_B02_T0:
	.string "MARAUDER:\nWhere is Celebi? Talk.$"
M04_B02_T1:
	.string "MARAUDER:\nThe Dark Ball. Behold, Tyranitar.$"
M04_B03_T0:
	.string "{PLAYER}:\nPikachu, let's do this!$"
M04_B03_T1:
	.string "BROCK:\nAsh! Crobat says the ferry!$"
M04_B03_T2:
	.string "{PLAYER}:\nWait for us! Jump!$"
M04_B03_T3:
	.string "MR. WHITE:\nGot you! Welcome aboard.$"
M04_B04_T0:
	.string "{PLAYER}:\nPikachu, what'd you see?$"
M04_B04_T1:
	.string "OAK:\nThat was Suicune. I've seen one.$"
M04_B04_T2:
	.string "OAK:\nAsh? Ash, you're breaking up...$"
M04_B05_T0:
	.string "MEOWTH:\nDis plane's blowin' away!$"
M04_B05_T1:
	.string "MR. WHITE:\nHold on. She flies too!$"
M04_B05_T2:
	.string "MISTY:\nA town in the trees!$"
M04_B06_T0:
	.string "DIANA:\nGrandma, we have guests!$"
M04_B06_T1:
	.string "{PLAYER}:\nThe trees are glowing blue...$"
M04_B06_T2:
	.string "{PLAYER}:\nHey! Are you okay? Hang on!$"
M04_B07_T0:
	.string "SAM:\nWho are you? Where's Celebi?$"
M04_B07_T1:
	.string "BROCK:\nEasy, you two. Easy.$"
M04_B07_T2:
	.string "TOWA:\nSam... your sketchbook.$"
M04_B07_T3:
	.string "TOWA:\nYou've been gone forty years.$"
M04_B07_T4:
	.string "SAM:\nForty years...? Then I'll help.$"
M04_B08_T0:
	.string "SAM:\nNo, Pikachu! Don't attack!$"
M04_B08_T1:
	.string "{PLAYER}:\nIt's okay. We won't hurt you.$"
M04_B08_T2:
	.string "JESSIE:\nPrepare for trouble!$"
M04_B08_T3:
	.string "JAMES:\nAnd make it double!$"
M04_B09_T0:
	.string "JAMES:\nWeezing, Smokescreen! Now!$"
M04_B09_T1:
	.string "JESSIE:\nHe's Rocket?! Since when?!$"
M04_B09_T2:
	.string "MARAUDER:\nDark Scizor, Sneasel. Chase.$"
M04_B10_T0:
	.string "MISTY:\nOw! My knee... go on!$"
M04_B10_T1:
	.string "{PLAYER}:\nBayleef, find the real one!$"
M04_B10_T2:
	.string "SAM:\nCharmeleon, Headbutt!$"
M04_B10_T3:
	.string "{PLAYER}:\nWe did it, Sam!$"
M04_B11_T0:
	.string "SAM:\nThe Lake of Life. It heals.$"
M04_B11_T1:
	.string "MISTY:\nCelebi's all better!$"
M04_B11_T2:
	.string "SAM:\nI'll draw you two. Hold still.$"
M04_B11_T3:
	.string "{PLAYER}:\nThe Metapod are evolving!$"
M04_B12_T0:
	.string "{PLAYER}:\nLet Celebi go! Now!$"
M04_B12_T1:
	.string "MARAUDER:\nOff my machine, boy.$"
M04_B12_T2:
	.string "JESSIE:\nPut me down, you rock pile!$"
M04_B12_T3:
	.string "MARAUDER:\nNext, Giovanni falls.$"
M04_B13_T0:
	.string "{PLAYER}:\nSuicune! It saved us!$"
M04_B13_T1:
	.string "BROCK:\nOnix, throw it in the lake!$"
M04_B13_T2:
	.string "BROCK:\nYou did great, Onix. Rest.$"
M04_B14_T0:
	.string "JESSIE:\nCelebi's in there. The middle!$"
M04_B14_T1:
	.string "SAM:\nCelebi! Remember us!$"
M04_B14_T2:
	.string "{PLAYER}:\nYou're not alone. Come back!$"
M04_B15_T0:
	.string "SAM:\nCelebi...? No. Please, no.$"
M04_B15_T1:
	.string "{PLAYER}:\nThe berries aren't working...$"
M04_B15_T2:
	.string "MISTY:\nLook! More Celebi!$"
M04_B16_T0:
	.string "{PLAYER}:\nI'm not letting go! Pikachu!$"
M04_B16_T1:
	.string "{PLAYER}:\nThunderbolt!$"
M04_B16_T2:
	.string "BROCK:\nHe's tied up. No Dark Balls.$"
M04_B17_T0:
	.string "SAM:\nI have to go back. Thank you.$"
M04_B17_T1:
	.string "{PLAYER}:\nSee you again someday, Sam.$"
M04_B17_T2:
	.string "OAK:\nTrue friends last.$"
M04_B17_T3:
	.string "OAK:\nYou'll meet again.$"
M04_B17_T4:
	.string "MISTY:\nWait — how did Oak know Sam?$"
M04_B17_T5:
	.string "NARRATOR:\nSam's sketch. Oak kept it.$"
M04_B17_T6:
	.string "NARRATOR:\nSam grew up to be Oak.$"
