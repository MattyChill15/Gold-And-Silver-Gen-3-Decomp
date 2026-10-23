// Matheus Lynar's playtesting: clear pause navigation and non-blocking EXP.
static const u8 sQualityBag[] = _("BAG");
static const u8 sQualityParty[] = _("POKEMON");
static const u8 sQualityBagHelp[] = _("USE ITEMS FROM YOUR BAG");
static const u8 sQualityPartyHelp[] = _("CHOOSE YOUR NEXT POKEMON");
static const u8 sQualityConfirm[] = _("A: OPEN   B: BACK TO BATTLE");
static const u8 sQualityTabs[] = _("L/R: MOVES  BAG  POKEMON");
static const u8 sQualityExp[] = _(" EXP");
static EWRAM_DATA u32 sFieldExp[PARTY_SIZE]={};
static EWRAM_DATA u8 sFieldLevel[PARTY_SIZE]={};
static EWRAM_DATA u8 sFieldCaughtName[POKEMON_NAME_LENGTH+1]={};
static EWRAM_DATA u8 sFieldCaught=0;
static const u8 sQualityLevelUp[] = _("LEVEL UP! LVL ");
static const u8 sQualityCaughtParty[] = _("CAUGHT / PARTY");
static const u8 sQualityCaughtPC[] = _("CAUGHT / PC");
EWRAM_DATA u32 gArenaQualityTelemetry[4]={}; // queued XP, shown XP, toasts, caught
#define EXP_TAG 0xA7C0
static const struct OamData sExpOam={.shape=SPRITE_SHAPE(64x64),.size=SPRITE_SIZE(64x64),.priority=0};
static void ExpSpriteTick(struct Sprite *sprite)
{
    if(gMain.callback2!=CB2_Overworld || ArePlayerFieldControlsLocked()
        || ScriptContext_IsEnabled() || !sprite->data[0]--)
    {
        u8 side=sprite->data[1];
        DestroySprite(sprite);
        FreeSpriteTilesByTag(EXP_TAG+side);
        if(side==1)FreeSpritePaletteByTag(EXP_TAG);
    }
}
static const struct SpriteTemplate sExpTemplate={.tileTag=EXP_TAG,.paletteTag=EXP_TAG,
    .oam=&sExpOam,.anims=gDummySpriteAnimTable,.images=NULL,
    .affineAnims=gDummySpriteAffineAnimTable,.callback=ExpSpriteTick};
void RealtimeArena_QueueExp(u8 party,u32 amount)
{
    if(party>=PARTY_SIZE || !amount)return;
    sFieldExp[party]+=amount;
    gArenaQualityTelemetry[0]+=amount;
}
void RealtimeArena_QueueLevel(u8 party,u8 level)
{if(party<PARTY_SIZE)sFieldLevel[party]=level;}
static void QueueCaught(struct Pokemon *mon,u8 destination)
{
    GetMonData(mon,MON_DATA_NICKNAME,sFieldCaughtName);
    sFieldCaughtName[POKEMON_NAME_LENGTH]=EOS;
    sFieldCaught=destination==MON_GIVEN_TO_PARTY?1:2;
}
// Series v1: ONE battle-summary card per win (all EXP, level-ups, items
// used and the catch) instead of one toast per party member.
static const u8 sQualityCard[] = _("BATTLE SUMMARY");
static const u8 sQualityItems[] = _("ITEMS USED ");
static const u8 sQualityLv[] = _(" LV");
EWRAM_DATA u8 gArenaItemsUsed=0;
static void CardPixel(u32 *tiles,u32 x,u32 y,u8 color)
{
    u32 i=(x/64)*512+((y/8)*8+(x%64)/8)*8+y%8;
    u32 shift=(x&7)*4;
    tiles[i]=(tiles[i]&~(15u<<shift))|((u32)color<<shift);
}
static void CardText(u32 *tiles,u32 x,u32 y,const u8 *text)
{
    while(*text!=EOS && x<124)
    {
        u8 c=*text++;u32 bits=0,row,col;
        if(c>=CHAR_0&&c<=CHAR_9)bits=sHudTinyGlyphs[c-CHAR_0];
        else if(c>=CHAR_A&&c<=CHAR_Z)bits=sHudTinyGlyphs[10+c-CHAR_A];
        else if(c>=CHAR_a&&c<=CHAR_z)bits=sHudTinyGlyphs[10+c-CHAR_a];
        else if(c==CHAR_PLUS)bits=0x05D0;
        for(row=0;row<5;row++)for(col=0;col<3;col++)
            if(bits&(1<<(14-row*3-col)))CardPixel(tiles,x+col,y+row,2);
        x+=4;
    }
}
EWRAM_DATA u32 gArenaSummaryCards=0;
static void ArenaExpFieldTick(void)
{
    u32 i,x,y,lv=0;u32 *tiles;u8 text[40],*end,ids[2];
    static const u16 palette[16]={0,RGB(3,7,10),RGB(29,31,29),RGB(8,29,23)};
    struct SpritePalette pal={palette,EXP_TAG};
    ArenaSeries_FieldTick();
    if(gPaletteFade.active || ArePlayerFieldControlsLocked() || ScriptContext_IsEnabled()
        || GetSpriteTileStartByTag(EXP_TAG)!=0xFFFF)return;
    for(i=0;i<PARTY_SIZE;i++)if(sFieldExp[i]||sFieldLevel[i])break;
    if(i==PARTY_SIZE && !sFieldCaught && !gArenaSeriesToast)return;
    tiles=AllocZeroed(4096);
    if(!tiles)return;
    for(y=1;y<63;y++)for(x=1;x<127;x++)CardPixel(tiles,x,y,1);
    for(x=3;x<125;x++)CardPixel(tiles,x,62,3);
    y=3;
    if(gArenaSeriesToast)
    {
        CardText(tiles,6,y,gArenaSeriesToast[0]);y+=9;
        CardText(tiles,6,y,gArenaSeriesToast[1]);y+=9;
        if(gArenaSeriesToast[2])CardText(tiles,6,y,gArenaSeriesToast[2]);
    }
    else
    {
    CardText(tiles,6,y,sQualityCard);y+=8;
    for(i=0;i<PARTY_SIZE;i++)
    {
        if(!sFieldExp[i]&&!sFieldLevel[i])continue;
        GetMonData(&gPlayerParty[i],MON_DATA_NICKNAME,text);text[10]=EOS;
        CardText(tiles,6,y,text);
        text[0]=CHAR_PLUS;end=ConvertIntToDecimalStringN(text+1,sFieldExp[i],STR_CONV_MODE_LEFT_ALIGN,6);
        StringCopy(end,sQualityExp);CardText(tiles,48,y,text);
        if(sFieldLevel[i])
        {
            ConvertIntToDecimalStringN(StringCopy(text,sQualityLv),sFieldLevel[i],STR_CONV_MODE_LEFT_ALIGN,3);
            CardText(tiles,92,y,text);lv=1;
        }
        y+=7;
    }
    if(gArenaItemsUsed)
    {
        ConvertIntToDecimalStringN(StringCopy(text,sQualityItems),gArenaItemsUsed,STR_CONV_MODE_LEFT_ALIGN,3);
        CardText(tiles,6,y,text);y+=7;
    }
    if(sFieldCaught && y<57)
    {
        CardText(tiles,6,y,sFieldCaughtName);
        CardText(tiles,56,y,sFieldCaught==1?sQualityCaughtParty:sQualityCaughtPC);
    }
    }
    if(LoadSpritePalette(&pal)==0xFF){Free(tiles);return;}
    for(x=0;x<2;x++)
    {
        struct SpriteSheet sheet={tiles+x*512,2048,EXP_TAG+x};
        struct SpriteTemplate t=sExpTemplate;t.tileTag+=x;
        if(LoadSpriteSheet(&sheet)==0xFFFF)break;
        ids[x]=CreateSprite(&t,88+x*64,40,0);
        if(ids[x]==MAX_SPRITES){FreeSpriteTilesByTag(EXP_TAG+x);break;}
        gSprites[ids[x]].data[0]=gArenaSeriesToast?300:200;gSprites[ids[x]].data[1]=x;
    }
    Free(tiles);
    if(x!=2)
    {
        if(x){DestroySprite(&gSprites[ids[0]]);FreeSpriteTilesByTag(EXP_TAG);}
        FreeSpritePaletteByTag(EXP_TAG);return;
    }
    if(gArenaSeriesToast){gArenaSeriesToast=NULL;PlayFanfare(MUS_OBTAIN_ITEM);gArenaQualityTelemetry[2]++;return;}
    if(lv)PlayFanfare(MUS_LEVEL_UP);
    for(i=0;i<PARTY_SIZE;i++){gArenaQualityTelemetry[1]+=sFieldExp[i];sFieldExp[i]=0;sFieldLevel[i]=0;}
    sFieldCaught=0;gArenaItemsUsed=0;gArenaSummaryCards++;
    gArenaQualityTelemetry[2]++;
}
