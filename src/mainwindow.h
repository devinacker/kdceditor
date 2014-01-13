/*
    This code is released under the terms of the MIT license.
    See COPYING.txt for details.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QLabel>
#include <QSettings>

#include "romfile.h"
#include "mapscene.h"
#include "level.h"
#include "previewwindow.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected slots:
    // file menu
    void openFile();
    void saveFile();
    void saveFileAs();
    int  closeFile();

    void setUnsaved();

    // level menu
    void loadCourseFromFile();
    void saveCourseToFile();

    void saveCurrentLevel();

    void levelProperties();

    void selectCourse();
    void prevLevel();
    void nextLevel();
    void prevCourse();
    void nextCourse();

    // help menu
    void showHelp();
    void showAbout();

    // display text on the statusbar
    void status(const QString &msg);

    // debug menu crap
    void dumpLevel();
    
    // toolbar updates
    void setOpenFileActions(bool val);
    void setEditActions(bool val);
    void setUndoRedoActions(bool val = true);
    void setLevelChangeActions(bool val);

protected:
    void closeEvent(QCloseEvent *);

private:
    Ui::MainWindow *ui;

    QSettings *settings;

    // Information about the currently open file
    QString fileName;
    ROMFile rom;
    bool    fileOpen, unsaved, saving;

    // The level data (28 courses, 8 holes each)
    int          level;
    leveldata_t* levels[224];
    leveldata_t  currentLevel;
    QLabel *levelLabel;

    // course background/palette settings
    // (background selectiosn repeat every 8 courses)
    int    background[8];
    int    palette[28];
    int    waterPalette[28];

    // renderin stuff
    MapScene *scene;
    // the preview window
    PreviewWindow *previewWin;

    // various funcs
    void setupSignals();
    void setupActions();
    void getSettings();
    void saveSettings();
    void updateTitle();
    void setLevel(int);
    QMessageBox::StandardButton checkSaveLevel();
    QMessageBox::StandardButton checkSaveROM();
};

#endif // MAINWINDOW_H
