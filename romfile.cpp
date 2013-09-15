/*
  romfile.cpp
  Contains functions for loading and saving data to/from a ROM file.

  Automatically detects valid game ROMs and allows reading/writing by CPU addresses, automatically
  adjusting for a 512-byte copier header if necessary.

  This code is released under the terms of the MIT license.
  See COPYING.txt for details.
*/

#include <QFile>
#include <QMessageBox>
#include <QSettings>

#include <cstring>

#include "romfile.h"
extern "C" {
#include "compress.h"
}

ROMFile::ROMFile() : QFile(),
    header(false),
    game(kirby),
    version(kirby_jp)
{}

game_e ROMFile::getGame() {
    return game;
}

version_e ROMFile::getVersion() {
    return version;
}

/*
  Converts a file offset to a LoROM address.
  If header == true, 512 bytes will be subtracted from the offset.

  Returns the corresponding SNES LoROM address (mapped from bank 80+)
  if successful, otherwise returns -1.
*/
uint ROMFile::toAddress(uint offset) {
    // outside of fast lorom range = invalid
    if (offset >= 0x800000) return -1;
    // within header area = invalid
    if (header && offset < 0x200) return -1;

    // adjust for header
    offset -= (header ? 0x200 : 0);
    // map bank number and 32kb range within bank
    int address = (offset & 0x7FFF) | 0x8000
                | (offset & 0x3F8000) << 1
                | 0x800000;

    return address;
}

/*
  Converts a LoROM address to a file offset.
  If header == true, 512 bytes will be added to the offset.

  Returns the corresponding file offset if successful, otherwise
  returns -1.
*/
uint ROMFile::toOffset(uint address) {
    uint offset = ((address & 0x7FFF) | ((address & 0x7F0000) >> 1))
                            + (header ? 0x200 : 0);

    return offset;
}

/*
  Opens the file and also verifies that it is one of the ROMS supported
  by the editor; displays a dialog and returns false on failure.

  For KDC, this checks for the string "ninten" at various offsets.
  It also determines whether the ROM is headered or not.
*/
const struct {int address; char string[7]; game_e game;} versions[] = {
    // Kirby Bowl (JP)
    {0x8ECE, "ninten", kirby},
    // Kirby's Dream Course (US/EU)
    {0x8ECC, "ninten", kirby},

    // Special Tee Shot (currently debug mode only)
    // checks title of rom (which can and may be changed); find something better
    {0xFFC0, "\xBD\xCD\xDF\xBC\xAC\xD9", sts},

    {0, "", kirby}
};

bool ROMFile::openROM(OpenMode flags) {
    if (!this->open(flags))
        return false;

    QSettings settings("settings.ini", QSettings::IniFormat, this);
    char buf[6];
    bool debug = settings.value("MainWindow/debug", false).toBool();

    header = this->size() % BANK_SIZE == 0x200;

    for (int i = 0; versions[i].address; i++) {
        if (!debug && versions[i].game == sts)
            continue;

        readData(versions[i].address, 6, buf);
        if (!memcmp(buf, versions[i].string, 6)) {
            game = versions[i].game;
            version = (version_e)i;
            return true;
        }
    }

    // no valid ROM detected - throw error
    QMessageBox::critical(NULL, "Open File",
                          "Please select a valid Kirby Bowl or Kirby's Dream Course ROM.",
                          QMessageBox::Ok);
    return false;
}

/*
  Reads data from a file into a pre-existing char buffer.
  If "size" == 0, the data is decompressed, with a maximum decompressed
  size of 65,536 bytes (64 kb).

  Returns the size of the data read from the file, or 0 if the read was
  unsuccessful.
*/
size_t ROMFile::readData(uint addr, uint size, void *buffer) {
    if (!size) {
        char packed[DATA_SIZE];
        this->seek(toOffset(addr));
        this->read(packed, DATA_SIZE);
        return unpack((uint8_t*)packed, (uint8_t*)buffer);
    } else {
        this->seek(toOffset(addr));
        return read((char*)buffer, size);
    }
}

uint8_t ROMFile::readByte(uint addr) {
    uint8_t data;
    readData(addr, 1, &data);
    return data;
}

uint16_t ROMFile::readInt16(uint addr) {
    uint16_t data;
    readData(addr, 2, &data);
    return data;
}

uint32_t ROMFile::readInt32(uint addr) {
    uint32_t data;
    readData(addr, 2, &data);
    return data;
}

/*
  Reads a 24-bit ROM pointer from a file, then dereferences the pointer and
  reads from the address pointed to. If "size" == 0, the data is decompressed.

  Returns the size of data read, or 0 if unsuccessful.
*/
size_t ROMFile::readFromPointer(uint addr, uint size, void *buffer) {
    uint pointer;
    // first, read the pointer
    this->seek(toOffset(addr));
    if (!(this->read((char*)&pointer, sizeof(uint)))) return 0;
    pointer &= 0x00FFFFFF;

    // then, read from where it points
    return readData(pointer, size, buffer);
}

/*
  Writes data to an ROM address in a file.
  Since this (currently) only deals with SNES ROMs, offsets will be moved up
  to 32kb boundaries when needed.

  Returns the next available address to write data to.
*/
uint ROMFile::writeData(uint addr, uint size, void *buffer) {
    uint offset = toOffset(addr);
    uint spaceLeft = BANK_SIZE - (offset % BANK_SIZE);

    // move offset forward if there's not enough space left in the bank
    if (size > spaceLeft)
        offset += spaceLeft;

    // now write data to file
    this->seek(offset);
    this->write((const char*)buffer, size);

    // return new ROM address
    return toAddress(pos());
}

uint ROMFile::writeByte(uint addr, uint8_t data) {
    return writeData(addr, 1, &data);
}

uint ROMFile::writeInt16(uint addr, uint16_t data) {
    return writeData(addr, 2, &data);
}

uint ROMFile::writeInt32(uint addr, uint32_t data) {
    return writeData(addr, 4, &data);
}

/*
  Writes data to an offset in a file, and then writes the 24-bit SNES pointer
  to that data into a second offset.

  Returns the next available offset to write data to.
*/
uint ROMFile::writeToPointer(uint pointer, uint addr,
                            uint size, void *buffer) {
    // write the data
    addr = writeData(addr, size, buffer);

    // write the data pointer
    // (do this AFTER data is written in case writeData needs to move to the next
    //  ROM bank)
    int startAddr = addr - size;
    this->seek(toOffset(pointer));
    this->write((const char*)&startAddr, 3);

    return addr;
}
