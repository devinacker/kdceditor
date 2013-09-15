/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#ifndef PREVIEWWINDOW_H
#define PREVIEWWINDOW_H

#include <QtWidgets/QDialog>
#include "level.h"
#include "previewscene.h"

namespace Ui {
class PreviewWindow;
}

class PreviewWindow : public QDialog
{
    Q_OBJECT
    
public:
    explicit PreviewWindow(QWidget *parent = 0, leveldata_t *currentLevel = 0);
    ~PreviewWindow();

    void setLevel(leveldata_t *level);
    
public slots:
    void refresh();
    void centerOn(int x, int y);
    void enableCenter(bool);
    void savePreview();

private:
    Ui::PreviewWindow *ui;

    leveldata_t    *level;

    PreviewScene   *scene;
    bool           center;

    // The maximum area of the playfield is 13312 tiles.
    // The max for each dimension here was chosen as 384, but the
    // length * width cannot ever exceed 13312 tiles (or 26624 bytes.)
    // There are two playfields (one per layer) with the same size and layout.
    uint16_t  playfield[2][384][384];

};

#endif // PREVIEWWINDOW_H
