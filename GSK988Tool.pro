QT += core gui network widgets

TARGET = GSK988Tool
TEMPLATE = app
CONFIG += c++14
INCLUDEPATH += src

msvc: QMAKE_CXXFLAGS += /utf-8
msvc: QMAKE_CFLAGS += /utf-8

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/network/tcpclient.cpp \
    src/network/mockserver.cpp \
    src/protocol/gsk988protocol.cpp \
    src/protocol/framebuilder.cpp \
    src/ui/realtimewidget.cpp \
    src/ui/commandwidget.cpp \
    src/ui/parsewidget.cpp \
    src/ui/logwidget.cpp

HEADERS += \
    src/mainwindow.h \
    src/network/tcpclient.h \
    src/network/mockserver.h \
    src/protocol/gsk988protocol.h \
    src/protocol/framebuilder.h \
    src/ui/realtimewidget.h \
    src/ui/commandwidget.h \
    src/ui/parsewidget.h \
    src/ui/logwidget.h
