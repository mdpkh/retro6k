import itertools
import sys


def decoderuns(inybbles):
    decodelen = 0
    runlen = 0
    runvalue = None
    for nyb in inybbles:
        v = nyb & 3
        c = nyb >> 2
        if c == 0:
            c = 4
        if v == runvalue:
            runlen <<= 2
            runlen += c
            continue
        if runlen > 0:
            yield (runvalue, runlen)
            decodelen += runlen
            if decodelen >= 1024:
                return
        runvalue = v
        runlen = c
    yield (runvalue, runlen)


def expandruns(iruns):
    crumbcount = 0
    for v, n in iruns:
        for i in range(n):
            yield v
            crumbcount += 1
            if crumbcount >= 1024:
                return


def packcrumbs(icrumbs):
    bytecount = 0
    while bytecount < 256:
        yield (  next(icrumbs, 0)
               | next(icrumbs, 0) << 2
               | next(icrumbs, 0) << 4
               | next(icrumbs, 0) << 6)
        bytecount += 1


def readnybbles(infile):
    while True:
        try:
            inbyte = infile.read(1)[0]
        except (IndexError, IOError, OSError):
            return
        yield inbyte & 15
        yield inbyte >> 4


infilename = sys.argv[1]
outfilename = sys.argv[2]
infile = open(infilename, 'rb')
toc = [infile.read(1)[0] | infile.read(1)[0] << 8]
pages = toc[0] >> 1
while len(toc) < pages:
    toc.append(infile.read(1)[0] | infile.read(1)[0] << 8)
outdata = []
for offset in toc:
    infile.seek(offset)
    inybbles = readnybbles(infile)
    iruns = decoderuns(inybbles)
    icrumbs = expandruns(iruns)
    ibytes = packcrumbs(icrumbs)
    outdata.append(bytes(ibytes))
infile.close()
outfile = open(outfilename, 'wb')
outfile.truncate(len(outdata) * 256)
offset = 0
for page in outdata:
    outfile.seek(offset)
    outfile.write(page)
    offset += 256
outfile.close()
