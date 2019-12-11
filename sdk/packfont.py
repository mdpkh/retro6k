import struct
import sys

import png


infilename = sys.argv[1]
outfilename = sys.argv[2]
infile = open(infilename, 'rb')
reader = png.Reader(
    file=infile)
width, height, rows, info = reader.read()
lines = height >> 3
iterrows = iter(rows)
outfile = open(outfilename, 'wb')
for page in range(lines):
    r = tuple(next(iterrows) for y in range(8))
    for c in range(16):
        x0 = c << 3
        for y in range(8):
            outfile.write(
                struct.pack(
                    '>H',
                      (r[y][x0] << 14)
                    | (r[y][x0 + 1] << 12)
                    | (r[y][x0 + 2] << 10)
                    | (r[y][x0 + 3] << 8)
                    | (r[y][x0 + 4] << 6)
                    | (r[y][x0 + 5] << 4)
                    | (r[y][x0 + 6] << 2)
                    | (r[y][x0 + 7])
                    ))
infile.close()
outfile.close()
