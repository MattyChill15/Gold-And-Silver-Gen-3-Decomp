M04_TowaHouse_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, M04_TowaHouse_OnFrame
	.byte 0

M04_TowaHouse_OnFrame::
	map_script_2 VAR_M04_SCENE, 23, M04_B07_Sketchbook
	map_script_2 VAR_M04_SCENE, 33, M04_B17_Epilogue
	.2byte 0
