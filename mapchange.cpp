/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#include "mapchange.h"
#include "level.h"

MapChange::MapChange(leveldata_t *level, uint x, uint y, uint w, uint l, QUndoCommand *parent) :
    QUndoCommand(parent)
{
    this->level = level;

    this->x = x;
    this->y = y;
    this->w = w;
    this->l = l;

    this->before = new maptile_t[l * w];
    this->after  = new maptile_t[l * w];

    this->first = true;

    // when instantiated, save the region's pre-edit state
    if (level) {
        for (uint row = 0; row < l; row++)
            for (uint col = 0; col < w; col++)
                before[(row * w) + col] = this->level->tiles[y + row][x + col];

        this->setText("edit");
    }
}

MapChange::~MapChange() {
    delete[] this->before;
    delete[] this->after;
}

void MapChange::undo() {
    // restore to the region's pre-edit state
    if (level) {
        for (uint row = 0; row < this->l; row++)
            for (uint col = 0; col < this->w; col++)
                this->level->tiles[y + row][x + col] = before[(row * w) + col];
    }
}

void MapChange::redo() {
    // if being pushed, save the region's post-edit state
    // otherwise restore it
    if (level) {
        for (uint row = 0; row < this->l; row++)
            for (uint col = 0; col < this->w; col++) {
                if (first) after[(row * w) + col] = this->level->tiles[y + row][x + col];
                else       this->level->tiles[y + row][x + col] = after[(row * w) + col];
            }

        if (first) {
            level->modified = true;
            level->modifiedRecently = true;
        }
    }

    first = false;
}

void MapChange::setText(const QString &text) {
    QUndoCommand::setText(QString(text)
                          .append(" from (%1, %2) to (%3, %4)")
                          .arg(x).arg(y).arg(x + w - 1).arg(y + l - 1));
}
