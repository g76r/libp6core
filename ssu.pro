# Copyright 2012 Hallowyn and others.
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
QT += network

TARGET = qtssu
TEMPLATE = lib

DEFINES += LIBQTSSU_LIBRARY

SOURCES += \
    httpd/uriprefixhandler.cpp \
    httpd/httpworker.cpp \
    httpd/httpserver.cpp \
    httpd/httpresponse.cpp \
    httpd/httprequest.cpp \
    httpd/httphandler.cpp \
    thread/threadedtaskthread.cpp \
    thread/threadedtask.cpp \
    thread/periodictaskthread.cpp \
    thread/periodictask.cpp \
    thread/blockingtimer.cpp \
    textview/viewscomposerhandler.cpp \
    textview/treehtmlview.cpp \
    textview/textview.cpp \
    textview/asynctextview.cpp


HEADERS +=\
    libqtssu_global.h \
    httpd/uriprefixhandler.h \
    httpd/httpworker.h \
    httpd/httpserver.h \
    httpd/httpresponse.h \
    httpd/httprequest.h \
    httpd/httphandler.h \
    thread/threadedtaskthread.h \
    thread/threadedtask.h \
    thread/periodictaskthread.h \
    thread/periodictask.h \
    thread/blockingtimer.h \
    textview/viewscomposerhandler.h \
    textview/treehtmlview.h \
    textview/textview.h \
    textview/asynctextview.h


unix {
    target.path = /usr/lib
    INSTALLS += target
}
