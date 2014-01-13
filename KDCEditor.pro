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
win32:RC_FILE = src/windows.rc

SOURCES += src/main.cpp\
    src/mainwindow.cpp \
    src/tileeditwindow.cpp \
    src/compress.c \
    src/level.cpp \
    src/kirby.cpp \
    src/mapscene.cpp \
    src/previewwindow.cpp \
    src/propertieswindow.cpp \
    src/romfile.cpp \
    src/metatile.cpp \
    src/metatile_terrain.cpp \
    src/metatile_borders.cpp \
    src/metatile_obstacles.cpp \
    src/coursewindow.cpp \
    src/previewscene.cpp \
    src/mapchange.cpp

HEADERS  += src/mainwindow.h \
    src/tileeditwindow.h \
    src/compress.h \
    src/level.h \
    src/kirby.h \
    src/mapscene.h \
    src/graphics.h \
    src/previewwindow.h \
    src/propertieswindow.h \
    src/romfile.h \
    src/metatile.h \
    src/coursewindow.h \
    src/version.h \
    src/previewscene.h \
    src/mapchange.h

FORMS    += src/mainwindow.ui \
    src/tileeditwindow.ui \
    src/previewwindow.ui \
    src/propertieswindow.ui \
    src/coursewindow.ui

RESOURCES += \
    src/images.qrc \
    src/icons.qrc

OTHER_FILES += \
    CHANGES.txt \
    README.txt \
    src/TODO.txt \
    src/coursefiles.txt \
    src/windows.rc
