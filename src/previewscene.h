/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#ifndef PREVIEWSCENE_H
#define PREVIEWSCENE_H

#include <QPixmap>
#include <QtWidgets/QGraphicsScene>
#include "level.h"

class PreviewScene : public QGraphicsScene {
    Q_OBJECT

private:
    leveldata_t *level;
    bool sprites;

    QPixmap tiles, dedede, enemies, gordo, player;

public:
    PreviewScene(QObject *parent, leveldata_t *currentLevel);
    void refresh(uint16_t (&playfield)[2][MAX_FIELD_HEIGHT][MAX_FIELD_WIDTH]);
};

#endif // PREVIEWSCENE_H
