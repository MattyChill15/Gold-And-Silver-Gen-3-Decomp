M03_UnownChamber_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, M03_UnownChamber_OnFrame
	.byte 0

M03_UnownChamber_OnFrame::
	map_script_2 VAR_M03_SCENE, 2, M03_B02_UnownBox
	map_script_2 VAR_M03_SCENE, 14, M03_B14_UnownFarewell
	.2byte 0
