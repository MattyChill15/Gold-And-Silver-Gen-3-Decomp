M03_CrystalHallWater_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, M03_CrystalHallWater_OnFrame
	.byte 0

M03_CrystalHallWater_OnFrame::
	map_script_2 VAR_M03_SCENE, 9, M03_B09_BrockCutaway
	map_script_2 VAR_M03_SCENE, 10, M03_B10_MistyCutaway
	.2byte 0
