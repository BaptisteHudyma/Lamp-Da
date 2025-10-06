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

cmap = {}
i = 0
cancel = False
for y in range(sz[1]):
    for x in range(sz[0]):
        if image.mode == 'RGBA':
            r, g, b = btes[i: i+3]
            i += 4
        else:
            r, g, b = btes[i: i+3]
            i += 3
        rgb = (r << 16) | (g << 8) | b
        if rgb not in cmap:
            if len(cmap) == 256 \
                    or (len(cmap) * 4 + len(btes) // 4 >= len(btes)):
                # colormap compression is useless
                cmap = {}
                cancel = True
                break
            # print(x, y, ':', hex(rgb), ':', len(cmap))
            cmap[rgb] = len(cmap)
    if cancel:
        break

print('cmap:', len(cmap), 'comp:', len(btes), '->',
      len(cmap) * 4 + len(btes) // 4)
if cmap:
    cmap_i = {v: k for k, v in cmap.items()}
    # print(cmap)
    # print(cmap_i)

with open(out_src, 'w') as of:

    print(f'''    struct {out_cls} {{
      static constexpr uint16_t width = {image.width};
      static constexpr uint16_t height = {image.height};''', file=of)
    if cmap:
        print(f'      static constexpr uint32_t colormapSize = {len(cmap)};',
              file=of)
        print('      static constexpr uint32_t colormap[] = {\n'
              '        ', end='', file=of)
        for i in sorted(cmap_i.keys()):
            rgb = cmap_i[i]
            end = '\n' if i % 8 == 7 else ' '
            print('0x%06x,' % rgb, end=end, file=of)
        print('\n      };', file=of)

        print('      static constexpr uint8_t indexData[] = {', file=of)
        i = 0
        for y in range(sz[1]):
            for x in range(sz[0]):
                if image.mode == 'RGBA':
                    r, g, b = btes[i: i+3]
                    i += 4
                else:
                    r, g, b = btes[i: i+3]
                    i += 3
                rgb = (r << 16) | (g << 8) | b
                end = '\n' if (x == sz[0] - 1 or x % 16 == 15) else ' '
                print('0x%02x,' % cmap[rgb], file=of, end=end)
        print('      };', file=of)

        print('      static constexpr uint32_t rgbData[] = {};', file=of)
        print('    };', file=of)
    else:
        print('''      static constexpr uint32_t colormapSize = 0;
      static constexpr uint32_t colormap[] = {};
      static constexpr uint8_t indexData[] = {};
      static constexpr uint32_t rgbData[] = { ''', file=of)
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

