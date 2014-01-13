/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#ifndef MAPCHANGE_H
#define MAPCHANGE_H

#include <QUndoCommand>

#include "level.h"

class MapChange : public QUndoCommand
{
public:
    explicit MapChange(leveldata_t *currLevel,
                       uint x, uint y, uint w, uint l,
                       QUndoCommand *parent = 0);
    ~MapChange();

    void undo();
    void redo();
    void setText(const QString &text);

private:
    leveldata_t *level;
    uint x, y, w, l;
    maptile_t *before, *after;
    bool first;
};

#endif // MAPCHANGE_H
