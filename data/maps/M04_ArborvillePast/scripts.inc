M04_ArborvillePast_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, M04_ArborvillePast_OnFrame
	.byte 0

M04_ArborvillePast_OnFrame::
	map_script_2 VAR_M04_SCENE, 16, M04_PK10_PikaBoo
	map_script_2 VAR_M04_SCENE, 17, M04_B01_Prologue
	map_script_2 VAR_M04_SCENE, 18, M04_B02_DarkBall
	.2byte 0
