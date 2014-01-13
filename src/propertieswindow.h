/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#ifndef PROPERTIESWINDOW_H
#define PROPERTIESWINDOW_H

#include <QDialog>

#include "level.h"

namespace Ui {
class PropertiesWindow;
}

class PropertiesWindow : public QDialog
{
    Q_OBJECT
    
public:
    explicit PropertiesWindow(QWidget *parent = 0);
    ~PropertiesWindow();

    void startEdit(leveldata_t *level, int *bg, int *pal, int *water);
    
public slots:
    void setMaxLevelWidth(int);
    void setMaxLevelLength(int);

private:
    Ui::PropertiesWindow *ui;

    leveldata_t *level;
    int *bg, *pal, *water;

private slots:
    void accept();
};

#endif // PROPERTIESWINDOW_H
