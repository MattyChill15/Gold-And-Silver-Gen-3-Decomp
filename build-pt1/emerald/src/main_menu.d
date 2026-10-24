build-pt1/emerald/src/main_menu.o: build-pt1/assets/graphics/birch_speech/bg0.pal.gbapal build-pt1/assets/graphics/birch_speech/bg1.pal.gbapal build-pt1/assets/graphics/birch_speech/bg2.pal.gbapal build-pt1/assets/graphics/birch_speech/map.bin.smolTM build-pt1/assets/graphics/birch_speech/shadow.png.4bpp.smol build-pt1/assets/graphics/interface/main_menu_bg.pal.gbapal build-pt1/assets/graphics/interface/main_menu_text.pal.gbapal include/assertf.h include/bg.h include/config/ai.h include/config/battle.h include/config/caps.h include/config/contest.h include/config/debug.h include/config/dexnav.h include/config/follower_npc.h include/config/general.h include/config/item.h include/config/map_preview_screen.h include/config/overworld.h include/config/pokemon.h include/config/save.h include/config/species_enabled.h include/config/summary_screen.h include/config/test.h include/config/text.h include/config/wild_encounter.h include/constants/abilities.h include/constants/apricorn_tree.h include/constants/arena_story_flags.h include/constants/battle.h include/constants/battle_frontier_trainers.h include/constants/battle_partner.h include/constants/berries.h include/constants/berry.h include/constants/characters.h include/constants/cries.h include/constants/difficulty.h include/constants/easy_chat.h include/constants/egg_ids.h include/constants/field_weather.h include/constants/flags.h include/constants/form_change_types.h include/constants/game_stat.h include/constants/global.h include/constants/hold_effects.h include/constants/items.h include/constants/map_groups.h include/constants/map_types.h include/constants/maps.h include/constants/moves.h include/constants/opponents.h include/constants/opponents_frlg.h include/constants/pokeball.h include/constants/pokedex.h include/constants/pokemon.h include/constants/region_map_sections.h include/constants/regions.h include/constants/rematches.h include/constants/rgb.h include/constants/rtc.h include/constants/siirtc.h include/constants/songs.h include/constants/sound.h include/constants/species.h include/constants/tms_hms.h include/constants/trainer_hill.h include/constants/trainer_tower.h include/constants/trainers.h include/constants/tv.h include/constants/vars.h include/constants/vars_frlg.h include/contest_effect.h include/data.h include/debug.h include/decompress.h include/difficulty.h include/event_data.h include/field_effect.h include/field_weather.h include/fpmath.h include/gametypes.h include/gba/defines.h include/gba/gba.h include/gba/io_reg.h include/gba/isagbprint.h include/gba/macro.h include/gba/multiboot.h include/gba/syscall.h include/gba/types.h include/global.berry.h include/global.fieldmap.h include/global.h include/global.tv.h include/gpu_regs.h include/graphics.h include/international_string_util.h include/link.h include/list_menu.h include/load_save.h include/main.h include/main_menu.h include/menu.h include/metaprogram.h include/mystery_event_menu.h include/mystery_gift_menu.h include/naming_screen.h include/nuzlocke.h include/oak_speech.h include/option_menu.h include/overworld.h include/palette.h include/party_menu.h include/pokeball.h include/pokedex.h include/pokemon.h include/pokemon_storage_system.h include/random.h include/rtc.h include/save.h include/scanline_effect.h include/script.h include/siirtc.h include/sound.h include/sprite.h include/string_util.h include/strings.h include/task.h include/test_result.h include/text.h include/text_window.h include/title_screen.h include/trainer_pokemon_sprites.h include/tx_rac_menu.h include/tx_randomizer_and_challenges.h include/wild_encounter_ow.h include/window.h
build-pt1/emerald/src/main_menu.d: include/assertf.h include/bg.h include/config/ai.h include/config/battle.h include/config/caps.h include/config/contest.h include/config/debug.h include/config/dexnav.h include/config/follower_npc.h include/config/general.h include/config/item.h include/config/map_preview_screen.h include/config/overworld.h include/config/pokemon.h include/config/save.h include/config/species_enabled.h include/config/summary_screen.h include/config/test.h include/config/text.h include/config/wild_encounter.h include/constants/abilities.h include/constants/apricorn_tree.h include/constants/arena_story_flags.h include/constants/battle.h include/constants/battle_frontier_trainers.h include/constants/battle_partner.h include/constants/berries.h include/constants/berry.h include/constants/characters.h include/constants/cries.h include/constants/difficulty.h include/constants/easy_chat.h include/constants/egg_ids.h include/constants/field_weather.h include/constants/flags.h include/constants/form_change_types.h include/constants/game_stat.h include/constants/global.h include/constants/hold_effects.h include/constants/items.h include/constants/map_groups.h include/constants/map_types.h include/constants/maps.h include/constants/moves.h include/constants/opponents.h include/constants/opponents_frlg.h include/constants/pokeball.h include/constants/pokedex.h include/constants/pokemon.h include/constants/region_map_sections.h include/constants/regions.h include/constants/rematches.h include/constants/rgb.h include/constants/rtc.h include/constants/siirtc.h include/constants/songs.h include/constants/sound.h include/constants/species.h include/constants/tms_hms.h include/constants/trainer_hill.h include/constants/trainer_tower.h include/constants/trainers.h include/constants/tv.h include/constants/vars.h include/constants/vars_frlg.h include/contest_effect.h include/data.h include/debug.h include/decompress.h include/difficulty.h include/event_data.h include/field_effect.h include/field_weather.h include/fpmath.h include/gametypes.h include/gba/defines.h include/gba/gba.h include/gba/io_reg.h include/gba/isagbprint.h include/gba/macro.h include/gba/multiboot.h include/gba/syscall.h include/gba/types.h include/global.berry.h include/global.fieldmap.h include/global.h include/global.tv.h include/gpu_regs.h include/graphics.h include/international_string_util.h include/link.h include/list_menu.h include/load_save.h include/main.h include/main_menu.h include/menu.h include/metaprogram.h include/mystery_event_menu.h include/mystery_gift_menu.h include/naming_screen.h include/nuzlocke.h include/oak_speech.h include/option_menu.h include/overworld.h include/palette.h include/party_menu.h include/pokeball.h include/pokedex.h include/pokemon.h include/pokemon_storage_system.h include/random.h include/rtc.h include/save.h include/scanline_effect.h include/script.h include/siirtc.h include/sound.h include/sprite.h include/string_util.h include/strings.h include/task.h include/test_result.h include/text.h include/text_window.h include/title_screen.h include/trainer_pokemon_sprites.h include/tx_rac_menu.h include/tx_randomizer_and_challenges.h include/wild_encounter_ow.h include/window.h
build-pt1/assets/graphics/birch_speech/bg0.pal.gbapal:
build-pt1/assets/graphics/birch_speech/bg1.pal.gbapal:
build-pt1/assets/graphics/birch_speech/bg2.pal.gbapal:
build-pt1/assets/graphics/birch_speech/map.bin.smolTM:
build-pt1/assets/graphics/birch_speech/shadow.png.4bpp.smol:
build-pt1/assets/graphics/interface/main_menu_bg.pal.gbapal:
build-pt1/assets/graphics/interface/main_menu_text.pal.gbapal:
include/assertf.h:
include/bg.h:
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
include/constants/field_weather.h:
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
include/constants/pokeball.h:
include/constants/pokedex.h:
include/constants/pokemon.h:
include/constants/region_map_sections.h:
include/constants/regions.h:
include/constants/rematches.h:
include/constants/rgb.h:
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
include/data.h:
include/debug.h:
include/decompress.h:
include/difficulty.h:
include/event_data.h:
include/field_effect.h:
include/field_weather.h:
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
include/graphics.h:
include/international_string_util.h:
include/link.h:
include/list_menu.h:
include/load_save.h:
include/main.h:
include/main_menu.h:
include/menu.h:
include/metaprogram.h:
include/mystery_event_menu.h:
include/mystery_gift_menu.h:
include/naming_screen.h:
include/nuzlocke.h:
include/oak_speech.h:
include/option_menu.h:
include/overworld.h:
include/palette.h:
include/party_menu.h:
include/pokeball.h:
include/pokedex.h:
include/pokemon.h:
include/pokemon_storage_system.h:
include/random.h:
include/rtc.h:
include/save.h:
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
include/text_window.h:
include/title_screen.h:
include/trainer_pokemon_sprites.h:
include/tx_rac_menu.h:
include/tx_randomizer_and_challenges.h:
include/wild_encounter_ow.h:
include/window.h:
ifndef build-pt1/assets/graphics/birch_speech/bg0.pal.gbapal
build-pt1/assets/graphics/birch_speech/bg0.pal.gbapal := defined
build-pt1/assets/graphics/birch_speech/bg0.pal.gbapal: graphics/birch_speech/bg0.pal
	@mkdir -p 'build-pt1/assets/graphics/birch_speech'
	$(GFX) $< $@ 
endif
ifndef build-pt1/assets/graphics/birch_speech/bg1.pal.gbapal
build-pt1/assets/graphics/birch_speech/bg1.pal.gbapal := defined
build-pt1/assets/graphics/birch_speech/bg1.pal.gbapal: graphics/birch_speech/bg1.pal
	@mkdir -p 'build-pt1/assets/graphics/birch_speech'
	$(GFX) $< $@ 
endif
ifndef build-pt1/assets/graphics/birch_speech/bg2.pal.gbapal
build-pt1/assets/graphics/birch_speech/bg2.pal.gbapal := defined
build-pt1/assets/graphics/birch_speech/bg2.pal.gbapal: graphics/birch_speech/bg2.pal
	@mkdir -p 'build-pt1/assets/graphics/birch_speech'
	$(GFX) $< $@ 
endif
ifndef build-pt1/assets/graphics/birch_speech/map.bin.smolTM
build-pt1/assets/graphics/birch_speech/map.bin.smolTM := defined
build-pt1/assets/graphics/birch_speech/map.bin.smolTM: graphics/birch_speech/map.bin
	@mkdir -p 'build-pt1/assets/graphics/birch_speech'
	$(SMOLTM) $< $@ 
endif
ifndef build-pt1/assets/graphics/birch_speech/shadow.png.4bpp
build-pt1/assets/graphics/birch_speech/shadow.png.4bpp := defined
build-pt1/assets/graphics/birch_speech/shadow.png.4bpp: graphics/birch_speech/shadow.png
	@mkdir -p 'build-pt1/assets/graphics/birch_speech'
	$(GFX) $< $@ 
endif
ifndef build-pt1/assets/graphics/interface/main_menu_bg.pal.gbapal
build-pt1/assets/graphics/interface/main_menu_bg.pal.gbapal := defined
build-pt1/assets/graphics/interface/main_menu_bg.pal.gbapal: graphics/interface/main_menu_bg.pal
	@mkdir -p 'build-pt1/assets/graphics/interface'
	$(GFX) $< $@ 
endif
ifndef build-pt1/assets/graphics/interface/main_menu_text.pal.gbapal
build-pt1/assets/graphics/interface/main_menu_text.pal.gbapal := defined
build-pt1/assets/graphics/interface/main_menu_text.pal.gbapal: graphics/interface/main_menu_text.pal
	@mkdir -p 'build-pt1/assets/graphics/interface'
	$(GFX) $< $@ 
endif
