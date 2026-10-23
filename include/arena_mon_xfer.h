#ifndef GUARD_ARENA_MON_XFER_H
#define GUARD_ARENA_MON_XFER_H
// Byte-level BoxPokemon codecs shared by the series trees (FireRed/Emerald
// pret format and the Johto pokeemerald-expansion format). Both formats are
// 80 bytes with the same header, XOR key and substruct order; only the
// substruct bit layouts differ. Ids are left in the source game's numbering;
// callers remap species/items/balls.
#define ARENA_BOXMON_SIZE 80
#define ARENA_PARTYMON_SIZE_PRET 100

struct ArenaMonFields
{
    u32 personality, otId, exp;
    u16 species, heldItem, moves[4];
    u8 nickname[10], otName[7];
    u8 language, markings, isBadEgg, hasSpecies, isEgg;
    u8 ppBonuses, friendship, ball; // ball: pret = ball item id (1-12); expansion = enum PokeBall
    u8 pp[4], evs[6], contest[6];   // contest: cool beauty cute smart tough sheen
    u8 pokerus, metLocation, metLevel, metGame, otGender;
    u8 ivs[6];                      // hp atk def spe spa spd (in-struct order)
    u8 abilityNum, fateful;
    u32 ribbons;                    // pret ribbon word minus fateful bit
};

bool8 ArenaXfer_DecodePret(const u8 *raw, struct ArenaMonFields *f);
void ArenaXfer_EncodePret(const struct ArenaMonFields *f, u8 *raw);
bool8 ArenaXfer_DecodeExpansion(const u8 *raw, struct ArenaMonFields *f);
#endif
