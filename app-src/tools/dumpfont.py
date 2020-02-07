import struct
import sys

import png


def glyphrows(data):
    return tuple(
        (
            (r & 0xc000) >> 14,
            (r & 0x3000) >> 12,
            (r & 0xc00) >> 10,
            (r & 0x300) >> 8,
            (r & 0xc0) >> 6,
            (r & 0x30) >> 4,
            (r & 0xc) >> 2,
            (r & 0x3),
        )
        for r in struct.unpack(
            '>8H', data))


def reserialize(glyphs):
    for line in range(16):
        c0 = line << 4
        for y in range(8):
            row = list(glyphs[c0][y])
            row.extend(glyphs[c0 + 1][y])
            row.extend(glyphs[c0 + 2][y])
            row.extend(glyphs[c0 + 3][y])
            row.extend(glyphs[c0 + 4][y])
            row.extend(glyphs[c0 + 5][y])
            row.extend(glyphs[c0 + 6][y])
            row.extend(glyphs[c0 + 7][y])
            row.extend(glyphs[c0 + 8][y])
            row.extend(glyphs[c0 + 9][y])
            row.extend(glyphs[c0 + 10][y])
            row.extend(glyphs[c0 + 11][y])
            row.extend(glyphs[c0 + 12][y])
            row.extend(glyphs[c0 + 13][y])
            row.extend(glyphs[c0 + 14][y])
            row.extend(glyphs[c0 + 15][y])
            yield row


infilename = sys.argv[1]
outfilename = sys.argv[2]
infile = open(infilename, 'rb')
indata = [infile.read(16) for c in range(256)]
infile.close()
glyphs = [glyphrows(d) for d in indata]
writer = png.Writer(
    width=128,
    height=128,
    bitdepth=2,
    palette=[
        (  0,   0,   0),
        (170,  85,  85),
        ( 85, 170, 170),
        (255, 255, 255)])
outfile = open(outfilename, 'wb')
writer.write(outfile, reserialize(glyphs))
