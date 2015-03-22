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


class ROMFile: public QFile {
public:

    enum version_e {
        kirby_jp,
        kirby_us,
        kirby_eu,
        sts_jp
    };

    enum game_e {
        kirby,
        sts
    };

    ROMFile();

    bool         openROM(OpenMode flags);

    ROMFile::game_e    getGame();
    ROMFile::ROMFile::version_e getVersion();

    uint toAddress(uint offset);
    uint toOffset(uint addr);

    size_t       readBytes(uint addr, uint size, void *buffer);
    uint8_t      readByte(uint addr);
    uint16_t     readInt16(uint addr);
    uint32_t     readInt32(uint addr);
    size_t       readFromPointer(uint addr, uint size, void *buffer);
    uint writeBytes(uint addr, uint size, void *buffer);
    uint writeByte(uint addr, uint8_t data);
    uint writeInt16(uint addr, uint16_t data);
    uint writeInt32(uint addr, uint32_t data);
    uint writeToPointer(uint ptr, uint addr, uint size, void *buffer);

private:
    bool header;
    ROMFile::game_e    game;
    ROMFile::version_e version;
};

#endif // FILE_H
