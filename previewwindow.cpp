/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#include "previewwindow.h"
#include "ui_previewwindow.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QSettings>
#include "previewscene.h"

PreviewWindow::PreviewWindow(QWidget *parent, leveldata_t *currentLevel) :
    QDialog(parent, Qt::Tool
                  | Qt::CustomizeWindowHint
                  | Qt::WindowTitleHint
                  | Qt::WindowMinimizeButtonHint
                  | Qt::WindowCloseButtonHint
//                | Qt::WindowStaysOnTopHint
            ),
    ui(new Ui::PreviewWindow),
    level(currentLevel),
    scene(new PreviewScene(this, currentLevel)),
    center(true)
{
    ui->setupUi(this);

    //remove margins around graphics view
    this->layout()->setContentsMargins(0,0,0,0);

    ui->graphicsView->setScene(scene);
}


PreviewWindow::~PreviewWindow()
{
    delete ui;
    delete scene;
}

void PreviewWindow::setLevel(leveldata_t *level) {
    this->level = level;
    refresh();
}

void PreviewWindow::refresh() {
    // rebuild the 3D tile map
    makeIsometricMap(this->playfield, this->level);
    // and draw it
    scene->refresh(this->playfield);
    // refresh the display
    ui->graphicsView->update();
}

/*
  Converts 2D map coordinates into 3D display coordinates
  in order to center the preview display on a specific tile.
*/
void PreviewWindow::centerOn(int x, int y) {
    if (!center) return;

    int z = level->tiles[y][x].height;
    int h = levelHeight(level);
    int l = level->header.length;

    // start at 32 pixels
    // move 32 right for each positive move on the x-axis (west to east)
    // and  32 left  for each positive move on the y-axis (north to south)
    //int centerX = 32 * (1 + x + (l - y - 1));
    int centerX = 32 * (x + l - y);
    // start at 16 *(h + 1) pixels
    // move 16 down for each positive move on the x-axis (west to east)
    // and  16 down for each positive move on the y-axis (north to south)
    // and  16 up   for each positive move on the z-axis (tile z)
    int centerY = 16 * (h + x + y - z + 1);

    ui->graphicsView->centerOn(centerX, centerY);
}

void PreviewWindow::enableCenter(bool center) {
    this->center = center;
}

void PreviewWindow::savePreview() {
    QList<QGraphicsItem*> items = scene->items();

    if (!items.size()) {
        QMessageBox::information(NULL, tr("Save Level to Image"),
                                 tr("No level is currently open."),
                                 QMessageBox::Ok);
        return;
    }

    QSettings settings("settings.ini");

    QString imageName = QFileDialog::getSaveFileName(this,
                                 tr("Save Level to Image"),
                                 settings.value("PreviewWindow/fileName", "").toString(),
                                 tr("PNG image (*.png)"));
    if (!imageName.isNull()) {
        if (!((QGraphicsPixmapItem*)items[0])->pixmap().save(imageName)) {
            QMessageBox::warning(NULL, tr("Save Level to Image"),
                                 tr("Error saving %1.").arg(imageName),
                                 QMessageBox::Ok);
        }

        settings.setValue("PreviewWindow/fileName", imageName);
    }
}
