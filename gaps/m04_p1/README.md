# M04 P1 gaps — Celebi: Voice of the Forest

Scaffold landed on the Arena Johto tree. Friend pin 1.8.25, launcher `Series.java`, and the Flint C FR pack are not in this repo and were not touched.

`FLAG_M04_DONE` sets the done flag, stores `VAR_M04_SCENE = 34`, and warps to `MAP_VIOLET_CITY_POKEMON_CENTER` (7, 7). It does not set `FLAG_ARENA_SHOW_SJ10`, `FLAG_ARENA_SHOW_SJ10A`, `FLAG_ARENA_SHOW_SJ10B`, `FLAG_ARENA_SHOW_P2_END`, or any series bit.

## sha1 / FILL

Local ship (box, 2026-10-25 ~7:05pm AZ) after PR #2 merge (arena @ 55eb9e32):

- ROM sha1: `85fdf3f81190a5ee4a98c015fcc9c5e351a3d06d`
- ELF sha1: `44d0e3671c4df3228af88726cbfcf49b06ba5bb1`
- FILL: EWRAM 91.16% / IWRAM 72.45% / ROM 84.44%
- Pack: `ops/arena/movie34_m04_p1_bake_20261025/`

Cloud VM cannot regenerate gitignored `.arena-dev/art/*.4bpp`; local box had the sheets.

Ship command, when the `.4bpp` sheets are present, is the one in `docs/ARENA_JOHTO_PROGRESS.md`:

```
make RELEASE=1 FILE_NAME=johto_p1 BUILD_DIR=build-p1-rel MATT_PART=1
```

## IDs checked against this tree

| Artifact | Live value |
|---|---|
| Flags 0x91C–0x91F | `FLAG_M04_STARTED/DONE/HIDE_CELEBI/HIDE_SUICUNE` = `SYSTEM_FLAGS + 0xBC..0xBF`. `FLAG_UNUSED_0x91C`–`0x91F` alias them. `DAILY_FLAGS_START` still keys off `FLAG_UNUSED_0x91F`, so the daily band stays at 0x920. |
| Scene var | `VAR_M04_SCENE` aliases `VAR_M03_SCENE` (`VAR_ROUTE_NUMBER`, 0x40DB). Values 16..33 run PK10 and B01–B17. 34 is exited. M03 ON_FRAME tables read only 1..14, so 16..34 does not replay M03. 15 stays M03's exited value. `vars.h` "Unused Var" slots were not taken. |
| Trainers | No new trainer IDs. `TRAINER_M04_DUNDEE` reuses `TRAINER_SIDNEY2` (860). `TRAINER_M04_MARAUDER_FOREST` reuses `TRAINER_PHOEBE2` (861). `TRAINER_M04_DARK_TYRANITAR` reuses `TRAINER_KIP2` (863). Alias-back matches M03. |
| Map group | `gMapGroup_M04` is appended. Existing map IDs do not shift. Six maps, layout reuse only, `MAPSEC_SPECIAL_AREA`, `show_map_name` false. No new tileset. |
| Hook | `goto_if_unset FLAG_M04_DONE, Arena_M04_StartFromSJ9A` sits immediately after `setflag FLAG_ARENA_SHOW_SJ9A` in `ArenaStory_SJ9A_30a_end`. That label is already inside `.if MATT_PART == 1`, after `SnorlaxOut`. |

`tools/arena/gen_story_scenes.py` was not run (this VM has no `ops/arena/gen.py`). The FREE list now skips 0x915–0x91F, so a regen cannot assign SJ6A, SJ9A, `FORCE_LOSS`, M03, or M04 flags. After it writes, it pastes the checked-in SJ6A and SJ9A blocks back if the new text omitted them, inserts the M03 and M04 goto lines if either setflag is present without its hook, and exits if either hook is missing or duplicated. Hand edits that live inside scenes the generator does emit (the SJ7 `FORCE_LOSS` body, the follower helpers) are still rewritten from `gen.py`.

## Speech

61 locked bodies from `LINES_M04` (PK10 has no drafted line). Each body is ≤36 characters. Wired as M03 msgboxes: `SPEAKER:\nbody`. Narrator is that msgbox with no namebox.

B11 order is heal/play, then sketch, then Metapod. B17 Oak is two msgboxes: `True friends last.` then `You'll meet again.` Misty's following line keeps the locked U+2014 em dash. `charmap.txt` has no separate dash glyph, so that code point encodes as the existing hyphen (`AE`).

## Mechanics

- PK10 is two Pikachu cries and the player exclamation emote. No msgbox.
- B03 Dundee is winnable. `FLAG_ARENA_NO_WHITEOUT` is set so a loss does not leave the movie. On a loss the party is healed and the fight retries.
- B04 clears `FLAG_M04_HIDE_SUICUNE`, shows Suicune, flashes white, then sets the flag and removes the object.
- B05 fades to black between the blimp lines and the town lines.
- B08 clears `FLAG_M04_HIDE_CELEBI`. B14 sets it again.
- B10 is a winnable Scizor/Sneasel fight with the same no-whiteout retry. Charmeleon is an overworld stub (cry + Headbutt SFX), not a party guest. The object uses `FLAG_M04_HIDE_SUICUNE` because there is no fifth flag, and that flag is set again before the warp to the lake.
- B13 sets `FLAG_ARENA_FORCE_LOSS` and `FLAG_ARENA_NO_WHITEOUT`, battles Lv60 Tyranitar, clears both flags, then `special HealPlayerParty`.
- B14 is dialogue only. No battle.
- Entry does not write a save. A reset back to a pre-SJ9A save replays SJ9A, then M04, while `FLAG_M04_DONE` is unset. Classic storyline does not run SJ9A, so M04 does not start on that path.

## UNVERIFIED

- Return tile `(7, 7)` on `MAP_VIOLET_CITY_POKEMON_CENTER`. SJ9A has no coord trigger. It runs from `ProcessPlayerFieldInput` on that map. The only warp is the door at `(7, 8)`. `(7, 7)` is one step north of that mat so the player is not left on the exit warp. Metatile walkability was not checked.
- Ferry warp-in `(28, 8)` is the SS Aqua 1F grandpa trigger on the source map (a walkable tile there). This map has no coord events, so that trigger does not run. Other warp-ins are source-map object or gate tiles, not metatile-checked: Arborville past and forest `(22, 61)` (two tiles north of the Ilex south gate at `(22, 63)`); Arborville `(35, 22)` (Route 29 youngster); Towa's house `(5, 5)` (Kurt's house interior; the door warp is `(4, 8)`); Lake of Life `(37, 36)` (Lake of Rage lass).
- Dundee's party is Fearow 28 / Ariados 29 / Ursaring 30. The pack only said winnable ~Lv30. Species and moves (level-up) were not specified.
- B10 is Scizor 30 / Sneasel 30 with level-up moves. "Dark" is the Dark Ball line, not a separate species. There is no Dark Ball item in `items.h`. Levels were not specified.
- B13 moves are Crunch / Earthquake / Rock Slide / Hyper Beam, IVs 31, mirroring the M03 Entei block. Level 60 is the pack's level. The moves are not.
- B03 and B10 use `FLAG_ARENA_NO_WHITEOUT` plus a loss retry. The beat table names that flag on B13. Without it, a loss whiteouts off the movie map and the beat cannot resume.
- Charmeleon shares `FLAG_M04_HIDE_SUICUNE` with Suicune. They are never on the same map while the flag is clear.
- Lake and forest weather is `WEATHER_NONE`, matching M03, not the source layout's shade or thunderstorm.
