// Opposing Flamethrowers cancel at their shared contact point. Presentation
// never owns damage: spent attacks consume their original PP, with no bonus hit.
static EWRAM_DATA struct {u16 age;u8 active,reach[2];s16 x,y;} sFlameClash={};
// starts, active ticks, explosions, finishes, contact x/y, phase, spare
EWRAM_DATA u32 gArenaClashTelemetry[8]={};
static bool8 ClashBusy(void){return sFlameClash.active;}
static void ClashReset(void)
{memset(&sFlameClash,0,sizeof(sFlameClash));memset(gArenaClashTelemetry,0,sizeof(gArenaClashTelemetry));}
static void ClashDetect(void)
{
    struct ArenaBody *a=&sArena.bodies[0],*b=&sArena.bodies[1];
    s16 dx=(b->x-a->x)/Q,dy=(b->y-a->y)/Q;
    u32 i;u16 distance=ArenaNav_Distance(a->x/Q,a->y/Q,b->x/Q,b->y/Q);
    if(!ArenaMoveFx_ClashReady()||sFlameClash.active||!a->actionLife||!b->actionLife||a->connected||b->connected
       ||ArenaMoveId(0,a->shotSlot)!=MOVE_FLAMETHROWER
       ||ArenaMoveId(1,b->shotSlot)!=MOVE_FLAMETHROWER
       ||distance<40||distance>180||SpecialBusy(0)||SpecialBusy(1)
       ||a->attackX*b->attackX+a->attackY*b->attackY>-50000)return;
    if(dx*a->attackX+dy*a->attackY<=0
       ||Abs(dx*a->attackY-dy*a->attackX)>20*Q
       ||!ArenaNav_LineClear(a->x/Q,a->y/Q,b->x/Q,b->y/Q,1))return;
    if(min(120,20+a->actionAge*7)+min(120,20+b->actionAge*7)<distance)return;
    sFlameClash.active=TRUE;sFlameClash.age=0;
    sFlameClash.x=(a->x+b->x)/(2*Q);sFlameClash.y=(a->y+b->y)/(2*Q)-8;
    for(i=0;i<2;i++)
    {
        struct ArenaBody *body=&sArena.bodies[i];
        sFlameClash.reach[i]=max(8,distance/2-5);
        sFlameReach[i]=sFlameClash.reach[i];body->connected=TRUE;
        body->knockX=body->knockY=0;body->moving=FALSE;
    }
    gArenaClashTelemetry[0]++;gArenaClashTelemetry[4]=sFlameClash.x;gArenaClashTelemetry[5]=sFlameClash.y;
    PlaySE(SE_M_FLAMETHROWER);
}
static void ClashTick(void)
{
    u32 i;if(!sFlameClash.active)return;
    sFlameClash.age++;gArenaClashTelemetry[1]++;
    gArenaClashTelemetry[6]=sFlameClash.age<80?1:2;
    if(sFlameClash.age==80)
    {
        for(i=0;i<2;i++){sArena.bodies[i].actionLife=0;sArena.bodies[i].shotTimer=0;sFlameReach[i]=0;}
        gArenaClashTelemetry[2]++;PlaySE(SE_M_EXPLOSION);
        ArenaFeedback_Embers(sFlameClash.x,sFlameClash.y,256,0);
        ArenaFeedback_Embers(sFlameClash.x,sFlameClash.y,-256,0);
    }
    if(sFlameClash.age>=128)
    {
        sFlameClash.active=FALSE;gArenaClashTelemetry[3]++;gArenaClashTelemetry[6]=0;
        for(i=0;i<2;i++)sArena.bodies[i].cooldown=60;
        sArena.attackBuffer=sArena.dashBuffer=0;
    }
}
static void ClashDraw(void)
{
    ArenaMoveFx_Clash(sFlameClash.x,sFlameClash.y,sArena.bodies[0].shotFacing,min(31,sFlameClash.age/4),sFlameClash.active&&!sArena.paused);
    if(sFlameClash.active)
    {
        u8 age=sFlameClash.age;
        // A short, bounded impact tremor, not a persistent camera offset.
        SetGpuReg(REG_OFFSET_BG1VOFS,!sArena.paused&&age>=80&&age<92
            ? ((age&1)?2:0x1FE):0);
    }
}
