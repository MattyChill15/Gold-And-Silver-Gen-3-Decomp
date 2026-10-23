// EA-027 @p_Itzo; wet-ground conduction EA-011 @GajoeDraws.
// Battle-local integer simulation. Background pixels and damage use the SAME
// wet-cell map. No save fields, scripted victories or per-cell damage stacking.
#define STORM_W 26
#define STORM_H 12
#define STORM_CELLS (STORM_W * STORM_H)
#define STORM_DURATION 1440
#define STORM_BATCH 8
static const u32 sStormWater[] = INCBIN_U32("graphics/arena/storm/water.8bpp");
static const u8 sStormWaterSparse[] = INCBIN_U8("graphics/arena/storm/water-sparse.bin");
static const u16 sStormWaterOffsets[] = INCBIN_U16("graphics/arena/storm/water-offsets.bin");
static const u32 sStormDischarge[] = INCBIN_U32("graphics/arena/storm/discharge.8bpp");
extern const u32 gArenaStageFlood[];
extern const u8 gArenaWaterHeight[];
extern const u8 gArenaWaterHeightTiles[];
extern const u8 gArenaWaterHeightBounds[];
extern const u32 gArenaWaterFullMask[];
#define sStormBackgrounds gArenaStageFlood
static EWRAM_DATA struct {
    u16 age, life, oldWeather, paint, waveAge, move;
    u8 level, side, hit, dirty, colors[7];
    u8 wetLevel, distance[STORM_CELLS]; // wet/height derived from ROM height map (EWRAM budget)
    u8 rain[10];
    const u8 *base;
    u32 *tiles;
} sStorm = {};
static const u32 sStormRain[8]={0x00002000,0x00002000,0x00020000,0x00020000,0x00200000,0x00200000,0,0};
static void StormSprites(void)
{
    struct SpriteSheet sheet={sStormRain,sizeof(sStormRain),0xA7F8};
    struct SpriteTemplate t=sShotTemplate;u32 i;
    t.tileTag=0xA7F8;
    LoadSpriteSheet(&sheet);
    for(i=0;i<10;i++) {
        sStorm.rain[i]=CreateSprite(&t,0,0,12);
        if(sStorm.rain[i]<MAX_SPRITES)gSprites[sStorm.rain[i]].invisible=TRUE;
    }
}
// starts, active ticks, wet cells, waves, conducted hits, avoided, expiries,
// painted tiles, native damage resolutions, allocation-free simulation steps.
EWRAM_DATA u32 gArenaStormTelemetry[10] = {};
static void StormBiome(const u32 *tiles,const u16 *pal)
{
    static const u8 rgb[7][3]={{3,8,10},{4,11,13},{6,14,16},{9,18,19},{14,22,22},{20,26,24},{27,29,26}};
    u32 i;u16 colors[7];
    sStorm.base=(const u8*)tiles;sStorm.paint=150;sStorm.dirty=sStorm.life!=0;
    // A switch cancels attacks, not weather. Never resolve an old owner's
    // pending discharge with the replacement Pokemon's stats.
    sStorm.waveAge=0;
    if(!sStorm.tiles)sStorm.tiles=AllocZeroed(STORM_BATCH*64);
    for(i=0;i<7;i++) {
        sStorm.colors[i]=240+i;colors[i]=RGB(rgb[i][0],rgb[i][1],rgb[i][2]);
    }
    LoadPalette(colors,240,sizeof(colors));
}
static void StormReset(void)
{
    // CB2_InitBattle has already reset the native heap before ResetBattle.
    // Never Free the previous encounter's now-stale pointer here.
    memset(&sStorm,0,sizeof(sStorm));memset(gArenaStormTelemetry,0,sizeof(gArenaStormTelemetry));
}
static u8 StormCellHeight(u32 i)
{return gArenaWaterHeight[(44+(i/STORM_W)*8)*240+20+(i%STORM_W)*8];}
static bool8 StormCellWet(u32 i){return sStorm.wetLevel>StormCellHeight(i);}
static s16 StormCell(s16 x,s16 y)
{
    if(x<16||x>=224||y<40||y>=136)return -1;
    return (y-40)/8*STORM_W+(x-16)/8;
}
static bool8 StormWet(s16 x,s16 y)
{return StormCell(x,y)>=0&&sStorm.life&&gArenaWaterHeight[y*240+x]<sStorm.level;}
bool8 RealtimeArena_IsFlooded(s16 x,s16 y)
{return sStorm.life && StormWet(x,y);}
static bool8 StormActive(void){return sStorm.life!=0;}
static u16 StormSpeed(u8 side,u16 speed)
{
    if(sStorm.life&&StormWet(sArena.bodies[side].x/Q,sArena.bodies[side].y/Q)
        &&ElementGrounded(side)&&!IS_BATTLER_OF_TYPE(side,TYPE_WATER))return speed*3/4;
    return speed;
}
static bool8 StormEvade(void)
{
    static const s16 refuge[4][2]={{48,112},{192,56},{120,32},{120,141}};
    u32 i; s32 best=0x7fffffff;
    struct ArenaBody *b=&sArena.bodies[1];
    if(!sStorm.waveAge||sStorm.side!=0||!ElementGrounded(1)||!StormWet(b->x/Q,b->y/Q))return FALSE;
    for(i=0;i<4;i++) {
        s32 d=ArenaNav_Distance(b->x/Q,b->y/Q,refuge[i][0],refuge[i][1]);
        if(d<best&&ArenaNav_CanStand(refuge[i][0],refuge[i][1])) {
            best=d;sArena.goal.x=refuge[i][0];sArena.goal.y=refuge[i][1];
        }
    }
    if(best==0x7fffffff)return FALSE;
    sArena.aiState=AI_EVADE;sArena.goalTimer=40;AiRoute();return TRUE;
}
static void StormStart(void)
{
    u32 i;
    if(!sStorm.tiles)return; // Never enable invisible hazards on allocation failure.
    if(!sStorm.life)sStorm.oldWeather=gBattleWeather;
    sStorm.age=0;sStorm.life=STORM_DURATION;sStorm.dirty=TRUE;
    gBattleWeather=B_WEATHER_RAIN_TEMPORARY;
    gWishFutureKnock.weatherDuration=5;
    gArenaStormTelemetry[0]++;PlaySE(SE_M_RAIN_DANCE);
}
static void StormMap(void)
{
    u32 i,count=0;
    sStorm.wetLevel=sStorm.level;
    for(i=0;i<STORM_CELLS;i++)count+=StormCellWet(i);
    gArenaStormTelemetry[2]=count;
}
static bool8 StormCharge(u8 side,u16 move,s16 x,s16 y)
{
    s16 cell=StormCell(x,y);u32 head=0,tail=0;
    u16 queue[STORM_CELLS];
    if(!sStorm.life||sStorm.waveAge||cell<0||!StormWet(x,y))return FALSE;
    memset(sStorm.distance,255,sizeof(sStorm.distance));
    sStorm.distance[cell]=0;queue[tail++]=cell;
    while(head<tail) {
        u16 p=queue[head++];s16 neighbors[4];u32 j;
        neighbors[0]=(p%STORM_W)?p-1:-1;
        neighbors[1]=(p%STORM_W<STORM_W-1)?p+1:-1;
        neighbors[2]=p>=STORM_W?p-STORM_W:-1;
        neighbors[3]=p+STORM_W<STORM_CELLS?p+STORM_W:-1;
        for(j=0;j<4;j++) {
            s16 n=neighbors[j];
            if(n>=0&&StormCellWet(n)&&sStorm.distance[n]==255) {
                sStorm.distance[n]=sStorm.distance[p]+1;queue[tail++]=n;
            }
        }
    }
    sStorm.side=side;sStorm.move=move;sStorm.waveAge=1;sStorm.hit=FALSE;
    gArenaStormTelemetry[3]++;return TRUE;
}
static void StormTick(void)
{
    u8 target=sStorm.side^1;
    if(!sStorm.life)return;
    sStorm.age++;sStorm.life--;gArenaStormTelemetry[1]++;
    sStorm.level=min(48,min(sStorm.age/4,sStorm.life/3));
    if(!(sStorm.age%8))StormMap();
    if(sStorm.waveAge) {
        s16 cell=StormCell(sArena.bodies[target].x/Q,sArena.bodies[target].y/Q);
        sStorm.waveAge++;
        if(!sStorm.hit&&cell>=0&&StormWet(sArena.bodies[target].x/Q,sArena.bodies[target].y/Q)&&sStorm.distance[cell]!=255
            &&sStorm.waveAge>=18+sStorm.distance[cell]*2
            &&sStorm.waveAge<34+sStorm.distance[cell]*2
            &&ElementGrounded(target)&&!SpecialInvulnerable(target)&&!sArena.bodies[target].dash) {
            s32 damage=0;
            bool8 blocked=GuardIntercept(sStorm.side,sStorm.move);
            if(!blocked)damage=RealtimeArena_ResolveDamage(sStorm.side,target,sStorm.move);
            gArenaStormTelemetry[8]++;
            if(damage>0)damage=max(1,damage/2);
            if(!blocked)ApplyResolvedHit(sStorm.side,sStorm.move,damage);
            gArenaStormTelemetry[4]++;sStorm.hit=TRUE;
            PlaySE(SE_M_THUNDERBOLT);
        }
        if(sStorm.waveAge>115){if(!sStorm.hit)gArenaStormTelemetry[5]++;sStorm.waveAge=0;}
    }
    if(!sStorm.life) {
        gBattleWeather=sStorm.oldWeather;sStorm.wetLevel=0;
        gArenaStormTelemetry[2]=0;
        sStorm.waveAge=0;sStorm.paint=150;sStorm.dirty=TRUE;gArenaStormTelemetry[6]++;
    }
    gArenaStormTelemetry[9]++;
}
// Stream a bounded batch of original 8bpp BG tiles through the existing VBlank
// uploader. No additional BG/OBJ palette, OAM slot or VRAM tile is needed.
static void StormDraw(u16 budget)
{
    u32 t,p,first,started,count=STORM_BATCH;u8 *out=(u8*)sStorm.tiles;
    for(t=0;t<10;t++)if(sStorm.rain[t]<MAX_SPRITES) {
        struct Sprite *s=&gSprites[sStorm.rain[t]];
        u32 phase=(sStorm.age*5+t*31)%112;
        s->invisible=!sStorm.life||sArena.toss.space;
        s->x=16+(t*47+sStorm.age/3)%208;s->y=24+phase;
        s->oam.priority=1;
    }
    if(!sStorm.base||!out||sArena.paused||sArena.toss.space)return;
    if(!sStorm.life&&!sStorm.dirty)return;
    // Gameplay/AI always wins the frame budget. Resume the cosmetic tile batch
    // next frame, instead of turning a shallow-water effect into a 30fps tax.
    if(budget<12)return;
    started=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
    first=sStorm.paint;count=min(count,510-first);
    for(t=0;t<count;t++) {
        u32 tile=first+t,tx=tile%30,ty=tile/30;
        s16 x=tx*8,y=ty*8,cell=StormCell(x,y),phase=-100;
        u8 *dest=out+t*64;
        bool8 wet=cell>=0&&sStorm.life&&gArenaWaterHeightBounds[tile*2]<sStorm.level;
        const u8 *dry=(const u8*)sStormBackgrounds+gArenaStage*76800+tile*64;
        const u8 *base=dry+(sStorm.life&&wet?38400:0);
        if(sStorm.life&&wet) {
            u32 pattern=((sStorm.age/8)&15)*128+(ty&7)*16+(tx&15);
            const u8 *water=(const u8*)sStormWater+pattern*64;
            const u8 *heights=gArenaWaterHeightTiles+tile*64;
            bool8 full=gArenaWaterHeightBounds[tile*2+1]+3<sStorm.level;
            if(sStorm.waveAge&&sStorm.distance[cell]!=255)phase=sStorm.waveAge-18-sStorm.distance[cell]*2;
            if(phase>=-12&&phase<28) {
                u32 frame=min(15,(phase+12)*16/40);
                water=(const u8*)sStormDischarge+(frame*128+(ty&7)*16+(tx&15))*64;
            }
            if((full || sStorm.level==48) && (phase < -12 || phase>=28)) {
                const u8 *marks=sStormWaterSparse+sStormWaterOffsets[pattern];
                u32 n=*marks++;
                if(full)CpuCopy16(base,dest,64);
                else {
                    const u32 *mask=gArenaWaterFullMask+tile*16;
                    for(p=0;p<16;p++)((u32*)dest)[p]=(((const u32*)dry)[p]&~mask[p])|(((const u32*)base)[p]&mask[p]);
                }
                for(p=0;p<n;p++){if(full || heights[marks[0]]+3<sStorm.level)dest[marks[0]]=marks[1];marks+=2;}
            } else for(p=0;p<64;p++) {
                u32 px=x+(p&7),py=y+(p>>3);
                s16 depth=sStorm.level-heights[p];
                // Palette-paired translucency preserves every terrain pixel.
                // A narrow ordered shoreline softens the edge, not the floor.
                if(depth<=0||(depth<3&&((px+py)&3)>=depth))dest[p]=dry[p];
                else dest[p]=depth>3&&water[p]?water[p]:base[p];
            }
        } else {
            CpuCopy16(base,dest,64);
        }
        if(gMain.vblankCounter1*228+(REG_VCOUNT+68)%228-started>=budget-8){count=t+1;break;}
    }
    ArenaRender_Copy(sStorm.tiles,(u8*)BG_VRAM+0x4000+first*64,count*64);
    sStorm.paint+=count;gArenaStormTelemetry[7]+=count;
    if(sStorm.paint==510){sStorm.paint=150;if(!sStorm.life)sStorm.dirty=FALSE;}
}
