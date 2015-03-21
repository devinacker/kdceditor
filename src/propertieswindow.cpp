/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#include <QMessageBox>
#include <algorithm>
#include "propertieswindow.h"
#include "ui_propertieswindow.h"
#include "kirby.h"
#include "level.h"

PropertiesWindow::PropertiesWindow(QWidget *parent) :
    QDialog(parent, Qt::CustomizeWindowHint
            | Qt::WindowTitleHint
            | Qt::WindowCloseButtonHint
            | Qt::MSWindowsFixedSizeDialogHint
           ),
    ui(new Ui::PropertiesWindow)
{
    ui->setupUi(this);

    // prevent window resizing
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    // add BG, palette, and music names to boxes
    for (int i = 0; i < NUM_BACKGROUNDS; i++)
        ui->comboBox_Background->addItem(bgNames[i].name);

    for (int i = 0; i < NUM_FG_PALETTES; i++) {
        ui->comboBox_Palette->addItem(paletteNames[i]);
        ui->comboBox_Water->addItem(paletteNames[i]);
    }
    for (StringMap::const_iterator i = musicNames.begin(); i != musicNames.end(); i++) {
        ui->comboBox_Music->addItem(i->second, i->first);
    }

    // set up signals to handle width/length constraints
    QObject::connect(ui->spinBox_Length, SIGNAL(valueChanged(int)),
                     this, SLOT(setMaxLevelWidth(int)));
    QObject::connect(ui->spinBox_Width, SIGNAL(valueChanged(int)),
                     this, SLOT(setMaxLevelLength(int)));
}

PropertiesWindow::~PropertiesWindow()
{
    delete ui;
}

void PropertiesWindow::setMaxLevelWidth(int length) {
    ui->spinBox_Width->setMaximum(std::min(MAX_2D_SIZE, MAX_2D_AREA / length));
}

void PropertiesWindow::setMaxLevelLength(int width) {
    ui->spinBox_Length->setMaximum(std::min(MAX_2D_SIZE, MAX_2D_AREA / width));
}

void PropertiesWindow::startEdit(leveldata_t *level,
                            int *bg, int *pal, int *water) {

    // save pointers
    this->level = level;
    this->bg    = bg;
    this->pal   = pal;
    this->water = water;

    // set height and width values
    ui->spinBox_Length->setValue(level->header.length);

    ui->spinBox_Width ->setValue(level->header.width);

    // set BG and FG stuff
    ui->comboBox_Background->setCurrentIndex(*bg);
    ui->comboBox_Palette   ->setCurrentIndex(*pal);
    ui->comboBox_Water     ->setCurrentIndex(*water);

    // set music value
    ui->comboBox_Music->setCurrentIndex(std::distance(musicNames.begin(),
                                                      musicNames.find(level->music)));

    this->exec();
}

void PropertiesWindow::accept() {
    // apply level size
    level->header.length = ui->spinBox_Length->value();
    level->header.width  = ui->spinBox_Width ->value();

    // apply FG/BG settings
    *bg    = ui->comboBox_Background->currentIndex();
    *pal   = ui->comboBox_Palette->currentIndex();
    *water = ui->comboBox_Water->currentIndex();

    // apply music setting
    level->music = ui->comboBox_Music->itemData(ui->comboBox_Music->currentIndex()).toUInt();

    level->modified = true;
    level->modifiedRecently = true;

    QDialog::accept();
}
