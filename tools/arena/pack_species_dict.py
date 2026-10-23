#!/usr/bin/env python3
"""Matt mod: lossless ROM saving. Merge each species' per-pose tile
dictionaries into one shared dictionary. Output keeps the exact format that
ArenaSprites_Decode reads (header[0]=byte offset to dictionary, header[1]=count,
then u16 tile map), so no engine change is needed. Rewrites sprites.inc."""
import re, struct, collections
from pathlib import Path
D = Path(__file__).resolve().parents[2] / '.arena-dev/pmd'
inc = (D / 'sprites.inc').read_text()
pat = re.compile(r'static const u32 (sPmdTiles(\d{4})\w+)\[\] = INCBIN_U32\("\.arena-dev/pmd/(\S+?\.4bpp)"\);')
groups = collections.OrderedDict()
for m in pat.finditer(inc):
    groups.setdefault(m.group(2), []).append((m.group(1), m.group(3)))
before = after = 0
defs = {}
for dex, anims in groups.items():
    tiles, index, parts = [], {}, []
    for name, fn in anims:
        b = (D / fn).read_bytes(); before += len(b)
        off, cnt = struct.unpack('<II', b[:8])
        assert off + cnt * 32 == len(b)
        local = [b[off + i * 32: off + i * 32 + 32] for i in range(cnt)]
        remap = []
        for t in local:
            if t not in index: index[t] = len(tiles); tiles.append(t)
            remap.append(index[t])
        mp = struct.unpack('<%dH' % ((off - 8) // 2), b[8:off])
        parts.append((name, struct.pack('<%dH' % len(mp), *[remap[i] for i in mp])))
    assert len(tiles) < 65536
    # Frame dedupe: every frame map (64 u16) goes into a per-species pool;
    # each animation keeps a u16 index per (direction, frame).
    pool, pindex, heads = [], {}, []
    for name, p in parts:
        idx = []
        for k in range(0, len(p), 128):
            fr = p[k:k + 128]
            if fr not in pindex: pindex[fr] = len(pool); pool.append(fr)
            idx.append(pindex[fr])
        tbl = struct.pack('<%dH' % len(idx), *idx)
        if len(tbl) % 4: tbl += b'\0\0'
        heads.append((name, tbl))
    pool_start = sum(12 + len(t) for _, t in heads)
    dict_start = pool_start + len(pool) * 128
    blob = bytearray(); pos = 0
    for name, tbl in heads:
        assert pos % 4 == 0
        defs[name] = (dex, pos // 4)
        blob += struct.pack('<III', dict_start - pos, len(tiles) | 0x80000000, pool_start - pos) + tbl
        pos += 12 + len(tbl)
    blob += b''.join(pool)
    blob += b''.join(tiles)
    (D / f'{dex}.blob').write_bytes(blob); after += len(blob)
seen = set()
def sub(m):
    name, dex = m.group(1), m.group(2)
    out = ''
    if dex not in seen:
        seen.add(dex)
        out = f'static const u32 sPmdBlob{dex}[] = INCBIN_U32(".arena-dev/pmd/{dex}.blob");\n'
    return out + f'#define {name} (sPmdBlob{dex} + {defs[name][1]})'
(D / 'sprites.inc').write_text(pat.sub(sub, inc))
print(f'species dictionaries: {before} -> {after} bytes (saved {before-after})')
