/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#ifndef KIRBY_H
#define KIRBY_H

#define KIRBY_GEOM_TYPES       0x1C
#define KIRBY_OBSTACLE_TYPES   0xC4
#define NUM_FG_PALETTES          10
#define FG_PALETTE_SIZE        0x20
#define WATER_PALETTE_SIZE    0x120
#define BG_PALETTE_SIZE        0x28
#define NUM_BACKGROUNDS           6

#include <QString>
#include <map>

typedef std::map<uint, QString> StringMap;

typedef struct {
    char name[50];
    int  palette[2];
    int  pointer1[2];
    int  pointer2[2];
    int  anim[2];
} bg_t;

extern const int   fgPaletteBase[2];
extern const int   waterBase[2][2];
extern const int   bgPaletteBase[2];

extern const int   paletteTable[2];
extern const int   waterTable[2][2];
extern const int   backgroundTable[4][2];
extern const int   musicTable[2];
extern const int   newMusicAddr[2];

extern const char* courseNames[][224 / 8];
extern const bg_t  bgNames[];
extern const char* paletteNames[];
extern const StringMap musicNames;

extern const StringMap kirbyGeometry;
extern const StringMap kirbyObstacles;

#endif // KIRBY_H
