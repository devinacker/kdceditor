/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#include "coursewindow.h"
#include "ui_coursewindow.h"

#include "kirby.h"
#include "level.h"
#include "romfile.h"

CourseWindow::CourseWindow(QWidget *parent) :
    QDialog(parent, Qt::CustomizeWindowHint
            | Qt::WindowTitleHint
            | Qt::WindowCloseButtonHint
            | Qt::MSWindowsFixedSizeDialogHint
           ),
    ui(new Ui::CourseWindow)
{
    ui->setupUi(this);

    // prevent window resizing
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);
}

CourseWindow::~CourseWindow()
{
    delete ui;
}

int CourseWindow::select(int level, game_e game) {
    // add course names to dropdown
    ui->comboBox->clear();
    for (int i = 0; i < numLevels[game] / 8; i++)
        ui->comboBox->addItem(courseNames[game][i]);

    ui->comboBox->setCurrentIndex(level / 8);
    ui->spinBox->setValue((level % 8) + 1);

    if (this->exec())
        return ui->comboBox->currentIndex() * 8
                + ui->spinBox->value() - 1;

    return level;
}
