"""Rebuild EA-027 ROM art from reviewed PNG frames (requires Pillow)."""
from pathlib import Path
import struct
from PIL import Image

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'graphics/arena/storm'
COLORS=['183c49','245561','32717b','489496','70b4ae','a4d0bd','d8e8cd']
LOOKUP={tuple(bytes.fromhex(c)):240+i for i,c in enumerate(COLORS)}

def tiled(im):
    assert im.size==(128,64)
    return bytes(LOOKUP[im.getpixel((tx*8+x,ty*8+y))[:3]] if im.getpixel((tx*8+x,ty*8+y))[3] else 0
                 for ty in range(8) for tx in range(16) for y in range(8) for x in range(8))

water=bytearray();electric=bytearray()
for f in range(16):
    im=Image.open(OUT/f'water-{f:02}.png').convert('RGBA')
    overlay=Image.open(OUT/f'discharge-{f:02}.png').convert('RGBA')
    assert all(p[3] in (0,255) and (not p[3] or p[:3] in LOOKUP) for p in im.getdata())
    assert overlay.size==im.size
    assert all(p[3] in (0,255) and (not p[3] or p[:3] in LOOKUP) for p in overlay.getdata())
    water.extend(tiled(im));im.alpha_composite(overlay);electric.extend(tiled(im))
(OUT/'water.8bpp').write_bytes(water)
(OUT/'discharge.8bpp').write_bytes(electric)
sparse=bytearray();offsets=[]
for i in range(16*128):
    offsets.append(len(sparse))
    entries=[(p,c) for p,c in enumerate(water[i*64:(i+1)*64]) if c]
    sparse.append(len(entries))
    for p,c in entries:sparse.extend((p,c))
assert max(offsets)<65536
(OUT/'water-sparse.bin').write_bytes(sparse)
(OUT/'water-offsets.bin').write_bytes(struct.pack('<2048H',*offsets))

background=bytearray()
for biome in ['forest','coast','cave','desert','gym']:
    art=ROOT/'.arena-dev/art'
    palette=struct.unpack('<256H',(art/(biome+'.gbapal')).read_bytes())
    rgb=[(c&31,(c>>5)&31,(c>>10)&31) for c in palette]
    def nearest(v):
        return min(range(16,240),key=lambda i:sum((a-b)**2 for a,b in zip(rgb[i],v)))
    remap=list(range(256))
    for i in range(240,247):remap[i]=nearest(rgb[i])
    # A teal-tinted floor, still readable THROUGH the sparse water clusters.
    shade=[nearest((r*2//3,g*3//4+2,min(31,b*3//4+4))) for r,g,b in rgb]
    raw=(art/(biome+'.8bpp')).read_bytes()
    assert len(raw)==38400
    background.extend(bytes(remap[c] for c in raw))
    background.extend(bytes(shade[c] for c in raw))
(OUT/'backgrounds.8bpp').write_bytes(background)

# Submerged contact variants: remove the lowest rows, retain a broken bright
# meniscus on the waterline. No scaling, new allocations or extra OAM in-game.
wetprops=bytearray()
heights=[18,18,19,17,14,17,15]
for biome in ['forest','coast','cave','desert','gym']:
    raw=(ROOT/'.arena-dev/art'/('props.4bpp' if biome=='forest' else biome+'-props.4bpp')).read_bytes()
    for i in range(7):
        for state in range(3):
            data=bytearray(raw[(i*3+state)*512:(i*3+state+1)*512])
            line=16+heights[i]//2-2
            for y in range(32):
                for x in range(32):
                    off=((y//8)*4+x//8)*32+(y%8)*4+(x%8)//2
                    shift=(x&1)*4; value=(data[off]>>shift)&15
                    if y>line: value=0
                    elif y==line and value: value=1 if x%5 in (1,2) else value
                    data[off]=(data[off]&~(15<<shift))|(value<<shift)
            wetprops.extend(data)
(OUT/'wet-props.4bpp').write_bytes(wetprops)
print('Storm: 16 water frames, 16 discharge frames, 5 remapped/shaded biomes.')
