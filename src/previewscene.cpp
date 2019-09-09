/*
  previewscene.cpp

  Contains a QGraphicsScene subclass for rendering levels' isometric views. This is used by the preview
  window to display the "real" view of the level being edited. In the future this may be used to add a
  mini preview to the tile edit window or something.

  For the code which actually generates the isometric tile maps, see level.cpp.

  This code is released under the terms of the MIT license.
  See COPYING.txt for details.
*/

#include <QPixmap>
#include <QPainter>

#include "previewscene.h"
#include "graphics.h"
#include "level.h"
#include "mapscene.h"
#include "metatile.h"

PreviewScene::PreviewScene(QObject *parent, leveldata_t *currentLevel)
    : QGraphicsScene(parent),
      level(currentLevel),
      sprites(true)
{
    // set up sprite pixmaps
    dedede.load  (":images/dedede.png");
    enemies.load (":images/enemies.png");
    gordo.load   (":images/gordo3d.png");
    player.load  (":images/kirby.png");
}

void PreviewScene::refresh(uint16_t (&playfield)[2][MAX_FIELD_HEIGHT][MAX_FIELD_WIDTH]) {

    // load the 3d tile resource
    // (NOTE: this is a temporary measure until actual graphic/palette
    // loading is implemented)
    // each palette zone is 216px tall and each row of tiles is
    // 32 tiles long.
    if (waterLevel(level))
        tiles.load(":images/3dtiles-water.png");
    else
        tiles.load(":images/3dtiles.png");

    int mapHeight = levelHeight(level);
    int mapWidth = level->header.width;
    int mapLength = level->header.length;

    int width = qMin(MAX_FIELD_WIDTH, (int)level->header.fieldWidth);
    int height = qMin(MAX_FIELD_HEIGHT, (int)level->header.fieldHeight);

    // reset the scene (remove all members)
    this->clear();

    // set the pixmap and scene size based on the playfield's size
    QPixmap pixmap(width * ISO_TILE_SIZE, height * ISO_TILE_SIZE);
    pixmap.fill(QColor(0, 0, 0, 0));

    this->setSceneRect(0, 0, width * ISO_TILE_SIZE, height * ISO_TILE_SIZE);

    // no level area = don't render anything
    if (mapLength + mapWidth == 0) {
        this->addPixmap(pixmap);
        this->update();
        return;
    }

    // used to render one tile w/ flip etc. for each layer
    QPixmap layer1tile(ISO_TILE_SIZE, ISO_TILE_SIZE);
    QPixmap layer2tile(ISO_TILE_SIZE, ISO_TILE_SIZE);

    QPainter layer1painter, layer2painter;
    QPainter painter;

    painter.begin(&pixmap);

    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            layer1tile.fill(QColor(0,0,0,0));
            layer2tile.fill(QColor(0,0,0,0));

            uint16_t tile1 = playfield[0][h][w];
            if (TILE(tile1)) {
                int      y1    = (TILE(tile1) / 32) * ISO_TILE_SIZE
                               + (PALN(tile1) * 216);
                int      x1    = (TILE(tile1) % 32) * ISO_TILE_SIZE;

                layer1painter.begin(&layer1tile);
                layer1painter.drawPixmap(0, 0,
                                         tiles, x1, y1, ISO_TILE_SIZE, ISO_TILE_SIZE);
                layer1painter.end();

                // flip the pixmap horizontally and/or vertically
                layer1tile = QPixmap::fromImage(layer1tile.toImage().mirrored(tile1 & FH, tile1 & FV));
            }

            uint16_t tile2 = playfield[1][h][w];
            if (TILE(tile2)) {
                int      y2    = (TILE(tile2) / 32) * ISO_TILE_SIZE
                               + (PALN(tile2) * 216);
                int      x2    = (TILE(tile2) % 32) * ISO_TILE_SIZE;

                layer2painter.begin(&layer2tile);
                layer2painter.drawPixmap(0, 0,
                                         tiles, x2, y2, ISO_TILE_SIZE, ISO_TILE_SIZE);
                layer2painter.end();

                // flip the pixmap horizontally and/or vertically
                layer2tile = QPixmap::fromImage(layer2tile.toImage().mirrored(tile2 & FH, tile2 & FV));
            }

            // draw the tile pixmaps onto the scene
            // if layer 1 has priority, draw layer 2 first
            if (tile1 & PRI) {
                painter.drawPixmap(w * ISO_TILE_SIZE, h * ISO_TILE_SIZE, layer2tile);
                painter.drawPixmap(w * ISO_TILE_SIZE, h * ISO_TILE_SIZE, layer1tile);
            } else {
                painter.drawPixmap(w * ISO_TILE_SIZE, h * ISO_TILE_SIZE, layer1tile);
                painter.drawPixmap(w * ISO_TILE_SIZE, h * ISO_TILE_SIZE, layer2tile);
            }
        }
    }
    // next, if sprites are enabled, draw them
    // most of this is copied from the 2D draw code
    if (sprites) {
        for (int y = 0; y < mapLength; y++) {
            for (int x = 0; x < mapWidth; x++) {
              const QPixmap* gfx;
                int frame;
                int obs = level->tiles[y][x].obstacle;
                int z = level->tiles[y][x].height;

                if (!Util::Instance()->GetPixmapSettingsForObstacle(obs, &gfx, &frame)) {
                  continue;
                }

                if (!Util::Instance()->IsObstacleCharacterType(obs)) {
                  continue;
                }

                // draw the selected obstacle
                if (obs) {
                    // horizontal: start at 0 pixels
                    // move TILE_SIZE / 2 right for each positive move on the x-axis (west to east)
                    // and  TILE_SIZE / 2 left  for each positive move on the y-axis (north to south)
                    int startX = (TILE_SIZE / 2) * (x + (mapLength - y - 1));
                    // start at h * TILE_SIZE / 4 tiles
                    // move TILE_SIZE / 4 down for each positive move on the x-axis (west to east)
                    // and  TILE_SIZE / 4 down for each positive move on the y-axis (north to south)
                    // and  TILE_SIZE / 4 up   for each positive move on the z-axis (tile z)
                    // and then adjust for height of sprites
                    int startY = (TILE_SIZE / 4) * (mapHeight + x + y - z + 4) - gfx->height();
                    // move down half a tile's worth if the sprite is on a slope
                    if (level->tiles[y][x].geometry >= stuff::slopes)
                        startY += TILE_SIZE / 8;

                    painter.drawPixmap(startX, startY,
                                       *gfx, frame * TILE_SIZE, 0,
                                       TILE_SIZE, gfx->height());
                }
            }
        }
    }

    // finally, add the 3d map pixmap onto the scene
    painter.end();

    this->addPixmap(pixmap);
    this->update();
}
