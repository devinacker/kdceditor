/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#ifndef MAPSCENE_H
#define MAPSCENE_H

#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsPixmapItem>
#include <QtWidgets/QGraphicsSceneMouseEvent>

#include "level.h"

// subclass of QGraphicsScene used to draw the 2d map and handle mouse/kb events for it
class MapScene : public QGraphicsScene {
    Q_OBJECT

private:
    static const QColor infoColor, infoBackColor;
    static const QColor selectionColor, selectionBorder;
    static const QColor layerColor;
    static const QFont infoFont;

    int tileX, tileY;
    int selX, selY, selLength, selWidth;
    bool selecting;

    maptile_t copyBuffer[64][64];
    uint copyWidth, copyLength;

    leveldata_t *level;

    QGraphicsPixmapItem *infoItem, *selectionItem;

    QPixmap tiles, kirby, enemies, traps, bounce, movers, rotate,
            conveyor, bumpers, water, warps, gordo, switches, dedede,
            unknown;

    void copyTiles(bool cut);
    void showTileInfo(QGraphicsSceneMouseEvent *event);
    void beginSelection(QGraphicsSceneMouseEvent *event);
    void updateSelection(QGraphicsSceneMouseEvent *event = NULL);
    void removeInfoItem();
    void cancelSelection(bool perma);
    void drawLevelMap();

private slots:
    void erase();

public:
    MapScene(QObject *parent = 0, leveldata_t *currentLevel = 0);
    void cancelSelection();

public slots:
    void editTiles();
    void cut();
    void copy();
    void paste();
    void deleteTiles();
    void refresh();

signals:
    void doubleClicked();
    void statusMessage(QString);
    void mouseOverTile(int x, int y);
    void edited();

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
};

#endif // MAPSCENE_H
