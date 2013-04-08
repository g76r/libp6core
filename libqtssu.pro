# Copyright 2012 Hallowyn and others.
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
QT += network

TARGET = qtssu
TEMPLATE = lib

DEFINES += LIBQTSSU_LIBRARY

QMAKE_CXXFLAGS += -Wextra
unix {
  OBJECTS_DIR = ../ssu-build-unix/obj
  RCC_DIR = ../ssu-build-unix/rcc
  MOC_DIR = ../ssu-build-unix/moc
}

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
    textview/htmltableview.cpp \
    textview/textview.cpp \
    textview/asynctextview.cpp \
    textview/htmllistview.cpp \
    httpd/filesystemhttphandler.cpp \
    util/ioutils.cpp \
    httpd/templatinghttphandler.cpp \
    textview/htmlsetview.cpp \
    mail/mailsender.cpp \
    util/timerwitharguments.cpp \
    textview/clockview.cpp \
    util/paramset.cpp \
    log/memorylogger.cpp \
    log/logmodel.cpp \
    log/logger.cpp \
    log/log.cpp \
    log/filelogger.cpp \
    util/paramsprovider.cpp \
    util/paramsetmodel.cpp \
    textview/htmltreeview.cpp \
    textview/csvtreeview.cpp \
    textview/csvtableview.cpp \
    textview/texttableview.cpp \
    log/qterrorcodes.cpp


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
    textview/htmltableview.h \
    textview/textview.h \
    textview/asynctextview.h \
    textview/htmllistview.h \
    httpd/filesystemhttphandler.h \
    util/ioutils.h \
    httpd/templatinghttphandler.h \
    textview/htmlsetview.h \
    mail/mailsender.h \
    util/timerwitharguments.h \
    textview/clockview.h \
    util/paramset.h \
    log/memorylogger.h \
    log/logmodel.h \
    log/logger.h \
    log/log.h \
    log/filelogger.h \
    util/paramsprovider.h \
    util/paramsetmodel.h \
    textview/htmltreeview.h \
    textview/csvtreeview.h \
    textview/csvtableview.h \
    textview/texttableview.h \
    httpd/httpcommon.h \
    log/qterrorcodes.h


unix {
    target.path = /usr/lib
    INSTALLS += target
}
