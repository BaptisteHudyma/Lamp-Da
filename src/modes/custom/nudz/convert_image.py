#!/usr/bin/env python

import sys
from PIL import Image
import os
import numpy as np

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

bpp = 32
if cmap:
    ncol = len(cmap) - 1
    # bits per pixel
    bpp = len([1 for i in range(8) if ncol >> i])
blen = int(np.ceil(len(btes) * bpp / 32))

print('cmap:', len(cmap), 'comp:', len(btes), '->',
      len(cmap) * 4 + blen, ', bpp:', bpp)
if cmap:
    cmap_i = {v: k for k, v in cmap.items()}
    # print(cmap)
    # print(cmap_i)

with open(out_src, 'w') as of:

    print(f'''struct {out_cls}
{{
  static constexpr uint16_t width = {image.width};
  static constexpr uint16_t height = {image.height};
  static constexpr uint16_t bitsPerPixel = {bpp};''', file=of)
    if cmap:
        print(f'  static constexpr uint32_t colormapSize = {len(cmap)};',
              file=of)
        print('  // clang-format off\n'
              '  static constexpr uint32_t colormap[] = {', file=of)
        end = '\n'
        for i in sorted(cmap_i.keys()):
            rgb = cmap_i[i]
            prefix = '     ' if end == '\n' else ''
            end = '\n' if i % 8 == 7 else ' '
            print('%s0x%06x,' % (prefix, rgb), end=end, file=of)
        print('\n  };', file=of)

        print('  static constexpr uint8_t indexData[] = {', file=of)
        i = 0
        end = '\n'
        current = 0
        cbits = 0
        count = 0
        for y in range(sz[1]):
            for x in range(sz[0]):
                if image.mode == 'RGBA':
                    r, g, b = btes[i: i+3]
                    i += 4
                else:
                    r, g, b = btes[i: i+3]
                    i += 3
                rgb = (r << 16) | (g << 8) | b
                index = cmap[rgb]
                towrite = None
                if cbits + bpp > 8:
                    current = (current << 8) | (index << (16 - bpp - cbits))
                    towrite = (current >> 8)
                    current = current & 0xff
                    cbits += bpp - 8
                    prefix = '     ' if end == '\n' else ''
                    end = '\n' if (count % 16 == 15) else ' '
                    count += 1
                    print('%s0x%02x,' % (prefix, towrite), file=of, end=end)
                else:
                    current |= index << (8 - bpp - cbits)
                    cbits += bpp
        if cbits < 8:
            prefix = '     ' if end == '\n' else ''
            count += 1
            print('%s0x%02x,' % (prefix, towrite), file=of)
        print('  };', file=of)

        print('  // clang-format on\n  static constexpr uint32_t rgbData[] = {};', file=of)
        print('};', file=of)
    else:
        print('''  static constexpr uint32_t colormapSize = 0;
  static constexpr uint32_t colormap[] = {};
  static constexpr uint8_t indexData[] = {};
  // clang-format off
  static constexpr uint32_t rgbData[] = { ''', file=of)
        i = 0
        end = '\n'
        for y in range(sz[1]):
            for x in range(sz[0]):
                if image.mode == 'RGBA':
                    r, g, b = btes[i: i+3]
                    i += 4
                else:
                    r, g, b = btes[i: i+3]
                    i += 3
                prefix = '     ' if end == '\n' else ''
                end = '\n' if (x == sz[0] - 1 or x % 16 == 15) else ' '
                print('%s0x%02x%02x%02x,' % (prefix, r, g, b), file=of,
                      end=end)

        print('  };\n  // clang-format on\n};', file=of)
