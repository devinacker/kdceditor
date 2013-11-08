#ifndef MAPCHANGE_H
#define MAPCHANGE_H

#include <QUndoCommand>

#include "level.h"

class MapChange : public QUndoCommand
{
public:
    explicit MapChange(leveldata_t *level,
                       uint x, uint y, uint l, uint w,
                       QUndoCommand *parent = 0);
    ~MapChange();

    void undo();
    void redo();
    void setText(const QString &text);

private:
    leveldata_t *level;
    uint x, y, l, w;
    maptile_t *before, *after;
    bool first;
};

#endif // MAPCHANGE_H
