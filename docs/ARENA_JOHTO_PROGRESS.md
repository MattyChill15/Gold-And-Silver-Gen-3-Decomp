# Arena Johto port — progress (2026-10-23 06:12 MST)
Branch `arena` (local only). Commits: ed44097d (copy), dbdee09e (overlay compiles).

## Done
- realtime_arena.c + arena_*.inc/.c compile and link on expansion with 0 errors:
  gBattleMoves->GetMove*(), status2/gStatuses3->volatiles, type chart->GetTypeModifier,
  usedHeldItems->partyState[].usedHeldItem, B_WEATHER_RAIN_NORMAL, TRAINER_BATTLE_PARAM.opponentA,
  LoadSpecialPokePic (Alloc buffer), uncompressed palettes, GiveCapturedMonToPlayer,
  GetEvolutionTargetSpecies(.., NULL, NULL, CHECK_EVO), CreateMonLegacy, CreateBoxMonAt,
  ArenaObjectPalette, default case for Gen4+ abilities (classic mode), EWRAM_INIT for initialized vars.
- Flags 0x2B1-0x2B8 (FLAG_ARENA_PRACTICE ... FLAG_ARENA_CHIEF_STORY). VAR_ARENA_GAME_STAMP = 0x40DC.
- Battle scripts BattleScript_ArenaEvolutionLearn / BattleScript_RealtimeArenaFaint.
- Story selector + hook slots: include/constants/arena_story.h, src/arena_story.c
  (callnative ArenaStory_TryHook; all slots NULL => Classic/stock runs).

## Not done (next)
1. Engine hooks: native-engine.patch hunks for battle_main.c (~286 lines), battle_script_commands.c
   (~665 lines), battle_controller_player.c, overworld.c, main_menu.c, reshow_battle_screen.c.
   Until wired, --gc-sections drops the overlay (ROM grew only 192 B: 22,550,980 B / 67.21%).
2. arena_series.c for Johto: seg 2 writer (party -> pret format), FR P2 importer (pret -> expansion
   species/item/move remap + struct conversion), story flag via seriesFlags bit CHIEF_STORY.
3. Emerald P1: sourceSegment==2 importer reading expansion boxes.
4. Part 1/Part 2 split, playtests, Drive backup.
5. Chief Story J1-J18 wiring (approved by Matt). Cut order if budget tight: J8, J13, J16.
   Dialogue <= 35 chars/line.

## Chief Story teams (Shorty, Gen 1-2)
- A (J5): Murkrow 16, Zubat 15, Dratini 17
- B (J10): Murkrow 33, Golbat 33, Sneasel 34, Houndoom 34, Dragonair 36
- C (J12): Murkrow 42, Crobat 42, Sneasel 43, Houndoom 44, Gyarados 43, Dragonair 45
- D (J16, optional): Murkrow 64, Crobat 65, Sneasel 65, Houndoom 66, Gyarados 66, Dragonite 68
- Full (J17): Tyranitar 72, Crobat 70 (King's Rock), Houndoom 71, Gyarados 71, Sneasel 70,
  Dragonite 74 (Leftovers); competitive moves
- J8 sailor: Tentacool 25, Machoke 26, Seaking 27
- J13 Rocket boss: Weezing 46, Arbok 46, Houndoom 47, Muk 47, Tyranitar 49
