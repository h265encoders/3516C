QT += core
QT -= gui

TARGET = WifiCtrl
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app
chip = HI3516C
include(../../../LinkLib/Link.pri)
include(../../libmaia-master/maia.pri)
DESTDIR +=../bin

SOURCES += main.cpp \
    WifiCtrl.cpp

HEADERS += \
    WifiCtrl.h

