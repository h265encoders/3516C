QT += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Encoder

TEMPLATE = app

chip = HI3516C
include(../../../LinkLib/Link.pri)
include(../../libmaia-master/maia.pri)
DESTDIR +=../bin

LIBS +=-lLinkUI

SOURCES += main.cpp \
    Worker.cpp \
    OLED.cpp

HEADERS += \
    Worker.h \
    OLED.h

FORMS += \
    OLED.ui

RESOURCES += \
    res.qrc

