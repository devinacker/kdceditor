/*
  metatile.cpp

  Contains code for generating metatiles - the individual parts of the isometric tilemap
  corresponding to individual tiles on the 2D map, based on a tile's terrain (or obstacle) and the
  ones to the north and west of it.

  Metatiles are stored as 8x8 arrays of tiles, where the first 4 columns represent a tile and its
  connection to the west (if any), and the other 4 represent its connection to the north.

  For (most of) the actual tile data, see the other metatile_*.cpp files.

  This code is released under the terms of the MIT license.
  See COPYING.txt for details.

*/

#include "metatile.h"
#include "graphics.h"

using namespace stuff;

// These tables map halves/edges of some terrain types to other types.
// This is done when a terrain type (mainly two-way slopes or diagonal slope top/bottom parts)
// can be drawn using parts of other types.
const stuff::type_e trueLeftTable[KIRBY_GEOM_TYPES] = {
    // nothing, flat, slopes up, slopes down
    nothing, flat, slopeEast, slopeSouthAndWestInner,
    // cardinal slopes
    slopeSouth, slopeEast, slopeNorth, slopeWest,
    // inner slopes (SE, NE, NW, SW)
    slopeSouth, slopeNorth, slopeWest, slopeSouthAndWestInner,
    // outer slopes
    slopeEast, slopeEast, slopeNorth, slopeSouthAndWestOuter,
    // diagonal slopes upper
    slopeSoutheastFull, slopeNortheastFull, flat, flat,
    // diagonal slopes lower
    flat, flat, flat, slopeSouthwestFull,
    // diagonal slopes full
    slopeSoutheastFull, slopeNortheastFull, slopeNorthwestFull, slopeSouthwestFull
};

const stuff::type_e trueRightTable[KIRBY_GEOM_TYPES] = {
    // nothing, flat, slopes up, slopes down
    nothing, flat, slopeSouth, slopeNorthAndEastInner,
    // cardinal slopes
    slopeSouth, slopeEast, slopeNorth, slopeWest,
    // inner slopes (SE, NE, NW, SW)
    slopeEast, slopeNorthAndEastInner, slopeNorth, slopeWest,
    // outer slopes
    slopeSouth, slopeNorthAndEastOuter, slopeWest, slopeSouth,
    // diagonal slopes upper
    slopeSoutheastFull, flat, flat, slopeSouthwestFull,
    // diagonal slopes lower
    flat, slopeNortheastFull, flat, flat,
    // diagonal slopes full
    slopeSoutheastFull, slopeNortheastFull, slopeNorthwestFull, slopeSouthwestFull
};

const stuff::type_e trueCenterLeftTable[KIRBY_GEOM_TYPES] = {
    // nothing, flat, slopes up, slopes down
    nothing, flat, slopeSouthAndWestOuter, slopeEast,
    // cardinal slopes
    slopeSouth, slopeEast, slopeNorth, slopeWest,
    // inner slopes (SE, NE, NW, SW)
    slopeEast, slopeEast, slopeNorth, slopeSouth,
    // outer slopes
    slopeSouth, slopeNorthAndEastOuter, slopeWest, slopeSouthAndWestOuter,
    // diagonal slopes upper
    flat, flat, nothing, slopeSouthwestFull,
    // diagonal slopes lower
    slopeSoutheastFull, slopeNortheastFull, flat, flat,
    // diagonal slopes full
    slopeSoutheastFull, slopeNortheastFull, slopeNorthwestFull, slopeSouthwestFull
};

const stuff::type_e trueCenterRightTable[KIRBY_GEOM_TYPES] = {
    // nothing, flat, slopes up, slopes down
    nothing, flat, slopeNorthAndEastOuter, slopeSouth,
    // cardinal slopes
    slopeSouth, slopeEast, slopeNorth, slopeWest,
    // inner slopes (SE, NE, NW, SW)
    slopeSouth, slopeEast, slopeWest, slopeSouth,
    // outer slopes
    slopeEast, slopeNorthAndEastOuter, slopeNorth, slopeSouthAndWestOuter,
    // diagonal slopes upper
    flat, slopeNortheastFull, nothing, flat,
    // diagonal slopes lower
    slopeSoutheastFull, flat, flat, slopeSouthwestFull,
    // diagonal slopes full
    slopeSoutheastFull, slopeNortheastFull, slopeNorthwestFull, slopeSouthwestFull
};

metatile_t buildMetatile(int center, int left, int right,
                         bool north, bool east, bool south, bool west,
                         bool northStart, bool westStart) {
    metatile_t result = {nothing, nothing, {{0}}};
    int trueCenterLeft, trueCenterRight, trueLeft, trueRight;

    // do initial type mapping here
    if (left >= nothing)
        trueLeft = trueLeftTable[left];
    else trueLeft = left;

    if (right >= nothing)
        trueRight = trueRightTable[right];
    else trueRight = right;

    trueCenterLeft  = trueCenterLeftTable[center];
    trueCenterRight = trueCenterRightTable[center];

    // get center tile
    metatile_t centerTile = findMetatile(metatilesTerrain, center, nothing);

    // get left edge
    metatile_t leftTile = findMetatile(metatilesTerrain, trueCenterLeft, trueLeft);
    // get right edge
    metatile_t rightTile = findMetatile(metatilesTerrain, trueCenterRight, trueRight);

    //get borders
    //also use center type mapping to maybe get bumpers for 2-way slopes etc
    metatile_t northBorder, southBorder, eastBorder, westBorder;

    if (center == slopeNorth && west && south) {
        westBorder = findMetatile(bordersAll, trueCenterLeft, trueRight);
        southBorder = westBorder;
    } else {
        if (westStart)
            westBorder = findMetatile(bordersWestStart, trueCenterLeft, trueRight);
        else
            westBorder = findMetatile(bordersWest, trueCenterLeft, trueRight);

        //southBorder = findMetatile(bordersSouth, trueCenterLeft, trueLeft);
        southBorder = findMetatile(bordersSouth, trueRightTable[center], trueLeft);
    }

    if (center == slopeWest && north && east) {
        northBorder = findMetatile(bordersAll, trueCenterRight, trueLeft);
        eastBorder = northBorder;
    } else {
        if (northStart)
            northBorder = findMetatile(bordersNorthStart, trueCenterRight, trueLeft);
        else
            northBorder = findMetatile(bordersNorth, trueCenterRight, trueLeft);

        //eastBorder = findMetatile(bordersEast, trueCenterRight, trueRight);
        eastBorder = findMetatile(bordersEast, trueLeftTable[center], trueRight);
    }

    // if the center is a lower part of a diagonal slope, put the edge tiles lower
    // (including bumpers)
    int yOffLeft = 0;
    int yOffRight = 0;
    int yOffBumper = 0;
    if (center >= slopesLower && center < endSlopesLower) {
        if (left != nothing)
            yOffLeft = 2;
        if (right != nothing)
            yOffRight = 2;

        yOffBumper = 2;
    }

    // combine tiles
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            if (south && y >= yOffBumper && TILE(southBorder.tiles[y - yOffBumper][x]))
                result.tiles[y][x] = southBorder.tiles[y - yOffBumper][x];
            else if (east && y >= yOffBumper && TILE(eastBorder.tiles[y - yOffBumper][x]))
                result.tiles[y][x] = eastBorder.tiles[y - yOffBumper][x];
            else if ((north || northStart) && y >= yOffBumper && TILE(northBorder.tiles[y - yOffBumper][x]))
                result.tiles[y][x] = northBorder.tiles[y - yOffBumper][x];
            else if ((west || westStart) && y >= yOffBumper && TILE(westBorder.tiles[y - yOffBumper][x]))
                result.tiles[y][x] = westBorder.tiles[y - yOffBumper][x];
            else if (y >= yOffLeft && x < 4 && left != nothing && TILE(leftTile.tiles[y - yOffLeft][x]))
                result.tiles[y][x] = leftTile.tiles[y - yOffLeft][x];
            else if (y >= yOffRight && x >= 4 && right != nothing && TILE(rightTile.tiles[y - yOffRight][x]))
                result.tiles[y][x] = rightTile.tiles[y - yOffRight][x];
            else result.tiles[y][x] = centerTile.tiles[y][x];
        }

    return result;
}

metatile_t buildObstacle(int center, int left, int right) {
    metatile_t result = {nothing, nothing, {{0}}};
    int trueCenter, trueLeft, trueRight;

    // do initial type mapping here
    if      (center >= warpSouth2
          && center <= warpWest2) trueCenter = center - 4;
    else if (center == warpRed2)  trueCenter = warpRed;
    else if (center == switchShine)    trueCenter = switchShineBright;
    else if (center == switchRotateOn) trueCenter = switchRotate;
    else if (center == switchWaterOff) trueCenter = switchWater;
    else if (center >= startLine
             && center <= kirbyStartLine) trueCenter = startLine;
    else if ((center >= rotate) & (center < endRotate)) {
        if (center & 1) trueCenter = rotateCCW;
        else            trueCenter = rotateCW;
    }
    else if ((center >= endRotate) & (center < endRotateOpposite)) {
        if (center & 1) trueCenter = rotateCCWOpposite;
        else            trueCenter = rotateCWOpposite;
    }
    else trueCenter = center;

    // get center tile
    metatile_t centerTile = findMetatile(metatilesObstacles, trueCenter, nothing);

    // do pre-left type mapping here
    if      (left == waterSouthAndEastOuter) trueLeft = waterEast;
    else if (left == waterNorthAndEastOuter) trueLeft = waterEast;
    else if (left >= startLine
             && left <= kirbyStartLine)      trueLeft = startLine;
    else if ((left >= rotate) & (left < endRotate)) {
        if (left & 1) trueLeft = rotateCCW;
        else          trueLeft = rotateCW;
    }
    else if ((left >= endRotate) & (left < endRotateOpposite)) {
        if (left & 1) trueLeft = rotateCCWOpposite;
        else          trueLeft = rotateCWOpposite;
    }
    else trueLeft = left;

    // get left edge
    metatile_t leftTile = findMetatile(metatilesObstacles, trueCenter, trueLeft);

    // do pre-right type mapping here
    if      (right == waterSouthAndEastOuter) trueRight = waterSouth;
    else if (right == waterSouthAndWestOuter) trueRight = waterSouth;
    else if (right >= startLine
             && right <= kirbyStartLine)      trueRight = startLine;
    else if ((right >= rotate) & (right < endRotate)) {
        if (right & 1) trueRight = rotateCCW;
        else           trueRight = rotateCW;
    }
    else if ((right >= endRotate) & (right < endRotateOpposite)) {
        if (right & 1) trueRight = rotateCCWOpposite;
        else           trueRight = rotateCWOpposite;
    }
    else trueRight = right;

    // get right edge
    metatile_t rightTile = findMetatile(metatilesObstacles, trueCenter, trueRight);

    // combine tiles
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 4; x++) {
            if (trueLeft != nothing && TILE(leftTile.tiles[y][x]) > 0)
                result.tiles[y][x] = leftTile.tiles[y][x];
            else result.tiles[y][x] = centerTile.tiles[y][x];

            if (trueRight != nothing && TILE(rightTile.tiles[y][x + 4]) > 0)
                result.tiles[y][x + 4] = rightTile.tiles[y][x + 4];
            else result.tiles[y][x + 4] = centerTile.tiles[y][x + 4];
        }

    return result;
}

metatile_t findMetatile(const metatile_t *array, int type, int other) {
    metatile_t result = {nothing, nothing, {{0}}};

    for (int i = 0; array[i].type; i++) {
        if (array[i].type == type) {
            // use (in order):
            // selected tile touching nothing
            // selected tile touching selected other type
            if (array[i].adjacent == 0 || array[i].adjacent == other)
                result = array[i];
            if (array[i].adjacent == other)
                break;
        }
    }
    return result;
}

const uint16_t bottomTile[2][8] =
{{ 16|PAL(7)   ,  17|PAL(7)   ,   1|PAL(7)   , 205|PAL(7)   , 202|PAL(7)|FB,   2|PAL(7)   , 18|PAL(7)   ,   19|PAL(7)   },
 {  0|PAL(0)   ,   0|PAL(0)   ,  16|PAL(7)   ,  17|PAL(7)   ,  18|PAL(7)   ,  19|PAL(7)   ,  0|PAL(0)   ,    0|PAL(0)   }};

const uint16_t stackTile[2][8] =
{{214|PAL(7)   , 203|PAL(7)|FB, 205|PAL(7)|FB,   1|PAL(7)   ,   2|PAL(7)   , 202|PAL(7)   ,  90|PAL(7)   , 215|PAL(7)   },
 {180|PAL(7)   , 205|PAL(7)   , 203|PAL(7)   , 203|PAL(7)|FB,  90|PAL(7)   ,  90|PAL(7)|FB, 202|PAL(7)|FB, 181|PAL(7)   }};
