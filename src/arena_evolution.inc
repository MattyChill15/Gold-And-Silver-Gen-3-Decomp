// Native level-up handoff: experience is awarded exactly once by Cmd_getexp.
// Battle scripts are suspended, not replaced; their learn-move and reward paths
// resume after the transformation. No frame-time or per-hit experience grants.
#define EVOLUTION_TAG 0xA890
#define EVOLUTION_REVEAL 120
#define EVOLUTION_DURATION 240
#define EVOLUTION_SETTLE 60
#define EVOLUTION_LEADIN 45
static void DestroyShot(struct ArenaShot *shot);
static const u32 sEvolutionFrames[] = INCBIN_U32("graphics/arena/evolution/effect.4bpp");
static const u16 sEvolutionPalette[] = INCBIN_U16("graphics/arena/evolution/palette.gbapal");
static const u32 sSpectralEvolutionFrames[] = INCBIN_U32("graphics/arena/evolution/spectral.4bpp");
static const u16 sSpectralEvolutionPalette[] = INCBIN_U16("graphics/arena/evolution/spectral.gbapal");
static EWRAM_DATA struct {
    MainCallback callback1;
    const u32 *frames;
    u16 age, species, oldSpecies, hpBefore, savedPalette[16];
    u8 party, sprite, bank, frame, committed;
} sEvolution = {};
// starts, finishes, cancellations, before species, after species, HP before/after, age
EWRAM_DATA u16 gArenaEvolutionTelemetry[8] = {};

static void EvolutionPose(u16 age)
{
    struct ArenaBody *b = &sArena.bodies[0];
    struct Sprite *p = &gSprites[b->sprite];
    const struct ArenaSpriteAnimation *anim = &b->art->animations[ARENA_ANIM_IDLE];
    u8 frame = ArenaSprites_Frame(anim, age / 2);
    ArenaSprites_Decode(anim, b->facing, frame, sMonFrameTiles[0]);
    ArenaRender_Copy(sMonFrameTiles[0], (u8 *)OBJ_VRAM0 + GetSpriteTileStartByTag(MON_TAG)*32, 2048);
    p->x = b->x / Q;
    p->y = b->y / Q + 8 - b->art->ground[b->facing] * MonVisualScale(0) / 256;
    p->x2 = p->y2 = 0;
    p->invisible = FALSE;
    SetOamMatrix(p->oam.matrixNum, 65536 / MonVisualScale(0), 0, 0, 65536 / MonVisualScale(0));
}

static void EvolutionCommit(void)
{
    struct Pokemon *mon = &gPlayerParty[sEvolution.party];
    struct BattlePokemon *battle = &gBattleMons[0];
    struct ArenaBody *body = &sArena.bodies[0];
    u16 hp;
    u8 lastAbility = gLastUsedAbility;
    SetMonData(mon, MON_DATA_SPECIES, &sEvolution.species);
    CalculateMonStats(mon);
    EvolutionRenameMon(mon, sEvolution.oldSpecies, sEvolution.species);
    hp = GetMonData(mon, MON_DATA_MAX_HP);
    SetMonData(mon, MON_DATA_HP, &hp);
    battle->species = sEvolution.species;
    battle->hp = battle->maxHP = hp;
    battle->attack = GetMonData(mon, MON_DATA_ATK);
    battle->defense = GetMonData(mon, MON_DATA_DEF);
    battle->speed = GetMonData(mon, MON_DATA_SPEED);
    battle->spAttack = GetMonData(mon, MON_DATA_SPATK);
    battle->spDefense = GetMonData(mon, MON_DATA_SPDEF);
    battle->ability = GetMonAbility(mon);
    gLastUsedAbility = lastAbility;
    battle->types[0] = gSpeciesInfo[battle->species].types[0];
    battle->types[1] = gSpeciesInfo[battle->species].types[1];
    GetMonData(mon, MON_DATA_NICKNAME, battle->nickname);
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(battle->species), FLAG_SET_SEEN);
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(battle->species), FLAG_SET_CAUGHT);
    IncrementGameStat(GAME_STAT_EVOLVED_POKEMON);
    gLeveledUpInBattle &= ~(1u << sEvolution.party);
    body->art = ArenaSprites_Get(battle->species);
    body->drawnFrame = body->drawnDirection = 255;
    LoadPalette(body->art->palette, OBJ_PLTT_ID(0), 32);
    sEvolution.committed = TRUE;
    gArenaEvolutionTelemetry[1]++;
    gArenaEvolutionTelemetry[6] = hp;
    sArena.hudDirty = TRUE;
    DrawHud();
    PlayCry_Normal(battle->species, 0);
}

static void EvolutionFinish(bool8 canceled)
{
    if (sEvolution.sprite < MAX_SPRITES)
        DestroySprite(&gSprites[sEvolution.sprite]);
    FreeSpriteTilesByTag(EVOLUTION_TAG);
    LoadPalette(sEvolution.savedPalette, OBJ_PLTT_ID(sEvolution.bank), 32);
    BlendPalette(16, 240, 0, RGB_BLACK);
    BlendPalette(OBJ_PLTT_ID(0), 16, 0, RGB_WHITE);
    gSprites[sArena.bodies[0].sprite].y2 = 0;
    if (canceled)
    {
        gArenaEvolutionTelemetry[2]++;
        // Same promise as the original B-button cancellation: no second offer
        // at the end of this battle, but a later level-up can offer it again.
        gLeveledUpInBattle &= ~(1u << sEvolution.party);
    }
    else
    {
        // Offer the evolved species' level-matched moves through the same
        // native learning/replace UI, after the original species' own offer.
        BattleScriptPushCursor();
        gBattlescriptCurrInstr = BattleScript_ArenaEvolutionLearn;
    }
    EvolutionPose(0);
    BuildOamBuffer();
    ArenaRender_Ready();
    gMain.callback1 = sEvolution.callback1;
    SetMainCallback2(BattleMainCB2);
}

static void CB2_ArenaEvolution(void)
{
    // Entire transformation AND its final pose are a hard simulation pause.
    // No arena ticks, tasks, AI, projectiles or damage callbacks run here.
    u16 age = sEvolution.age < EVOLUTION_LEADIN ? 0
        : min(sEvolution.age + 1 - EVOLUTION_LEADIN, EVOLUTION_DURATION);
    u8 frame = min(31, (age * 32) / EVOLUTION_DURATION);
    u8 light = age < 40 ? age / 5 : age < 160 ? 8 : (EVOLUTION_DURATION-age)/10;
    struct ArenaBody *body = &sArena.bodies[0];
    struct Sprite *p = &gSprites[body->sprite];
    ++sEvolution.age;
    gArenaEvolutionTelemetry[7] = sEvolution.age;
    if (age < EVOLUTION_REVEAL && JOY_NEW(B_BUTTON))
    { EvolutionFinish(TRUE); return; }
    if (sEvolution.age == EVOLUTION_LEADIN + 1) PlaySE(SE_M_TELEPORT);
    if (sEvolution.age == EVOLUTION_LEADIN + EVOLUTION_REVEAL) EvolutionCommit();
    if (sEvolution.age >= EVOLUTION_LEADIN + EVOLUTION_DURATION + EVOLUTION_SETTLE)
    { EvolutionFinish(FALSE); return; }
    EvolutionPose(age);
    BlendPalette(16, 240, light, sEvolution.oldSpecies == SPECIES_GASTLY ? RGB(3,0,6) : RGB(2,5,8));
    BlendPalette(OBJ_PLTT_ID(0), 16,
        age < 40 ? age/5 : age < 112 ? 8+(age-40)/10 : age < 140 ? 16 : age < 180 ? (180-age)*16/40 : 0,
        sEvolution.oldSpecies == SPECIES_GASTLY ? RGB(26,22,31) : RGB_WHITE);
    // Lift the silhouette without moving the combat position or shadow.
    p->y2 = age < 50 ? -age/12 : age < 150 ? -4 : age < 190 ? -(190-age)/10 : 0;
    if (sEvolution.sprite < MAX_SPRITES)
    {
        struct Sprite *fx = &gSprites[sEvolution.sprite];
        fx->invisible = age == 0;
        fx->x = body->x / Q;
        fx->y = body->y / Q - 9;
        if (frame != sEvolution.frame)
        {
            ArenaRender_Copy(sEvolution.frames + frame*512,
                (u8 *)OBJ_VRAM0 + fx->oam.tileNum*32, 2048);
            sEvolution.frame = frame;
        }
    }
    BuildOamBuffer();
    ArenaRender_Ready();
    UpdatePaletteFade();
}

bool8 RealtimeArena_TryEvolution(u8 party)
{
    struct Pokemon *mon;
    u16 species, oldSpecies;
    u8 bank, sprite;
    u32 i;
    struct SpriteSheet sheet = {sEvolutionFrames, 2048, EVOLUTION_TAG};
    struct SpriteTemplate effect = {.tileTag=EVOLUTION_TAG, .paletteTag=SHOT_TAG+1,
        .oam=&sSendoutFxOam, .anims=gDummySpriteAnimTable,
        .affineAnims=gDummySpriteAffineAnimTable, .callback=SpriteCallbackDummy};
    // Only the active, living single-battle actor. Benched/unsupported Pokémon
    // and Nincada's two-Pokémon evolution retain the original post-battle flow.
    if (!gRealtimeArenaQuietResult || party >= PARTY_SIZE || party != gBattlerPartyIndexes[0]
        || gBattlersCount != 2 || !gBattleMons[0].hp || !sArena.bodies[0].art)
        return FALSE;
    mon = &gPlayerParty[party];
    oldSpecies = GetMonData(mon, MON_DATA_SPECIES);
    if (oldSpecies == SPECIES_NINCADA) return FALSE;
    species = GetEvolutionTargetSpecies(mon, EVO_MODE_NORMAL, 0, NULL, NULL, CHECK_EVO);
    if (!species || !ArenaSprites_Get(species)) return FALSE;
    if (oldSpecies == SPECIES_GASTLY) sheet.data = sSpectralEvolutionFrames;
    // Borrow the inactive enemy projectile bank, NOT the player's bank:
    // native floor shadows share colors 3/4 of the player's palette.
    bank = IndexOfSpritePaletteTag(SHOT_TAG+1);
    if (bank >= 16 || LoadSpriteSheet(&sheet) == 0xFFFF) return FALSE;
    sprite = CreateSprite(&effect, 0, 0, 0);
    if (sprite == MAX_SPRITES)
    { FreeSpriteTilesByTag(EVOLUTION_TAG); return FALSE; }
    memset(&sEvolution, 0, sizeof(sEvolution));
    sEvolution.frames = oldSpecies == SPECIES_GASTLY ? sSpectralEvolutionFrames : sEvolutionFrames;
    sEvolution.sprite=sprite;sEvolution.bank=bank;sEvolution.party=party;
    sEvolution.species=species;sEvolution.oldSpecies=oldSpecies;sEvolution.frame=255;
    sEvolution.hpBefore=gBattleMons[0].hp;
    memcpy(sEvolution.savedPalette, &gPlttBufferUnfaded[OBJ_PLTT_ID(bank)], 32);
    LoadPalette(oldSpecies == SPECIES_GASTLY ? sSpectralEvolutionPalette : sEvolutionPalette, OBJ_PLTT_ID(bank), 32);
    // The defeated opponent's projectiles and both trail sprites must disappear
    // before borrowing their palette. Hiding only the bolt left stray trails.
    for(i=0;i<SHOTS_COUNT;i++)
        if(sArena.shots[i].life) DestroyShot(&sArena.shots[i]);
    sArena.attackBuffer=sArena.dashBuffer=0;
    for(i=0;i<2;i++)
    {
        sArena.bodies[i].actionLife=sArena.bodies[i].shotTimer=0;
        ArenaMoveFx_Action(i,NULL,0,0,0,0,FALSE,TRUE);
        ArenaMoveFx_Flame(i,MOVE_FLAMETHROWER,0,0,0,0,0,FALSE,FALSE);
    }
    gArenaEvolutionTelemetry[0]++;
    gArenaEvolutionTelemetry[3]=oldSpecies;gArenaEvolutionTelemetry[4]=species;
    gArenaEvolutionTelemetry[5]=sEvolution.hpBefore;gArenaEvolutionTelemetry[7]=0;
    sEvolution.callback1=gMain.callback1;
    gMain.callback1=NULL;
    sArena.hudDirty=TRUE;
    DrawHud();
    SetVBlankCallback(VBlank_Arena);
    SetMainCallback2(CB2_ArenaEvolution);
    return TRUE;
}
