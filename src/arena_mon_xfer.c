// See include/arena_mon_xfer.h. Kept byte-identical across the series trees.
#include "global.h"
#include "arena_mon_xfer.h"

static const u8 sXferOrder[4][24] =
{
    {0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 2, 3, 1, 1, 2, 3, 2, 3, 1, 1, 2, 3, 2, 3},
    {1, 1, 2, 3, 2, 3, 0, 0, 0, 0, 0, 0, 2, 3, 1, 1, 3, 2, 2, 3, 1, 1, 3, 2},
    {2, 3, 1, 1, 3, 2, 2, 3, 1, 1, 3, 2, 0, 0, 0, 0, 0, 0, 3, 2, 3, 2, 1, 1},
    {3, 2, 3, 2, 1, 1, 3, 2, 3, 2, 1, 1, 3, 2, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0},
};

static u32 Rd32(const u8 *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((u32)p[3] << 24); }
static u16 Rd16(const u8 *p) { return p[0] | (p[1] << 8); }
static void Wr32(u8 *p, u32 v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }
static void Wr16(u8 *p, u16 v) { p[0] = v; p[1] = v >> 8; }

// Decrypts the 48-byte secure area into sub[4][12] (logical order 0..3).
// Returns FALSE if the checksum does not match.
static bool8 Unpack(const u8 *raw, u8 sub[4][12], struct ArenaMonFields *f)
{
    u32 key, i, k, sum = 0;
    u8 plain[48];
    memset(f, 0, sizeof(*f));
    f->personality = Rd32(raw);
    f->otId = Rd32(raw + 4);
    memcpy(f->nickname, raw + 8, 10);
    f->language = raw[18];
    f->isBadEgg = raw[19] & 1;
    f->hasSpecies = (raw[19] >> 1) & 1;
    f->isEgg = (raw[19] >> 2) & 1;
    memcpy(f->otName, raw + 20, 7);
    f->markings = raw[27];
    key = f->personality ^ f->otId;
    for (i = 0; i < 48; i += 4)
        Wr32(plain + i, Rd32(raw + 32 + i) ^ key);
    for (i = 0; i < 48; i += 2)
        sum += Rd16(plain + i);
    for (k = 0; k < 4; k++)
        memcpy(sub[k], plain + 12 * sXferOrder[k][f->personality % 24], 12);
    return (u16)sum == Rd16(raw + 28);
}

bool8 ArenaXfer_DecodePret(const u8 *raw, struct ArenaMonFields *f)
{
    u8 s[4][12];
    u32 i, w;
    bool8 ok = Unpack(raw, s, f);
    f->species = Rd16(s[0]);
    f->heldItem = Rd16(s[0] + 2);
    f->exp = Rd32(s[0] + 4);
    f->ppBonuses = s[0][8];
    f->friendship = s[0][9];
    for (i = 0; i < 4; i++)
    {
        f->moves[i] = Rd16(s[1] + 2 * i);
        f->pp[i] = s[1][8 + i];
    }
    memcpy(f->evs, s[2], 6);
    memcpy(f->contest, s[2] + 6, 6);
    f->pokerus = s[3][0];
    f->metLocation = s[3][1];
    w = Rd16(s[3] + 2);
    f->metLevel = w & 0x7F;
    f->metGame = (w >> 7) & 0xF;
    f->ball = (w >> 11) & 0xF;
    f->otGender = w >> 15;
    w = Rd32(s[3] + 4);
    for (i = 0; i < 6; i++)
        f->ivs[i] = (w >> (5 * i)) & 0x1F;
    f->isEgg |= (w >> 30) & 1;
    f->abilityNum = w >> 31;
    w = Rd32(s[3] + 8);
    f->ribbons = w & 0x7FFFFFFF;
    f->fateful = w >> 31;
    return ok && f->hasSpecies;
}

void ArenaXfer_EncodePret(const struct ArenaMonFields *f, u8 *raw)
{
    u8 s[4][12], plain[48];
    u32 i, k, key, sum = 0, w;
    memset(raw, 0, ARENA_BOXMON_SIZE);
    memset(s, 0, sizeof(s));
    if (f->species == 0)
        return;
    Wr32(raw, f->personality);
    Wr32(raw + 4, f->otId);
    memcpy(raw + 8, f->nickname, 10);
    raw[18] = f->language;
    raw[19] = (f->isBadEgg & 1) | 2 | ((f->isEgg & 1) << 2);
    memcpy(raw + 20, f->otName, 7);
    raw[27] = f->markings;
    Wr16(s[0], f->species);
    Wr16(s[0] + 2, f->heldItem);
    Wr32(s[0] + 4, f->exp);
    s[0][8] = f->ppBonuses;
    s[0][9] = f->friendship;
    for (i = 0; i < 4; i++)
    {
        Wr16(s[1] + 2 * i, f->moves[i]);
        s[1][8 + i] = f->pp[i];
    }
    memcpy(s[2], f->evs, 6);
    memcpy(s[2] + 6, f->contest, 6);
    s[3][0] = f->pokerus;
    s[3][1] = f->metLocation;
    Wr16(s[3] + 2, (f->metLevel & 0x7F) | ((f->metGame & 0xF) << 7) | ((f->ball & 0xF) << 11) | ((f->otGender & 1) << 15));
    for (w = 0, i = 0; i < 6; i++)
        w |= (u32)(f->ivs[i] & 0x1F) << (5 * i);
    w |= (u32)(f->isEgg & 1) << 30 | (u32)(f->abilityNum & 1) << 31;
    Wr32(s[3] + 4, w);
    Wr32(s[3] + 8, (f->ribbons & 0x7FFFFFFF) | ((u32)(f->fateful & 1) << 31));
    for (k = 0; k < 4; k++)
        memcpy(plain + 12 * sXferOrder[k][f->personality % 24], s[k], 12);
    for (i = 0; i < 48; i += 2)
        sum += Rd16(plain + i);
    Wr16(raw + 28, sum);
    key = f->personality ^ f->otId;
    for (i = 0; i < 48; i += 4)
        Wr32(raw + 32 + i, Rd32(plain + i) ^ key);
}

// pokeemerald-expansion substructs (include/pokemon.h, GCC LSB-first bitfields).
bool8 ArenaXfer_DecodeExpansion(const u8 *raw, struct ArenaMonFields *f)
{
    u8 s[4][12];
    u32 i, w;
    bool8 ok = Unpack(raw, s, f);
    f->species = Rd16(s[0]) & 0x7FF;
    f->heldItem = Rd16(s[0] + 2) & 0x3FF;
    f->exp = Rd32(s[0] + 4) & 0x1FFFFF;
    f->ppBonuses = s[0][8];
    f->friendship = s[0][9];
    f->ball = Rd16(s[0] + 10) & 0x3F;
    w = Rd32(s[1]);
    f->moves[0] = w & 0x7FF;
    f->moves[1] = (w >> 16) & 0x7FF;
    w = Rd32(s[1] + 4);
    f->moves[2] = w & 0x7FF;
    f->moves[3] = (w >> 16) & 0x7FF;
    for (i = 0; i < 4; i++)
        f->pp[i] = s[1][8 + i] & 0x7F;
    memcpy(f->evs, s[2], 6);
    memcpy(f->contest, s[2] + 6, 6);
    f->pokerus = s[3][0];
    f->metLocation = s[3][1];
    w = Rd16(s[3] + 2);
    f->metLevel = w & 0x7F;
    f->metGame = (w >> 7) & 0xF;
    f->otGender = w >> 15;
    w = Rd32(s[3] + 4);
    for (i = 0; i < 6; i++)
        f->ivs[i] = (w >> (5 * i)) & 0x1F;
    f->isEgg |= (w >> 30) & 1;
    w = Rd32(s[3] + 8);
    f->ribbons = w & 0x07FFFFFF;   // cool..world ribbons share the pret bit positions
    f->abilityNum = (w >> 29) & 3;
    f->fateful = w >> 31;
    return ok && f->hasSpecies;
}
