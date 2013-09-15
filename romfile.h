/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#ifndef FILE_H
#define FILE_H

#include <cstdio>
#include <QFile>
#include <cstdint>

#define BANK_SIZE 0x8000

typedef enum {
    kirby_jp  = 0,
    kirby_us  = 1,
    sts_jp    = 2
} version_e;

typedef enum {
    kirby   = 0,
    sts     = 1
} game_e;

class ROMFile: public QFile {
public:
    ROMFile();

    bool         openROM(OpenMode flags);

    game_e    getGame();
    version_e getVersion();

    uint toAddress(uint offset);
    uint toOffset(uint addr);

    size_t       readData(uint addr, uint size, void *buffer);
    uint8_t      readByte(uint addr);
    uint16_t     readInt16(uint addr);
    uint32_t     readInt32(uint addr);
    size_t       readFromPointer(uint addr, uint size, void *buffer);
    uint writeData(uint addr, uint size, void *buffer);
    uint writeByte(uint addr, uint8_t data);
    uint writeInt16(uint addr, uint16_t data);
    uint writeInt32(uint addr, uint32_t data);
    uint writeToPointer(uint ptr, uint addr, uint size, void *buffer);

private:
    bool header;
    game_e game;
    version_e version;
};

#endif // FILE_H
