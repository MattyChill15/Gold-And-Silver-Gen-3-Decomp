#ifndef GUARD_MATT_CONFIG_H
#define GUARD_MATT_CONFIG_H
// ===== Matt's personal mod switches (edit, then rebuild) =====
// 1 = player's Pokemon moves/dodges by itself in arena battles; you only
//     pick (L/R) and press (A) attacks. 0 = original manual steering.
#define MATT_AUTO_SIM_COMBAT 1
// 1 = your Pokemon also picks and fires its own moves (needs AUTO_SIM).
//     Picks by type effectiveness/STAB/power with variety; normal misses.
#define MATT_AUTO_ATTACK 1
// 1 = "Catch it?" YES/NO before each normal wild battle. YES = caught
//     instantly; NO = normal (arena) battle. Not used in Safari/Pyramid.
#define MATT_CATCH_OR_FIGHT 1
// Starters (bag shown left -> middle -> right). Level given at pickup.
#define MATT_STARTER_1 SPECIES_VIBRAVA
#define MATT_STARTER_2 SPECIES_MUDKIP
#define MATT_STARTER_3 SPECIES_ABRA
#define MATT_STARTER_LEVEL 5
// Catch rate multiplier applied to every species' base catch rate (1 = normal).
#define MATT_CATCH_RATE_MULT 1
// 1 = every ball always catches (compile-time). Also toggled in-game, see README.
#define MATT_ALWAYS_CATCH 0
// 1 = start with a Master Ball and it is never used up.
#define MATT_INFINITE_MASTER_BALL 1
// 1 = party-wide Exp. Share (Gen 6 style): battlers full exp, all other
//     healthy party members half. Fainted ones get none. (An EXP. SHARE
//     item is also given at new game.)
#define MATT_PARTY_EXP_SHARE 1
// Auto-use bag potions/cures for the player's active mon in the arena.
#ifndef MATT_AUTO_ITEMS
#define MATT_AUTO_ITEMS 1
#endif
// 1 = your Pokemon auto-switches out in arena battles when it is about to
//     faint with no heal left, or is badly outmatched by type and a healthy
//     teammate matches up clearly better (max 2 per battle, 8 s cooldown).
#ifndef MATT_AUTO_SWITCH
#define MATT_AUTO_SWITCH 1
#endif
// 1 = after beating a trainer, skip their in-battle defeat speech; only the
//     "defeated X" line and the prize money are shown. Map scripts (badges,
//     TMs, rematches, story) are untouched.
#ifndef MATT_SKIP_TRAINER_DEFEAT_TEXT
#define MATT_SKIP_TRAINER_DEFEAT_TEXT 1
#endif

#endif
