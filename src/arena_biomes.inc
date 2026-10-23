// Community scenery is separate from the five material palette biomes.
EWRAM_DATA u8 gArenaBiome = 0;

extern const u32 *const gArenaStageTiles[];
extern const u16 *const gArenaStagePalettes[];
#define sStageTiles gArenaStageTiles
#define sStagePalettes gArenaStagePalettes
EWRAM_DATA u8 gArenaStage = 0;
static void LoadArenaBiome(void)
{
    u16 map=gSaveBlock1Ptr->location.mapNum | (gSaveBlock1Ptr->location.mapGroup<<8);
    gArenaBiome=0;gArenaStage=0;
    switch(gBattleEnvironment)
    {
    case BATTLE_ENVIRONMENT_LONG_GRASS:gArenaStage=5;break;
    case BATTLE_ENVIRONMENT_UNDERWATER:
    case BATTLE_ENVIRONMENT_WATER:
    case BATTLE_ENVIRONMENT_POND:gArenaBiome=1;gArenaStage=1;break;
    case BATTLE_ENVIRONMENT_CAVE:
    case BATTLE_ENVIRONMENT_MOUNTAIN:gArenaBiome=2;gArenaStage=2;break;
    case BATTLE_ENVIRONMENT_SAND:gArenaBiome=3;gArenaStage=map==MAP_ROUTE111?3:6;break;
    case BATTLE_ENVIRONMENT_BUILDING:gArenaBiome=4;gArenaStage=4;break;
    case BATTLE_ENVIRONMENT_PLAIN:gArenaBiome=4;gArenaStage=7;break;
    }
    switch(map)
    {
    case MAP_PETALBURG_WOODS:gArenaBiome=0;gArenaStage=0;break;
    case MAP_MT_CHIMNEY:gArenaBiome=2;gArenaStage=16;break;
    case MAP_ROUTE119:
    case MAP_ROUTE120:if(gArenaBiome!=1){gArenaBiome=0;gArenaStage=5;}break;
    case MAP_ROUTE109:if(gArenaBiome!=1){gArenaBiome=3;gArenaStage=6;}break;
    case MAP_RUSTBORO_CITY_GYM:gArenaStage=8;break;
    case MAP_DEWFORD_TOWN_GYM:gArenaStage=9;break;
    case MAP_MAUVILLE_CITY_GYM:gArenaStage=10;break;
    case MAP_LAVARIDGE_TOWN_GYM_1F:
    case MAP_LAVARIDGE_TOWN_GYM_B1F:gArenaStage=11;break;
    case MAP_PETALBURG_CITY_GYM:gArenaStage=12;break;
    case MAP_FORTREE_CITY_GYM:gArenaStage=13;break;
    case MAP_MOSSDEEP_CITY_GYM:gArenaStage=14;break;
    case MAP_SOOTOPOLIS_CITY_GYM_1F:
    case MAP_SOOTOPOLIS_CITY_GYM_B1F:gArenaStage=15;break;
    }
    if(gBattleTypeFlags&BATTLE_TYPE_TRAINER)
    {
        u8 pic=gTrainers[gTrainerBattleOpponent_A].trainerPic;
        if(pic>=TRAINER_PIC_LEADER_ROXANNE&&pic<=TRAINER_PIC_LEADER_JUAN)
            gArenaStage=8+pic-TRAINER_PIC_LEADER_ROXANNE;
    }
    if(gArenaStage>=8&&gArenaStage<16)gArenaBiome=4;
    LoadPalette(sStagePalettes[gArenaStage],0,512);
    StormBiome(sStageTiles[gArenaStage],sStagePalettes[gArenaStage]);
    LoadBgTiles(1,sStageTiles[gArenaStage],38400,0);
    LoadBgTilemap(1,sForestMap,sizeof(sForestMap),0);
}
