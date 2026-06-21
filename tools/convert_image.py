#!/usr/bin/env python

import sys
from PIL import Image
import os
import numpy as np
import glob

# Convert an image to a generated c++ file containing an exploitable image file
# First parameter is the image, second one is the output C++ file name

in_img = sys.argv[1]
out_src = sys.argv[2]
auto_classname = True
if len(sys.argv) >= 4:
    out_cls = sys.argv[3]
    auto_classname = False
else:
    out_cls = os.path.basename(out_src)
    out_cls = out_cls.split('.')[0]
    if out_cls.endswith('_image'):
        out_cls = out_cls[:-6]
    out_cls = out_cls[0].upper() + out_cls[1:] + 'ImageTy'

has_alpha = ('_mask' in in_img)
has_anim = ('_anim_' in in_img)
# if auto_classname:
#     out_cls = out_cls.replace('_mask', '')
#     if has_anim:
#         out_cls = out_cls.split('_anim_', 1)[0] + 'ImageTy'

frames = [in_img]
if has_anim:
    fext = os.path.basename(in_img).rsplit('.', 1)
    bname = fext[0]
    base = bname.rsplit('_', 1)
    frame = int(base[1])
    if frame != 0:
        sys.exit(0)  # run only on 1st frame
    pat = os.path.join(os.path.dirname(in_img), f'{base[0]}_*.{fext[1]}')
    frames = sorted(glob.glob(pat))

print('has_alpha:', has_alpha)
print('has_anim:', has_anim)
print('out_cls:', out_cls)
widths = []
heights = []
x_offsets = []
y_offsets = []
bpps = []
cmlap_sizes = []
cmaps = []
index_datas = []
rgb_datas = []

with open(out_src, 'w') as of:

    print('// GENERATED FILE, DO NOT MODIFY\n'
          '/// Compressed image storage class\n\n'
          '// clang-format off', file=of)
    print(f'''struct {out_cls}
{{
  static constexpr uint16_t frames = {len(frames):2d};        ///< number of frame images''', file=of)

    for image_f in frames:
        image = Image.open(image_f)
        btes = image.tobytes()
        sz = (image.width, image.height)

        cmap = {}
        i = 0
        cancel = False
        pmin = [None, None]
        pmax = [None, None]
        for y in range(sz[1]):
            for x in range(sz[0]):
                if image.mode == 'RGBA':
                    r, g, b = btes[i: i+3]
                    if has_alpha:
                        a = btes[i+3]
                        if a != 0:
                            if pmin[0] is None:
                                pmin = [x, y]
                                pmax = [x, y]
                            else:
                                pmin = [min(pmin[0], x), min(pmin[1], y)]
                                pmax = [max(pmax[0], x), max(pmax[1], y)]
                    i += 4
                else:
                    r, g, b = btes[i: i+3]
                    a = 0xff
                    i += 3
                if has_alpha:
                    rgb = (r << 24) | (g << 16) | (b << 8) | a
                else:
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

        offset = [0, 0]
        if has_alpha and pmin[0] is not None:
            sz[0] = pmax[0] - pmin[0] + 1
            sz[1] = pmax[1] - pmin[1] + 1
            offset = pmin

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

        widths.append(sz[0])
        heights.append(sz[1])
        x_offsets.append(offset[0])
        y_offsets.append(offset[1])
        bpps.append(bpp)
        bdata = []

        cmap_dat = []
        cmaps.append(cmap_dat)
        data = []
        index_datas.append(data)
        rgb_data = []
        rgb_datas.append(rgb_data)
        if cmap:
            for i in sorted(cmap_i.keys()):
                rgb = cmap_i[i]
                fmt = '0x%08x' if has_alpha else '0x%06x'
                cmap_dat.append(fmt % rgb)

            i = 0
            current = 0
            cbits = 0
            for y in range(image.height):
                for x in range(image.width):
                    if image.mode == 'RGBA':
                        r, g, b = btes[i: i+3]
                        if has_alpha:
                            a = btes[i+3]
                        i += 4
                    else:
                        r, g, b = btes[i: i+3]
                        a = 0xff
                        i += 3
                    if x < offset[0] or x >= offset[0] + sz[0] \
                            or y < offset[1] or y >= offset[1] + sz[1]:
                        continue  # outside used part
                    if has_alpha:
                        rgb = (r << 24) | (g << 16) | (b << 8) | a
                    else:
                        rgb = (r << 16) | (g << 8) | b
                    index = cmap[rgb]
                    towrite = None
                    if cbits + bpp > 8:
                        current = (current << 8) | (index << (16 - bpp - cbits))
                        towrite = (current >> 8)
                        current = current & 0xff
                        cbits += bpp - 8
                        data.append(towrite)
                    else:
                        current |= index << (8 - bpp - cbits)
                        cbits += bpp
            if cbits < 8:
                towrite = current & 0xff
                data.append(towrite)

        else:
            print('''  static constexpr uint32_t colormapSize = 0;
    static constexpr uint32_t colorx_offsets = []
map[] = {};
    static constexpr uint8_t indexData[] = {};
    static constexpr uint32_t rgbData[] = { ''', file=of)
            i = 0
            end = '\n'
            for y in range(sz[1]):
                for x in range(sz[0]):
                    if image.mode == 'RGBA':
                        r, g, b = btes[i: i+3]
                        if has_alpha:
                            a = btes[i+3]
                        i += 4
                    else:
                        r, g, b = btes[i: i+3]
                        a = 0xff
                        i += 3
                    prefix = '     ' if end == '\n' else ''
                    end = '\n' if (x == sz[0] - 1 or x % 16 == 15) else ' '
                    if has_alpha:
                        print('%s0x%02x%02x%02x%02x,' % (prefix, r, g, b, a),
                              file=of, end=end)
                    else:
                        print('%s0x%02x%02x%02x,' % (prefix, r, g, b), file=of,
                              end=end)

    print(f'  static constexpr bool hasAlpha = {" true;" if has_alpha else "false;"}       ///< overlays have an alpha channel (RGBA)', file=of)

    print('  static constexpr uint16_t width[] = {         ///< width of the images',
          file=of)
    print('    ' + ', '.join([f'{w}' for w in widths]) + ' };', file=of)
    print('  static constexpr uint16_t height[] = {        ///< height of the images',
          file=of)
    print('    ' + ', '.join([f'{h}' for h in heights]) + ' };', file=of)
    print('  static constexpr uint16_t x_offset[] = {      ///< x offset of the images',
          file=of)
    print('    ' + ', '.join([f'{w}' for w in x_offsets]) + ' };', file=of)
    print('  static constexpr uint16_t y_offset[] = {      ///< y offset of the images',
          file=of)
    print('    ' + ', '.join([f'{w}' for w in y_offsets]) + ' };', file=of)
    print('  static constexpr uint16_t bitsPerPixel[] = {  ///< used bits per pixel', file=of)
    print('    ' + ', '.join([f'{b}' for b in bpps]) + ' };', file=of)

    print('  static constexpr uint32_t colormapSize[] = {  ///< size of the colormaps', file=of)
    print('    ' + ', '.join([f'{len(cmap)}' for cmap in cmaps]) + ' };', file=of)

    print('  static constexpr uint32_t indexFrameLength[] = {  ///< size of the compressed image frames', file=of)
    print('    ' + ', '.join([f'{len(data)}' for data in index_datas]) + ' };', file=of)

    print('  static constexpr uint32_t rgbFrameLength[] = {  ///< size of the RGB image frames', file=of)
    print('    ' + ', '.join([f'{len(data)}' for data in rgb_datas]) + ' };', file=of)

    print('  /// map index to color\n'
          '  static constexpr uint32_t colormap[] = {', file=of)
    for cmap_dat in cmaps:
        end = '\n'
        for i, rgb in enumerate(cmap_dat):
            prefix = '      ' if end == '\n' else ''
            end = '\n' if i % 8 == 7 else ' '
            print(f'{prefix}{rgb},', end=end, file=of)
            i += 1
        print(file=of)
    print('  };', file=of)

    print('  /// bit packed image', file=of)
    print('  static constexpr uint8_t indexData[] = {', file=of)
    for im in range(len(index_datas)):
        if len(cmaps[im]) != 0:
            end = '\n'
            for count, x in enumerate(index_datas[im]):
                prefix = '      ' if end == '\n' else ''
                end = '\n' if (count % 16 == 15) else ' '
                print('%s0x%02x,' % (prefix, x), file=of, end=end)
            print(file=of)
    print('  };', file=of)

    print('  /// Store the RGB data', file=of)
    print('  static constexpr uint8_t rgbData[] = {', file=of)
    # for i in range(len(index_datas)):
        # if len(cmaps[i]) != 0:
            # print('    {},', file=of)
    print('  };', file=of)

    print('};\n  // clang-format on', file=of)
