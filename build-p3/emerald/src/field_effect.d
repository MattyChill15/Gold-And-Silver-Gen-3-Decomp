build-p3/emerald/src/field_effect.o: build-p3/assets/graphics/birch_speech/birch.png.4bpp build-p3/assets/graphics/birch_speech/birch.png.gbapal build-p3/assets/graphics/birch_speech/unused_beauty.png_num_tiles_822__Wnum_tiles.4bpp build-p3/assets/graphics/field_effects/palettes/hof_monitor.pal.gbapal build-p3/assets/graphics/field_effects/palettes/pokeball_glow.pal.gbapal build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_bottom_left.png.4bpp build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_bottom_right.png.4bpp build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_top_left.png.4bpp build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_top_right.png.4bpp build-p3/assets/graphics/field_effects/pics/field_move_streaks.png.4bpp build-p3/assets/graphics/field_effects/pics/field_move_streaks.png.gbapal build-p3/assets/graphics/field_effects/pics/field_move_streaks_indoors.png.4bpp build-p3/assets/graphics/field_effects/pics/field_move_streaks_indoors.png.gbapal build-p3/assets/graphics/field_effects/pics/hof_monitor_big.png.4bpp build-p3/assets/graphics/field_effects/pics/hof_monitor_frlg.png.4bpp build-p3/assets/graphics/field_effects/pics/hof_monitor_frlg.png.gbapal build-p3/assets/graphics/field_effects/pics/hof_monitor_small.png.4bpp build-p3/assets/graphics/field_effects/pics/pokeball_glow.png.4bpp build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/0.png.4bpp build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/1.png.4bpp build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/frlg.png.4bpp build-p3/assets/graphics/field_effects/pics/spotlight.png.4bpp build-p3/assets/graphics/field_effects/pics/spotlight.png.gbapal graphics/field_effects/pics/field_move_streaks.bin graphics/field_effects/pics/field_move_streaks_indoors.bin include/assertf.h include/config/ai.h include/config/battle.h include/config/caps.h include/config/contest.h include/config/debug.h include/config/dexnav.h include/config/follower_npc.h include/config/general.h include/config/item.h include/config/map_preview_screen.h include/config/overworld.h include/config/pokemon.h include/config/save.h include/config/species_enabled.h include/config/summary_screen.h include/config/test.h include/config/text.h include/config/wild_encounter.h include/constants/abilities.h include/constants/apricorn_tree.h include/constants/arena_story_flags.h include/constants/battle.h include/constants/battle_frontier_trainers.h include/constants/battle_partner.h include/constants/berries.h include/constants/berry.h include/constants/characters.h include/constants/cries.h include/constants/difficulty.h include/constants/easy_chat.h include/constants/egg_ids.h include/constants/event_object_movement.h include/constants/event_objects.h include/constants/field_effects.h include/constants/field_weather.h include/constants/flags.h include/constants/follower_npc.h include/constants/form_change_types.h include/constants/game_stat.h include/constants/global.h include/constants/hold_effects.h include/constants/items.h include/constants/map_event_ids.h include/constants/map_groups.h include/constants/map_types.h include/constants/maps.h include/constants/metatile_behaviors.h include/constants/moves.h include/constants/opponents.h include/constants/opponents_frlg.h include/constants/pokeball.h include/constants/pokedex.h include/constants/pokemon.h include/constants/region_map_sections.h include/constants/regions.h include/constants/rematches.h include/constants/rgb.h include/constants/siirtc.h include/constants/songs.h include/constants/sound.h include/constants/species.h include/constants/tms_hms.h include/constants/trainer_hill.h include/constants/trainer_tower.h include/constants/trainers.h include/constants/tv.h include/constants/vars.h include/constants/vars_frlg.h include/contest_effect.h include/data.h include/debug.h include/decompress.h include/difficulty.h include/event_data.h include/event_object_movement.h include/field_camera.h include/field_control_avatar.h include/field_effect.h include/field_effect_helpers.h include/field_player_avatar.h include/field_screen_effect.h include/field_weather.h include/fieldmap.h include/fldeff.h include/follower_npc.h include/fpmath.h include/gametypes.h include/gba/defines.h include/gba/gba.h include/gba/io_reg.h include/gba/isagbprint.h include/gba/macro.h include/gba/multiboot.h include/gba/syscall.h include/gba/types.h include/global.berry.h include/global.fieldmap.h include/global.h include/global.tv.h include/gpu_regs.h include/main.h include/malloc.h include/menu.h include/metaprogram.h include/metatile_behavior.h include/mirage_tower.h include/oras_dowse.h include/overworld.h include/palette.h include/party_menu.h include/pokemon.h include/pokemon_storage_system.h include/script.h include/siirtc.h include/sound.h include/sprite.h include/task.h include/test_result.h include/text.h include/trainer.h include/trainer_pokemon_sprites.h include/trig.h include/util.h include/wild_encounter_ow.h include/window.h
build-p3/emerald/src/field_effect.d: include/assertf.h include/config/ai.h include/config/battle.h include/config/caps.h include/config/contest.h include/config/debug.h include/config/dexnav.h include/config/follower_npc.h include/config/general.h include/config/item.h include/config/map_preview_screen.h include/config/overworld.h include/config/pokemon.h include/config/save.h include/config/species_enabled.h include/config/summary_screen.h include/config/test.h include/config/text.h include/config/wild_encounter.h include/constants/abilities.h include/constants/apricorn_tree.h include/constants/arena_story_flags.h include/constants/battle.h include/constants/battle_frontier_trainers.h include/constants/battle_partner.h include/constants/berries.h include/constants/berry.h include/constants/characters.h include/constants/cries.h include/constants/difficulty.h include/constants/easy_chat.h include/constants/egg_ids.h include/constants/event_object_movement.h include/constants/event_objects.h include/constants/field_effects.h include/constants/field_weather.h include/constants/flags.h include/constants/follower_npc.h include/constants/form_change_types.h include/constants/game_stat.h include/constants/global.h include/constants/hold_effects.h include/constants/items.h include/constants/map_event_ids.h include/constants/map_groups.h include/constants/map_types.h include/constants/maps.h include/constants/metatile_behaviors.h include/constants/moves.h include/constants/opponents.h include/constants/opponents_frlg.h include/constants/pokeball.h include/constants/pokedex.h include/constants/pokemon.h include/constants/region_map_sections.h include/constants/regions.h include/constants/rematches.h include/constants/rgb.h include/constants/siirtc.h include/constants/songs.h include/constants/sound.h include/constants/species.h include/constants/tms_hms.h include/constants/trainer_hill.h include/constants/trainer_tower.h include/constants/trainers.h include/constants/tv.h include/constants/vars.h include/constants/vars_frlg.h include/contest_effect.h include/data.h include/debug.h include/decompress.h include/difficulty.h include/event_data.h include/event_object_movement.h include/field_camera.h include/field_control_avatar.h include/field_effect.h include/field_effect_helpers.h include/field_player_avatar.h include/field_screen_effect.h include/field_weather.h include/fieldmap.h include/fldeff.h include/follower_npc.h include/fpmath.h include/gametypes.h include/gba/defines.h include/gba/gba.h include/gba/io_reg.h include/gba/isagbprint.h include/gba/macro.h include/gba/multiboot.h include/gba/syscall.h include/gba/types.h include/global.berry.h include/global.fieldmap.h include/global.h include/global.tv.h include/gpu_regs.h include/main.h include/malloc.h include/menu.h include/metaprogram.h include/metatile_behavior.h include/mirage_tower.h include/oras_dowse.h include/overworld.h include/palette.h include/party_menu.h include/pokemon.h include/pokemon_storage_system.h include/script.h include/siirtc.h include/sound.h include/sprite.h include/task.h include/test_result.h include/text.h include/trainer.h include/trainer_pokemon_sprites.h include/trig.h include/util.h include/wild_encounter_ow.h include/window.h
build-p3/assets/graphics/birch_speech/birch.png.4bpp:
build-p3/assets/graphics/birch_speech/birch.png.gbapal:
build-p3/assets/graphics/birch_speech/unused_beauty.png_num_tiles_822__Wnum_tiles.4bpp:
build-p3/assets/graphics/field_effects/palettes/hof_monitor.pal.gbapal:
build-p3/assets/graphics/field_effects/palettes/pokeball_glow.pal.gbapal:
build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_bottom_left.png.4bpp:
build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_bottom_right.png.4bpp:
build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_top_left.png.4bpp:
build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_top_right.png.4bpp:
build-p3/assets/graphics/field_effects/pics/field_move_streaks.png.4bpp:
build-p3/assets/graphics/field_effects/pics/field_move_streaks.png.gbapal:
build-p3/assets/graphics/field_effects/pics/field_move_streaks_indoors.png.4bpp:
build-p3/assets/graphics/field_effects/pics/field_move_streaks_indoors.png.gbapal:
build-p3/assets/graphics/field_effects/pics/hof_monitor_big.png.4bpp:
build-p3/assets/graphics/field_effects/pics/hof_monitor_frlg.png.4bpp:
build-p3/assets/graphics/field_effects/pics/hof_monitor_frlg.png.gbapal:
build-p3/assets/graphics/field_effects/pics/hof_monitor_small.png.4bpp:
build-p3/assets/graphics/field_effects/pics/pokeball_glow.png.4bpp:
build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/0.png.4bpp:
build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/1.png.4bpp:
build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/frlg.png.4bpp:
build-p3/assets/graphics/field_effects/pics/spotlight.png.4bpp:
build-p3/assets/graphics/field_effects/pics/spotlight.png.gbapal:
graphics/field_effects/pics/field_move_streaks.bin:
graphics/field_effects/pics/field_move_streaks_indoors.bin:
include/assertf.h:
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
include/constants/cries.h:
include/constants/difficulty.h:
include/constants/easy_chat.h:
include/constants/egg_ids.h:
include/constants/event_object_movement.h:
include/constants/event_objects.h:
include/constants/field_effects.h:
include/constants/field_weather.h:
include/constants/flags.h:
include/constants/follower_npc.h:
include/constants/form_change_types.h:
include/constants/game_stat.h:
include/constants/global.h:
include/constants/hold_effects.h:
include/constants/items.h:
include/constants/map_event_ids.h:
include/constants/map_groups.h:
include/constants/map_types.h:
include/constants/maps.h:
include/constants/metatile_behaviors.h:
include/constants/moves.h:
include/constants/opponents.h:
include/constants/opponents_frlg.h:
include/constants/pokeball.h:
include/constants/pokedex.h:
include/constants/pokemon.h:
include/constants/region_map_sections.h:
include/constants/regions.h:
include/constants/rematches.h:
include/constants/rgb.h:
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
include/data.h:
include/debug.h:
include/decompress.h:
include/difficulty.h:
include/event_data.h:
include/event_object_movement.h:
include/field_camera.h:
include/field_control_avatar.h:
include/field_effect.h:
include/field_effect_helpers.h:
include/field_player_avatar.h:
include/field_screen_effect.h:
include/field_weather.h:
include/fieldmap.h:
include/fldeff.h:
include/follower_npc.h:
include/fpmath.h:
include/gametypes.h:
include/gba/defines.h:
include/gba/gba.h:
include/gba/io_reg.h:
include/gba/isagbprint.h:
include/gba/macro.h:
include/gba/multiboot.h:
include/gba/syscall.h:
include/gba/types.h:
include/global.berry.h:
include/global.fieldmap.h:
include/global.h:
include/global.tv.h:
include/gpu_regs.h:
include/main.h:
include/malloc.h:
include/menu.h:
include/metaprogram.h:
include/metatile_behavior.h:
include/mirage_tower.h:
include/oras_dowse.h:
include/overworld.h:
include/palette.h:
include/party_menu.h:
include/pokemon.h:
include/pokemon_storage_system.h:
include/script.h:
include/siirtc.h:
include/sound.h:
include/sprite.h:
include/task.h:
include/test_result.h:
include/text.h:
include/trainer.h:
include/trainer_pokemon_sprites.h:
include/trig.h:
include/util.h:
include/wild_encounter_ow.h:
include/window.h:
ifndef build-p3/assets/graphics/birch_speech/birch.png.4bpp
build-p3/assets/graphics/birch_speech/birch.png.4bpp := defined
build-p3/assets/graphics/birch_speech/birch.png.4bpp: graphics/birch_speech/birch.png
	@mkdir -p 'build-p3/assets/graphics/birch_speech'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/birch_speech/birch.png.gbapal
build-p3/assets/graphics/birch_speech/birch.png.gbapal := defined
build-p3/assets/graphics/birch_speech/birch.png.gbapal: graphics/birch_speech/birch.png
	@mkdir -p 'build-p3/assets/graphics/birch_speech'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/birch_speech/unused_beauty.png_num_tiles_822__Wnum_tiles.4bpp
build-p3/assets/graphics/birch_speech/unused_beauty.png_num_tiles_822__Wnum_tiles.4bpp := defined
build-p3/assets/graphics/birch_speech/unused_beauty.png_num_tiles_822__Wnum_tiles.4bpp: graphics/birch_speech/unused_beauty.png
	@mkdir -p 'build-p3/assets/graphics/birch_speech'
	$(GFX) $< $@ -num_tiles 822 -Wnum_tiles
endif
ifndef build-p3/assets/graphics/field_effects/palettes/hof_monitor.pal.gbapal
build-p3/assets/graphics/field_effects/palettes/hof_monitor.pal.gbapal := defined
build-p3/assets/graphics/field_effects/palettes/hof_monitor.pal.gbapal: graphics/field_effects/palettes/hof_monitor.pal
	@mkdir -p 'build-p3/assets/graphics/field_effects/palettes'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/palettes/pokeball_glow.pal.gbapal
build-p3/assets/graphics/field_effects/palettes/pokeball_glow.pal.gbapal := defined
build-p3/assets/graphics/field_effects/palettes/pokeball_glow.pal.gbapal: graphics/field_effects/palettes/pokeball_glow.pal
	@mkdir -p 'build-p3/assets/graphics/field_effects/palettes'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_bottom_left.png.4bpp
build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_bottom_left.png.4bpp := defined
build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_bottom_left.png.4bpp: graphics/field_effects/pics/deoxys_rock_fragment_bottom_left.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_bottom_right.png.4bpp
build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_bottom_right.png.4bpp := defined
build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_bottom_right.png.4bpp: graphics/field_effects/pics/deoxys_rock_fragment_bottom_right.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_top_left.png.4bpp
build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_top_left.png.4bpp := defined
build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_top_left.png.4bpp: graphics/field_effects/pics/deoxys_rock_fragment_top_left.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_top_right.png.4bpp
build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_top_right.png.4bpp := defined
build-p3/assets/graphics/field_effects/pics/deoxys_rock_fragment_top_right.png.4bpp: graphics/field_effects/pics/deoxys_rock_fragment_top_right.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/field_move_streaks.png.4bpp
build-p3/assets/graphics/field_effects/pics/field_move_streaks.png.4bpp := defined
build-p3/assets/graphics/field_effects/pics/field_move_streaks.png.4bpp: graphics/field_effects/pics/field_move_streaks.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/field_move_streaks.png.gbapal
build-p3/assets/graphics/field_effects/pics/field_move_streaks.png.gbapal := defined
build-p3/assets/graphics/field_effects/pics/field_move_streaks.png.gbapal: graphics/field_effects/pics/field_move_streaks.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/field_move_streaks_indoors.png.4bpp
build-p3/assets/graphics/field_effects/pics/field_move_streaks_indoors.png.4bpp := defined
build-p3/assets/graphics/field_effects/pics/field_move_streaks_indoors.png.4bpp: graphics/field_effects/pics/field_move_streaks_indoors.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/field_move_streaks_indoors.png.gbapal
build-p3/assets/graphics/field_effects/pics/field_move_streaks_indoors.png.gbapal := defined
build-p3/assets/graphics/field_effects/pics/field_move_streaks_indoors.png.gbapal: graphics/field_effects/pics/field_move_streaks_indoors.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/hof_monitor_big.png.4bpp
build-p3/assets/graphics/field_effects/pics/hof_monitor_big.png.4bpp := defined
build-p3/assets/graphics/field_effects/pics/hof_monitor_big.png.4bpp: graphics/field_effects/pics/hof_monitor_big.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/hof_monitor_frlg.png.4bpp
build-p3/assets/graphics/field_effects/pics/hof_monitor_frlg.png.4bpp := defined
build-p3/assets/graphics/field_effects/pics/hof_monitor_frlg.png.4bpp: graphics/field_effects/pics/hof_monitor_frlg.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/hof_monitor_frlg.png.gbapal
build-p3/assets/graphics/field_effects/pics/hof_monitor_frlg.png.gbapal := defined
build-p3/assets/graphics/field_effects/pics/hof_monitor_frlg.png.gbapal: graphics/field_effects/pics/hof_monitor_frlg.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/hof_monitor_small.png.4bpp
build-p3/assets/graphics/field_effects/pics/hof_monitor_small.png.4bpp := defined
build-p3/assets/graphics/field_effects/pics/hof_monitor_small.png.4bpp: graphics/field_effects/pics/hof_monitor_small.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/pokeball_glow.png.4bpp
build-p3/assets/graphics/field_effects/pics/pokeball_glow.png.4bpp := defined
build-p3/assets/graphics/field_effects/pics/pokeball_glow.png.4bpp: graphics/field_effects/pics/pokeball_glow.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/0.png.4bpp
build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/0.png.4bpp := defined
build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/0.png.4bpp: graphics/field_effects/pics/pokecenter_monitor/0.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics/pokecenter_monitor'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/1.png.4bpp
build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/1.png.4bpp := defined
build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/1.png.4bpp: graphics/field_effects/pics/pokecenter_monitor/1.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics/pokecenter_monitor'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/frlg.png.4bpp
build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/frlg.png.4bpp := defined
build-p3/assets/graphics/field_effects/pics/pokecenter_monitor/frlg.png.4bpp: graphics/field_effects/pics/pokecenter_monitor/frlg.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics/pokecenter_monitor'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/spotlight.png.4bpp
build-p3/assets/graphics/field_effects/pics/spotlight.png.4bpp := defined
build-p3/assets/graphics/field_effects/pics/spotlight.png.4bpp: graphics/field_effects/pics/spotlight.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
ifndef build-p3/assets/graphics/field_effects/pics/spotlight.png.gbapal
build-p3/assets/graphics/field_effects/pics/spotlight.png.gbapal := defined
build-p3/assets/graphics/field_effects/pics/spotlight.png.gbapal: graphics/field_effects/pics/spotlight.png
	@mkdir -p 'build-p3/assets/graphics/field_effects/pics'
	$(GFX) $< $@ 
endif
