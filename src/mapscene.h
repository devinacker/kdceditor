/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#ifndef MAPSCENE_H
#define MAPSCENE_H

#include <QWidget>
#include <QMouseEvent>
#include <QtWidgets/QUndoStack>
#include <QFontMetrics>
#include "level.h"

// subclass of QGraphicsScene used to draw the 2d map and handle mouse/kb events for it
class MapScene : public QWidget {
    Q_OBJECT

private:
    static const QColor infoColor, infoBackColor;
    static const QColor selectionColor, selectionBorder;
    static const QColor layerColor;
    static const QFont infoFont;
    static const QFontMetrics infoFontMetrics;

    int tileX, tileY;
    int selX, selY, selLength, selWidth;
    bool selecting;

    maptile_t copyBuffer[MAX_2D_SIZE][MAX_2D_SIZE];
    uint copyWidth, copyLength;

    QUndoStack stack;

    leveldata_t *level;

    //QGraphicsPixmapItem *infoItem, *selectionItem;

    QPixmap tiles, kirby, enemies, traps, bounce, movers, rotate,
            conveyor, bumpers, water, warps, gordo, switches, dedede,
            unknown;

    void copyTiles(bool cut);
    void showTileInfo(QMouseEvent *event);
    void beginSelection(QMouseEvent *event);
    void updateSelection(QMouseEvent *event = NULL);

public:
    explicit MapScene(QWidget *parent = 0, leveldata_t *currentLevel = 0);

    bool canUndo() const;
    bool canRedo() const;
    bool isClean() const;

    void cancelSelection();

public slots:
    void editTiles();
    void undo();
    void redo();
    void clearStack();
    void setClean();
    void cut();
    void copy();
    void paste();
    void deleteTiles();
    void raiseTiles();
    void lowerTiles();
    void refresh(bool keepMouse = true);

signals:
    void doubleClicked();
    void statusMessage(QString);
    void mouseOverTile(int x, int y);
    void edited();

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void paintEvent(QPaintEvent *event);
};

#endif // MAPSCENE_H
