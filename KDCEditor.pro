#-------------------------------------------------
#
# Project created by QtCreator 2013-01-12T02:12:24
#
#-------------------------------------------------

# Note: make sure docs directory is in executable directory on launch
# or else Help > Contents will fail

QT       += core widgets

QMAKE_CFLAGS += -std=c99

# Use the latter, if your compiler supports it
#QMAKE_CXXFLAGS += -std=c++0x -U__STRICT_ANSI__
QMAKE_CXXFLAGS += -std=c++11

TARGET = KDCEditor
TEMPLATE = app

# OS-specific metadata and stuff
win32:RC_FILE = windows.rc

SOURCES += main.cpp\
        mainwindow.cpp \
    tileeditwindow.cpp \
    compress.c \
    level.cpp \
    kirby.cpp \
    mapscene.cpp \
    previewwindow.cpp \
    propertieswindow.cpp \
    romfile.cpp \
    metatile.cpp \
    metatile_terrain.cpp \
    metatile_borders.cpp \
    metatile_obstacles.cpp \
    coursewindow.cpp \
    previewscene.cpp

HEADERS  += mainwindow.h \
    tileeditwindow.h \
    compress.h \
    level.h \
    kirby.h \
    mapscene.h \
    graphics.h \
    previewwindow.h \
    propertieswindow.h \
    romfile.h \
    metatile.h \
    coursewindow.h \
    version.h \
    previewscene.h

FORMS    += mainwindow.ui \
    tileeditwindow.ui \
    previewwindow.ui \
    propertieswindow.ui \
    coursewindow.ui

RESOURCES += \
    images.qrc \
    icons.qrc

OTHER_FILES += \
    CHANGES.txt \
    README.txt \
    TODO.txt \
    coursefiles.txt \
    windows.rc
