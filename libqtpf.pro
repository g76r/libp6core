QT       -= gui

TARGET = qtpf
TEMPLATE = lib

DEFINES += LIBQTPF_LIBRARY

SOURCES += \
    pfutils.cpp \
    pfparser.cpp \
    pfoptions.cpp \
    pfnode.cpp \
    pfhandler.cpp \
    pfdomhandler.cpp \
    pfcontent.cpp \
    pfarray.cpp \
    pfioutils.cpp

HEADERS +=\
        libqtpf_global.h \
    pfutils.h \
    pfparser.h \
    pfoptions.h \
    pfnode.h \
    pfinternals.h \
    pfhandler.h \
    pfdomhandler.h \
    pfcontent.h \
    pfarray.h \
    pfioutils.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
