// Protect only prevents damage. Hyper Beam disintegration is presentation,
// never a reflected attack, extra damage, PP charge or offensive reward.
static EWRAM_DATA u8 sGuardLife[2]={},sGuardAge[2]={};
static EWRAM_DATA struct {
    u8 life,age,side,dir;
    s16 x,y;
} sGuardAbsorb={};
// casts, blocks, dissolves, reflected attacks (always 0), reflected damage
// events (always 0), visual age, visual life, expiries.
EWRAM_DATA u32 gArenaGuardTelemetry[8]={};
static bool8 GuardBusy(void){return sGuardAbsorb.life!=0;}
static void GuardReset(void)
{
    memset(sGuardLife,0,sizeof(sGuardLife));
    memset(sGuardAge,0,sizeof(sGuardAge));
    memset(&sGuardAbsorb,0,sizeof(sGuardAbsorb));
    memset(gArenaGuardTelemetry,0,sizeof(gArenaGuardTelemetry));
}
static void GuardStart(u8 side)
{
    sGuardLife[side]=60;sGuardAge[side]=0;
    gArenaGuardTelemetry[0]++;PlaySE(SE_M_REFLECT);
}
static void GuardRebind(void)
{
    u32 i;
    for(i=0;i<2;i++)if(sArenaSendoutMask&(1<<i))sGuardLife[i]=sGuardAge[i]=0;
    // Menus are disallowed during absorption; no stale cosmetic continuation.
    memset(&sGuardAbsorb,0,sizeof(sGuardAbsorb));
}
static bool8 GuardIntercept(u8 side,u16 move)
{
    u8 defender=side^1;
    struct ArenaBody *a=&sArena.bodies[side],*d=&sArena.bodies[defender];
    if(!sGuardLife[defender])return FALSE;
    gArenaGuardTelemetry[1]++;
    ArenaFeedback_Wall(d->x/Q,d->y/Q-10);
    if(move==MOVE_HYPER_BEAM&&!sGuardAbsorb.life)
    {
        sGuardAbsorb.life=64;sGuardAbsorb.age=0;sGuardAbsorb.side=defender;
        sGuardAbsorb.x=d->x/Q;sGuardAbsorb.y=d->y/Q;
        sGuardAbsorb.dir=(a->shotFacing+4)%8;
        // The spent shot is stopped at the shield, not redirected. Preserve
        // normal recovery and the original Protect duration/PP contract.
        a->actionLife=0;a->shotTimer=0;a->cooldown=180;
        gArenaGuardTelemetry[2]++;sArena.hitstop=3;
    }
    PlaySE(SE_M_REFLECT);
    return TRUE;
}
static void GuardTick(void)
{
    u32 i;
    for(i=0;i<2;i++)if(sGuardLife[i])
    {sGuardAge[i]++;if(!--sGuardLife[i])gArenaGuardTelemetry[7]++;}
    if(sGuardAbsorb.life){sGuardAbsorb.age++;sGuardAbsorb.life--;}
    gArenaGuardTelemetry[5]=sGuardAbsorb.age;
    gArenaGuardTelemetry[6]=sGuardAbsorb.life;
}
static void GuardDraw(void)
{
    u32 i;
    if(sArena.paused)return;
    for(i=0;i<2;i++)
    {
        struct ArenaBody *b=&sArena.bodies[i];
        if(sGuardLife[i])ArenaMoveFx_Guard(i,b->x/Q,b->y/Q-8,
            sGuardAge[i]<16?sGuardAge[i]/4:4+((sGuardAge[i]-16)/5)%4,4);
        else if(b->shotTimer&&ArenaMoveId(i,b->shotSlot)==MOVE_HYPER_BEAM&&b->shotElapsed<96)
            ArenaMoveFx_Guard(i,b->x/Q,b->y/Q-12,24+min(7,b->shotElapsed/12),4);
    }
    if(sGuardAbsorb.life)
    {
        u8 side=sGuardAbsorb.side,age=sGuardAbsorb.age;
        if(age<30)
        {
            struct ArenaBody *source=&sArena.bodies[side^1];
            s16 dx=Abs(source->x/Q-sGuardAbsorb.x),dy=Abs(source->y/Q-sGuardAbsorb.y);
            u8 reach=min(120,max(8,max(dx,dy)+min(dx,dy)/2-17));
            // Fade in place, never travel back toward the attacker.
            ArenaMoveFx_Flame(side^1,MOVE_HYPER_BEAM,source->x/Q,source->y/Q-8,
                (sGuardAbsorb.dir+4)%8,age<18?12+age:66+(age-18)/2,reach,TRUE,FALSE);
        }
        ArenaMoveFx_Guard(side,sGuardAbsorb.x,sGuardAbsorb.y-8,
            8+min(15,age/4),sGuardAbsorb.dir);
    }
}
