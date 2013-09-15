/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#ifndef LEVEL_H
#define LEVEL_H

#include "romfile.h"
#include <cstdint>

#define MAX_2D_AREA 2048
#define MAX_3D_AREA 13312

extern const int numLevels[];
extern const int clippingTable[];

// location of where to write new level data
extern const uint newDataAddress[];

/*
  The level header. Some fields currently unknown.
  Most of this should be generated automatically by the editor.
*/
#pragma pack(1)
typedef struct {
    uint16_t dummy1;
    uint16_t width, length;
    uint16_t dummy2;
    uint16_t fieldWidth, fieldHeight;
    uint16_t alignHoriz, alignVert;
    char     mapID[12];
} header_t;

/*
  Level tile info (in chunks 1 to 4).
*/
typedef struct {
    uint8_t geometry, obstacle, height;
    struct {
        unsigned bumperSouth: 1;
        unsigned bumperEast:  1;
        unsigned bumperNorth: 1;
        unsigned bumperWest:  1;
        unsigned dummy:       3;
        unsigned layer:       1;
    } flags;
} maptile_t;

/*
  Z-clipping hash table entry (in chunk 10).
*/
typedef struct {
    uint8_t xLower, xUpper, prio;
    uint16_t zref;
} clip_t;
#pragma pack()

extern const maptile_t noTile;

/*
  Level tile info as it is passed to/from the tile edit window
  (and possibly a "copy/paste tile properties" feature in the future.)
*/
typedef struct {
    int  geometry, obstacle, height,
         bumperSouth, bumperEast, bumperNorth, bumperWest,
         layer, minHeight, maxHeight;
} tileinfo_t;

/*
  Definition for level data.
  Currently consists of tile and obstacle data and flags, as well as modified state
  (both overall and for the current session) and music track.
*/
typedef struct {
    header_t  header;

    // The maximum area of a map is 2048 maptiles. 64 is the maximum
    // size of either dimension needed to fit all of the original levels
    // (but the maximum size of each dimension depends on the size of the other,
    // so that width/length is always <= 2048.
    maptile_t tiles[64][64];

    // have any of the tile data fields been changed from the original data?
    // (determined based on their position in the ROM file, also set as soon
    // as level is edited)
    bool      modified;
    // have any of the tile data fields been changed in this session?
    // (set when modified, cleared when saved)
    bool      modifiedRecently;

    // music track number. see kirby.cpp (or stuff.cpp if i ever rename it)
    uint8_t   music;
} leveldata_t;

/*
  Functions for loading/saving level data
*/
leveldata_t*  loadLevel(ROMFile& file, uint num);
uint          saveLevel(ROMFile& file, uint num, leveldata_t *level, uint offset);
// uint saveAllLevels(ROMFile& file, leveldata_t **levels, uint num, uint offset = 0);
size_t        makeClipTable(const leveldata_t *level, uint8_t *buffer);
void          makeIsometricMap(uint16_t playfield[2][384][384], leveldata_t *level);

uint          levelHeight(const leveldata_t *level);
bool          waterLevel(const leveldata_t *level);
#endif // LEVEL_H
