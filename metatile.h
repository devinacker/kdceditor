/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#ifndef METATILE_H
#define METATILE_H

#include "kirby.h"

namespace stuff {
enum type_e {
    nothing = 0,
    wall = -1,

    // terrain tile types
    flat       = 0x1,
    slopesUp   = 0x2,
    slopesDown = 0x3,
    slopes     = 0x4,
    slopeSouth = 0x4,
    slopeEast  = 0x5,
    slopeNorth = 0x6,
    slopeWest  = 0x7,
    slopesDouble = 0x8,
    slopeSouthAndEastInner = 0x8,
    slopeNorthAndEastInner = 0x9,
    slopeNorthAndWestInner = 0xA,
    slopeSouthAndWestInner = 0xB,
    slopeSouthAndEastOuter = 0xC,
    slopeNorthAndEastOuter = 0xD,
    slopeNorthAndWestOuter = 0xE,
    slopeSouthAndWestOuter = 0xF,
    endSlopes           = 0x10,
    slopesUpper         = 0x10,
    slopeSoutheastUpper = 0x10,
    slopeNortheastUpper = 0x11,
    slopeNorthwestUpper = 0x12,
    slopeSouthwestUpper = 0x13,
    endSlopesUpper      = 0x14,
    slopesLower         = 0x14,
    slopeSoutheastLower = 0x14,
    slopeNortheastLower = 0x15,
    slopeNorthwestLower = 0x16,
    slopeSouthwestLower = 0x17,
    endSlopesLower      = 0x18,
    slopesFull         = 0x18,
    slopeSoutheastFull = 0x18,
    slopeNortheastFull = 0x19,
    slopeNorthwestFull = 0x1A,
    slopeSouthwestFull = 0x1B,

    // obstacle tile types
    extraTiles = 0x100,

    sand   = 0x4,
    spikes = 0x5,
    spikesEx = spikes | extraTiles,

    current      = 0x10,
    currentSouth = 0x10,
    currentEast  = 0x11,
    currentNorth = 0x12,
    currentWest  = 0x13,
    endCurrent   = 0x14,

    arrowSouth = 0x14,
    arrowEast  = 0x15,
    arrowNorth = 0x16,
    arrowWest  = 0x17,

    boosterSouth = 0x18,
    boosterEast  = 0x19,
    boosterNorth = 0x1A,
    boosterWest  = 0x1B,

    ventNorthSouth = 0x1C,
    ventEastWest   = 0x1D,

    bounce      = 0x20,
    bounceSouth = 0x20,
    bounceEast  = 0x21,
    bounceNorth = 0x22,
    bounceNorthEx = bounceNorth | extraTiles,
    bounceWest  = 0x23,
    bounceWestEx  = bounceWest  | extraTiles,
    bounceFlat  = 0x24,

    bumperNorthSouth = 0x28,
    bumperEastWest   = 0x29,
    bumperSouthWest  = 0x2A,
    bumperNorthWest  = 0x2B,
    bumperNorthEast  = 0x2C,
    bumperSouthEast  = 0x2D,

    belts         = 0x30,
    beltSouth     = 0x30,
    beltEast      = 0x31,
    beltNorth     = 0x32,
    beltWest      = 0x33,
    beltSlopes    = 0x34,
    beltNorthUp   = 0x34,
    beltSouthDown = 0x35,
    beltWestUp    = 0x36,
    beltEastDown  = 0x37,
    beltSouthUp   = 0x38,
    beltNorthDown = 0x39,
    beltEastUp    = 0x3A,
    beltWestDown  = 0x3B,

    switchShineBright = 0x58,
    switchBright    = 0x58,
    switchShine     = 0x59,
    switchRotate    = 0x5A,
    switchRotateOff = 0x5A,
    switchRotateOn  = 0x5B,
    switchWater     = 0x5C,
    switchWaterOn   = 0x5C,
    switchWaterOff  = 0x5D,

    water      = 0x61,
    waterSouth = 0x64,
    waterEast  = 0x65,
    waterNorth = 0x66,
    waterWest  = 0x67,
    waterSouthAndEastInner = 0x68,
    waterNorthAndEastInner = 0x69,
    waterNorthAndWestInner = 0x6A,
    waterSouthAndWestInner = 0x6B,
    waterSouthAndEastOuter = 0x6C,
    waterNorthAndEastOuter = 0x6D,
    waterNorthAndWestOuter = 0x6E,
    waterSouthAndWestOuter = 0x6F,
    endWater   = 0x70,


    rotate                 = 0x70,
    rotateCW               = 0x70,
    rotateCCW              = 0x71,
    endRotate              = 0x78,
    rotateCWOpposite       = 0x78,
    rotateCCWOpposite      = 0x79,
    endRotateOpposite      = 0x7C,

    warpSouth = 0xB0,
    warpEast  = 0xB1,
    warpNorth = 0xB2,
    warpWest  = 0xB3,

    // remapped to B0-B3 at render time
    warpSouth2 = 0xB4,
    warpEast2  = 0xB5,
    warpNorth2 = 0xB6,
    warpWest2  = 0xB7,

    warpRed  = 0xB8,
    // remapped to B8
    warpRed2 = 0xB9,

    // all remapped to startLine
    startLineWest  = 0xC0,
    startLine      = 0xC1,
    startLineEast  = 0xC2,
    kirbyStartLine = 0xC3

};
}

typedef struct {
    stuff::type_e type, adjacent;
    uint16_t   tiles[8][8];
} metatile_t;

extern const uint16_t stackTile[2][8];
extern const uint16_t bottomTile[2][8];
extern const metatile_t metatilesTerrain[];
extern const metatile_t metatilesObstacles[];

extern const stuff::type_e trueCenterLeftTable[];
extern const stuff::type_e trueCenterRightTable[];

extern const metatile_t bordersSouth[];
extern const metatile_t bordersEast[];
extern const metatile_t bordersNorth[];
extern const metatile_t bordersWest[];
extern const metatile_t bordersNorthStart[];
extern const metatile_t bordersWestStart[];
extern const metatile_t bordersAll[];

metatile_t buildMetatile(int, int, int,
                         bool, bool, bool, bool,
                         bool northStart, bool westStart);
metatile_t buildObstacle(int, int, int);
metatile_t findMetatile(const metatile_t *array, int type, int other);

#endif // METATILE_H
