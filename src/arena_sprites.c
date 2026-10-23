#include "arena_sprites.h"
#ifndef ARENA_SPRITES_HOST
#include "constants/species.h"
#if defined(ARENA_JOHTO) && MATT_PART == 3
#include "../.arena-dev/pmd/sprites_p3.inc"
#elif defined(ARENA_JOHTO) && MATT_PART == 2
#include "../.arena-dev/pmd/sprites_p2.inc"
#elif defined(ARENA_JOHTO)
#include "../.arena-dev/pmd/sprites_p1.inc"
#else
#include "../.arena-dev/pmd/sprites.inc"
#endif

const struct ArenaSpriteSet *ArenaSprites_Get(u16 species)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sPmdSets); i++)
        if (sPmdSets[i].species == species) return &sPmdSets[i];
    return NULL;
}
#endif

// Decode only the current 64x64 frame into one stable per-battler buffer.
// The queued VBlank transfer must not share scratch memory with the other mon.
// All headers/indices are exhaustively checked offline; input is compiled ROM.
bool8 ArenaSprites_Decode(const struct ArenaSpriteAnimation *anim, u8 direction, u8 frame, u32 *destination)
{
    const u32 *header = (const u32 *)anim->tiles;
    const u16 *map;
    const u32 *dictionary;
    u32 tile, word;
    u32 count = header[1];
    if (direction >= 8 || frame >= anim->frames) return 0;
    if (count & 0x80000000)
    {
        // Matt mod: frames index a per-species pool of unique 64-tile maps.
        u16 index = ((const u16 *)(anim->tiles + 12))[direction * anim->frames + frame];
        count &= 0x7FFFFFFF;
        map = (const u16 *)(anim->tiles + header[2]) + index * 64;
    }
    else map = (const u16 *)(anim->tiles + 8) + (direction * anim->frames + frame) * 64;
    dictionary = (const u32 *)(anim->tiles + header[0]);
    for (tile=0; tile<64; tile++)
    {
        const u32 *source;
        if (map[tile] >= count) return 0;
        source = dictionary + map[tile] * 8;
        for (word=0; word<8; word++) *destination++ = source[word];
    }
    return 1;
}

u8 ArenaSprites_Frame(const struct ArenaSpriteAnimation *anim, u16 tick)
{
    u32 i;
    tick %= anim->totalTicks;
    for (i = 0; i + 1 < anim->frames && tick >= anim->durations[i]; i++)
        tick -= anim->durations[i];
    return i;
}
