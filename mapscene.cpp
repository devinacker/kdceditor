/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#include <QPixmap>
#include <QPainter>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include "kirby.h"
#include "level.h"
#include "mainwindow.h"
#include "mapscene.h"
#include "tileeditwindow.h"
#include "graphics.h"

#define MAP_TEXT_OFFSET 8
#define MAP_TEXT_PAD 2

const QFont MapScene::infoFont("Consolas", 8);

const QColor MapScene::infoColor(255, 192, 192, 192);
const QColor MapScene::infoBackColor(255, 192, 192, 64);

const QColor MapScene::selectionColor(255, 192, 192, 128);
const QColor MapScene::selectionBorder(255, 192, 192, 255);

const QColor MapScene::layerColor(0, 192, 224, 192);

/*
  Overridden constructor which inits some scene info
 */
MapScene::MapScene(QObject *parent, leveldata_t *currentLevel)
    : QGraphicsScene(parent),

      tileX(-1), tileY(-1),
      selLength(0), selWidth(0), selecting(false),
      copyWidth(0), copyLength(0),
      level(currentLevel),
      infoItem(NULL), selectionItem(NULL)

{
    bounce.load  (":images/bounce.png");
    bumpers.load (":images/bumpers.png");
    conveyor.load(":images/conveyor.png");
    dedede.load  (":images/dedede.png");
    enemies.load (":images/enemies.png");
    gordo.load   (":images/gordo.png");
    kirby.load   (":images/kirby.png");
    movers.load  (":images/movers.png");
    rotate.load  (":images/rotate.png");
    tiles.load   (":images/terrain.png");
    traps.load   (":images/traps.png");
    warps.load   (":images/warps.png");
    water.load   (":images/water.png");
    switches.load(":images/switches.png");
    unknown.load (":images/unknown.png");

    QObject::connect(this, SIGNAL(edited()),
                     this, SLOT(refresh()));
}

/*
  clear the scene AND erase the item pointers
*/
void MapScene::erase() {
    clear();
    infoItem = NULL;
    selectionItem = NULL;
}

/*
  Edit the currently selected tiles (if any)
*/
void MapScene::editTiles() {
    if (selWidth == 0 || selLength == 0)
        return;

    // send the level and selection info to a new tile edit window instance
    TileEditWindow win;
    win.startEdit(level, QRect(selX, selY, selWidth, selLength));

    // redraw the map scene with the new properties
    emit edited();
    updateSelection();
}

/*
  Redraw the scene
*/
void MapScene::refresh() {
    tileX = -1;
    tileY = -1;
    drawLevelMap();
    removeInfoItem();
    updateSelection();
}

/*
  Handle when the mouse is pressed on the scene
*/
void MapScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    // if the level is not being displayed, don't do anything
    if (!isActive()) return;

    // left button: start or continue selection
    // right button: cancel selection
    if (event->buttons() & Qt::LeftButton) {
        beginSelection(event);

        event->accept();

    } else if (event->buttons() & Qt::RightButton) {
        cancelSelection(true);

        event->accept();
    }
}

/*
  Handle when a double-click occurs (used to start the tile edit window)
*/
void MapScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
    if (isActive()) {
        editTiles();
        emit doubleClicked();

        event->accept();
    }
}

/*
  Handle when the left mouse button is released
*/
void MapScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        selecting = false;

        // normalize selection dimensions (i.e. handle negative height/width)
        // so that selections made down and/or to the left are handled appropriately
        if (selWidth < 0) {
            selX += selWidth + 1;
            selWidth *= -1;
        }
        if (selLength < 0) {
            selY += selLength + 1;
            selLength *= -1;
        }

        event->accept();
    }
}

/*
  Handle when the mouse is moved over the scene
 */
void MapScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    // if inactive, don't handle mouse moves
    if (!isActive()) return;

    // behave differently based on left mouse button status
    if (selecting && event->buttons() & Qt::LeftButton) {
        // left button down: destroy tile info pixmap
        // and generate/show selection
        removeInfoItem();
        updateSelection(event);
    } else {
        showTileInfo(event);
    }

    event->accept();
}

/*
  Cut/copy/paste functions
*/
void MapScene::cut() {
    copyTiles(true);
}

void MapScene::copy() {
    copyTiles(false);
}

void MapScene::copyTiles(bool cut = false) {
    // if there is no selection, don't do anything
    if (selWidth == 0 || selLength == 0) return;

    // otherwise, move stuff into the buffer
    for (int i = 0; i < selLength; i++) {
        for (int j = 0; j < selWidth; j++) {
            copyBuffer[i][j] = level->tiles[selY + i][selX + j];
            if (cut)
                level->tiles[selY + i][selX + j] = noTile;
        }
    }

    copyWidth = selWidth;
    copyLength = selLength;

    if (cut) {
        level->modified = true;
        level->modifiedRecently = true;
        emit edited();
    }

    updateSelection();
    emit statusMessage(QString("%1 (%2, %3) to (%4, %5)")
                       .arg(cut ? "Cut" : "Copied")
                       .arg(selX).arg(selY)
                       .arg(selX + selWidth - 1)
                       .arg(selY + selLength - 1));
}

void MapScene::paste() {
    // if there is no selection or copy buffer, don't do anything
    if (selWidth == 0 || selLength == 0
            || copyWidth == 0 || copyLength == 0) return;

    // otherwise, move stuff into the level from the buffer
    for (uint i = 0; i < copyLength && selY + i < 64; i++) {
        for (uint j = 0; j < copyWidth && selX + j < 64; j++) {
            level->tiles[selY + i][selX + j] = copyBuffer[i][j];
        }
    }

    level->modified = true;
    level->modifiedRecently = true;
    emit edited();
    updateSelection();

    emit statusMessage(QString("Pasted (%1, %2) to (%3, %4)")
                       .arg(selX).arg(selY)
                       .arg(selX + copyWidth - 1)
                       .arg(selY + copyLength - 1));
}

void MapScene::deleteTiles() {
    // if there is no selection, don't do anything
    if (selWidth == 0 || selLength == 0) return;

    // otherwise, delete stuff
    for (int i = 0; i < selLength && selY + i < 64; i++) {
        for (int j = 0; j < selWidth && selX + j < 64; j++) {
            level->tiles[selY + i][selX + j] = noTile;
        }
    }

    level->modified = true;
    level->modifiedRecently = true;
    emit edited();
    updateSelection();

    emit statusMessage(QString("Deleted (%1, %2) to (%3, %4)")
                       .arg(selX).arg(selY)
                       .arg(selX + selWidth - 1)
                       .arg(selY + selLength - 1));
}

/*
  Start a new selection on the map scene.
  Called when the mouse is clicked outside of any current selection.
*/
void MapScene::beginSelection(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->scenePos();

    int x = pos.x() / TILE_SIZE;
    int y = pos.y() / TILE_SIZE;

    // ignore invalid click positions
    // (use the floating point X coord to avoid roundoff stupidness)
    if (x >= level->header.width || y >= level->header.length
            || pos.x() < 0 || y < 0)
        return;

    // is the click position outside of the current selection?
    if (x < selX || x >= selX + selWidth || y < selY || y >= selY + selLength) {
        selecting = true;
        selX = x;
        selY = y;
        selWidth = 1;
        selLength = 1;
        updateSelection(event);
    }
}

/*
  Update the selected range of map tiles.
  Called when the mouse is over the MapScene with the left button held down.
*/
void MapScene::updateSelection(QGraphicsSceneMouseEvent *event) {
    int x = selX;
    int y = selY;

    // if event is not null, this was triggered by a mouse action
    // and so the selection area should be updated
    if (event) {
        QPointF pos = event->scenePos();

        x = pos.x() / TILE_SIZE;
        y = pos.y() / TILE_SIZE;

        // ignore invalid mouseover/click positions
        // (use the floating point X coord to avoid roundoff stupidness)
        if (x >= level->header.width || y >= level->header.length
                || pos.x() < 0 || y < 0)
            return;

        // update the selection size
        if (x >= selX)
            selWidth = x - selX + 1;
        else
            selWidth = x - selX - 1;
        if (y >= selY)
            selLength = y - selY + 1;
        else
            selLength = y - selY - 1;
    }

    if (selWidth == 0 || selLength == 0) return;

    // temporarily destroy the old selection pixmap
    cancelSelection(false);

    QRect selArea(0, 0, abs(selWidth) * TILE_SIZE, abs(selLength) * TILE_SIZE);
    QPixmap selPixmap(selArea.width(), selArea.height());
    selPixmap.fill(selectionColor);

    int top = std::min(y, selY);
    int left = std::min(x, selX);

    selectionItem = addPixmap(selPixmap);
    selectionItem->setOffset(left * TILE_SIZE, top * TILE_SIZE);

    if (event)
        emit statusMessage(QString("Selected (%1, %2) to (%3, %4)")
                           .arg(left).arg(top)
                           .arg(left + abs(selWidth) - 1)
                           .arg(top + abs(selLength) - 1));

    // also, pass the mouseover coords to the main window
    emit mouseOverTile(x, y);
}

/*
  Display information about a map tile being hovered over.
  Called when the mouse is over the MapScene without the left button held down.
*/
void MapScene::showTileInfo(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->scenePos();
    // if hte mouse is moved onto a different tile, erase the old one
    // and draw the new one
    if ((pos.x() / TILE_SIZE) != tileX || (pos.y() / TILE_SIZE) != tileY) {
        tileX = pos.x() / TILE_SIZE;
        tileY = pos.y() / TILE_SIZE;

        removeInfoItem();

        // brand new pixmap
        QPixmap infoPixmap(TILE_SIZE, TILE_SIZE);
        infoPixmap.fill(QColor(0,0,0,0));

        // ignore invalid mouseover positions
        // (use the floating point X coord to avoid roundoff stupidness)
        if (tileX < level->header.width && tileY < level->header.length
                && pos.x() >= 0 && tileY >= 0) {
            QPainter painter(&infoPixmap);

            // render background
            painter.fillRect(0, 0, TILE_SIZE, TILE_SIZE,
                             MapScene::infoBackColor);

            // render tile info
            painter.setFont(MapScene::infoFont);
            maptile_t tile = level->tiles[tileY][tileX];

            // only draw bottom part if terrain != 0 (i.e. not empty space)
            if (tile.geometry) {
                // bottom corner: terrain + obstacle
                painter.fillRect(0, TILE_SIZE - 2 * MAP_TEXT_PAD - MAP_TEXT_OFFSET,
                                 6 * 5 + 2 * MAP_TEXT_PAD, 8 + 2 * MAP_TEXT_PAD,
                                 MapScene::infoColor);

                painter.drawText(MAP_TEXT_PAD, TILE_SIZE - MAP_TEXT_PAD,
                                 QString("%1 %2")
                                         .arg((uint)tile.geometry, 2, 16, QLatin1Char('0'))
                                         .arg((uint)tile.obstacle, 2, 16, QLatin1Char('0'))
                                         .toUpper());
            }

            // show tile contents on the status bar
            QString stat(QString("(%1,%2,%3)").arg(tileX).arg(tileY).arg(tile.height));
            try {
                stat.append(QString(" %1").arg(kirbyGeometry.at(tile.geometry)));

                if (tile.obstacle)
                    stat.append(QString(" / %1").arg(kirbyObstacles.at(tile.obstacle)));
            } catch (const std::out_of_range &dummy) {}

            emit statusMessage(stat);
        }

        infoItem = addPixmap(infoPixmap);
        infoItem->setOffset(tileX * TILE_SIZE, tileY * TILE_SIZE);

        // also, pass the mouseover coords to the main window
        emit mouseOverTile(tileX, tileY);
    }
}

/*
  Remove the selection pixmap from the scene.
  if "perma" is true, the selection rectangle is also reset.
*/

// public version used to deselect when changing levels
void MapScene::cancelSelection() {
    cancelSelection(true);
}

void MapScene::cancelSelection(bool perma) {
    if (perma) {
        selWidth = 0;
        selLength = 0;
        selX = 0;
        selY = 0;
    }

    if (selectionItem) {
        removeItem(selectionItem);
        delete selectionItem;
        selectionItem = NULL;
    }
}

void MapScene::removeInfoItem() {
    // infoItem is no longer valid so kill it
    if (infoItem) {
        removeItem(infoItem);
        delete infoItem;
        infoItem = NULL;
    }
}

void MapScene::drawLevelMap() {
    // reset the scene (remove all members)
    erase();

    // if level is null , minimize the scene and return
    if (!level) {
        setSceneRect(0, 0, 0, 0);
        return;
    }

    int width = level->header.width;
    int height = level->header.length;

    // set the pixmap and scene size based on the level's size
    QPixmap pixmap(width * TILE_SIZE, height * TILE_SIZE);
    pixmap.fill(QColor(128, 128, 128));

    setSceneRect(0, 0, width * TILE_SIZE, height * TILE_SIZE);

    // no width/height = don't draw anything
    if (width + height == 0) {
        addPixmap(pixmap);
        update();
        return;
    }

    // assign a painter to the target pixmap
    QPainter painter;
    //if (width + height)
        painter.begin(&pixmap);

    // slowly blit shit from the tile resource onto the pixmap
    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            maptile_t *tile = &level->tiles[h][w];
            int geo = tile->geometry;
            if (geo) {
                painter.drawPixmap(w * TILE_SIZE, h * TILE_SIZE,
                                   tiles,
                                   (geo - 1) * TILE_SIZE, 0,
                                   TILE_SIZE, TILE_SIZE);
            }

            // include obstacles and all other stuff in the same pass
            int obs = tile->obstacle;

            QPixmap gfx;
            int frame = 0;

            /*
             *
             * START OF KIRBY OBSTACLE CHECK
             * (TODO: move the enum from metatile.h and use it instead of magic numbers
             */

            // whispy woods (index 0x00 in enemies.png)
            if (obs == 0x02) {
                gfx = enemies;
                frame = 0;

            // sand trap (index 0 in traps.png)
            } else if (obs == 0x04) {
                gfx = traps;
                frame = 0;

            // spike pit (index 1 in traps.png)
            } else if (obs == 0x05) {
                gfx = traps;
                frame = 1;

            // kirby's start pos (kirby.png)
            } else if (obs == 0x0c) {
                gfx = kirby;
                frame = 0;

            // dedede (frame 0 in dedede.png)
            } else if (obs == 0x0d) {
                gfx = dedede;
                frame = 0;

            // current, arrows, boosters, vents
            // (ind. 00 to 0d in movers.png)
            } else if (obs >= 0x10 && obs <= 0x1d) {
                gfx = movers;
                frame = obs - 0x10;

            // bouncy pads (ind. 0 to 4 in bounce.png)
            } else if (obs >= 0x20 && obs <= 0x24) {
                gfx = bounce;
                frame = obs - 0x20;

            // bumpers (start at index 4 in bumpers.png)
            } else if (obs >= 0x28 && obs <= 0x2d) {
                gfx = bumpers;
                frame = obs - 0x28 + 4;

            // conveyor belts (ind. 0 to b in conveyor.png)
            } else if (obs >= 0x30 && obs <= 0x3b) {
                gfx = conveyor;
                frame = obs - 0x30;

            // most enemies (ind. 01 to 13 in enemies.png)
            } else if (obs >= 0x40 && obs <= 0x52) {
                gfx = enemies;
                frame = obs - 0x40 + 1;

            // transformer (ind. 14 in enemies.png
            } else if (obs == 0x57) {
                gfx = enemies;
                frame = 0x14;

            // switches (ind. 0 to 5 in switches.png)
            } else if (obs >= 0x58 && obs <= 0x5d) {
                gfx = switches;
                frame = obs - 0x58;

            // water hazards (ind. 0 to e in water.png)
            // (note types 62 & 63 seem unused)
            } else if (obs >= 0x61 && obs <= 0x6f) {
                gfx = water;
                frame = obs - 0x61;

            // rotating spaces (ind. 0-b in rotate.png)
            } else if (obs >= 0x70 && obs <= 0x7b) {
                gfx = rotate;
                frame = obs & 0x01;

            // gordo (ind. 00 to 21 in gordo.png)
            } else if (obs >= 0x80 && obs <= 0xa1) {
                gfx = gordo;
                frame = obs - 0x80;

            // kracko (index 15-17 in enemies.png)
            } else if (obs >= 0xac && obs <= 0xae) {
                gfx = enemies;
                frame = obs - 0xac + 0x15;

            // warps (ind. 0 to 9 in warps.png)
            } else if (obs >= 0xb0 && obs <= 0xb9) {
                gfx = warps;
                frame = obs - 0xb0;

            // starting line (ind. 1 to 4 in dedede.png)
            } else if (obs >= 0xc0 && obs <= 0xc3) {
                gfx = dedede;
                frame = obs - 0xc0 + 1;

            // anything else - question mark (or don't draw)
            } else {
#ifdef QT_DEBUG
                gfx = unknown;
                frame = 0;
#else
                obs = 0;
#endif
            }

            /*
             *
             * END OF KIRBY OBSTACLE CHECK
             *
             */

            // draw the selected obstacle
            if (obs) {
                painter.drawPixmap(w * TILE_SIZE,
                                   (h + 1) * TILE_SIZE - gfx.height(),
                                   gfx, frame * TILE_SIZE, 0,
                                   TILE_SIZE, gfx.height());
            }

            // render side bumpers (ind. 0 - 3 in bumpers.png)
            if (tile->flags.bumperSouth)
                painter.drawPixmap(w * TILE_SIZE, h * TILE_SIZE,
                                   bumpers, 0 * TILE_SIZE, 0,
                                   TILE_SIZE, TILE_SIZE);
            if (tile->flags.bumperEast)
                painter.drawPixmap(w * TILE_SIZE, h * TILE_SIZE,
                                   bumpers, 1 * TILE_SIZE, 0,
                                   TILE_SIZE, TILE_SIZE);
            if (tile->flags.bumperNorth)
                painter.drawPixmap(w * TILE_SIZE, h * TILE_SIZE,
                                   bumpers, 2 * TILE_SIZE, 0,
                                   TILE_SIZE, TILE_SIZE);
            if (tile->flags.bumperWest)
                painter.drawPixmap(w * TILE_SIZE, h * TILE_SIZE,
                                   bumpers, 3 * TILE_SIZE, 0,
                                   TILE_SIZE, TILE_SIZE);

            painter.setFont(MapScene::infoFont);

            if (geo) {
                // draw the tile height in the bottom right corner
                painter.fillRect((w+1) * TILE_SIZE - 12 - 2 * MAP_TEXT_PAD,
                                 (h+1) * TILE_SIZE - 8 - 2 * MAP_TEXT_PAD,
                                 12 + 2 * MAP_TEXT_PAD, 8 + 2 * MAP_TEXT_PAD,
                                 MapScene::infoColor);

                painter.drawText((w+1) * TILE_SIZE - 12 - MAP_TEXT_PAD,
                                 (h+1) * TILE_SIZE - MAP_TEXT_PAD,
                                 QString("%1").arg(tile->height, 2));
            }

            if (tile->flags.layer) {
                // draw an indicator for layer 2
                painter.fillRect((w+1) * TILE_SIZE - 12 - 2 * MAP_TEXT_PAD,
                                 h * TILE_SIZE,
                                 12 + 2 * MAP_TEXT_PAD, 8 + 2 * MAP_TEXT_PAD,
                                 MapScene::layerColor);

                painter.drawText((w+1) * TILE_SIZE - 12 - MAP_TEXT_PAD,
                                 h * TILE_SIZE + MAP_TEXT_PAD + 8,
                                 "L2");
            }
        }
    }

    // draw tile grid
    for (int h = TILE_SIZE; h < height * TILE_SIZE; h += TILE_SIZE)
        painter.drawLine(0, h, width * TILE_SIZE, h);
    for (int w = TILE_SIZE; w < width * TILE_SIZE; w += TILE_SIZE)
        painter.drawLine(w, 0, w, height * TILE_SIZE);

    // put the new finished pixmap into the scene
    //if (width + height > 0)
        painter.end();

    addPixmap(pixmap);
    update();
}
