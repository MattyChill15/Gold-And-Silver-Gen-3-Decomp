#!/usr/bin/env python3
"""1.8.0 SJ1: Yellow (True Ash) chapter-1 record carries Bulbasaur into Johto P1.
Patches only the Yellow/story bits of a copy of the FR P2 record (checksum recomputed)."""
import shutil, struct, sys, json
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent)); from ptlib import Emu
import playtest_johto as J, argparse; J.A = argparse.Namespace(drv="/workspace/fr/smoke/playtest_drv")
ROOT = J.ROOT; DRV = '/workspace/fr/smoke/playtest_drv'; OUT = Path('/tmp/johto_bulba'); shutil.rmtree(OUT, ignore_errors=True); OUT.mkdir()
SRC = ROOT / '.arena-dev/pt/fr_p2_complete.sav'
def patched(name, yellow, story):
    b = bytearray(SRC.read_bytes()); o = 31 * 0x1000 + 4
    sf = struct.unpack_from('<I', b, o + 0x24)[0] & ~(2 | 12)
    sf |= (2 if yellow else 0) | (story << 2); struct.pack_into('<I', b, o + 0x24, sf)
    struct.pack_into('<I', b, o + 0x2F0, sum(struct.unpack_from('<188I', b, o)) & 0xffffffff)
    p = OUT / name; p.write_bytes(b); return p
res = {}
for name, y, st in (('classic', 0, 0), ('true_ash', 1, 2)):
    s = patched(name + '.sav', y, st)
    e = Emu(DRV, str(ROOT / 'jpt_p1.gba'), str(ROOT / 'jpt_p1.elf'), str(s)); e.run(1500); e.close()
    e = Emu(DRV, str(ROOT / 'jpt_p1.gba'), str(ROOT / 'jpt_p1.elf'), str(s)); inf = J.boot_new(e)
    i = e.cmd(23); e.shot(str(OUT / f'{name}.png')); e.close()
    res[name] = dict(field=inf, party=i[0], pc=i[1], slot0=i[2] & 0xffff, slot1=i[2] >> 16, story=i[3] & 0xff, imported=i[3] >> 8)
    print(name, res[name], flush=True)
c, t = res['classic'], res['true_ash']
ok = c['imported'] == 1 and t['imported'] == 1 and t['story'] == 2 and (t['party'] == c['party'] + 1 or (t['party'] == c['party'] and t['pc'] == c['pc'] + 1) or c['party'] == t['party'] and c['pc'] == t['pc'])
print(json.dumps(res)); print('PASS' if ok else 'FAIL')
