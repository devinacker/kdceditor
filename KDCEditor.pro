QT       += core widgets

QMAKE_CFLAGS += -std=c99
QMAKE_CXXFLAGS += -std=c++11

TARGET = KDCEditor
TEMPLATE = app
CONFIG += c++11

CONFIG(debug, debug|release) {
    DESTDIR = debug
}
CONFIG(release, debug|release) {
    DESTDIR = release
}

OBJECTS_DIR = obj/$$DESTDIR
MOC_DIR = $$OBJECTS_DIR
RCC_DIR = $$OBJECTS_DIR

# copy docs and samples on build
copydata.commands += \
    $(COPY_DIR) \"$$PWD/docs\" $$DESTDIR &&\
    $(COPY_FILE) \"$$PWD/CHANGES.txt\" $$DESTDIR &&\
    $(COPY_FILE) \"$$PWD/COPYING.txt\" $$DESTDIR &&\
    $(COPY_DIR) \"$$PWD/samples\" $$DESTDIR

first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata

# OS-specific metadata and stuff
win32:RC_FILE = src/windows.rc
macx:ICON = src/images/main.icns

# build on OS X with xcode/clang and libc++
macx:QMAKE_CXXFLAGS += -stdlib=libc++

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
    README.md \
    src/TODO.txt \
    src/coursefiles.txt \
    src/windows.rc
