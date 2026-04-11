QT += core gui network widgets serialport

TARGET = GSK988Tool
TEMPLATE = app
CONFIG += c++14
INCLUDEPATH += src

msvc: QMAKE_CXXFLAGS += /utf-8
msvc: QMAKE_CFLAGS += /utf-8

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/transport/tcptransport.cpp \
    src/transport/serialtransport.cpp \
    src/network/mockserver.cpp \
    src/protocol/gsk988/gsk988protocol.cpp \
    src/protocol/gsk988/gsk988framebuilder.cpp \
    src/protocol/gsk988/gsk988widgetfactory.cpp \
    src/protocol/gsk988/gsk988realtimewidget.cpp \
    src/protocol/gsk988/gsk988commandwidget.cpp \
    src/protocol/gsk988/gsk988parsewidget.cpp \
    src/protocol/modbus/modbusprotocol.cpp \
    src/protocol/modbus/modbusframebuilder.cpp \
    src/protocol/modbus/modbuswidgetfactory.cpp \
    src/protocol/modbus/modbusrealtimewidget.cpp \
    src/protocol/modbus/modbuscommandwidget.cpp \
    src/protocol/modbus/modbusparsewidget.cpp \
    src/ui/logwidget.cpp

HEADERS += \
    src/mainwindow.h \
    src/transport/itransport.h \
    src/transport/tcptransport.h \
    src/transport/serialtransport.h \
    src/network/mockserver.h \
    src/protocol/iprotocol.h \
    src/protocol/gsk988/gsk988protocol.h \
    src/protocol/gsk988/gsk988framebuilder.h \
    src/protocol/gsk988/gsk988widgetfactory.h \
    src/protocol/gsk988/gsk988realtimewidget.h \
    src/protocol/gsk988/gsk988commandwidget.h \
    src/protocol/gsk988/gsk988parsewidget.h \
    src/protocol/modbus/modbusprotocol.h \
    src/protocol/modbus/modbusframebuilder.h \
    src/protocol/modbus/modbuswidgetfactory.h \
    src/protocol/modbus/modbusrealtimewidget.h \
    src/protocol/modbus/modbuscommandwidget.h \
    src/protocol/modbus/modbusparsewidget.h \
    src/ui/iprotocolwidgetfactory.h \
    src/ui/logwidget.h
