M04_Ferry_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, M04_Ferry_OnFrame
	.byte 0

M04_Ferry_OnFrame::
	map_script_2 VAR_M04_SCENE, 19, M04_B03_Dundee
	map_script_2 VAR_M04_SCENE, 20, M04_B04_Suicune
	.2byte 0
