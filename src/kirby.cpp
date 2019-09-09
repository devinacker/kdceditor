/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#include "kirby.h"
#include "level.h"
#include "metatile.h"
#include <QString>

#include <QtDebug>

using namespace stuff;

const int fgPaletteBase[]  =  {0xD4A9, 0xD8ED, 0xD8ED};
const int waterBase[][3]   = {{0xA444, 0xE030, 0xE030},
                              {0xA46E, 0xE05A, 0xE05A}};

const int paletteTable[]       =  {0x80D425, 0x80D869, 0x80D869};
const int waterTable[][3]      = {{0x8484AF, 0x8484AF, 0x8484AF},
                                  {0x8484F1, 0x8484F1, 0x8484F1}};
const int backgroundTable[][3] = {{0x80D0AF, 0x80D517, 0x80D517},
                                  {0x80D304, 0x80D748, 0x80D748},
                                  {0x80D324, 0x80D768, 0x80D768},
                                  {0x84CD23, 0x84CD42, 0x84CD42}};

const int musicTable[]   = {0x80C533, 0x80C99D, 0x80C99D};
const int newMusicAddr[] = {0x80F440, 0x80F950, 0x80F950};

// first dimension is indexed by game number (0 = kirby, 1 = sts)
const char* courseNames[][224 / 8] = {
    // KDC / Kirby Bowl courses
    {
        "1P Course 1",
        "1P Course 2",
        "1P Course 3",
        "1P Course 4",
        "1P Course 5",
        "1P Course 6",
        "1P Course 7",
        "1P Course 8",
        "1P Extra Course 1",
        "1P Extra Course 2",
        "1P Extra Course 3",
        "1P Extra Course 4",
        "1P Extra Course 5",
        "1P Extra Course 6",
        "1P Extra Course 7",
        "1P Extra Course 8",
        "2P Course 1",
        "2P Course 2",
        "2P Course 3",
        "2P Course 4",
        "Demo Course 1",
        "Demo Course 2",
        "Demo Course 3 / Test Course",
        "Test Course",
        "2P Extra Course 1",
        "2P Extra Course 2",
        "2P Extra Course 3",
        "2P Extra Course 4"
    },
    // Special Tee Shot courses
    {
        "Beginner Course",
        "Amateur Course",
        "Professional Course",
        "Master Course",
        "Extra Course 1",
        "Extra Course 2",
        "Extra Course 3",
        "Extra Course 4",
        "Gold Course"
    }
};

const bg_t bgNames[] = {
    {"Background 1 (clouds)",
     {0x8290, 0xC290, 0xC290},
     {0x92BEB1, 0x90A836, 0x90A836},
     {0x94AC83, 0x928000, 0x92855B},
     {0xCD33, 0xCD52, 0xCD52}},

    {"Background 2 (stars & moon)",
     {0x83D0, 0xC3D0, 0xC3D0},
     {0x92D18C, 0x90B1B2, 0x90B1B2},
     {0x94EDAB, 0x8EFB5F, 0x8EFB5F},
     {0xCECA, 0xCEE9, 0xCEE9}},

    {"Background 3 (waterfalls)",
     {0x8330, 0xC330, 0xC330},
     {0x93A043, 0x90ED83, 0x90ED83},
     {0x94967D, 0x91E7A2, 0x91EE62},
     {0xCE79, 0xCE98, 0xCE98}},

    {"Background 4 (jigsaw)",
     {0x82E0, 0xC2E0, 0xC2E0},
     {0x93E286, 0x91AD5B, 0x91B41B},
     {0x93D5F8, 0x91A0CD, 0x91A78D},
     {0xCE64, 0xCE83, 0xCE83}},

    {"Background 5 (candy)",
     {0x8380, 0xC380, 0xC380},
     {0x92AB0F, 0x909494, 0x909494},
     {0x93FA68, 0x91E20F, 0x91E8CF},
     {0xCFAE, 0xCFCD, 0xCFCD}},

    {"Background 6 (ocean)",
     {0x85E0, 0xC5E0, 0xC5E0},
     {0x9398A1, 0x90E5E1, 0x90E5E1},
     {0x94DA7C, 0x92B347, 0x92B8A2},
     {0xCFF3, 0xD012, 0xD012}}
/*
    something here (the GFX?) gets loaded to a different address than normal
    so it's not usable with the rest of the course BGs
    {"Background 7 (diamonds)",
     {0xD368, 0},
     {0x93EEC0, 0},
     {0x9798EF, 0},
     {0xD12D, 0}},
*/
};

const char* paletteNames[] = {
    "Course 1 (blue)",
    "Course 2 (green)",
    "Course 3 (purple)",
    "Course 4 (pink)",
    "Course 5 (tan)",
    "Course 6 (beige)",
    "Course 7 (grey)",
    "Course 8 (red)",
    "Extra course 7/8 (dark grey)",
    "Demo course (teal)"
};

const StringMap musicNames ({
    {0x7e, "7E: (none)"},
    {0x80, "80: Epilogue"},
    {0x82, "82: Title"},
    {0x83, "83: Opening demo (JP only)"},
    {0x84, "84: High scores"},
    {0x85, "85: Space Valley (course 2/7b)"},
    {0x86, "86: Over Water (course 1b)"},
    {0x87, "87: The Tricky Stuff (course 5b)"},
    {0x88, "88: Castles of Cake (course 6)"},
    {0x89, "89: Green Fields (course 5a/7a)"},
    {0x8a, "8A: The First Hole (course 1a/3)"},
    {0x8b, "8B: Iceberg Ocean (course 8)"},
    {0x8c, "8C: Last Hole"},
    {0x8d, "8D: Jigsaw Plains (course 4)"},
    {0x8f, "8F: Continue?"},
    {0x92, "92: Final score"},
    {0x93, "93: 2P course select"},
    {0x94, "94: Eyecatch"},
    {0x95, "95: Main menu"},
    {0x96, "96: 1P course select"},
    {0x97, "97: Scorecard"},
    {0x9a, "9A: Demo play"},
    {0x9b, "9B: Dedede 1"},
    {0x9c, "9C: Dedede 2"},
    {0x9f, "9F: Game over"}
});

const StringMap kirbyGeometry ({
    {0, "00: None"},
    {1, "01: Flat"},
    {2, "02: Four slopes up towards center"},

    //  this type is unusable
    //  {3, "03: Four slopes down into ground"},

    {4, "04: Slope down towards south"},
    {5, "05: Slope down towards east"},
    {6, "06: Slope down towards north"},
    {7, "07: Slope down towards west"},

    {8, "08: Slopes down towards south and east (inner)"},
    {9, "09: Slopes down towards north and east (inner)"},
    {10, "0A: Slopes down towards north and west (inner)"},
    {11, "0B: Slopes down towards south and west (inner)"},

    {12, "0C: Slopes down towards south and east (outer)"},
    {13, "0D: Slopes down towards north and east (outer)"},
    {14, "0E: Slopes down towards north and west (outer)"},
    {15, "0F: Slopes down towards south and west (outer)"},

    {16, "10: Slope down towards southeast (top)"},
    {17, "11: Slope down towards northeast (top)"},
    {18, "12: Slope down towards northwest (top)"},
    {19, "13: Slope down towards southwest (top)"},

    {20, "14: Slope down towards southeast (bottom)"},
    {21, "15: Slope down towards northeast (bottom)"},
    {22, "16: Slope down towards northwest (bottom)"},
    {23, "17: Slope down towards southwest (bottom)"},

    {24, "18: Slope down towards southeast (middle)"},
    {25, "19: Slope down towards northeast (middle)"},
    {26, "1A: Slope down towards northwest (middle)"},
    {27, "1B: Slope down towards southwest (middle)"}
});

const StringMap kirbyObstacles ({
    {0x00, "00: None"},

    {0x02, "02: Whispy Woods"},

    {0x04, "04: Sand trap"},
    {0x05, "05: Spike pit"},

    /*
        hole can't be placed independently in KDC
        (unless it can be placed on obstacle layer w/ correct palette somehow)
    {0x08, "08: Hole"},
    */

    {0x0c, "0C: Kirby"},
    {0x0d, "0D: King Dedede (course 24-1 only)"},

    {0x10, "10: Current (south)"},
    {0x11, "11: Current (east)"},
    {0x12, "12: Current (north)"},
    {0x13, "13: Current (west)"},

    {0x14, "14: Arrow (south)"},
    {0x15, "15: Arrow (east)"},
    {0x16, "16: Arrow (north)"},
    {0x17, "17: Arrow (west)"},

    {0x18, "18: Booster (south)"},
    {0x19, "19: Booster (east)"},
    {0x1a, "1A: Booster (north)"},
    {0x1b, "1B: Booster (west)"},

    {0x1c, "1C: Air vent (north-south)"},
    {0x1d, "1D: Air vent (east-west)"},

    {0x20, "20: Bounce (use with tile 04)"},
    {0x21, "21: Bounce (use with tile 05)"},
    {0x22, "22: Bounce (use with tile 06)"},
    {0x23, "23: Bounce (use with tile 07)"},
    {0x24, "24: Bounce"},

    {0x28, "28: Bumper (north to south)"},
    {0x29, "29: Bumper (east to west)"},
    {0x2a, "2A: Bumper (south to west)"},
    {0x2b, "2B: Bumper (north to west)"},
    {0x2c, "2C: Bumper (north to east)"},
    {0x2d, "2D: Bumper (south to east)"},

    {0x30, "30: Conveyor belt (south)"},
    {0x31, "31: Conveyor belt (east)"},
    {0x32, "32: Conveyor belt (north)"},
    {0x33, "33: Conveyor belt (west)"},

    {0x34, "34: Conveyor belt (north, use with tile 04)"},
    {0x35, "35: Conveyor belt (south, use with tile 04)"},
    {0x36, "36: Conveyor belt (west, use with tile 05)"},
    {0x37, "37: Conveyor belt (east, use with tile 05)"},
    {0x38, "38: Conveyor belt (south, use with tile 06)"},
    {0x39, "39: Conveyor belt (north, use with tile 06)"},
    {0x3a, "3A: Conveyor belt (east, use with tile 07)"},
    {0x3b, "3B: Conveyor belt (west, use with tile 07)"},

    {0x40, "40: Waddle Dee"},
    {0x41, "41: Rocky"},
    {0x42, "42: Waddle Doo"},
    {0x43, "43: Flamer"},
    {0x44, "44: Spiney"},
    {0x45, "45: Twister"},
    {0x46, "46: Wheelie"},
    {0x47, "47: Sparky"},
    {0x48, "48: Starman"},
    {0x49, "49: Chilly"},
    {0x4a, "4A: Broom Hatter"},
    {0x4b, "4B: Squishy"},
    {0x4c, "4C: Kabu"},
    {0x4d, "4D: Gaspar"},
    {0x4e, "4E: Pumpkin"},
    {0x4f, "4F: UFO"},
    {0x50, "50: Gaspar (higher)"},
    {0x51, "51: Pumpkin (higher)"},
    {0x52, "52: UFO (higher)"},
    {0x57, "57: Transformer"},

    {0x58, "58: Mr. Bright switch"},
    {0x59, "59: Mr. Shine switch"},
    {0x5a, "5A: Rotating space switch (off)"},
    {0x5b, "5B: Rotating space switch (on)"},
    {0x5c, "5C: Water switch (on)"},
    {0x5d, "5D: Water switch (off)"},

    {0x61, "61: Water hazard"},
    {0x64, "64: Water hazard (use with tile 04)"},
    {0x65, "65: Water hazard (use with tile 05)"},
    {0x66, "66: Water hazard (use with tile 06)"},
    {0x67, "67: Water hazard (use with tile 07)"},
    {0x68, "68: Water hazard (use with tile 08)"},
    {0x69, "69: Water hazard (use with tile 09)"},
    {0x6a, "6A: Water hazard (use with tile 0A)"},
    {0x6b, "6B: Water hazard (use with tile 0B)"},
    {0x6c, "6C: Water hazard (use with tile 0C)"},
    {0x6d, "6D: Water hazard (use with tile 0D)"},
    {0x6e, "6E: Water hazard (use with tile 0E)"},
    {0x6f, "6F: Water hazard (use with tile 0F)"},

    /*
          Most of these are not used in KDC.
    */
    {0x70, "70: Rotating space (clockwise, always on)"},
    {0x71, "71: Rotating space (counterclockwise, always on)"},
    {0x72, "72: Rotating space (clockwise, always on, slow)"},
    {0x73, "73: Rotating space (counterclockwise, always on, slow)"},
    {0x74, "74: Rotating space (clockwise, switch)"},
    {0x75, "75: Rotating space (counterclockwise, switch)"},
    {0x76, "76: Rotating space (clockwise, switch, slow)"},
    {0x77, "77: Rotating space (counterclockwise, switch, slow)"},
    {0x78, "78: Rotating space (clockwise, switch-opposite)"},
    {0x79, "79: Rotating space (counterclockwise, switch-opposite)"},
    {0x7a, "7A: Rotating space (clockwise, switch-opposite, slow)"},
    {0x7b, "7B: Rotating space (counterclockwise, switch-opposite, slow)"},

    {0x80, "80: Gordo (moves south, faces south)"},
    {0x81, "81: Gordo (moves east, faces south)"},
    {0x82, "82: Gordo (moves north, faces south)"},
    {0x83, "83: Gordo (moves west, faces south)"},
    {0x84, "84: Gordo (moves south, faces east)"},
    {0x85, "85: Gordo (moves east, faces east)"},
    {0x86, "86: Gordo (moves north, faces east)"},
    {0x87, "87: Gordo (moves west, faces east)"},
    {0x88, "88: Gordo (moves south, faces north)"},
    {0x89, "89: Gordo (moves east, faces north)"},
    {0x8a, "8A: Gordo (moves north, faces north)"},
    {0x8b, "8B: Gordo (moves west, faces north)"},
    {0x8c, "8C: Gordo (moves south, faces west)"},
    {0x8d, "8D: Gordo (moves east, faces west)"},
    {0x8e, "8E: Gordo (moves north, faces west)"},
    {0x8f, "8F: Gordo (moves west, faces west)"},

    {0x90, "90: Gordo (moves up/down, faces south)"},
    {0x91, "91: Gordo (moves up/down, faces east)"},
    {0x92, "92: Gordo (moves up/down, faces north)"},
    {0x93, "93: Gordo (moves up/down, faces west)"},
    {0x94, "94: Gordo (moves down/up, faces south)"},
    {0x95, "95: Gordo (moves down/up, faces east)"},
    {0x96, "96: Gordo (moves down/up, faces north)"},
    {0x97, "97: Gordo (moves down/up, faces west)"},

    {0x98, "98: Gordo path (north-south)"},
    {0x99, "99: Gordo path (east-west)"},
    {0x9a, "9A: Gordo path (northwest corner)"},
    {0x9b, "9B: Gordo path (southwest corner)"},
    {0x9c, "9C: Gordo path (southeast corner)"},
    {0x9d, "9D: Gordo path (northeast corner)"},
    {0x9e, "9E: Gordo endpoint (south)"},
    {0x9f, "9F: Gordo endpoint (east)"},
    {0xa0, "A0: Gordo endpoint (north)"},
    {0xa1, "A1: Gordo endpoint (west)"},

    {0xac, "AC: Kracko (no lightning)"},
    {0xad, "AD: Kracko (lightning 1)"},
    {0xae, "AE: Kracko (lightning 2)"},

    {0xb0, "B0: Blue warp 1 (south)"},
    {0xb1, "B1: Blue warp 1 (east)"},
    {0xb2, "B2: Blue warp 1 (north)"},
    {0xb3, "B3: Blue warp 1 (west)"},
    {0xb4, "B4: Blue warp 2 (south)"},
    {0xb5, "B5: Blue warp 2 (east)"},
    {0xb6, "B6: Blue warp 2 (north)"},
    {0xb7, "B7: Blue warp 2 (west)"},
    {0xb8, "B8: Red warp 1"},
    {0xb9, "B9: Red warp 2"},

    {0xc0, "C0: Starting line (west end)"},
    {0xc1, "C1: Starting line"},
    {0xc2, "C2: Starting line (east end)"},
    {0xc3, "C3: Kirby (course 24-1 only)"},
});

/*
  This array maps conveyor belt types to their counterparts for the different slope types.
  Dimension 1 is the belt direction, dimension 2 is the slope direction
*/
const stuff::type_e conveyorMap[4][4] = {
//slope      south          east          north          west
//beltSouth
            {beltSouthDown, nothing,      beltSouthUp,   nothing},
//beltEast
            {nothing,       beltEastDown, nothing,       beltEastUp},
//beltNorth
            {beltNorthUp,   nothing,      beltNorthDown, nothing},
//beltWest
            {nothing,       beltWestUp,   nothing,       beltWestDown}
};

Util* Util::_instance = nullptr;

Util::Util() {
 _bounce.load  (":images/bounce.png");
 _bumpers.load (":images/bumpers.png");
 _conveyor.load(":images/conveyor.png");
 _dedede.load  (":images/dedede.png");
 _enemies.load (":images/enemies.png");
 _gordo.load   (":images/gordo3d.png");
 _movers.load  (":images/movers.png");
 _player.load  (":images/kirby.png");
 _rotate.load  (":images/rotate.png");
 _switches.load(":images/switches.png");
 _tiles.load   (":images/terrain.png");
 _traps.load   (":images/traps.png");
 _unknown.load (":images/unknown.png");
 _warps.load   (":images/warps.png");
 _water.load   (":images/water.png");
}

Util* Util::Instance() {
  if (_instance == nullptr) {
    _instance = new Util();
  }

  return _instance;
}

bool Util::IsObstacleCharacterType(const int& obstacle) {
  if (obstacle == 0x02) {  // whispy woods
    return true;
  } else if (obstacle == 0x0c || obstacle == 0xc3) {  // kirby
    return true;
  } else if (obstacle == 0x0d) {  // dedede
    return true;
  } else if (obstacle >= 0x40 && obstacle <= 0x52) {  // most enemies
    return true;
  } else if (obstacle == 0x57) {  // transformer
    return true;
  } else if (obstacle >= 0x80 && obstacle <= 0x97) {  // gordo
    return true;
  } else if (obstacle >= 0xac && obstacle <= 0xae) {  // kracko
    return true;
  } else {
    return false;
  }
}

bool Util::GetPixmapSettingsForObstacle(const int& obstacle, const QPixmap** pixmap, int* frame) {
  if (obstacle == 0) {
    return false;
  }

  // whispy woods (index 0x00 in enemies.png)
  if (obstacle == 0x02) {
    (*pixmap) = &_enemies;
    (*frame) = 0;

    // kirby's start pos (kirby.png)
    // (this time also use the final boss version)
  } else if (obstacle == 0x0c || obstacle == 0xc3) {
    (*pixmap) = &_player;
    (*frame) = 0;

    // dedede (frame 0 in dedede.png)
  } else if (obstacle == 0x0d) {
    (*pixmap) = &_dedede;
    (*frame) = 0;

    // most enemies (ind. 01 to 13 in enemies.png)
  } else if (obstacle >= 0x40 && obstacle <= 0x52) {
    (*pixmap) = &_enemies;
    (*frame) = obstacle - 0x40 + 1;

    // transformer (ind. 14 in enemies.png
  } else if (obstacle == 0x57) {
    (*pixmap) = &_enemies;
    (*frame) = 0x14;

    // gordo (ind. 00 to 21 in gordo.png)
  } else if (obstacle >= 0x80 && obstacle <= 0x97) {
    (*pixmap) = &_gordo;
    (*frame) = obstacle - 0x80;

    // kracko (index 15-17 in enemies.png)
  } else if (obstacle >= 0xac && obstacle <= 0xae) {
    (*pixmap) = &_enemies;
    (*frame) = obstacle - 0xac + 0x15;

    // sand trap (index 0 in traps.png)
  } else if (obstacle == 0x04) {
    (*pixmap) = &_traps;
    (*frame) = 0;

    // spike pit (index 1 in traps.png)
  } else if (obstacle == 0x05) {
    (*pixmap) = &_traps;
    (*frame) = 1;

    // current, arrows, boosters, vents
    // (ind. 00 to 0d in movers.png)
  } else if (obstacle >= 0x10 && obstacle <= 0x1d) {
    (*pixmap) = &_movers;
    (*frame) = obstacle - 0x10;

    // bouncy pads (ind. 0 to 4 in bounce.png)
  } else if (obstacle >= 0x20 && obstacle <= 0x24) {
    (*pixmap) = &_bounce;
    (*frame) = obstacle - 0x20;

    // bumpers (start at index 4 in bumpers.png)
  } else if (obstacle >= 0x28 && obstacle <= 0x2d) {
    (*pixmap) = &_bumpers;
    (*frame) = obstacle - 0x28 + 4;

    // conveyor belts (ind. 0 to b in conveyor.png)
  } else if (obstacle >= 0x30 && obstacle <= 0x3b) {
    (*pixmap) = &_conveyor;
    (*frame) = obstacle - 0x30;

    // switches (ind. 0 to 5 in switches.png)
  } else if (obstacle >= 0x58 && obstacle <= 0x5d) {
    (*pixmap) = &_switches;
    (*frame) = obstacle - 0x58;

    // water hazards (ind. 0 to e in water.png)
    // (note types 62 & 63 seem unused)
  } else if (obstacle >= 0x61 && obstacle <= 0x6f) {
    (*pixmap) = &_water;
    (*frame) = obstacle - 0x61;

    // rotating spaces (ind. 0-b in rotate.png)
  } else if (obstacle >= 0x70 && obstacle <= 0x7b) {
    (*pixmap) = &_rotate;
    (*frame) = obstacle & 0x01;

    // warps (ind. 0 to 9 in warps.png)
  } else if (obstacle >= 0xb0 && obstacle <= 0xb9) {
    (*pixmap) = &_warps;
    (*frame) = obstacle - 0xb0;

    // starting line (ind. 1 to 4 in dedede.png)
  } else if (obstacle >= 0xc0 && obstacle <= 0xc3) {
    (*pixmap) = &_dedede;
    (*frame) = obstacle - 0xc0 + 1;

    // anything else - question mark (or don't draw)
  } else {
#ifdef QT_DEBUG
    (*pixmap) = &_unknown;
    (*frame) = 0;
#endif
    return false;
  }

  return true;
}

bool Util::ApplyTileToExistingTile(const tileinfo_t& tileInfo, maptile_t* newTile) {
  const bool relativeHeight = tileInfo.height == -1;

  if (newTile->geometry == 0 && tileInfo.geometry == -1) {
    return false;
  } else if (tileInfo.geometry == 0) {
    *newTile = noTile;
    return false;
  }

  if (tileInfo.geometry >= 0)
    newTile->geometry = tileInfo.geometry;
  if (tileInfo.obstacle >= 0) {
    // handle multiple types for water hazard based on terrain value
    if (tileInfo.obstacle == water
        && newTile->geometry >= slopes && newTile->geometry < endSlopes)
      newTile->obstacle = water - 1 + newTile->geometry;
    // handle multiple types for bounce pads
    else if (tileInfo.obstacle == bounceFlat
             && newTile->geometry >= slopes && newTile->geometry < slopesDouble)
      newTile->obstacle = bounce + newTile->geometry - slopes;
    // handle multiple types for conveyor belts
    else if (tileInfo.obstacle >= belts && tileInfo.obstacle < beltSlopes
             && newTile->geometry >= slopes && newTile->geometry < slopesDouble)
      newTile->obstacle = conveyorMap[tileInfo.obstacle - belts][newTile->geometry - slopes];
    else
      newTile->obstacle = tileInfo.obstacle;
  }

  if (tileInfo.bumperNorth >= 0)
    newTile->flags.bumperNorth = tileInfo.bumperNorth;
  if (tileInfo.bumperSouth >= 0)
    newTile->flags.bumperSouth = tileInfo.bumperSouth;
  if (tileInfo.bumperEast >= 0)
    newTile->flags.bumperEast = tileInfo.bumperEast;
  if (tileInfo.bumperWest>= 0)
    newTile->flags.bumperWest = tileInfo.bumperWest;

  if (relativeHeight)
    newTile->height += tileInfo.height;
  else
    newTile->height = tileInfo.height;

  if (tileInfo.layer >= 0) {
    newTile->flags.layer = tileInfo.layer;
  }

  newTile->flags.dummy = 0;

  return true;
}
