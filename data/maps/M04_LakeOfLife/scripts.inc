M04_LakeOfLife_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, M04_LakeOfLife_OnFrame
	.byte 0

M04_LakeOfLife_OnFrame::
	map_script_2 VAR_M04_SCENE, 27, M04_B11_Lake
	map_script_2 VAR_M04_SCENE, 28, M04_B12_Mech
	map_script_2 VAR_M04_SCENE, 29, M04_B13_Onix
	map_script_2 VAR_M04_SCENE, 30, M04_B14_DarkBall
	map_script_2 VAR_M04_SCENE, 31, M04_B15_Revive
	map_script_2 VAR_M04_SCENE, 32, M04_B16_Unmask
	.2byte 0
