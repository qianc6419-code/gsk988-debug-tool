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
    src/protocol/fanuc/fanucprotocol.cpp \
    src/protocol/fanuc/fanucframebuilder.cpp \
    src/protocol/fanuc/fanucwidgetfactory.cpp \
    src/protocol/fanuc/fanucrealtimewidget.cpp \
    src/protocol/fanuc/fanuccommandwidget.cpp \
    src/protocol/fanuc/fanucparsewidget.cpp \
    src/protocol/siemens/siemensframebuilder.cpp \
    src/protocol/siemens/siemensprotocol.cpp \
    src/protocol/siemens/siemenswidgetfactory.cpp \
    src/protocol/siemens/siemensrealtimewidget.cpp \
    src/protocol/siemens/siemenscommandwidget.cpp \
    src/protocol/siemens/siemensparsewidget.cpp \
    src/protocol/mazak/mazakframebuilder.cpp \
    src/protocol/mazak/mazakdllwrapper.cpp \
    src/protocol/mazak/mazakprotocol.cpp \
    src/protocol/mazak/mazakwidgetfactory.cpp \
    src/protocol/mazak/mazakrealtimewidget.cpp \
    src/protocol/mazak/mazakcommandwidget.cpp \
    src/protocol/mazak/mazakparsewidget.cpp \
    src/protocol/knd/kndframebuilder.cpp \
    src/protocol/knd/kndrestclient.cpp \
    src/protocol/knd/kndprotocol.cpp \
    src/protocol/knd/kndwidgetfactory.cpp \
    src/protocol/knd/kndrealtimewidget.cpp \
    src/protocol/knd/kndcommandwidget.cpp \
    src/protocol/knd/kndparsewidget.cpp \
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
    src/protocol/fanuc/fanucprotocol.h \
    src/protocol/fanuc/fanucframebuilder.h \
    src/protocol/fanuc/fanucwidgetfactory.h \
    src/protocol/fanuc/fanucrealtimewidget.h \
    src/protocol/fanuc/fanuccommandwidget.h \
    src/protocol/fanuc/fanucparsewidget.h \
    src/protocol/siemens/siemensframebuilder.h \
    src/protocol/siemens/siemensprotocol.h \
    src/protocol/siemens/siemenswidgetfactory.h \
    src/protocol/siemens/siemensrealtimewidget.h \
    src/protocol/siemens/siemenscommandwidget.h \
    src/protocol/siemens/siemensparsewidget.h \
    src/protocol/mazak/mazakframebuilder.h \
    src/protocol/mazak/mazakdllwrapper.h \
    src/protocol/mazak/mazakprotocol.h \
    src/protocol/mazak/mazakwidgetfactory.h \
    src/protocol/mazak/mazakrealtimewidget.h \
    src/protocol/mazak/mazakcommandwidget.h \
    src/protocol/mazak/mazakparsewidget.h \
    src/protocol/knd/kndframebuilder.h \
    src/protocol/knd/kndrestclient.h \
    src/protocol/knd/kndprotocol.h \
    src/protocol/knd/kndwidgetfactory.h \
    src/protocol/knd/kndrealtimewidget.h \
    src/protocol/knd/kndcommandwidget.h \
    src/protocol/knd/kndparsewidget.h \
    src/ui/iprotocolwidgetfactory.h \
    src/ui/logwidget.h
