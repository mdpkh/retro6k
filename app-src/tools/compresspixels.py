import sys


def bytepack(inybbles):
    while True:
        try:
            lo = next(inybbles)
        except StopIteration:
            return
        try:
            hi = next(inybbles)
        except StopIteration:
            yield lo
            return
        yield lo | hi << 4


def crumbify(idata):
    for b in idata:
        yield b & 3
        b >>= 2
        yield b & 3
        b >>= 2
        yield b & 3
        b >>= 2
        yield b & 3


def lengthcode(n):
    digits = []
    while n > 0:
        digits.insert(0, n & 3)
        n >>= 2
    for i in range(len(digits) - 1, 0, -1):
        if digits[i] <= 0:
            digits[i] += 4
            digits[i - 1] -= 1
    while digits[0] == 0:
        digits.pop(0)
    return [d & 3 for d in digits]


def nibblify(iruns):
    for v, n in iruns:
        nseq = lengthcode(n)
        for c in nseq:
            yield v | c << 2


def runlengthencode(idata):
    runvalue = None
    runlen = 0
    for v in idata:
        if v == runvalue:
            runlen += 1
            continue
        if runlen > 0:
            yield (runvalue, runlen)
        runvalue = v
        runlen = 1
    yield (runvalue, runlen)


infilename = sys.argv[1]
outfilename = sys.argv[2]
infile = open(infilename, 'rb')
data = []
while True:
    inchunk = infile.read(256)
    if len(inchunk) < 256:
        break
    data.append(inchunk)
print('{:d} pages read.'.format(len(data)))
infile.close()
cdata = []
toc = []
offset = 2 * len(data)
for chunk in data:
    idata = iter(chunk)
    icrumbs = crumbify(idata)
    iruns = runlengthencode(icrumbs)
    inybbles = nibblify(iruns)
    ibytes = bytepack(inybbles)
    compressed = bytes(ibytes)
    cdata.append(compressed)
    toc.append(offset)
    offset += len(compressed)
print('Total compressed size: {:d} bytes.'.format(offset))
outfile = open(outfilename, 'wb')
for offset in toc:
    outfile.write(bytes((offset & 255, offset >> 8)))
for chunk in cdata:
    outfile.write(chunk)
outfile.close()

    
