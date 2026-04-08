QT += core gui widgets network

CONFIG += c++17

TARGET = GSK988Protocol
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/network/tcpclient.cpp \
    src/network/mockserver.cpp \
    src/protocol/gsk988protocol.cpp \
    src/protocol/framebuilder.cpp \
    src/protocol/crc16.cpp \
    src/ui/realtimewidget.cpp \
    src/ui/commandtablewidget.cpp \
    src/ui/dataparsewidget.cpp \
    src/ui/logwidget.cpp \
    src/ui/paramwidget.cpp \
    src/core/commandregistry.cpp \
    src/core/mockconfig.cpp

HEADERS += \
    src/mainwindow.h \
    src/network/tcpclient.h \
    src/network/mockserver.h \
    src/protocol/gsk988protocol.h \
    src/protocol/framebuilder.h \
    src/protocol/crc16.h \
    src/ui/realtimewidget.h \
    src/ui/commandtablewidget.h \
    src/ui/dataparsewidget.h \
    src/ui/logwidget.h \
    src/ui/paramwidget.h \
    src/core/commandregistry.h \
    src/core/mockconfig.h

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
