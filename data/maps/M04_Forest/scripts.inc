M04_Forest_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, M04_Forest_OnFrame
	.byte 0

M04_Forest_OnFrame::
	map_script_2 VAR_M04_SCENE, 24, M04_B08_Celebi
	map_script_2 VAR_M04_SCENE, 25, M04_B09_Marauder
	map_script_2 VAR_M04_SCENE, 26, M04_B10_Bayleef
	.2byte 0
