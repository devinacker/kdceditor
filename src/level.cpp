/*
  level.cpp

  Contains functions for loading and saving level data, as well as generating the isometric
  tile maps based on the 2D map data.

  This code is released under the terms of the MIT license.
  See COPYING.txt for details.
*/

#include "compress.h"
#include "romfile.h"
#include "metatile.h"
#include "level.h"
#include "graphics.h"

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <vector>

#include <QString>
#include <QCoreApplication>

using namespace stuff;

// number of levels in Kirby's Dream Course and Special Tee Shot
const int numLevels[] = {28 * 8, 9 * 8};

//Locations of chunk data in ROM (using CPU addressing.)
const int headerTable[]    = {0x8aa670, 0x88a770, 0x88a770};
const int terrainTable[]   = {0x8aa970, 0x88aa70, 0x88aa70, 0x85da80};
const int obstacleTable[]  = {0x8aac70, 0x88ad70, 0x88ad70, 0x85dd08};
const int heightTable[]    = {0x8aaf70, 0x88b070, 0x88b070, 0x85db58};
const int flagsTable[]     = {0x8ab270, 0x88b370, 0x88b370, 0x85dc30};
const int rowStartTable[]  = {0x8ab570, 0x88b670, 0x88b670, 0x85dde0};
const int rowEndTable[]    = {0x8ab870, 0x88b970, 0x88b970, 0x85deb8};
const int rowOffsetTable[] = {0x8abb70, 0x88bc70, 0x88bc70, 0x85df90};
const int layer1Table[]    = {0x8abe70, 0x88bf70, 0x88bf70, 0x85e068};
const int layer2Table[]    = {0x8ac170, 0x88c270, 0x88c270, 0x85e140};
const int clippingTable[]  = {0x8ac470, 0x88c570, 0x88c570, 0x85e218};

// for Special Tee Shot only
const int widthTable       = 0x85fa26;
const int lengthTable      = 0x85fab6;

const uint newDataAddress[] = {0xa88000, 0xa08000, 0xa08000, 0xa08000};

// blank tile used for rendering playfield
const maptile_t noTile = {0, 0, 0, {0, 0, 0, 0, 0, 0}};


/*
  Returns the maximum tile height of a level.
*/
uint levelHeight(const leveldata_t *level) {
    int h = 0;
    for (int i = 0; i < level->header.length; i++)
        for (int j = 0; j < level->header.width; j++)
            if (level->tiles[i][j].height > h)
                h = level->tiles[i][j].height;

    return h;
}

/*
  Returns true if the level contains water hazards, otherwise false.
  Used to determine which tiles to use (conveyor belts or water).
*/
bool waterLevel(const leveldata_t *level) {
    for (int i = 0; i < level->header.length; i++)
        for (int j = 0; j < level->header.width; j++)
            if (level->tiles[i][j].obstacle >= water
                    && level->tiles[i][j].obstacle < endWater)
                return true;

    return false;
}

/*
  Load a level by number. Returns pointer to the level data as a struct.
  Returns null if a level failed and the user decided not to continue.
*/
leveldata_t* loadLevel (ROMFile& file, uint num) {
    leveldata_t *level = (leveldata_t*)malloc(sizeof(leveldata_t));
    memset((void*)level, 0, sizeof(leveldata_t));

    ROMFile::version_e ver = file.getVersion();
    ROMFile::game_e    game = file.getGame();

    // load the header
    fflush(stdout);
    // If the header read was unsuccessful, display an error.
    // This is the only time here when a read is checked for success, because the editor
    // writes header pointers and other level data pointers at the same time, so if one succeeds,
    // the rest probably will, and vice-versa.
    bool gotLevel;
    if (game == ROMFile::kirby)
        gotLevel = file.readFromPointer(headerTable[ver] + (num * 3), sizeof(header_t), &level->header);
    else
        gotLevel = true;

    // if this is Special Tee Shot, grab the level length and width from the tables
    // (note, because STS support is obviously unfinished:
    // each header field is now stored in its own table. there may be more, fewer,
    // or otherwise different fields than in KDC. based on the differences I've seen
    // so far, i'm almost certain there are different fields)
    if (game == ROMFile::sts) {
        level->header.width  = file.readByte(widthTable + num * 2);
        level->header.length = file.readByte(lengthTable + num * 2);
    }

    if (!gotLevel
        || level->header.width * level->header.length > MAX_2D_AREA) {
        QMessageBox::StandardButton button = QMessageBox::warning(0,
                                              QString("Error"),
                                              QString("Unable to load level %1-%2. The ROM may be corrupted.\n\nContinue loading ROM?")
                                                                  .arg((num / 8) + 1).arg((num % 8) + 1),
                                              QMessageBox::Yes | QMessageBox::No);

        if (button == QMessageBox::No) {
            free(level);
            return NULL;
        }

        // if the level fails to load just set up some default length/width
        // to allow the user to continue editing.
        level->header.length = 10;
        level->header.width  = 10;

        return level;
    }

    uint8_t buffer[4][CHUNK_SIZE] = {{0}};

    // read chunk buffers
    uint width = level->header.width;
    uint length = level->header.length;

    // if the level data begins in the expanded ROM area, mark it modified
    // (so it will be saved correctly)
    uint firstPointer = 0;
    file.readBytes(terrainTable[ver] + (num * 3), 3, &firstPointer);
    if (firstPointer >= newDataAddress[ver])
        level->modified = true;

    // get chunk 1 (terrain bytes)
    file.readFromPointer(terrainTable[ver] + (num * 3), 0, buffer[0]);
    // get chunk 2 (obstacle bytes)
    file.readFromPointer(obstacleTable[ver] + (num * 3), 0, buffer[1]);
    // get chunk 3 (height bytes)
    file.readFromPointer(heightTable[ver] + (num * 3), 0, buffer[2]);
    // get chunk 4 (flag bytes)
    file.readFromPointer(flagsTable[ver] + (num * 3), 0, buffer[3]);

    // read last row first
    for (uint i = 0; i < length; i++) {
        for (uint j = 0; j < width; j++) {
            level->tiles[length-i-1][j].geometry = buffer[0][i*width + j];
            level->tiles[length-i-1][j].obstacle = buffer[1][i*width + j];
            level->tiles[length-i-1][j].height = buffer[2][i*width + j];
            memcpy((void*)&level->tiles[length-i-1][j].flags,
                   (const void*)&buffer[3][i*width + j],
                   1);
        }
    }

    return level;
}

/*
 * New version of saveLevel that returns a list of chunks
 */
QList<QByteArray*> saveLevel(leveldata_t *level, int *fieldSize) {
    // buffers for compressed and uncompressed data
    uint8_t packed[CHUNK_SIZE], unpacked[CHUNK_SIZE];
    size_t packedSize = 0;
    QList<QByteArray*> chunks;

    // level length, width
    int length = level->header.length;
    int width  = level->header.width;
    int height = levelHeight(level);

    // step 1: save level header

    // update horizontal and vertical sprite offsets
    level->header.alignHoriz = 0;
    level->header.alignVert = 16 * (length + height + 2);

    // use some bogus values for the two unknown header parts
    // the first one has something to do with Gordo movement (i.e. north/south paths will not
    // function correctly if it is below (?) a certain value, which differs from level to level
    // in the original game and I don't know what the value actually represents).
    // the second one is likely unused
    level->header.dummy1 = 0xFFFF;
    level->header.dummy2 = 0xFFFF;

    chunks.append(new QByteArray((const char*)&level->header, sizeof(header_t)));

    // step 2: compress and save terrain in chunk 1
    // build uncompressed data buffer from terrain data
    // (remember, levels are stored bottom-up)
    for (int y = 0; y < length; y++)
        for (int x = 0; x < width; x++)
            unpacked[y * width + x] = level->tiles[length - y - 1][x].geometry;

    packedSize = pack(unpacked, length * width, packed, 1);
    chunks.append(new QByteArray((const char*)packed, packedSize));

    // step 3: compress and save obstacles in chunk 2
    // build uncompressed data buffer from obstacle data
    for (int y = 0; y < length; y++)
        for (int x = 0; x < width; x++)
            unpacked[y * width + x] = level->tiles[length - y - 1][x].obstacle;

    packedSize = pack(unpacked, length * width, packed, 1);
    chunks.append(new QByteArray((const char*)packed, packedSize));

    // step 4: compress and save heights in chunk 3
    // build uncompressed data buffer from terrain data
    for (int y = 0; y < length; y++)
        for (int x = 0; x < width; x++)
            unpacked[y * width + x] = level->tiles[length - y - 1][x].height;

    packedSize = pack(unpacked, length * width, packed, 1);
    chunks.append(new QByteArray((const char*)packed, packedSize));

    // step 5: compress and save flags in chunk 4
    // build uncompressed data buffer from terrain data
    for (int y = 0; y < length; y++)
        for (int x = 0; x < width; x++)
            memcpy((void*)&unpacked[y * width + x],
                   (void*)&level->tiles[length - y - 1][x].flags,
                   1);
            //unpacked[y * width + x] = level->tiles[length - y - 1][x].flags;

    packedSize = pack(unpacked, length * width, packed, 1);
    chunks.append(new QByteArray((const char*)packed, packedSize));

    // step 6: create packed playfield tilemaps and write them
    auto playfield = new uint16_t[2][MAX_FIELD_HEIGHT][MAX_FIELD_WIDTH];

    makeIsometricMap(playfield, level);

    uint16_t rowStarts [CHUNK_SIZE / 2] = {0};
    uint16_t rowEnds   [CHUNK_SIZE / 2] = {0};
    uint16_t rowOffsets[CHUNK_SIZE / 2] = {0};

    uint16_t layer[2][BIG_CHUNK_SIZE / 2] = {{0}};
    int index = 0;
    int rowLen = 0;

    // iterate through each row of the playfield to find where the row starts and ends
    // then copy stuff into the tile buffers
    for (int row = 0; row < level->header.fieldHeight; row++) {
        int start, end;

        // find the row start position
        for (start = 0; start < level->header.fieldWidth; start++)
            if (TILE(playfield[0][row][start]) != 0
                    || TILE(playfield[1][row][start]) != 0)
                break;

        // find the row end position
        for (end = level->header.fieldWidth - 1; end > start; end--)
            if (TILE(playfield[0][row][end]) != 0
                    || TILE(playfield[1][row][end]) != 0)
                break;

        rowLen = end - start + 1;

        // copy playfield row into pack buffer if there is enough room
        if (start != level->header.fieldWidth && index + rowLen < (BIG_CHUNK_SIZE / 2)) {
            memcpy(&layer[0][index], &(playfield[0][row][start]), rowLen * 2);
            memcpy(&layer[1][index], &(playfield[1][row][start]), rowLen * 2);

            rowStarts[row]  = start;
            rowEnds[row]    = end;
            rowOffsets[row] = index;
        } else {
        // otherwise, start writing empty rows of tiles to prevent garbage at the bottom
            rowStarts[row]  = 0xFFFF;
            rowEnds[row]    = 0xFFFF;
            rowOffsets[row] = 0xFFFF;
        }

        index += rowLen;
    }

    delete[] playfield;

    if (fieldSize) {
        *fieldSize = index * 2;
    }

    // step 7: write playfield chunks
    packedSize = pack((uint8_t*)&rowStarts[0], level->header.fieldHeight * 2, packed, 1);
    chunks.append(new QByteArray((const char*)packed, packedSize));

    packedSize = pack((uint8_t*)&rowEnds[0], level->header.fieldHeight * 2, packed, 1);
    chunks.append(new QByteArray((const char*)packed, packedSize));

    packedSize = pack((uint8_t*)&rowOffsets[0], level->header.fieldHeight * 2, packed, 1);
    chunks.append(new QByteArray((const char*)packed, packedSize));

    packedSize = pack((uint8_t*)&layer[0][0], index * 2, packed, 1);
    chunks.append(new QByteArray((const char*)packed, packedSize));

    packedSize = pack((uint8_t*)&layer[1][0], index * 2, packed, 1);
    chunks.append(new QByteArray((const char*)packed, packedSize));

    // step 8: do clipping table
    // TODO: update clipping table info based on STS expanded format?
    size_t clipSize = makeClipTable(level, unpacked);
    packedSize = pack(unpacked, clipSize, packed, 1);
    chunks.append(new QByteArray((const char*)packed, packedSize));

    return chunks;
}

/*
 * Save all modified levels to ROM using worker threads
 */
uint saveAllLevels(ROMFile& file, leveldata_t **levels) {

    ROMFile::version_e ver = file.getVersion();
    ROMFile::game_e    game = file.getGame();

    uint addr = newDataAddress[ver];

    SaveWorker* workers[numLevels[game]];

    // spawn and start worker threads
    for (int i = 0; i < numLevels[game]; i++) {
        if (levels[i]->modified)
            workers[i] = new SaveWorker(levels[i]);
        else
            workers[i] = NULL;
    }

    // join and save data from worker threads
    for (int num = 0; num < numLevels[game]; num++) {

        if (!workers[num]) continue;

        const QList<QByteArray*>& chunks = workers[num]->getChunks();
        QByteArray *data;

        int fieldSize = workers[num]->getFieldSize();
        if (fieldSize > BIG_CHUNK_SIZE) {
            QMessageBox::warning(0, "Save ROM",
                     QString("Unable to save the entire 3D tilemap for course %1-%2 because it is too large\n(%3 / %4 bytes)."
                             "\n\nPlease decrease the length, width, and/or height of the course in order to reduce the tilemap size.")
                     .arg((num / 8) + 1).arg((num % 8) + 1).arg(fieldSize).arg(BIG_CHUNK_SIZE),
                     QMessageBox::Ok);
        }

        // step 1: save level header
        data = chunks[0];

        if (game == ROMFile::kirby) {
            addr = file.writeToPointer(headerTable[ver] + 3 * num, addr, data->size(), data->data());
        } else {
            // TODO: save the STS level info
        }

        // step 2: save terrain in chunk 1
        data = chunks[1];
        addr = file.writeToPointer(terrainTable[ver] + 3 * num, addr, data->size(), data->data());

        // step 3: save obstacles in chunk 2
        data = chunks[2];
        addr = file.writeToPointer(obstacleTable[ver] + 3 * num, addr, data->size(), data->data());

        // step 4: save heights in chunk 3
        data = chunks[3];
        addr = file.writeToPointer(heightTable[ver] + 3 * num, addr, data->size(), data->data());

        // step 5: save flags in chunk 4
        data = chunks[4];
        addr = file.writeToPointer(flagsTable[ver] + 3 * num, addr, data->size(), data->data());

        // step 6: write playfield chunks
        data = chunks[5];
        addr = file.writeToPointer(rowStartTable[ver] + 3 * num, addr, data->size(), data->data());

        data = chunks[6];
        addr = file.writeToPointer(rowEndTable[ver] + 3 * num, addr, data->size(), data->data());

        data = chunks[7];
        addr = file.writeToPointer(rowOffsetTable[ver] + 3 * num, addr, data->size(), data->data());

        data = chunks[8];
        addr = file.writeToPointer(layer1Table[ver] + 3 * num, addr, data->size(), data->data());

        data = chunks[9];
        addr = file.writeToPointer(layer2Table[ver] + 3 * num, addr, data->size(), data->data());

        // step 7: do clipping table
        // TODO: update clipping table info based on STS expanded format
        if (game == ROMFile::kirby) {
            data = chunks[10];
            addr = file.writeToPointer(clippingTable[ver] + 3 * num, addr, data->size(), data->data());
        }

        delete workers[num];

        QCoreApplication::processEvents();
    }

    return addr;
}

/*
  Generates a level's Z-clipping table (chunk 10) and puts it into a buffer.
  Returns the size of the generated chunk.
*/
size_t makeClipTable(const leveldata_t *level, uint8_t *buffer) {
    int l = level->header.length;
    int w = level->header.width;

    std::vector<clip_t> *table = new std::vector<clip_t>[l + w];

    for (int y = 0; y < l; y++) {
        // the game itself stores tile data "south to north"
        // (and this is important when generating this data)
        int realY = l - y - 1;
        clip_t clip;

        for (int x = w; x > 0; x--) {
            clip.zref = (realY * w) + x;
            // first, check for a gap to the north
            if (y > 0 && level->tiles[y][x].geometry > 0
                    && level->tiles[y - 1][x].geometry == 0) {
                // how far does the gap go?
                for (clip.xLower = x; clip.xLower >= 1
                                   && level->tiles[y - 1][clip.xLower - 1].geometry == 0; clip.xLower--);

                clip.xUpper = x + 1;

                // set priority
                clip.prio = level->tiles[y][x].flags.layer ? 2 : 1;

                // add this entry to the table
                table[realY + x + 1].push_back(clip);
            }

            // next, check for a gap to the west
            if (x > 0 && level->tiles[y][x].geometry > 0
                    && level->tiles[y][x - 1].geometry == 0) {
                // how far does the gap go?
                for (clip.xLower = x - 1; clip.xLower >= 1
                                   && level->tiles[y][clip.xLower - 1].geometry == 0; clip.xLower--);

                clip.xUpper = x;

                // set priority
                clip.prio = level->tiles[y][x].flags.layer ? 2 : 1;

                // add this entry to the table
                table[realY + x - 1].push_back(clip);

                // skip over the gap that we just checked
                x = clip.xLower;
            }
        }
    }

    //write table into buffer
    uint offset = 2 * (l + w);
    uint16_t offsets[128];
    for (int i = 0; i < l + w; i++) {
        std::vector<clip_t>& thisClip = table[i];

        // don't care about empty parts of the table
        uint size = thisClip.size();
        if (!size) {
            offsets[i] = 0xFFFF;
            continue;
        }

        offsets[i] = offset;

        // write the number of clip_t in this part
        buffer[offset++] = (uint8_t)size;

        // write all of the individual clip_t
        for (uint j = 0; j < size; j++) {
            memcpy(&(buffer[offset]), &(thisClip[j]), sizeof(clip_t));
            offset += sizeof(clip_t);
        }
    }

    // copy the table index pointers to the beginning of the buffer
    memcpy(buffer, offsets, 2 * (w + l));

    delete[] table;
    return offset;
}

/*
  Builds the 3D metatile map based on the 2D map.
  This needs some serious rewriting. It was pretty much totally improvised, revised,
  revised, revised, revised, and revised again and again until it looked right.
*/
void makeIsometricMap(uint16_t playfield[2][MAX_FIELD_HEIGHT][MAX_FIELD_WIDTH], leveldata_t *level) {
    // erase the old map
    memset(playfield, 0, 2*MAX_FIELD_HEIGHT*MAX_FIELD_WIDTH*sizeof(uint16_t));

    int h = levelHeight(level);
    int l = level->header.length;
    int w = level->header.width;

    level->header.fieldHeight = qMin(MAX_FIELD_HEIGHT, 2 * (h + w + l + 2));
    level->header.fieldWidth  = qMin(MAX_FIELD_WIDTH, 4 * (w + l));

    // render "back to front" - that is, from north to south, west to east
    for (int x = 0; x < level->header.width; x++) {
        for (int y = 0; y < level->header.length; y++) {

            // do not render non-terrain tiles at all
            if (level->tiles[y][x].geometry == 0) continue;

            maptile_t thisTile, leftTile, rightTile, backTile;
            bool useExtraTiles = false;
            thisTile = level->tiles[y][x];

            int z = thisTile.height;

            // horizontal: start at 0 tiles
            // move 4 right for each positive move on the x-axis (west to east)
            // and  4 left  for each positive move on the y-axis (north to south)
            int startX = 4 * (x + (l - y - 1));
            // start at 2 * h tiles
            // move 2 down for each positive move on the x-axis (west to east)
            // and  2 down for each positive move on the y-axis (north to south)
            // and  2 up   for each positive move on the z-axis (tile z)
            int startY = 2 * (h + x + y - z);

            // draw obstacles (namely bumpers) lower if on a diagonal slope bottom
            int startYObs = 0;
            if (thisTile.geometry >= slopesLower && thisTile.geometry < endSlopesLower)
                startYObs = 2;

            // figure out which metatiles to use based on the current 2d tile and its neighbors
            // ("left" is west, "right" is north)
            if (x > 0)
                leftTile = level->tiles[y][x-1];
            else leftTile = noTile;
            if (y > 0)
                rightTile = level->tiles[y-1][x];
            else rightTile = noTile;
            // backTile is used in some rare cases for obstacles
            // (spikes, some bounce slopes) where borders have to be removed between
            // the left and right tiles when there is a 2x2 or bigger area of the same obstacle.
            if (x > 0 && y > 0)
                backTile = level->tiles[y-1][x-1];
            else backTile = noTile;
            if ((thisTile.obstacle == spikes
                    && thisTile.obstacle == leftTile.obstacle
                    && thisTile.obstacle == rightTile.obstacle
                    && thisTile.obstacle == backTile.obstacle)

             || (thisTile.obstacle == bounceNorth
                    && thisTile.obstacle == leftTile.obstacle
                    && thisTile.obstacle == backTile.obstacle)

             || (thisTile.obstacle == bounceWest
                    && thisTile.obstacle == rightTile.obstacle
                    && thisTile.obstacle == backTile.obstacle)) {

                useExtraTiles = true;
            }

            // determine whether each edge is touching either a wall or another thing
            int leftEdge, rightEdge;

            // don't make tile connections when adjacent tiles are on different layers
            if (thisTile.flags.layer && !leftTile.flags.layer)
                leftEdge = nothing;
            else if (leftTile.geometry >= slopes || leftTile.geometry == slopesUp) {
                // Don't connect slopes that go to the south and/or west
                if (leftTile.height - thisTile.height > 1
                    || (leftTile.height - thisTile.height == 1
                        && thisTile.geometry < endSlopesUpper
                        && leftTile.geometry != slopeEast
                        && leftTile.geometry != slopeSouthAndEastOuter
                        && leftTile.geometry != slopeNorthAndEastOuter
                        && (leftTile.geometry == slopeNorth
                            || leftTile.geometry == slopeWest
                            || leftTile.geometry == slopeSouthAndWestInner
                            || leftTile.geometry == slopeNorthAndEastInner
                            || leftTile.geometry == slopeNorthAndWestInner
                            || leftTile.geometry == slopeNorthAndWestOuter
                            || leftTile.geometry == slopeSoutheastFull
                            || leftTile.geometry == slopeNortheastUpper
                            || leftTile.geometry == slopeNorthwestUpper
                            || leftTile.geometry == slopeSouthwestUpper
                            || thisTile.geometry == slopeSouth
                            || thisTile.geometry == slopeSouthAndEastOuter
                            || thisTile.geometry == slopeSouthAndWestInner))
                    || (leftTile.height == thisTile.height
                        && thisTile.geometry < slopesFull
                        && leftTile.geometry == slopeSouthwestFull))
                    leftEdge = wall;
                else leftEdge = leftTile.geometry;
            }
            // normal height difference = touching wall
            else if (leftTile.geometry && leftTile.height > thisTile.height)
                leftEdge = wall;
            else leftEdge = leftTile.geometry;

            // same thing, but for right edge
            if (thisTile.flags.layer && !rightTile.flags.layer)
                rightEdge = nothing;
            else if (rightTile.geometry >= slopes || rightTile.geometry == slopesUp) {
                // Don't connect slopes that go to the north and/or ??
                if (rightTile.height - thisTile.height > 1
                        || (rightTile.height - thisTile.height == 1
                            && thisTile.geometry < endSlopesUpper
                            && rightTile.geometry != slopeSouth
                            && rightTile.geometry != slopeSouthAndEastOuter
                            && rightTile.geometry != slopeSouthAndWestOuter
                            && (rightTile.geometry == slopeNorth
                                || rightTile.geometry == slopeWest
                                //|| ??
                                || rightTile.geometry == slopeNorthAndWestInner
                                || rightTile.geometry == slopeNorthAndWestOuter
                                || rightTile.geometry == slopeNorthAndEastInner
                                || rightTile.geometry == slopeSouthAndWestInner
                                || rightTile.geometry == slopeNortheastFull
                                || rightTile.geometry == slopeNortheastUpper
                                || rightTile.geometry == slopeNorthwestUpper
                                || rightTile.geometry == slopeSouthwestUpper
                                || thisTile.geometry == slopeEast
                                || thisTile.geometry == slopeSouthAndEastOuter
                                || thisTile.geometry == slopeNorthAndEastInner))
                        || (rightTile.height == thisTile.height
                            && thisTile.geometry < slopesFull
                            && rightTile.geometry == slopeNortheastFull))
                    rightEdge = wall;
                else rightEdge = rightTile.geometry;
            }
            // normal height difference = touching wall
            else if (rightTile.geometry && rightTile.height > thisTile.height)
                rightEdge = wall;
            else rightEdge = rightTile.geometry;

            // search through metatile definitions to find ones that match current setup
            metatile_t meta = buildMetatile(thisTile.geometry, leftEdge, rightEdge,
                                            thisTile.flags.bumperNorth,
                                            thisTile.flags.bumperEast,
                                            thisTile.flags.bumperSouth,
                                            thisTile.flags.bumperWest,
            // "northStart" and "westStart" args are used when a tile without bumpers is
            // adjacent to a tile WITH bumpers, for a couple of cases where using the normal
            // connection tiles will cause very noticeable gaps in the bumpers (like 3-4?)
            // in the future I may expand this to handle all such cases for all four bumpers,
            // but the difference right now is barely noticeable compared to the original game's
            // tile maps (usually a couple of pixels)
                                            (!thisTile.flags.bumperNorth && leftTile.flags.bumperNorth),
                                            (!thisTile.flags.bumperWest  && rightTile.flags.bumperWest)
                                            );

            leftEdge  = (leftEdge  == wall) ? 0 : leftTile.obstacle;
            rightEdge = (rightEdge == wall) ? 0 : rightTile.obstacle;

            metatile_t obs;
            if (useExtraTiles)
                obs  = buildObstacle(thisTile.obstacle | extraTiles, leftEdge, rightEdge);
            else
                obs  = buildObstacle(thisTile.obstacle, leftEdge, rightEdge);


            // now lay some tiles down
            int terrainLayer = thisTile.flags.layer;
            // Use layer 2 for the left or right edge tiles, if the left or right edge
            // moves from layer 1 to layer 2. This way the layer 2 tiles will not be
            // incorrectly drawn on top
            int terrainLeftLayer  = (thisTile.flags.layer == 0
                                     && leftTile.flags.layer == 1) ? 1 : terrainLayer;
            int terrainRightLayer = (thisTile.flags.layer == 0
                                     && rightTile.flags.layer == 1) ? 1 : terrainLayer;

            int terrainPrio      = terrainLayer      ? PRI : 0;
            int terrainLeftPrio  = terrainLeftLayer  ? PRI : 0;
            int terrainRightPrio = terrainRightLayer ? PRI : 0;

            // the edge sizes are used to determine how many rows of tiles to apply
            // the layer / prio changes to
            int leftEdgeSize  = 4;
            int rightEdgeSize = 4;

            if (trueCenterLeftTable[thisTile.geometry] == slopeSouth
             || trueCenterLeftTable[thisTile.geometry] == slopeSouthAndWestOuter
             || trueCenterLeftTable[thisTile.geometry] == slopeSouthAndWestInner)
                leftEdgeSize = 6;

            if (trueCenterRightTable[thisTile.geometry] == slopeEast
             || trueCenterRightTable[thisTile.geometry] == slopeNorthAndEastOuter
             || trueCenterRightTable[thisTile.geometry] == slopeNorthAndEastInner)
                rightEdgeSize = 6;

            // first, do base/height tiles
            int leftBaseHeight = z + 1;
            // restrict drawing of support tiles for layer 2 when necessary
            // so that layer 2's support tiles aren't drawn on top of layer 1 tiles
            // to the southeast
            if (level->header.length > y + 1 && level->tiles[y + 1][x].geometry) {
                leftBaseHeight = z - level->tiles[y+1][x].height + 1;

                // if going layer 1->2 then slopes = more
                if ((thisTile.flags.layer < level->tiles[y + 1][x].flags.layer)
                        && (level->tiles[y + 1][x].geometry >= slopes))
                    leftBaseHeight++;

                // if going layer 2->1, don't draw as much
                else if (thisTile.flags.layer && !level->tiles[y + 1][x].flags.layer)
                    leftBaseHeight--;
            }

            int rightBaseHeight = z + 1;
            if (level->header.width > x + 1 && level->tiles[y][x + 1].geometry) {
                rightBaseHeight = z - level->tiles[y][x+1].height + 1;

                // if going layer 1->2 then slopes = more
                if ((thisTile.flags.layer < level->tiles[y][x + 1].flags.layer)
                        && (level->tiles[y][x + 1].geometry >= slopes))
                    rightBaseHeight++;

                // if going layer 2->1, don't draw as much
                else if (thisTile.flags.layer && !level->tiles[y][x + 1].flags.layer)
                    rightBaseHeight--;
            }

            // do the height tiles
            for (int tileY = 2; tileY <= (2 * leftBaseHeight) || tileY < (2 * rightBaseHeight); tileY += 2)
                for (int tileX = 0; tileX < 4; tileX++) {
                    // left side
                    if (tileY <= 2 * leftBaseHeight) {
                        playfield[terrainLayer][startY + 4 + tileY][startX + tileX]
                                = stackTile[0][tileX] | terrainPrio;
                        playfield[terrainLayer][startY + 5 + tileY][startX + tileX]
                                = stackTile[1][tileX] | terrainPrio;
                    }
                    // right side
                    if (tileY <= 2 * rightBaseHeight) {
                        playfield[terrainLayer][startY + 4 + tileY][startX + 4 + tileX]
                                = stackTile[0][tileX + 4] | terrainPrio;
                        playfield[terrainLayer][startY + 5 + tileY][startX + 4 + tileX]
                                = stackTile[1][tileX + 4] | terrainPrio;
                    }
                }
            // draw the base tiles
            if ((leftBaseHeight == z + 1) || (rightBaseHeight == z + 1))
            for (int tileX = 0; tileX < 4; tileX++) {
                // left side
                if (leftBaseHeight == z + 1) {
                    playfield[terrainLayer][startY + 6 + (2 * z)][startX + tileX]
                            = bottomTile[0][tileX] | terrainPrio;
                    playfield[terrainLayer][startY + 7 + (2 * z)][startX + tileX]
                            = bottomTile[1][tileX] | terrainPrio;
                }
                // right side
                if (rightBaseHeight == z + 1) {
                    playfield[terrainLayer][startY + 6 + (2 * z)][startX + tileX + 4]
                            = bottomTile[0][tileX + 4] | terrainPrio;
                    playfield[terrainLayer][startY + 7 + (2 * z)][startX + tileX + 4]
                            = bottomTile[1][tileX + 4] | terrainPrio;
                }
            }

            // now the actual tile itself
            for (int tileY = 0; tileY < 8; tileY++)
                for (int tileX = 0; tileX < 8; tileX++) {
                    int layer, prio;

                    if (tileY < leftEdgeSize && tileX < 4) {
                        layer = terrainLeftLayer;
                        prio = terrainLeftPrio;
                    } else if (tileY < rightEdgeSize) {
                        layer = terrainRightLayer;
                        prio = terrainRightPrio;
                    } else {
                        layer = terrainLayer;
                        prio = terrainPrio;
                    }

                    if (TILE(meta.tiles[tileY][tileX]))
                        playfield[layer][startY + tileY][startX + tileX]
                                = meta.tiles[tileY][tileX] | prio;

                    if (TILE(obs.tiles[tileY][tileX]))
                      playfield[layer ^ 1][startY + startYObs + tileY][startX + tileX]
                                = obs.tiles[tileY][tileX] | PRI;

                }
        }
    }
}
