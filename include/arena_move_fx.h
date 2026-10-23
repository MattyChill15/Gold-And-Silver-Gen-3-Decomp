#ifndef GUARD_ARENA_MOVE_FX_H
#define GUARD_ARENA_MOVE_FX_H
#include "arena_moves.h"
void ArenaMoveFx_Init(void);
bool8 ArenaMoveFx_ClashReady(void);
void ArenaMoveFx_Clash(s16 x,s16 y,u8 dir,u8 frame,bool8 active);
void ArenaMoveFx_Guard(u8 side,s16 x,s16 y,u8 frame,u8 dir);
void ArenaMoveFx_Flame(u8 side,u16 move,s16 x,s16 y,u8 direction,u8 age,u8 reach,bool8 active,bool8 clashing);
u8 ArenaMoveFx_Palette(u8 material);
void ArenaMoveFx_Action(u8 side, const struct ArenaMoveProfile *profile,
                       s16 x, s16 y, u8 direction, u8 age, bool8 active, bool8 paused);
#define ARENA_BOLT_SLOTS 6
u8 ArenaMoveFx_CreateBolt(const struct ArenaMoveProfile *profile, s16 x, s16 y, u8 direction, u8 slot);
void ArenaMoveFx_Bolt(u8 sprite, const struct ArenaMoveProfile *profile,
                     s16 x, s16 y, u8 direction, u8 age);
#endif
