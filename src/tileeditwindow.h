/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#ifndef TILEEDITWINDOW_H
#define TILEEDITWINDOW_H

#include <QtWidgets/QDialog>

#include "level.h"

namespace Ui {
class TileEditWindow;
}

class TileEditWindow : public QDialog
{
    Q_OBJECT
    
public:
    explicit TileEditWindow(QWidget *parent = 0);
    ~TileEditWindow();

    int startEdit(leveldata_t *level, QRect sel);
    
private:
    Ui::TileEditWindow *ui;

    leveldata_t *level;
    tileinfo_t tileInfo;
    int selX, selY, selLength, selWidth;
    bool relativeHeight;

private slots:
    void accept();
};

#endif // TILEEDITWINDOW_H
