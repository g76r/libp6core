# Copyright 2016 Hallowyn and others.
# This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
# Libqtssu is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# Libqtssu is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
# You should have received a copy of the GNU Affero General Public License
# along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.

QT -= gui
QT += core

TARGET = test
CONFIG += console largefile c++11
CONFIG -= app_bundle
INCLUDEPATH += ../..
win32:CONFIG(debug,debug|release):LIBS += \
  -L../../../build-libqtssu-windows/debug
win32:CONFIG(release,debug|release):LIBS += \
  -L../../../build-libqtssu-windows/release
unix:LIBS += -L../..
LIBS += -lqtssu

exists(/usr/bin/ccache):QMAKE_CXX = ccache g++
exists(/usr/bin/ccache):QMAKE_CXXFLAGS += -fdiagnostics-color=always
QMAKE_CXXFLAGS += -Wextra

SOURCES += test.cpp

HEADERS +=

