#include "mapchange.h"
#include "level.h"

MapChange::MapChange(leveldata_t *level, uint x, uint y, uint l, uint w, QUndoCommand *parent) :
    QUndoCommand(parent)
{
    this->level = level;

    this->x = x;
    this->y = y;
    this->l = l;
    this->w = w;

    this->before = new maptile_t[l * w];
    this->after  = new maptile_t[l * w];

    this->first = true;

    // when instantiated, save the region's pre-edit state
    if (level) {
        for (uint i = 0; i < l; i++)
            for (uint j = 0; j < w; j++)
                before[(i * w) + j] = this->level->tiles[y + i][x + j];

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
        for (uint i = 0; i < this->l; i++)
            for (uint j = 0; j < this->w; j++)
                this->level->tiles[y + i][x + j] = before[(i * w) + j];
    }
}

void MapChange::redo() {
    // if being pushed, save the region's post-edit state
    // otherwise restore it
    if (level) {
        for (uint i = 0; i < this->l; i++)
            for (uint j = 0; j < this->w; j++) {
                if (first) after[(i * w) + j] = this->level->tiles[y + i][x + j];
                else       this->level->tiles[y + i][x + j] = after[(i * w) + j];
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
