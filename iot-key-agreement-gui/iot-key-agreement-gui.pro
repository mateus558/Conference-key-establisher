#-------------------------------------------------
#
# Project created by QtCreator 2020-03-23T14:15:57
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = iot-key-agreement-gui
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    paramsgui.cpp

HEADERS += \
        mainwindow.h \
    paramsgui.h

FORMS += \
        mainwindow.ui \
    paramsgui.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../iot-key-agreement-core/release/ -liot-key-agreement-core
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../iot-key-agreement-core/debug/ -liot-key-agreement-core
else:unix: LIBS += -L$$OUT_PWD/../iot-key-agreement-core/ -liot-key-agreement-core
unix:LIBS+= -L/usr/lib -lgmp -lgmpxx

INCLUDEPATH += $$PWD/../iot-key-agreement-core
DEPENDPATH += $$PWD/../iot-key-agreement-core
