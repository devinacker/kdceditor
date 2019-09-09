/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#include <cstdlib>

#include "tileeditwindow.h"
#include "ui_tileeditwindow.h"

#include "kirby.h"
#include "level.h"
#include "metatile.h"

using namespace stuff;

uint getCheckBox(const QCheckBox *box);
void setCheckBox(QCheckBox *box, int state);

TileEditWindow::TileEditWindow(QWidget *parent) :
    QDialog(parent, Qt::CustomizeWindowHint
                  | Qt::WindowTitleHint
                  | Qt::WindowCloseButtonHint
                  | Qt::MSWindowsFixedSizeDialogHint
      ),
    ui(new Ui::TileEditWindow)
{
    ui->setupUi(this);

    // prevent window resizing
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    // add geometry types to dropdown
    for (StringMap::const_iterator i = kirbyGeometry.begin(); i != kirbyGeometry.end(); i++) {
        if (i->second.size()) {
            ui->comboBox_Terrain->addItem(i->second, i->first);
        }
    }

    // add obstacle types to dropdown (only valid ones)
    for (StringMap::const_iterator i = kirbyObstacles.begin(); i != kirbyObstacles.end(); i++) {
        if (i->second.size()) {
            ui->comboBox_Obstacle->addItem(i->second, i->first);
        }
    }
}

TileEditWindow::~TileEditWindow()
{
    delete ui;
}

int TileEditWindow::startEdit(leveldata_t *level, QRect sel) {
    this->level = level;

    // iterate through the tiles in the selection and build up the
    // selection info.
    selX      = sel.x();
    selY      = sel.y();
    selWidth  = sel.width();
    selLength = sel.height();
    maptile_t tile = level->tiles[selY][selX];

    // set initial values to compare against
    // (to determine when the selection covers multiple values)
    tileInfo.geometry    = tile.geometry;
    tileInfo.obstacle    = tile.obstacle;
    tileInfo.height      = tile.height;
    tileInfo.bumperEast  = tile.flags.bumperEast;
    tileInfo.bumperNorth = tile.flags.bumperNorth;
    tileInfo.bumperSouth = tile.flags.bumperSouth;
    tileInfo.bumperWest  = tile.flags.bumperWest;

    tileInfo.layer       = tile.flags.layer;

    tileInfo.maxHeight   = tile.height;
    tileInfo.minHeight   = tile.height;

    for (int v = selY; v < selY + selLength; v++) {
        for (int h = selX; h < selX + selWidth; h++) {
            tile = level->tiles[v][h];
            if (tile.geometry != tileInfo.geometry)
                tileInfo.geometry = -1;
            if (tile.obstacle != tileInfo.obstacle)
                tileInfo.obstacle = -1;
            if (tile.height != tileInfo.height)
                tileInfo.height = -1;

            tileInfo.minHeight = std::min((int)tileInfo.minHeight, (int)tile.height);
            tileInfo.maxHeight = std::max((int)tileInfo.maxHeight, (int)tile.height);

            if (tile.flags.bumperEast != tileInfo.bumperEast)
                tileInfo.bumperEast = -1;
            if (tile.flags.bumperNorth != tileInfo.bumperNorth)
                tileInfo.bumperNorth = -1;
            if (tile.flags.bumperSouth != tileInfo.bumperSouth)
                tileInfo.bumperSouth = -1;
            if (tile.flags.bumperWest != tileInfo.bumperWest)
                tileInfo.bumperWest = -1;

            if (tile.flags.layer != tileInfo.layer)
                tileInfo.layer = -1;
        }
    }

    // multiple height values = raise/lower instead of set height
    bool relativeHeight = tileInfo.height == -1;

    // set the control values to the stuff in the tileinfo
    if (tileInfo.geometry == -1) {
        ui->comboBox_Terrain->insertItem(0, tr("(multiple)"), 0);
        ui->comboBox_Terrain->setCurrentIndex(0);
    } else {
        ui->comboBox_Terrain->setCurrentIndex(std::distance(kirbyGeometry.begin(),
                                                            kirbyGeometry.find(tileInfo.geometry)));
    }
    if (tileInfo.obstacle == -1) {
        ui->comboBox_Obstacle->insertItem(0, tr("(multiple)"));
        ui->comboBox_Obstacle->setCurrentIndex(0);
    } else {
        ui->comboBox_Obstacle->setCurrentIndex(std::distance(kirbyObstacles.begin(),
                                                             kirbyObstacles.find(tileInfo.obstacle)));
    }

    // set bumper checkboxes to correct values
    setCheckBox(ui->checkBox_North, tileInfo.bumperNorth);
    setCheckBox(ui->checkBox_South, tileInfo.bumperSouth);
    setCheckBox(ui->checkBox_East, tileInfo.bumperEast);
    setCheckBox(ui->checkBox_West, tileInfo.bumperWest);

    // set up the height slider
    relativeHeight = tileInfo.height == -1;
    int min, max, value;
    if (relativeHeight) {
        ui->label_Height->setText(tr("Raise/Lower"));
        min = 0 - tileInfo.minHeight;
        max = MAX_HEIGHT - tileInfo.maxHeight;
        value = 0;
    } else {
        ui->label_Height->setText(tr("Height"));
        min = 0;
        max = MAX_HEIGHT;
        value = tileInfo.height;
    }
    ui->horizontalSlider_Height->setMinimum(min);
    ui->spinBox_Height->setMinimum(min);
    ui->horizontalSlider_Height->setMaximum(max);
    ui->spinBox_Height->setMaximum(max);
    ui->horizontalSlider_Height->setValue(value);

    // set up the layer radio buttons
    ui->radioButton_Keep->setChecked(tileInfo.layer == -1);
    ui->radioButton_Layer1->setChecked(tileInfo.layer == 0);
    ui->radioButton_Layer2->setChecked(tileInfo.layer == 1);

    // run the modal dialog
    return this->exec();
}

void TileEditWindow::accept() {
    // get terrain and obstacle values
    // (if multiple geom/obstacle values are selected, only use index >0)
    if (tileInfo.geometry != -1 || ui->comboBox_Terrain->currentIndex() > 0)
        tileInfo.geometry = ui->comboBox_Terrain->itemData(ui->comboBox_Terrain->currentIndex()).toUInt();

    if (tileInfo.obstacle != -1 || ui->comboBox_Obstacle->currentIndex() > 0)
        //tileInfo.obstacle = stringVal(ui->comboBox_Obstacle->currentText());
        tileInfo.obstacle = ui->comboBox_Obstacle->itemData(ui->comboBox_Obstacle->currentIndex()).toUInt();

    // get bumper checkbox status
    tileInfo.bumperNorth = getCheckBox(ui->checkBox_North);
    tileInfo.bumperSouth = getCheckBox(ui->checkBox_South);
    tileInfo.bumperEast  = getCheckBox(ui->checkBox_East);
    tileInfo.bumperWest  = getCheckBox(ui->checkBox_West);

    // get height slider status
    int newHeight = ui->horizontalSlider_Height->value();
    tileInfo.height = newHeight;

    // get layer selection status
    bool layer1    = ui->radioButton_Layer1->isChecked();
    bool layer2    = ui->radioButton_Layer2->isChecked();
    if (layer2) {
      tileInfo.layer = 1;
    } else if (layer1) {
      tileInfo.layer = 0;
    } else {
      tileInfo.layer = -1;
    }

    // after the tile edit window is done, apply the changes to the tiles
    for (int v = selY; v < selY + selLength; v++) {
        for (int h = selX; h < selX + selWidth; h++) {
            maptile_t *newTile = &(level->tiles[v][h]);
            Util::Instance()->ApplyTileToExistingTile(tileInfo, newTile);
        }
    }

    QDialog::accept();
}

/*
  Set a checkbox's tristate value and check state based on the
  appropriate value in tileinfo.
*/
void setCheckBox(QCheckBox *box, int state) {
    box->setTristate(state == -1);
    switch (state) {
        case -1:
            box->setCheckState(Qt::PartiallyChecked);
            break;
        case 1:
            box->setCheckState(Qt::Checked);
            break;
        default:
            box->setCheckState(Qt::Unchecked);
    }
}

/*
  Get a checkbox's value for tileinfo based on its check state
*/
uint getCheckBox(const QCheckBox *box) {
    return (box->checkState() == Qt::PartiallyChecked)
            ? -1 : box->checkState() / 2;
}
