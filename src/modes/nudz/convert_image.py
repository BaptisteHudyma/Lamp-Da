#!/usr/bin/env python

import sys
from PIL import Image
import os

in_img = sys.argv[1]
out_src = sys.argv[2]
if len(sys.argv) >= 4:
    out_cls = sys.argv[3]
else:
    out_cls = 'ImageTy'
    out_cls = os.path.basename(out_src)
    out_cls = out_cls.split('.')[0]
    if out_cls.endswith('_image'):
        out_cls = out_cls[:-6]
    out_cls = out_cls[0].upper() + out_cls[1:] + 'ImageTy'

image = Image.open(in_img)
btes = image.tobytes()
sz = (image.width, image.height)

with open(out_src, 'w') as of:

    print(f'''    struct {out_cls} {{
      static constexpr uint16_t width = {image.width};
      static constexpr uint16_t height = {image.height};
      static constexpr uint32_t rgbData[] = {{ ''', file=of)
    i = 0
    for y in range(sz[1]):
        for x in range(sz[0]):
            if image.mode == 'RGBA':
                r, g, b = btes[i: i+3]
                i += 4
            else:
                r, g, b = btes[i: i+3]
                i += 3
            end = '\n' if x == sz[0] - 1 else ' '
            print('0x%02x%02x%02x,' % (r, g, b), file=of, end=end)

    print('      };\n    };', file=of)

