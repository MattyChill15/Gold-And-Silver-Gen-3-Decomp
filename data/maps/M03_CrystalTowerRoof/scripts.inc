M03_CrystalTowerRoof_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, M03_CrystalTowerRoof_OnFrame
	.byte 0

M03_CrystalTowerRoof_OnFrame::
	map_script_2 VAR_M03_SCENE, 11, M03_B11_EnteiForceLoss
	map_script_2 VAR_M03_SCENE, 12, M03_B12_CharizardSave
	map_script_2 VAR_M03_SCENE, 13, M03_B13_ClashAndTruce
	.2byte 0
