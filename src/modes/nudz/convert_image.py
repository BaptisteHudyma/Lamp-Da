#!/usr/bin/env python

import sys
from PIL import Image
import os

in_img = sys.argv[1]
out_src = sys.argv[2]
if len(sys.argv) >= 4:
    out_var = sys.argv[3]
else:
    out_var = os.path.basename(out_src)
    out_var = out_var.split('.')[0]

image = Image.open(in_img)
btes = image.tobytes()
sz = (25, 22)

with open(out_src, 'w') as of:

    print(f'static constexpr uint32_t {out_var}[] = {{', file=of)
    i = 0
    for y in range(sz[1]):
        for x in range(sz[0]):
            if image.mode == 'RGBA':
                r, g, b = btes[i: i+3]
                i += 4
            else:
                r, g, b = btes[i: i+3]
                i += 3
            print('0x%02x%02x%02x,' % (r, g, b), file=of)

    print('};', file=of)

