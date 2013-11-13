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

/*
  This array maps conveyor belt types to their counterparts for the different slope types.
  Dimension 1 is the belt direction, dimension 2 is the slope direction
*/
const stuff::type_e conveyorMap[4][4] = {
//slope      south          east          north          west
//beltSouth
            {beltSouthDown, nothing,      beltSouthUp,   nothing},
//beltEast
            {nothing,       beltEastDown, nothing,       beltEastUp},
//beltNorth
            {beltNorthUp,   nothing,      beltNorthDown, nothing},
//beltWest
            {nothing,       beltWestUp,   nothing,       beltWestDown}
};

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
    bool relativeHeight = tileInfo.height == -1;

    // get layer selection status
    bool layer1    = ui->radioButton_Layer1->isChecked();
    bool layer2    = ui->radioButton_Layer2->isChecked();

    // after the tile edit window is done, apply the changes to the tiles
    for (int v = selY; v < selY + selLength; v++) {
        for (int h = selX; h < selX + selWidth; h++) {
            maptile_t *newTile = &(level->tiles[v][h]);

            if (newTile->geometry == 0 && tileInfo.geometry == -1) {
                continue;
            } else if (tileInfo.geometry == 0) {
                *newTile = noTile;
                continue;
            }

            if (tileInfo.geometry >= 0)
                newTile->geometry = tileInfo.geometry;
            if (tileInfo.obstacle >= 0) {
                // handle multiple types for water hazard based on terrain value
                if (tileInfo.obstacle == water
                        && newTile->geometry >= slopes && newTile->geometry < endSlopes)
                    newTile->obstacle = water - 1 + newTile->geometry;
                // handle multiple types for bounce pads
                else if (tileInfo.obstacle == bounceFlat
                         && newTile->geometry >= slopes && newTile->geometry < slopesDouble)
                    newTile->obstacle = bounce + newTile->geometry - slopes;
                // handle multiple types for conveyor belts
                else if (tileInfo.obstacle >= belts && tileInfo.obstacle < beltSlopes
                         && newTile->geometry >= slopes && newTile->geometry < slopesDouble)
                    newTile->obstacle = conveyorMap[tileInfo.obstacle - belts][newTile->geometry - slopes];
                else
                    newTile->obstacle = tileInfo.obstacle;
            }

            if (tileInfo.bumperNorth >= 0)
                newTile->flags.bumperNorth = tileInfo.bumperNorth;
            if (tileInfo.bumperSouth >= 0)
                newTile->flags.bumperSouth = tileInfo.bumperSouth;
            if (tileInfo.bumperEast >= 0)
                newTile->flags.bumperEast = tileInfo.bumperEast;
            if (tileInfo.bumperWest>= 0)
                newTile->flags.bumperWest = tileInfo.bumperWest;

            if (relativeHeight)
                newTile->height += newHeight;
            else
                newTile->height = newHeight;

            if      (layer2) newTile->flags.layer = 1;
            else if (layer1) newTile->flags.layer = 0;

            newTile->flags.dummy = 0;
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
