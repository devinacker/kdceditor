/*
  mainwindow.cpp

  It's the main window. Surprise!
  This class also handles opening/closing ROM files and contains the loaded levels and course
  data, which is passed to the other windows, as well as loading/saving individual course files.

  This code is released under the terms of the MIT license.
  See COPYING.txt for details.
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QStandardPaths>

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "romfile.h"
#include "kirby.h"
#include "level.h"
#include "propertieswindow.h"
#include "coursewindow.h"
#include "version.h"

#if defined(Q_OS_WIN32)
#include <windows.h>
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings(new QSettings(
                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/settings.ini",
                 QSettings::IniFormat, this)),

    fileOpen(false),
    unsaved(false),
    saving(false),
    level(0),

    levelLabel(new QLabel()),
    scene(new MapScene(this, &currentLevel)),
    previewWin(new PreviewWindow(this, &currentLevel))
{
    ui->setupUi(this);

    for (int i = 0; i < 224; i++)
        levels[i] = NULL;

    currentLevel.header.width = 0;
    currentLevel.header.length = 0;
    currentLevel.modifiedRecently = false;

    fileName   = settings->value("MainWindow/fileName", "").toString();

    ui->scrollArea->setBackgroundRole(QPalette::Mid);
    ui->scrollArea->setWidget(scene);
    ui->scrollArea->setAlignment(Qt::AlignCenter);
    // remove margins around map view and other stuff
    this->centralWidget()->layout()->setContentsMargins(0,0,0,0);

    setupSignals();
    setupActions();
    getSettings();
    setOpenFileActions(false);
    updateTitle();

#if defined(Q_OS_WIN32)
    // lousy attempt to get the right-sized icons used by Windows, since Qt will only
    // resize one icon in the icon resource (and not the one i want it to use for the
    // titlebar, anyway), at least in 4.7 (and 4.8?)
    // let's assume the small icon is already handled by Qt and so we only need to
    // care about the big one (For alt-tabbing, etc.)
    // icon resource 0 is defined in windows.rc
    SendMessage((HWND)this->winId(),
                WM_SETICON, ICON_BIG,
                (LPARAM)LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(0)));

#elif defined(Q_OS_MAC)
    // silly macs and their weird keyboards
    ui->action_Delete->setShortcut(Qt::Key_Backspace);

#endif
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
    delete levelLabel;
    delete scene;
    delete previewWin;
    delete settings;
}

void MainWindow::setupSignals() {
    // file menu
    QObject::connect(ui->action_Open_ROM, SIGNAL(triggered()),
                     this, SLOT(openFile()));
    QObject::connect(ui->action_Save_ROM, SIGNAL(triggered()),
                     this, SLOT(saveFile()));
    QObject::connect(ui->action_Save_ROM_As, SIGNAL(triggered()),
                     this, SLOT(saveFileAs()));
    QObject::connect(ui->action_Close_ROM, SIGNAL(triggered()),
                     this, SLOT(closeFile()));

    QObject::connect(ui->action_Exit, SIGNAL(triggered()),
                     this, SLOT(close()));

    // edit menu
    QObject::connect(ui->action_Undo, SIGNAL(triggered()),
                     scene, SLOT(undo()));
    QObject::connect(ui->action_Redo, SIGNAL(triggered()),
                     scene, SLOT(redo()));

    QObject::connect(ui->action_Cut, SIGNAL(triggered()),
                     scene, SLOT(cut()));
    QObject::connect(ui->action_Copy, SIGNAL(triggered()),
                     scene, SLOT(copy()));
    QObject::connect(ui->action_Paste, SIGNAL(triggered()),
                     scene, SLOT(paste()));
    QObject::connect(ui->action_Delete, SIGNAL(triggered()),
                     scene, SLOT(deleteTiles()));
    QObject::connect(ui->action_Edit_Tiles, SIGNAL(triggered()),
                     scene, SLOT(editTiles()));

    QObject::connect(scene, SIGNAL(edited()),
                     previewWin, SLOT(refresh()));
    QObject::connect(scene, SIGNAL(edited()),
                     this, SLOT(setUnsaved()));
    QObject::connect(scene, SIGNAL(edited()),
                     this, SLOT(setUndoRedoActions()));

    // level menu
    QObject::connect(ui->action_Save_Level, SIGNAL(triggered()),
                     this, SLOT(saveCurrentLevel()));
    QObject::connect(ui->action_Save_Level_to_Image, SIGNAL(triggered()),
                     previewWin, SLOT(savePreview()));

    QObject::connect(ui->action_Load_Level_from_File, SIGNAL(triggered()),
                     this, SLOT(loadLevelFromFile()));
    QObject::connect(ui->action_Save_Level_to_File, SIGNAL(triggered()),
                     this, SLOT(saveLevelToFile()));
    QObject::connect(ui->action_Load_Course_from_File, SIGNAL(triggered()),
                     this, SLOT(loadCourseFromFile()));
    QObject::connect(ui->action_Save_Course_to_File, SIGNAL(triggered()),
                     this, SLOT(saveCourseToFile()));

    QObject::connect(ui->action_Level_Properties, SIGNAL(triggered()),
                     this, SLOT(levelProperties()));

    QObject::connect(ui->action_Show_Preview, SIGNAL(triggered()),
                     previewWin, SLOT(show()));
    QObject::connect(ui->action_Center_Preview, SIGNAL(toggled(bool)),
                     previewWin, SLOT(enableCenter(bool)));

    QObject::connect(ui->action_Select_Course, SIGNAL(triggered()),
                     this, SLOT(selectCourse()));
    QObject::connect(ui->action_Previous_Level, SIGNAL(triggered()),
                     this, SLOT(prevLevel()));
    QObject::connect(ui->action_Next_Level, SIGNAL(triggered()),
                     this, SLOT(nextLevel()));
    QObject::connect(ui->action_Previous_Course, SIGNAL(triggered()),
                     this, SLOT(prevCourse()));
    QObject::connect(ui->action_Next_Course, SIGNAL(triggered()),
                     this, SLOT(nextCourse()));

    // help menu
    QObject::connect(ui->action_Contents, SIGNAL(triggered()),
                     this, SLOT(showHelp()));
    QObject::connect(ui->action_About, SIGNAL(triggered()),
                     this, SLOT(showAbout()));


    // debug menu
    QObject::connect(ui->action_Dump_Level, SIGNAL(triggered()),
                     this, SLOT(dumpLevel()));

    // other window-related stuff
    // refresh the preview window when the level is edited by double-clicking
    // the 2D view
    QObject::connect(scene, SIGNAL(doubleClicked()),
                     previewWin, SLOT(refresh()));
    // receive status bar messages from scene
    QObject::connect(scene, SIGNAL(statusMessage(QString)),
                     ui->statusBar, SLOT(showMessage(QString)));
    // center the 3D view when mousing over the 2D view
    QObject::connect(scene, SIGNAL(mouseOverTile(int,int)),
                     previewWin, SLOT(centerOn(int,int)));
}

void MainWindow::setupActions() {
    // from file menu
    ui->toolBar->addAction(ui->action_Open_ROM);
    ui->toolBar->addAction(ui->action_Save_ROM);
    ui->toolBar->addSeparator();

    // from edit menu
    ui->toolBar->addAction(ui->action_Undo);
    ui->toolBar->addAction(ui->action_Redo);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->action_Edit_Tiles);
    ui->toolBar->addAction(ui->action_Level_Properties);
    ui->toolBar->addSeparator();

    // from level menu
    ui->toolBar->addAction(ui->action_Show_Preview);
    ui->toolBar->addAction(ui->action_Center_Preview);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->action_Select_Course);
    ui->toolBar->addAction(ui->action_Previous_Course);
    ui->toolBar->addAction(ui->action_Next_Course);
    ui->toolBar->addAction(ui->action_Previous_Level);
    ui->toolBar->addAction(ui->action_Next_Level);
    ui->toolBar->addWidget(levelLabel);
}

void MainWindow::getSettings() {
    // set up windows positions/dimensions
    if (settings->contains("MainWindow/geometry"))
        this      ->setGeometry(settings->value("MainWindow/geometry").toRect());
    if (settings->value("MainWindow/maximized", false).toBool())
        this      ->showMaximized();
    if (settings->contains("PreviewWindow/geometry"))
        previewWin->setGeometry(settings->value("PreviewWindow/geometry").toRect());

    ui->action_Center_Preview->setChecked(settings->value("PreviewWindow/center", true).toBool());

    // display friendly message
    status(tr("Welcome to the untitled Kirby's Dream Course editor, version %1.")
           .arg(INFO_VERS));

#ifdef QT_NO_DEBUG
    ui->menuDebug->menuAction()->setVisible(settings->value("MainWindow/debug", false).toBool());
#endif
}

void MainWindow::saveSettings() {
    // save window settings
    settings->setValue("MainWindow/fileName", fileName);
    settings->setValue("MainWindow/maximized", this->isMaximized());
    if (!this->isMaximized())
        settings->setValue("MainWindow/geometry", this->geometry());

    settings->setValue("PreviewWindow/geometry", previewWin->geometry());
    settings->setValue("PreviewWindow/center", ui->action_Center_Preview->isChecked());
}

/*
  friendly status bar function
*/
void MainWindow::status(const QString& msg) {
    ui->statusBar->showMessage(msg);
}

void MainWindow::setUnsaved() {
    unsaved = true;
}

/*
  actions that are disabled when a file is not open
*/
void MainWindow::setOpenFileActions(bool val) {
    ui->action_Dump_Header        ->setEnabled(val);
    ui->action_Dump_Level         ->setEnabled(val);
    ui->action_Select_Course      ->setEnabled(val);
    ui->action_Show_Preview       ->setEnabled(val);
    ui->action_Save_Level_to_Image->setEnabled(val);
    setEditActions(val);
    setLevelChangeActions(val);
    // setEditActions may disable this
    ui->action_Open_ROM->setEnabled(true);
}

/*
  actions that are disabled while saving the file
*/
void MainWindow::setEditActions(bool val) {
    setUndoRedoActions(val);
    ui->action_Cut             ->setEnabled(val);
    ui->action_Copy            ->setEnabled(val);
    ui->action_Paste           ->setEnabled(val);
    ui->action_Delete          ->setEnabled(val);
    ui->action_Close_ROM       ->setEnabled(val);
    ui->action_Open_ROM        ->setEnabled(val);
    ui->action_Save_ROM        ->setEnabled(val);
    ui->action_Save_ROM_As     ->setEnabled(val);
    ui->action_Save_Level      ->setEnabled(val);
    ui->action_Edit_Tiles      ->setEnabled(val);
    ui->action_Level_Properties->setEnabled(val);
    ui->action_Load_Level_from_File->setEnabled(val);
    ui->action_Save_Level_to_File->setEnabled(val);
    ui->action_Load_Course_from_File->setEnabled(val);
    ui->action_Save_Course_to_File->setEnabled(val);
}

/*
 *actions that depend on the state of the undo stack
 */
void MainWindow::setUndoRedoActions(bool val) {
    ui->action_Undo->setEnabled(val && scene->canUndo());
    ui->action_Redo->setEnabled(val && scene->canRedo());
}

/*
  actions that depend on the current level number
*/
void MainWindow::setLevelChangeActions(bool val) {
    ROMFile::game_e game = rom.getGame();

    ui->action_Previous_Course->setEnabled(val && level >= 8);
    ui->action_Next_Course    ->setEnabled(val && level < (numLevels[game] - 8));
    ui->action_Previous_Level ->setEnabled(val && level > 0);
    ui->action_Next_Level     ->setEnabled(val && level < (numLevels[game] - 1));
}

/*
  put the current file name in the title bar
*/
void MainWindow::updateTitle() {
    if (fileOpen)
        this->setWindowTitle(QString("%1 - %2").arg(INFO_TITLE)
                             .arg(QString(fileName).split("/").last()));
    else
        this->setWindowTitle(INFO_TITLE);
}

/*
  main window close
*/
void MainWindow::closeEvent(QCloseEvent *event) {
    if (!saving && closeFile() != -1)
        event->accept();
    else
        event->ignore();
}

/*
  File menu item slots
*/
void MainWindow::openFile() {

    // open file dialog
    QString newFileName = QFileDialog::getOpenFileName(this,
                                 tr("Open ROM"),
                                 fileName,
                                 tr("SNES ROM images (*.sfc *.smc);;All files (*.*)"));

    if (!newFileName.isNull() && !closeFile()) {
        status(tr("Opening file %1").arg(newFileName));

        // open file
        rom.setFileName(newFileName);
        if (rom.openROM(QIODevice::ReadOnly)) {

            fileName = newFileName;
            unsaved  = false;

            fileOpen = true;

            ROMFile::game_e game = rom.getGame();

            for (int i = 0; i < numLevels[game]; i++) {
                levels[i] = loadLevel(rom, i);

                // if the user aborted level load, give up and close the ROM
                if (!levels[i]) {
                    closeFile();
                    return;
                }
            }

            int ver = rom.getVersion();

            // get level music selections
            for (int i = 0; i < numLevels[game] / 8; i++ ) {
                uint16_t ptr = rom.readInt16(musicTable[ver] + 2 * i);
                for (int j = 0; j < 8; j++) {
                    int level = (i * 8) + j;

                    levels[level]->music = rom.readByte(ptr + j);

                    // dirty hack due to the fact that music track 0x83 was deleted
                    // in the US/EU version of KDC, and further music tracks shifted down
                    if (ver != ROMFile::kirby_jp && levels[level]->music >= 0x83)
                        levels[level]->music++;
                }
            }

            // get course info
            // get foreground and water palettes using distance from palette base addrs.
            // (these are for all 28 courses)
            uint16_t ptr;
            for (int i = 0; i < 28; i++) {
                ptr = rom.readInt16(paletteTable[ver] + 2 * i);
                palette[i] = (ptr - fgPaletteBase[ver]) / FG_PALETTE_SIZE;

                ptr = rom.readInt16(waterTable[0][ver] + 2 * i);
                waterPalette[i] = (ptr - waterBase[0][ver]) / WATER_PALETTE_SIZE;
            }
            // get background number by comparing palette pointers
            // (since they're shorter and require no number mangling)
            // these are only for courses 0-7, and then repeat
            for (int i = 0; i < 8; i++) {
                ptr = rom.readInt16(backgroundTable[0][ver] + 2 * i);
                // compare current background selection with possible ones
                for (int j = NUM_BACKGROUNDS - 1; j >= 0; j--) {
                    if (bgNames[j].palette[ver] == ptr) {
                        background[i] = j;
                        break;
                    }
                }
            }

            // show first level
            setLevel(0);
            previewWin->show();
            setOpenFileActions(true);
            updateTitle();
            rom.close();

        } else {
            // if file open fails, display an error
            QMessageBox::warning(this,
                                 tr("Error"),
                                 tr("Unable to open %1.")
                                 .arg(newFileName),
                                 QMessageBox::Ok);
        }
    }
}

void MainWindow::saveFile() {
    if (!fileOpen || checkSaveLevel() == QMessageBox::Cancel)
        return;

    ROMFile::game_e game = rom.getGame();
    if (game == ROMFile::sts) {
        QMessageBox::information(this, tr("Save File"),
                                 tr("Saving changes to Special Tee Shot is currently not supported."),
                                 QMessageBox::Ok);
        return;
    }

    // don't save levels in STS right now

    // If there is a problem opening the original file for saving
    // (i.e. it was moved or deleted), let the user select a different one
    while (!QFile::exists(fileName)
           || (QFile::exists(fileName) && !rom.openROM(QIODevice::ReadWrite))) {
        QMessageBox::critical(this, tr("Save File"),
                              tr("Unable to open\n%1\nfor saving. Please select a different ROM.")
                              .arg(fileName),
                              QMessageBox::Ok);

        QString newFileName = QFileDialog::getSaveFileName(this,
                                     tr("Save ROM"),
                                     fileName,
                                     tr("SNES ROM images (*.sfc *.smc);;All files (*.*)"));

        // if the user pressed cancel, then don't save after all
        if (newFileName.isNull())
            return;
        // otherwise try again
        else {
            fileName = newFileName;
            rom.setFileName(fileName);
        }
    }

    status(tr("Saving to file ") + fileName);

    // disable saving/editing while already saving
    setEditActions(false);
    QCoreApplication::processEvents();
    saving = true;

    // save levels to ROM
    uint addr = saveAllLevels(rom, levels);

    // Pad the current ROM bank to 32kb to make sure it is
    // mapped correctly
    int spaceLeft = BANK_SIZE - (addr % BANK_SIZE);

    if (spaceLeft)
        rom.writeByte(addr + spaceLeft - 1, 0);

    status(tr("Saving music table"));
    QCoreApplication::processEvents();

    int ver = rom.getVersion();

    // save music table
    // (uses a bigger table, repointed)
    for (int i = 0; i < numLevels[game] / 8; i++) {
        rom.writeInt16(musicTable[ver] + (2 * i), newMusicAddr[ver] + (8 * i));
    }
    for (int i = 0; i < numLevels[game]; i++) {
        // dirty hack due to the fact that music track 0x83 was deleted
        // in the US/EU version of KDC, and further music tracks shifted down
        if (ver != ROMFile::kirby_jp && levels[level]->music >= 0x84)
            rom.writeByte(newMusicAddr[ver] + i, levels[i]->music - 1);
        else
            rom.writeByte(newMusicAddr[ver] + i, levels[i]->music);
    }

    // save course settings to ROM
    status(tr("Saving course settings"));
    QCoreApplication::processEvents();

    // first: backgrounds for courses 0-7
    uint16_t ptr, bank;

    for (int i = 0; i < 8; i++) {
        ptr = bgNames[background[i]].palette[ver];
        rom.writeInt16(backgroundTable[0][ver] + (2 * i),       ptr);
        // nighttime palette table comes right after; each night palette comes after day version
        ptr += BG_PALETTE_SIZE;
        rom.writeInt16(backgroundTable[0][ver] + (2 * i) + 0x10, ptr);

        // background tilemap pointers have to be written in two points
        // because HAL's way of storing long pointers sucks
        ptr  = bgNames[background[i]].pointer1[ver] & 0xFFFF;
        bank = bgNames[background[i]].pointer1[ver] >> 16;
        rom.writeInt16(backgroundTable[1][ver] + (4 * i),     bank);
        rom.writeInt16(backgroundTable[1][ver] + (4 * i) + 2, ptr);

        ptr  = bgNames[background[i]].pointer2[ver] & 0xFFFF;
        bank = bgNames[background[i]].pointer2[ver] >> 16;
        rom.writeInt16(backgroundTable[2][ver] + (4 * i),     bank);
        rom.writeInt16(backgroundTable[2][ver] + (4 * i) + 2, ptr);

        // background animation function pointer
        ptr = bgNames[background[i]].anim[ver];
        rom.writeInt16(backgroundTable[3][ver] + (2 * i), ptr);
    }
    // second: foregrounds for courses 0-27
    for (int i = 0; i < 28; i++) {
        ptr = fgPaletteBase[ver] + (palette[i] * FG_PALETTE_SIZE);
        rom.writeInt16(paletteTable[ver] + (2 * i), ptr);
        // nighttime palette
        ptr += (NUM_FG_PALETTES * FG_PALETTE_SIZE);
        rom.writeInt16(paletteTable[ver] + (2 * (i + 33)), ptr);

        ptr = waterBase[0][ver] + (waterPalette[i] * WATER_PALETTE_SIZE);
        rom.writeInt16(waterTable[0][ver] + (2 * i), ptr);
        ptr = waterBase[1][ver] + (waterPalette[i] * WATER_PALETTE_SIZE);
        rom.writeInt16(waterTable[1][ver] + (2 * i), ptr);
    }

    status(tr("Saved %1").arg(fileName));
    updateTitle();

    unsaved = false;
    rom.close();

    // re-enable editing
    setEditActions(true);
    saving = false;
}

void MainWindow::saveFileAs() {
    // get a new save location
    QString newFileName = QFileDialog::getSaveFileName(this,
                                 tr("Save ROM"),
                                 fileName,
                                 tr("SNES ROM images (*.sfc *.smc);;All files (*.*)"));

    if (newFileName.isNull() == false) {
        // copy old file onto new file
        if (QFile::exists(newFileName) && !QFile::remove(newFileName)) {
            QMessageBox::critical(this, tr("Save File As"),
                                  tr("Unable to update destination file.\n\nMake sure it is not open in another program, and then try again."),
                                  QMessageBox::Ok);
            return;
        }
        QFile::copy(fileName, newFileName);

        // set file name to new one, then save file
        fileName = newFileName;
        rom.setFileName(fileName);
        saveFile();
    }
}

/*
  Close the currently open file, prompting the user to save changes
  if necessary.
  Return values:
    -1: user cancels file close (file remains open)
     0: file closed successfully (or was already closed)
*/
int MainWindow::closeFile() {
    if (!fileOpen)
        return 0;

    if (checkSaveROM() == QMessageBox::Cancel)
        return -1;

    // deallocate all level data
    for (int i = 0; i < 224; i++) {
        if (levels[i])
            free(levels[i]);

        levels[i] = NULL;
    }

    // clear level displays
    currentLevel.header.length = 0;
    currentLevel.header.width  = 0;
    currentLevel.modifiedRecently = false;

    scene->cancelSelection();
    scene->refresh();
    scene->clearStack();
    previewWin->refresh();
    previewWin->hide();

    levelLabel->setText("");
    setOpenFileActions(false);
    fileOpen = false;
    updateTitle();
    update();

    return 0;
}

/*
  Level menu item slots
*/
#pragma pack(1)
typedef struct {
    char     magic[4];
    uint8_t  game, bgNum, palNum, waterNum;
    uint8_t  music[8];
    uint32_t levelPtr[8];
} coursefile_t;
#pragma pack()

void MainWindow::loadLevelFromFile() {
    if (!fileOpen) return;

    // prompt the user for a course to load
    QString newFileName = QFileDialog::getOpenFileName(this,
                                 tr("Load Level"), "",
                                 tr("Level files (*.kdcl)"));

    QFile file(newFileName);

    if (!newFileName.isNull() && file.open(QIODevice::ReadOnly)) {
        char magic[5] = {0};
        uint8_t game, music;

        file.seek(0);

        // check file magic number
        file.read(magic, 5);
        if (strcmp(magic, "KDCL")) {
            QMessageBox::warning(this, tr("Load Level"),
                                 tr("%1\nis not a valid level file.")
                                 .arg(newFileName),
                                 QMessageBox::Ok);

            file.close();
            return;
        }

        file.read((char*)&game, 1);
        file.read((char*)&music, 1);

        // TODO: check game and display warning if different from current

        // load level header
        header_t tempHeader;
        file.read((char*)&tempHeader, sizeof(header_t));

        if (tempHeader.width * tempHeader.length > MAX_2D_AREA) {
            QMessageBox::StandardButton button = QMessageBox::warning(0,
                                                  QString("Error"),
                                                  QString("Unable to load level due to an invalid level size. The course may be corrupted.\n\nContinue loading?"),
                                                  QMessageBox::Yes | QMessageBox::No);

            if (button == QMessageBox::No) {
                return;
            }
        }

        leveldata_t *lev = levels[level];
        lev->header = tempHeader;

        // load tile data
        for (int y = 0; y < lev->header.length; y++) {
            for (int x = 0; x < lev->header.width; x++) {
                file.read((char*)&(lev->tiles[y][x].geometry), 1);
                file.read((char*)&(lev->tiles[y][x].obstacle), 1);
                file.read((char*)&(lev->tiles[y][x].height), 1);
                file.read((char*)&(lev->tiles[y][x].flags), 1);
            }
        }

        // set music data
        lev->music = music;

        // mark level as modified
        lev->modified = true;
        lev->modifiedRecently = false;
        setLevel(level);
        unsaved = true;
    }

    status(tr("Loaded level %1.").arg(newFileName));
    file.close();
}

void MainWindow::saveLevelToFile() {
    if (!fileOpen || checkSaveLevel() == QMessageBox::Cancel) return;

    // prompt the user for a level to save to
    QString newFileName = QFileDialog::getSaveFileName(this,
                                 tr("Save Level"), "",
                                 tr("Level files (*.kdcl)"));

    QFile file(newFileName);

    if (!newFileName.isNull() && file.open(QIODevice::WriteOnly)) {

        file.seek(0);

        leveldata_t *lev = &currentLevel;

        char header[7] = "KDCL";
        header[5] = rom.getGame();
        header[6] = lev->music;

        // write file header
        file.write(header, 7);

        // save level header
        file.write((const char*)&lev->header, sizeof(header_t));

        // save tile data
        for (int y = 0; y < lev->header.length; y++) {
            for (int x = 0; x < lev->header.width; x++) {
                file.write((const char*)&(lev->tiles[y][x].geometry), 1);
                file.write((const char*)&(lev->tiles[y][x].obstacle), 1);
                file.write((const char*)&(lev->tiles[y][x].height), 1);
                file.write((const char*)&(lev->tiles[y][x].flags), 1);
            }
        }
    }

    status(tr("Saved level %1.").arg(newFileName));
    file.close();
}

void MainWindow::loadCourseFromFile() {
    if (!fileOpen) return;

    int course = level / 8;

    QMessageBox::StandardButton load = QMessageBox::warning(this,
                                            tr("Load Course"),
                                            tr("This will completely replace %1. Continue?")
                                            .arg(courseNames[rom.getGame()][course]),
                                            QMessageBox::Yes | QMessageBox::No);

    if (load == QMessageBox::No) return;

    // prompt the user for a course to load
    QString newFileName = QFileDialog::getOpenFileName(this,
                                 tr("Load Course"), "",
                                 tr("Course files (*.kdc)"));

    QFile file(newFileName);

    if (!newFileName.isNull() && file.open(QIODevice::ReadOnly)) {
        int courseStart = course * 8;
        coursefile_t info;

        file.seek(0);

        // check file magic number
        file.read((char*)&info, sizeof(coursefile_t));
        if (strcmp(info.magic, "KDC")) {
            QMessageBox::warning(this, tr("Load Course"),
                                 tr("%1\nis not a valid course file.")
                                 .arg(newFileName),
                                 QMessageBox::Ok);

            file.close();
            return;
        }

        // TODO: check game and display warning if different from current

        // iterate through levels and load them
        for (int i = 0; i < 8; i++) {
            leveldata_t *lev = levels[courseStart + i];

            // pointer value of 0xFFFFFFFF = don't load this one
            if (info.levelPtr[i] == 0xFFFFFFFF) continue;

            // otherwise, seek and load
            file.seek(info.levelPtr[i]);

            // load level header
            header_t tempHeader;
            file.read((char*)&tempHeader, sizeof(header_t));

            if (tempHeader.width * tempHeader.length > MAX_2D_AREA) {
                QMessageBox::StandardButton button = QMessageBox::warning(0,
                                                      QString("Error"),
                                                      QString("Unable to load level %1 due to an invalid level size. The course may be corrupted.\n\nContinue loading?")
                                                      .arg(i + 1),
                                                      QMessageBox::Yes | QMessageBox::No);

                if (button == QMessageBox::No) {
                    return;
                }

                continue;
            }

            lev->header = tempHeader;

            // load tile data
            for (int y = 0; y < lev->header.length; y++) {
                for (int x = 0; x < lev->header.width; x++) {
                    file.read((char*)&(lev->tiles[y][x].geometry), 1);
                    file.read((char*)&(lev->tiles[y][x].obstacle), 1);
                    file.read((char*)&(lev->tiles[y][x].height), 1);
                    file.read((char*)&(lev->tiles[y][x].flags), 1);
                }
            }

            // set music data
            lev->music = info.music[i];

            // mark level as modified
            lev->modified = true;
        }

        // set course graphics info
        background  [course % 8] = info.bgNum;
        palette     [course]     = info.palNum;
        waterPalette[course]     = info.waterNum;

        currentLevel.modifiedRecently = false;
        setLevel(courseStart);
        unsaved = true;
    }

    status(tr("Loaded course %1.").arg(newFileName));
    file.close();
}

void MainWindow::saveCourseToFile() {
    if (!fileOpen || checkSaveLevel() == QMessageBox::Cancel) return;

    int course = level / 8;

    // prompt the user for a course to save to
    QString newFileName = QFileDialog::getSaveFileName(this,
                                 tr("Save Course"), "",
                                 tr("Course files (*.kdc)"));

    QFile file(newFileName);

    if (!newFileName.isNull() && file.open(QIODevice::WriteOnly)) {
        int courseStart = course * 8;
        coursefile_t info;

        file.seek(0);

        // populate file header
        strcpy(info.magic, "KDC");
        info.game     = rom.getGame();
        info.bgNum    = background  [course % 8];
        info.palNum   = palette     [course];
        info.waterNum = waterPalette[course];

        int offset = sizeof(coursefile_t);

        for (int i = 0; i < 8; i++) {
            leveldata_t *lev = levels[courseStart + i];

            info.music[i] = lev->music;

            // TODO: "save modified levels only" when config stuff is added
//            if (lev->modified) {
                info.levelPtr[i] = offset;
                // generate next pointer values based on size of current level
//              offset += sizeof(header_t) + (lev->header.width * lev->header.length * sizeof(maptile_t));
                offset += sizeof(header_t) + (lev->header.width * lev->header.length * 4);
//            } else info.levelPtr[i] = 0xFFFFFFFF;
        }

        // write level header
        file.write((const char*)&info, sizeof(coursefile_t));

        // iterate through levels and load them
        for (int i = 0; i < 8; i++) {
            leveldata_t *lev = levels[courseStart + i];

            // TODO: "save modified levels only" when config stuff is added
            // if (!lev->modified) continue;

            // save level header
            file.write((const char*)&lev->header, sizeof(header_t));

            // save tile data
            for (int y = 0; y < lev->header.length; y++) {
                for (int x = 0; x < lev->header.width; x++) {
                    file.write((const char*)&(lev->tiles[y][x].geometry), 1);
                    file.write((const char*)&(lev->tiles[y][x].obstacle), 1);
                    file.write((const char*)&(lev->tiles[y][x].height), 1);
                    file.write((const char*)&(lev->tiles[y][x].flags), 1);
                }
            }
        }
    }

    status(tr("Saved course %1.").arg(newFileName));
    file.close();
}

void MainWindow::levelProperties() {
    if (currentLevel.header.length == 0) return;

    int course = level / 8;

    PropertiesWindow win(this);
    win.startEdit(&currentLevel,
                   &background[course % 8],
                   &palette[course],
                   &waterPalette[course]);

    // update 2D and 3D displays
    scene->refresh();
    previewWin->refresh();
}

void MainWindow::selectCourse() {
    CourseWindow win(this);

    int newLevel = win.select(level, rom.getGame());
    if (newLevel != level)
        setLevel(newLevel);
}

void MainWindow::prevLevel() {
    if (level) setLevel(level - 1);
}
void MainWindow::prevCourse() {
    if (level >= 8) setLevel((level/8 - 1) * 8);
}
void MainWindow::nextLevel() {
    if (level < numLevels[rom.getGame()]) setLevel(level + 1);
}
void MainWindow::nextCourse() {
    if (level < numLevels[rom.getGame()] - 8) setLevel((level/8 + 1) * 8);
}

/*
  Help menu item slots
*/
void MainWindow::showHelp() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(QCoreApplication::applicationDirPath()
                                   + "/docs/index.htm"));
}

void MainWindow::showAbout() {
    QMessageBox::information(this,
                             tr("About"),
                             tr("%1 version %2\nby Devin Acker (Revenant)")
                             .arg(INFO_TITLE).arg(INFO_VERS),
                             QMessageBox::Ok,
                             QMessageBox::Ok);
}

/*
  Display a particular level in the window
  Called when loading a ROM and also changing levels.
*/

void MainWindow::setLevel(int level) {
    if (level < 0 || level > numLevels[rom.getGame()] || !fileOpen)
        return;

    // save changes to the level?
    if (checkSaveLevel() == QMessageBox::Cancel) return;

    ROMFile::game_e game = rom.getGame();

    this->level = level;
    currentLevel = *(levels[level]);

    // update button enabled states
    setLevelChangeActions(true);

    // set up the graphics view
    scene->cancelSelection();
    scene->refresh();
    scene->clearStack();
    setUndoRedoActions(false);

    previewWin->refresh();

    // display the level name in the toolbar label
    levelLabel->setText(QString(" Level %1 - %2 (%3)")
                        .arg((level / 8) + 1, 2).arg((level % 8) + 1, 2)
                        .arg(courseNames[game][level / 8]));
}

void MainWindow::saveCurrentLevel() {
    if (!fileOpen || currentLevel.modifiedRecently == false)
        return;

    scene->setClean();
    currentLevel.modifiedRecently = false;
    unsaved = true;

    *(levels[level]) = currentLevel;

    status(tr("Level saved."));
}

/*
  Prompts the user to save or not save the level.
  If "yes" is selected, the level will be saved.
  (note: no changes will be made to the ROM until the ROM itself is saved)
  Returns the button pressed by the user.
*/
QMessageBox::StandardButton MainWindow::checkSaveLevel() {
    // if the level has not been recently modified, don't do anything
    if (!fileOpen || currentLevel.modifiedRecently == false)
        return QMessageBox::No;

    QMessageBox::StandardButton button =
            QMessageBox::question(this, tr("Save Level"),
                                  tr("Save changes to current level?"),
                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    // yes = save current level to group of all levels
    if (button == QMessageBox::Yes) {
        saveCurrentLevel();
    }

    return button;
}

/*
  Prompts the user to save or not save the entire ROM.
  Called when exiting the program or closing the ROM.
  If "yes" is selected, the ROM will be saved.
 */
QMessageBox::StandardButton MainWindow::checkSaveROM() {
    // if the ROM has not been recently modified, don't do anything
    if (!unsaved || !fileOpen)
        return QMessageBox::No;

    QMessageBox::StandardButton button =
            QMessageBox::question(this, tr("Save ROM"),
                                  tr("Save changes to ") + fileName + tr("?"),
                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    // yes = save current level to group of all levels
    if (button == QMessageBox::Yes) {
        saveFile();
    }

    return button;
}

void MainWindow::dumpLevel() {
    int l = currentLevel.header.length;
    int w = currentLevel.header.width;

    FILE *txt = fopen("currentlevel.txt", "w");

    fprintf(txt, "Level 0x%02X (course %d hole %d).\n", level, (level / 8) + 1, (level % 8) + 1);
    fprintf(txt, "Unknown value 1:  0x%04X\n", currentLevel.header.dummy1);
    fprintf(txt, "Level dimensions: %d w x %d l\n", currentLevel.header.width, currentLevel.header.length);
    fprintf(txt, "Unknown value 2:  0x%04X\n", currentLevel.header.dummy2);
    fprintf(txt, "Playfield size:   %d h x %d v\n", currentLevel.header.fieldWidth, currentLevel.header.fieldHeight);
    fprintf(txt, "Sprite alignment: %d h, %d v\n", currentLevel.header.alignHoriz, currentLevel.header.alignVert);

    fprintf(txt, "\nChunk 1: Tile terrain\n\n");

    for (int y = 0; y < l; y++) {
        for (int x = 0; x < w; x++)
            fprintf(txt, "%02X ", currentLevel.tiles[y][x].geometry);

        fprintf(txt, "\n");
    }

    fprintf(txt, "\nChunk 2: Tile obstacles\n\n");

    for (int y = 0; y < l; y++) {
        for (int x = 0; x < w; x++)
            fprintf(txt, "%02X ", currentLevel.tiles[y][x].obstacle);

        fprintf(txt, "\n");
    }

    fprintf(txt, "\nChunk 3: Tile height\n\n");

    for (int y = 0; y < l; y++) {
        for (int x = 0; x < w; x++)
            fprintf(txt, "%02X ", currentLevel.tiles[y][x].height);

        fprintf(txt, "\n");
    }

    fprintf(txt, "\nChunk 4: Tile flags\n\n");

    for (int y = 0; y < l; y++) {
        for (int x = 0; x < w; x++)
            fprintf(txt, "%02X ", *(char*)&currentLevel.tiles[y][x].flags);

        fprintf(txt, "\n");
    }

    fprintf(txt, "\nChunk 10: Clipping table\n\n");
    fprintf(txt, "Index\t\t\tx-\tx+\tprio\tzref\n");

    // generate and print chunk 10 for current level
    uint8_t clipTable[2048];
    uint8_t count;
    uint16_t ptr;
    clip_t clipTest;

    makeClipTable(&currentLevel, clipTable);

    for (int i = l + w - 1; i >= 0; i--) {
        fprintf(txt, "%d\t", i);

        memcpy(&ptr, &clipTable[2 * i], 2);
        if (ptr == 0xFFFF) {
            fprintf(txt, "\n");
            continue;
        }

        memcpy(&count, &clipTable[ptr], 1);

        int x, y, xref, yref;
        for (int j = 0; j < count; j++) {
            memcpy(&clipTest, &clipTable[ptr + 1 + (j * sizeof(clip_t))], sizeof(clip_t));

            xref = clipTest.zref % w;
            yref = l - (clipTest.zref / w) - 1;

            if (xref < clipTest.xUpper) {
                x = xref;
                y = yref - 1;
            } else {
                x = xref - 1;
                y = yref;
            }

            fprintf(txt, "(%2d, %2d)\t%2d\t%2d\t%02X\t%4X\t(%2d,%2d)\n",
                    x, y, clipTest.xLower, clipTest.xUpper, clipTest.prio,
                    clipTest.zref, xref, yref);
            if (j < count - 1)
                fprintf(txt, "\t");
        }
        if (!count)
            fprintf(txt, "\n");
    }

    fclose(txt);

    QDesktopServices::openUrl(QUrl("currentlevel.txt"));
}
