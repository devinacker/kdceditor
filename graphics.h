/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#ifndef GRAPHICS_H
#define GRAPHICS_H

#define PAL(x)  (x << 10)
#define PALN(x) (x >> 10 & 0x7)
#define TILE(x) (x & 0x3FF)
#define PRI     0x2000
#define FH      0x4000
#define FV      0x8000
#define FB      FH|FV

#define TILE_SIZE 64
#define ISO_TILE_SIZE 8

#endif // GRAPHICS_H
