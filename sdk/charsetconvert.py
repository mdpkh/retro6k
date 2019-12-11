import png

w, h, p, m = png.Reader('charset.png').read()
c = 32
xs = [0] * 17
ip = iter(p)
while c < 128:
    wr = [next(ip) for y in xrange(16)]
    sp = 0
    for x in xrange(w):
        if wr[0][x] & 2:
            xs[sp] = x
            sp += 1
            if sp > 16:
                break
    for rc in xrange(16):
        packed = [0] * 16
        for y in xrange(16):
            for x in xrange(xs[rc], xs[rc + 1]):
                packed[y] >>= 1
                packed[y] |= (wr[y][x] & 1) << 15
            packed[y] >>= (16 - xs[rc + 1] + xs[rc])
        print '{{ {:d}, {{ {:#x}, {:#x}, {:#x}, {:#x}, {:#x}, {:#x}, {:#x}, {:#x}, {:#x}, {:#x}, {:#x}, {:#x}, {:#x}, {:#x}, {:#x}, {:#x} }} }},'.format(xs[rc + 1] - xs[rc], *packed)
    c += 16

