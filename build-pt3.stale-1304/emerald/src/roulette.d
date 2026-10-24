build-pt3/emerald/src/roulette.o: build-pt3/assets/graphics/roulette/azurill.png.gbapal build-pt3/assets/graphics/roulette/ball.png.4bpp.smol build-pt3/assets/graphics/roulette/ball.png.gbapal build-pt3/assets/graphics/roulette/ball_counter.png.4bpp.smol build-pt3/assets/graphics/roulette/ball_counter.png.gbapal build-pt3/assets/graphics/roulette/credit.png.gbapal build-pt3/assets/graphics/roulette/cursor.png.4bpp.smol build-pt3/assets/graphics/roulette/cursor.png.gbapal build-pt3/assets/graphics/roulette/grid.bin.smolTM build-pt3/assets/graphics/roulette/grid_icons.png.4bpp.smol build-pt3/assets/graphics/roulette/grid_icons.png.gbapal build-pt3/assets/graphics/roulette/makuhita.png.gbapal build-pt3/assets/graphics/roulette/roulette_tilt.4bpp.smol build-pt3/assets/graphics/roulette/shadow.png.4bpp.smol build-pt3/assets/graphics/roulette/shadow.png.gbapal build-pt3/assets/graphics/roulette/shroomish.png.gbapal build-pt3/assets/graphics/roulette/skitty.png.gbapal build-pt3/assets/graphics/roulette/tailow.png.gbapal build-pt3/assets/graphics/roulette/unused_1.pal.gbapal build-pt3/assets/graphics/roulette/unused_2.pal.gbapal build-pt3/assets/graphics/roulette/unused_3.pal.gbapal build-pt3/assets/graphics/roulette/unused_4.pal.gbapal build-pt3/assets/graphics/roulette/wheel.bin.smolTM build-pt3/assets/graphics/roulette/wheel.png.gbapal build-pt3/assets/graphics/roulette/wheel_icons.4bpp.smol build-pt3/assets/graphics/roulette/wynaut.png.gbapal include/assertf.h include/bg.h include/coins.h include/config/ai.h include/config/battle.h include/config/caps.h include/config/contest.h include/config/debug.h include/config/dexnav.h include/config/follower_npc.h include/config/general.h include/config/item.h include/config/map_preview_screen.h include/config/overworld.h include/config/pokemon.h include/config/save.h include/config/species_enabled.h include/config/summary_screen.h include/config/test.h include/config/text.h include/config/wild_encounter.h include/constants/abilities.h include/constants/apricorn_tree.h include/constants/arena_story_flags.h include/constants/battle.h include/constants/battle_frontier_trainers.h include/constants/battle_partner.h include/constants/berries.h include/constants/berry.h include/constants/characters.h include/constants/coins.h include/constants/cries.h include/constants/easy_chat.h include/constants/egg_ids.h include/constants/flags.h include/constants/form_change_types.h include/constants/game_stat.h include/constants/global.h include/constants/hold_effects.h include/constants/items.h include/constants/map_groups.h include/constants/map_types.h include/constants/maps.h include/constants/moves.h include/constants/opponents.h include/constants/opponents_frlg.h include/constants/pokedex.h include/constants/pokemon.h include/constants/region_map_sections.h include/constants/regions.h include/constants/rematches.h include/constants/rgb.h include/constants/roulette.h include/constants/rtc.h include/constants/siirtc.h include/constants/songs.h include/constants/sound.h include/constants/species.h include/constants/tms_hms.h include/constants/trainer_hill.h include/constants/trainer_tower.h include/constants/trainers.h include/constants/tv.h include/constants/vars.h include/constants/vars_frlg.h include/contest_effect.h include/decompress.h include/event_data.h include/field_screen_effect.h include/fpmath.h include/gametypes.h include/gba/defines.h include/gba/gba.h include/gba/io_reg.h include/gba/isagbprint.h include/gba/m4a_internal.h include/gba/macro.h include/gba/multiboot.h include/gba/syscall.h include/gba/types.h include/global.berry.h include/global.fieldmap.h include/global.h include/global.tv.h include/gpu_regs.h include/graphics.h include/m4a.h include/main.h include/malloc.h include/menu.h include/menu_helpers.h include/metaprogram.h include/overworld.h include/palette.h include/palette_util.h include/pokemon.h include/random.h include/roulette.h include/rtc.h include/scanline_effect.h include/script.h include/siirtc.h include/sound.h include/sprite.h include/string_util.h include/strings.h include/task.h include/test_result.h include/text.h include/trig.h include/tv.h include/wild_encounter_ow.h include/window.h
build-pt3/emerald/src/roulette.d: include/assertf.h include/bg.h include/coins.h include/config/ai.h include/config/battle.h include/config/caps.h include/config/contest.h include/config/debug.h include/config/dexnav.h include/config/follower_npc.h include/config/general.h include/config/item.h include/config/map_preview_screen.h include/config/overworld.h include/config/pokemon.h include/config/save.h include/config/species_enabled.h include/config/summary_screen.h include/config/test.h include/config/text.h include/config/wild_encounter.h include/constants/abilities.h include/constants/apricorn_tree.h include/constants/arena_story_flags.h include/constants/battle.h include/constants/battle_frontier_trainers.h include/constants/battle_partner.h include/constants/berries.h include/constants/berry.h include/constants/characters.h include/constants/coins.h include/constants/cries.h include/constants/easy_chat.h include/constants/egg_ids.h include/constants/flags.h include/constants/form_change_types.h include/constants/game_stat.h include/constants/global.h include/constants/hold_effects.h include/constants/items.h include/constants/map_groups.h include/constants/map_types.h include/constants/maps.h include/constants/moves.h include/constants/opponents.h include/constants/opponents_frlg.h include/constants/pokedex.h include/constants/pokemon.h include/constants/region_map_sections.h include/constants/regions.h include/constants/rematches.h include/constants/rgb.h include/constants/roulette.h include/constants/rtc.h include/constants/siirtc.h include/constants/songs.h include/constants/sound.h include/constants/species.h include/constants/tms_hms.h include/constants/trainer_hill.h include/constants/trainer_tower.h include/constants/trainers.h include/constants/tv.h include/constants/vars.h include/constants/vars_frlg.h include/contest_effect.h include/decompress.h include/event_data.h include/field_screen_effect.h include/fpmath.h include/gametypes.h include/gba/defines.h include/gba/gba.h include/gba/io_reg.h include/gba/isagbprint.h include/gba/m4a_internal.h include/gba/macro.h include/gba/multiboot.h include/gba/syscall.h include/gba/types.h include/global.berry.h include/global.fieldmap.h include/global.h include/global.tv.h include/gpu_regs.h include/graphics.h include/m4a.h include/main.h include/malloc.h include/menu.h include/menu_helpers.h include/metaprogram.h include/overworld.h include/palette.h include/palette_util.h include/pokemon.h include/random.h include/roulette.h include/rtc.h include/scanline_effect.h include/script.h include/siirtc.h include/sound.h include/sprite.h include/string_util.h include/strings.h include/task.h include/test_result.h include/text.h include/trig.h include/tv.h include/wild_encounter_ow.h include/window.h
build-pt3/assets/graphics/roulette/azurill.png.gbapal:
build-pt3/assets/graphics/roulette/ball.png.4bpp.smol:
build-pt3/assets/graphics/roulette/ball.png.gbapal:
build-pt3/assets/graphics/roulette/ball_counter.png.4bpp.smol:
build-pt3/assets/graphics/roulette/ball_counter.png.gbapal:
build-pt3/assets/graphics/roulette/credit.png.gbapal:
build-pt3/assets/graphics/roulette/cursor.png.4bpp.smol:
build-pt3/assets/graphics/roulette/cursor.png.gbapal:
build-pt3/assets/graphics/roulette/grid.bin.smolTM:
build-pt3/assets/graphics/roulette/grid_icons.png.4bpp.smol:
build-pt3/assets/graphics/roulette/grid_icons.png.gbapal:
build-pt3/assets/graphics/roulette/makuhita.png.gbapal:
build-pt3/assets/graphics/roulette/roulette_tilt.4bpp.smol:
build-pt3/assets/graphics/roulette/shadow.png.4bpp.smol:
build-pt3/assets/graphics/roulette/shadow.png.gbapal:
build-pt3/assets/graphics/roulette/shroomish.png.gbapal:
build-pt3/assets/graphics/roulette/skitty.png.gbapal:
build-pt3/assets/graphics/roulette/tailow.png.gbapal:
build-pt3/assets/graphics/roulette/unused_1.pal.gbapal:
build-pt3/assets/graphics/roulette/unused_2.pal.gbapal:
build-pt3/assets/graphics/roulette/unused_3.pal.gbapal:
build-pt3/assets/graphics/roulette/unused_4.pal.gbapal:
build-pt3/assets/graphics/roulette/wheel.bin.smolTM:
build-pt3/assets/graphics/roulette/wheel.png.gbapal:
build-pt3/assets/graphics/roulette/wheel_icons.4bpp.smol:
build-pt3/assets/graphics/roulette/wynaut.png.gbapal:
include/assertf.h:
include/bg.h:
include/coins.h:
include/config/ai.h:
include/config/battle.h:
include/config/caps.h:
include/config/contest.h:
include/config/debug.h:
include/config/dexnav.h:
include/config/follower_npc.h:
include/config/general.h:
include/config/item.h:
include/config/map_preview_screen.h:
include/config/overworld.h:
include/config/pokemon.h:
include/config/save.h:
include/config/species_enabled.h:
include/config/summary_screen.h:
include/config/test.h:
include/config/text.h:
include/config/wild_encounter.h:
include/constants/abilities.h:
include/constants/apricorn_tree.h:
include/constants/arena_story_flags.h:
include/constants/battle.h:
include/constants/battle_frontier_trainers.h:
include/constants/battle_partner.h:
include/constants/berries.h:
include/constants/berry.h:
include/constants/characters.h:
include/constants/coins.h:
include/constants/cries.h:
include/constants/easy_chat.h:
include/constants/egg_ids.h:
include/constants/flags.h:
include/constants/form_change_types.h:
include/constants/game_stat.h:
include/constants/global.h:
include/constants/hold_effects.h:
include/constants/items.h:
include/constants/map_groups.h:
include/constants/map_types.h:
include/constants/maps.h:
include/constants/moves.h:
include/constants/opponents.h:
include/constants/opponents_frlg.h:
include/constants/pokedex.h:
include/constants/pokemon.h:
include/constants/region_map_sections.h:
include/constants/regions.h:
include/constants/rematches.h:
include/constants/rgb.h:
include/constants/roulette.h:
include/constants/rtc.h:
include/constants/siirtc.h:
include/constants/songs.h:
include/constants/sound.h:
include/constants/species.h:
include/constants/tms_hms.h:
include/constants/trainer_hill.h:
include/constants/trainer_tower.h:
include/constants/trainers.h:
include/constants/tv.h:
include/constants/vars.h:
include/constants/vars_frlg.h:
include/contest_effect.h:
include/decompress.h:
include/event_data.h:
include/field_screen_effect.h:
include/fpmath.h:
include/gametypes.h:
include/gba/defines.h:
include/gba/gba.h:
include/gba/io_reg.h:
include/gba/isagbprint.h:
include/gba/m4a_internal.h:
include/gba/macro.h:
include/gba/multiboot.h:
include/gba/syscall.h:
include/gba/types.h:
include/global.berry.h:
include/global.fieldmap.h:
include/global.h:
include/global.tv.h:
include/gpu_regs.h:
include/graphics.h:
include/m4a.h:
include/main.h:
include/malloc.h:
include/menu.h:
include/menu_helpers.h:
include/metaprogram.h:
include/overworld.h:
include/palette.h:
include/palette_util.h:
include/pokemon.h:
include/random.h:
include/roulette.h:
include/rtc.h:
include/scanline_effect.h:
include/script.h:
include/siirtc.h:
include/sound.h:
include/sprite.h:
include/string_util.h:
include/strings.h:
include/task.h:
include/test_result.h:
include/text.h:
include/trig.h:
include/tv.h:
include/wild_encounter_ow.h:
include/window.h:
ifndef build-pt3/assets/graphics/roulette/azurill.png.gbapal
build-pt3/assets/graphics/roulette/azurill.png.gbapal := defined
build-pt3/assets/graphics/roulette/azurill.png.gbapal: graphics/roulette/azurill.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/ball.png.4bpp
build-pt3/assets/graphics/roulette/ball.png.4bpp := defined
build-pt3/assets/graphics/roulette/ball.png.4bpp: graphics/roulette/ball.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/ball.png.gbapal
build-pt3/assets/graphics/roulette/ball.png.gbapal := defined
build-pt3/assets/graphics/roulette/ball.png.gbapal: graphics/roulette/ball.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/ball_counter.png.4bpp
build-pt3/assets/graphics/roulette/ball_counter.png.4bpp := defined
build-pt3/assets/graphics/roulette/ball_counter.png.4bpp: graphics/roulette/ball_counter.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/ball_counter.png.gbapal
build-pt3/assets/graphics/roulette/ball_counter.png.gbapal := defined
build-pt3/assets/graphics/roulette/ball_counter.png.gbapal: graphics/roulette/ball_counter.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/credit.png.gbapal
build-pt3/assets/graphics/roulette/credit.png.gbapal := defined
build-pt3/assets/graphics/roulette/credit.png.gbapal: graphics/roulette/credit.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/cursor.png.4bpp
build-pt3/assets/graphics/roulette/cursor.png.4bpp := defined
build-pt3/assets/graphics/roulette/cursor.png.4bpp: graphics/roulette/cursor.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/cursor.png.gbapal
build-pt3/assets/graphics/roulette/cursor.png.gbapal := defined
build-pt3/assets/graphics/roulette/cursor.png.gbapal: graphics/roulette/cursor.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/grid.bin.smolTM
build-pt3/assets/graphics/roulette/grid.bin.smolTM := defined
build-pt3/assets/graphics/roulette/grid.bin.smolTM: graphics/roulette/grid.bin
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(SMOLTM) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/grid_icons.png.4bpp
build-pt3/assets/graphics/roulette/grid_icons.png.4bpp := defined
build-pt3/assets/graphics/roulette/grid_icons.png.4bpp: graphics/roulette/grid_icons.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/grid_icons.png.gbapal
build-pt3/assets/graphics/roulette/grid_icons.png.gbapal := defined
build-pt3/assets/graphics/roulette/grid_icons.png.gbapal: graphics/roulette/grid_icons.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/makuhita.png.gbapal
build-pt3/assets/graphics/roulette/makuhita.png.gbapal := defined
build-pt3/assets/graphics/roulette/makuhita.png.gbapal: graphics/roulette/makuhita.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/roulette_tilt.4bpp.smol
build-pt3/assets/graphics/roulette/roulette_tilt.4bpp.smol := defined
build-pt3/assets/graphics/roulette/roulette_tilt.4bpp.smol: graphics/roulette/roulette_tilt.4bpp
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(SMOL) -w $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/shadow.png.4bpp
build-pt3/assets/graphics/roulette/shadow.png.4bpp := defined
build-pt3/assets/graphics/roulette/shadow.png.4bpp: graphics/roulette/shadow.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/shadow.png.gbapal
build-pt3/assets/graphics/roulette/shadow.png.gbapal := defined
build-pt3/assets/graphics/roulette/shadow.png.gbapal: graphics/roulette/shadow.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/shroomish.png.gbapal
build-pt3/assets/graphics/roulette/shroomish.png.gbapal := defined
build-pt3/assets/graphics/roulette/shroomish.png.gbapal: graphics/roulette/shroomish.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/skitty.png.gbapal
build-pt3/assets/graphics/roulette/skitty.png.gbapal := defined
build-pt3/assets/graphics/roulette/skitty.png.gbapal: graphics/roulette/skitty.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/tailow.png.gbapal
build-pt3/assets/graphics/roulette/tailow.png.gbapal := defined
build-pt3/assets/graphics/roulette/tailow.png.gbapal: graphics/roulette/tailow.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/unused_1.pal.gbapal
build-pt3/assets/graphics/roulette/unused_1.pal.gbapal := defined
build-pt3/assets/graphics/roulette/unused_1.pal.gbapal: graphics/roulette/unused_1.pal
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/unused_2.pal.gbapal
build-pt3/assets/graphics/roulette/unused_2.pal.gbapal := defined
build-pt3/assets/graphics/roulette/unused_2.pal.gbapal: graphics/roulette/unused_2.pal
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/unused_3.pal.gbapal
build-pt3/assets/graphics/roulette/unused_3.pal.gbapal := defined
build-pt3/assets/graphics/roulette/unused_3.pal.gbapal: graphics/roulette/unused_3.pal
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/unused_4.pal.gbapal
build-pt3/assets/graphics/roulette/unused_4.pal.gbapal := defined
build-pt3/assets/graphics/roulette/unused_4.pal.gbapal: graphics/roulette/unused_4.pal
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/wheel.bin.smolTM
build-pt3/assets/graphics/roulette/wheel.bin.smolTM := defined
build-pt3/assets/graphics/roulette/wheel.bin.smolTM: graphics/roulette/wheel.bin
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(SMOLTM) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/wheel.png.gbapal
build-pt3/assets/graphics/roulette/wheel.png.gbapal := defined
build-pt3/assets/graphics/roulette/wheel.png.gbapal: graphics/roulette/wheel.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/wheel_icons.4bpp.smol
build-pt3/assets/graphics/roulette/wheel_icons.4bpp.smol := defined
build-pt3/assets/graphics/roulette/wheel_icons.4bpp.smol: graphics/roulette/wheel_icons.4bpp
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(SMOL) -w $< $@ 
endif
ifndef build-pt3/assets/graphics/roulette/wynaut.png.gbapal
build-pt3/assets/graphics/roulette/wynaut.png.gbapal := defined
build-pt3/assets/graphics/roulette/wynaut.png.gbapal: graphics/roulette/wynaut.png
	@mkdir -p 'build-pt3/assets/graphics/roulette'
	$(GFX) $< $@ 
endif
