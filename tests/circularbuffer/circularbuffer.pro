# Copyright 2015 Hallowyn and others.
# This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
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
CONFIG += console largefile
CONFIG -= app_bundle
INCLUDEPATH += ../..

exists(/usr/bin/ccache):QMAKE_CXX = ccache g++
exists(/usr/bin/ccache):QMAKE_CXXFLAGS += -fdiagnostics-color=always
QMAKE_CXXFLAGS += -Wextra

#unix:debug:QMAKE_CXXFLAGS += -ggdb
#unix {
#  OBJECTS_DIR = ../build-libqtssu-unix/obj
#  RCC_DIR = ../build-libqtssu-unix/rcc
#  MOC_DIR = ../build-libqtssu-unix/moc
#}

SOURCES += test.cpp

HEADERS +=

