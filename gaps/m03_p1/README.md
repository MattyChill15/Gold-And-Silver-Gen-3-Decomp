# M03 P1 gaps — Spell of the Unown

Scaffold landed on the Arena Johto tree. M04 Celebi is out of scope. Friend pin, launcher `Series.java`, and the Flint C FR pack are not in this repo and were not touched.

`FLAG_M03_DONE` sets the done flag, stores `VAR_M03_SCENE = 15`, and warps to `MAP_GOLDENROD_CITY` (28, 38), south of the Pokémon Center door. It does not set `FLAG_ARENA_SHOW_SJ7`, `FLAG_ARENA_SHOW_SJ7_LOSS`, `FLAG_ARENA_SHOW_P2_END`, or any series bit. SJ7 Whitney stays on the stock gym script.

## sha1 / FILL

Local ship (box, 2026-10-25 ~5:34pm AZ) after PR #1 merge:

- ROM sha1: `8fbbffbce8b1d2aca290b031b45ac0a96a99ca46`
- ELF sha1: `5475d4f66118c176a26d87ddba8aebc55d42239d`
- FILL: EWRAM 91.16% / IWRAM 72.45% / ROM 84.42%
- Pack: `ops/arena/movie34_m03_p1_bake_20261025/`

Cloud VM still cannot regenerate gitignored `.arena-dev/art/*.4bpp` from missing pngs; local box already had the `.4bpp` sheets.

`arm-none-eabi-gcc 13.2.1` and binutils are installed. Host tools (`preproc`, `mapjson`, `trainerproc`, `gbagfx`) build. The ship command from `docs/ARENA_JOHTO_PROGRESS.md` does not finish:

```
make RELEASE=1 FILE_NAME=johto_p1 BUILD_DIR=build-p1-rel MATT_PART=1
tools/gbagfx/gbagfx .arena-dev/art/actions.png .arena-dev/art/actions.4bpp
Failed to open ".arena-dev/art/actions.png" for reading.
make: *** [Makefile:431: .arena-dev/art/actions.4bpp] Error 1
```

`*.4bpp` / `*.8bpp` / `*.gbapal` are gitignored. `tools/arena/dev.sh` `build_background` writes `.arena-dev/art/actions.4bpp` with `pack_move_fx` and does not write `actions.png`. Make's `%.4bpp: %.png` rule still requires that png. The same gap covers the other realtime-arena sheets. `graphics/arena/cover-smoke/README.md` points at private `export_redesign.py` and `pack_doll.py`, which are not in this tree. `tools/arena/pack_signatures150.py` stops on missing `graphics/arena/signatures150/frame-00.png`.

What did assemble, with `NODEP=1` so the missing art was not pulled in:

- `build-p1-rel/emerald-release/data/event_scripts.o` (M03 script, seven map scripts, SJ6A hook). Exit 0.
- `build-p1-rel/emerald-release/data/maps.o` (mapjson accepted all seven M03 maps). Exit 0.
- `src/data/trainers.h` contains `TRAINER_M03_LISA` (Nidorina 18 / Flaaffy 19 / Sunflora 20) and `TRAINER_M03_ENTEI_ILLUSION` (Entei 60, Flamethrower / Fire Blast / Bite / Stomp, IVs 31).

## IDs checked against this tree

| Artifact guess | Live value |
|---|---|
| Flags 0x918–0x91B | `FLAG_M03_STARTED/DONE/HIDE_ENTEI/HIDE_CHARIZARD` = `SYSTEM_FLAGS + 0xB8..0xBB`. `FLAG_UNUSED_0x918`–`0x91B` alias them. `0x91C`–`0x91F` stay unused for M04. |
| Next free `0x40xx` var | `VARS_END` is `0x40FF` with no empty slot. `VAR_M03_SCENE` aliases `VAR_ROUTE_NUMBER` (`0x40DB`), which Johto scripts do not read. |
| Scene values 0..14, 14 = done | 0 idle, 1..14 running, **15 exited**. Leaving the var at 14 would refire B14 `ON_FRAME` every frame. |
| New trainers past 863 | Trainer flag space is full (`TRAINERS_COUNT_EMERALD` 864). `TRAINER_M03_LISA` reuses dummy `TRAINER_CHANSEY4` (858). `TRAINER_M03_ENTEI_ILLUSION` reuses `TRAINER_CHANSEY5` (859). |
| `LAYOUT_RUINS_OF_ALPH_INNER_CHAMBER` | `LAYOUT_RUINS_OF_ALPH_PUZZLE_AND_REWARD_CHAMBERS` |
| `LAYOUT_GOLDENROD_CITY_POKEMON_CENTER_1F` | `LAYOUT_GOLDENROD_CITY_POKEMON_CENTER` |
| `LAYOUT_TIN_TOWER_ROOF` | `LAYOUT_TIN_TOWER_ROOF_DAY` |
| `LAYOUT_ARENA_CHARICIFIC_VALLEY` (fallback `LAYOUT_ROUTE36`) | `MAP_ARENA_CHARICIFIC_VALLEY` uses `LAYOUT_ROUTE46`. M03 reuses that. |
| `gMapGroup_M03` inserted among existing groups | Appended. Existing map IDs do not shift. |
| `tools/arena/gen_story_scenes.py` regenerate | Not run. The live `arena_story_scenes.inc` has hand-added SJ6A / SJ9A / `FORCE_LOSS` hooks the generator would wipe. The generator `FREE` list now skips `0x918`–`0x91B` so a future run cannot reuse them. The first 55 flag assignments are unchanged. |

`MAPSEC_SPECIAL_AREA` exists. Every M03 map sets `show_map_name` false and reuses an existing layout (no new tileset).

## Speech

59 locked lines from `LINES_M03` (the scope note said 60). Each body is ≤36 characters. Wired as M01 msgboxes: `SPEAKER:\nbody`. Narrator is that msgbox with no namebox. Story scripts in this tree do not call `setspeaker`.

Beat order follows `LINES_M03`, which the timing audit says matches the film. The off-meter nicknames for B03–B08 are swapped relative to those keys (Lisa was labeled B04, crystal B03, center B05, abduction B06, approach B07, door B08). Mechanics sit on the beat that speaks them.

## Mechanics that differ from a literal nickname reading

- B03 Lisa is winnable (~lv20 Nidorina / Flaaffy / Sunflora). `FLAG_ARENA_NO_WHITEOUT` is set so a loss does not dump the player out of the movie. On a loss the party is healed and the fight retries. A win plays the post lines and auto-chains B04.
- B09 and B10 are dialogue plus a fade after the final line. They are not Brock or Misty trainer battles. B09 repositions the player to (12, 8) during the fade.
- B11 sets `FLAG_ARENA_FORCE_LOSS` and `FLAG_ARENA_NO_WHITEOUT`, battles Lv60 Entei, clears both flags, then `special HealPlayerParty` before B12.
- B12–B13 are an overworld stub: `clearflag FLAG_M03_HIDE_CHARIZARD`, `addobject 3`, flamethrower / earthquake SFX. No guest Charizard inject.

## P2 backlog

- Guest Charizard (party inject and the real Entei clash).
- Brock and Misty as real cutaway battles if a later pass wants them.
- Entei overworld gfx distinct from the species sprite.
- Manor / roof objects are placed and hidden on `FLAG_M03_HIDE_ENTEI` until the abduction `clearflag`.
- Classic storyline does not run SJ6A, so M03 does not start on that path. Same lines, Show entry only.
- Mid-beat save resumes from the `ON_FRAME` row for vars 1–14. Var 15 does not replay.
- Entry does not write a save. A reset back to a pre-SJ6A save replays SJ6A, then M03, while `FLAG_M03_DONE` is unset.
