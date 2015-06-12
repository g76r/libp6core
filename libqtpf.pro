# Copyright 2012-2015 Hallowyn and others.
# See the NOTICE file distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file to you under
# the Apache License, Version 2.0 (the "License"); you may not use this
# file except in compliance with the License. You may obtain a copy of the
# License at http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.

QT -= gui

TARGET = qtpf
TEMPLATE = lib

DEFINES += LIBQTPF_LIBRARY

exists(/usr/bin/ccache):QMAKE_CXX = ccache g++
QMAKE_CXXFLAGS += -Wextra
unix:debug:QMAKE_CXXFLAGS += -ggdb
unix {
  OBJECTS_DIR = build-libqtpf-unix/obj
  RCC_DIR = build-libqtpf-unix/rcc
  MOC_DIR = build-libqtpf-unix/moc
}

SOURCES += \
    pf/pfutils.cpp \
    pf/pfparser.cpp \
    pf/pfoptions.cpp \
    pf/pfnode.cpp \
    pf/pfhandler.cpp \
    pf/pfdomhandler.cpp \
    pf/pfarray.cpp

HEADERS +=\
    pf/libqtpf_global.h \
    pf/pfutils.h \
    pf/pfparser.h \
    pf/pfoptions.h \
    pf/pfnode.h \
    pf/pfinternals.h \
    pf/pfhandler.h \
    pf/pfdomhandler.h \
    pf/pfarray.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
