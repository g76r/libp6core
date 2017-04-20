# Copyright 2015-2017 Hallowyn, Gregoire Barbier and others.
# This file is part of libpumpkin, see <http://libpumpkin.g76r.eu/>.
# Libpumpkin is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# Libpumpkin is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
# You should have received a copy of the GNU Affero General Public License
# along with libpumpkin.  If not, see <http://www.gnu.org/licenses/>.

QT -= gui
QT += core

TARGET = test
CONFIG += console largefile
CONFIG -= app_bundle
INCLUDEPATH += ../..

exists(/usr/bin/ccache):QMAKE_CXX = ccache g++
exists(/usr/bin/ccache):QMAKE_CXXFLAGS += -fdiagnostics-color=always
QMAKE_CXXFLAGS += -Wextra

SOURCES += test.cpp

HEADERS +=

