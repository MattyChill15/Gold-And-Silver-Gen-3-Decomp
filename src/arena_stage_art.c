#include "global.h"
// Large community art belongs in the linker's expandable extra region, after
// native fixed-address graphics, not before Emerald's original graphic anchors.
#include "../graphics/arena/oswarlin/stages.inc"
const u32 gArenaStageFlood[] = INCBIN_U32("graphics/arena/oswarlin/flood.8bpp");
const u8 gArenaWaterHeight[] = INCBIN_U8("graphics/arena/oswarlin/water-height.bin");
const u8 gArenaWaterHeightTiles[] = INCBIN_U8("graphics/arena/oswarlin/water-height-tiled.bin");
const u8 gArenaWaterHeightBounds[] = INCBIN_U8("graphics/arena/oswarlin/water-height-bounds.bin");
const u32 gArenaWaterFullMask[] = INCBIN_U32("graphics/arena/oswarlin/water-full-mask.bin");
