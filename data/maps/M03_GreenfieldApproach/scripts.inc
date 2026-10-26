M03_GreenfieldApproach_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, M03_GreenfieldApproach_OnFrame
	.byte 0

M03_GreenfieldApproach_OnFrame::
	map_script_2 VAR_M03_SCENE, 3, M03_B03_LisaChallenge
	map_script_2 VAR_M03_SCENE, 4, M03_B04_CrystalTown
	map_script_2 VAR_M03_SCENE, 5, M03_B05_EnteiTakesDelia
	map_script_2 VAR_M03_SCENE, 7, M03_B07_Approach
	.2byte 0
