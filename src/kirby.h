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

#include "level.h"
#include <QPixmap>
#include <QString>
#include <map>

typedef std::map<uint, QString> StringMap;

typedef struct {
    char name[50];
    int  palette[3];
    int  pointer1[3];
    int  pointer2[3];
    int  anim[3];
} bg_t;

extern const int   fgPaletteBase[3];
extern const int   waterBase[2][3];
extern const int   bgPaletteBase[3];

extern const int   paletteTable[3];
extern const int   waterTable[2][3];
extern const int   backgroundTable[4][3];
extern const int   musicTable[3];
extern const int   newMusicAddr[3];

extern const char* courseNames[][224 / 8];
extern const bg_t  bgNames[];
extern const char* paletteNames[];
extern const StringMap musicNames;

extern const StringMap kirbyGeometry;
extern const StringMap kirbyObstacles;

class Util {

 public:
  static Util* Instance();

  bool GetPixmapSettingsForObstacle(const int& obstacle, const QPixmap** pixmap, int* frame);
  bool ApplyTileToExistingTile(const tileinfo_t& tileInfo, maptile_t* newTile);
  bool IsObstacleCharacterType(const int& obstacle);

 private:
  explicit Util();
  static Util* _instance;

  QPixmap _bounce;
  QPixmap _bumpers;
  QPixmap _conveyor;
  QPixmap _dedede;
  QPixmap _enemies;
  QPixmap _gordo;
  QPixmap _movers;
  QPixmap _player;
  QPixmap _rotate;
  QPixmap _switches;
  QPixmap _tiles;
  QPixmap _traps;
  QPixmap _unknown;
  QPixmap _warps;
  QPixmap _water;
};

#endif // KIRBY_H
