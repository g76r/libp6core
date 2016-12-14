# Copyright 2012-2015 Hallowyn and others.
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
QT += network sql
CONFIG += largefile c++11

TARGET = qtssu
TEMPLATE = lib

DEFINES += LIBQTSSU_LIBRARY

exists(/usr/bin/ccache):QMAKE_CXX = ccache g++
exists(/usr/bin/ccache):QMAKE_CXXFLAGS += -fdiagnostics-color=always
QMAKE_CXXFLAGS += -Wextra -Woverloaded-virtual
unix:CONFIG(debug,debug|release):QMAKE_CXXFLAGS += -ggdb
unix {
  OBJECTS_DIR = ../build-libqtssu-unix/obj
  RCC_DIR = ../build-libqtssu-unix/rcc
  MOC_DIR = ../build-libqtssu-unix/moc
  #QMAKE_CXXFLAGS += -O0 -pg -fprofile-arcs -ftest-coverage
  #QMAKE_LFLAGS += -pg -fprofile-arcs
}

SOURCES += \
    httpd/httpworker.cpp \
    httpd/httpserver.cpp \
    httpd/httpresponse.cpp \
    httpd/httprequest.cpp \
    httpd/httphandler.cpp \
    thread/blockingtimer.cpp \
    textview/htmltableview.cpp \
    textview/textview.cpp \
    httpd/filesystemhttphandler.cpp \
    util/ioutils.cpp \
    httpd/templatinghttphandler.cpp \
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
    textview/csvtableview.cpp \
    textview/texttableview.cpp \
    log/qterrorcodes.cpp \
    modelview/textmatrixmodel.cpp \
    util/timeformats.cpp \
    httpd/pipelinehttphandler.cpp \
    httpd/basicauthhttphandler.cpp \
    auth/authenticator.cpp \
    auth/inmemoryauthenticator.cpp \
    auth/usersdatabase.cpp \
    auth/inmemoryusersdatabase.cpp \
    auth/authorizer.cpp \
    auth/inmemoryrulesauthorizer.cpp \
    util/htmlutils.cpp \
    httpd/imagehttphandler.cpp \
    httpd/graphvizimagehttphandler.cpp \
    textview/htmlitemdelegate.cpp \
    textview/textviewitemdelegate.cpp \
    modelview/shareduiitem.cpp \
    modelview/shareduiitemstablemodel.cpp \
    modelview/shareduiitemsmodel.cpp \
    log/qtloglogger.cpp \
    modelview/shareduiitemstreemodel.cpp \
    util/relativedatetime.cpp \
    log/loggerthread.cpp \
    log/multiplexerlogger.cpp \
    httpd/uploadhttphandler.cpp \
    csv/csvfile.cpp \
    csv/csvfilemodel.cpp \
    modelview/shareduiitemdocumentmanager.cpp \
    modelview/shareduiitemlist.cpp \
    util/characterseparatedexpression.cpp \
    util/paramsprovidermerger.cpp \
    modelview/shareduiitemslogmodel.cpp \
    util/stringsparamsprovider.cpp \
    util/regexpparamsprovider.cpp \
    modelview/genericshareduiitem.cpp \
    modelview/inmemoryshareduiitemdocumentmanager.cpp \
    sql/inmemorydatabasedocumentmanager.cpp \
    util/coreundocommand.cpp \
    modelview/shareduiitemdocumenttransaction.cpp \
    modelview/shareduiitemsmatrixmodel.cpp \
    ftp/ftpclient.cpp \
    ftp/ftpscript.cpp \
    util/stringutils.cpp \
    net/dummysocket.cpp \
    net/readonlyresourcescache.cpp


HEADERS +=\
    libqtssu_global.h \
    httpd/httpworker.h \
    httpd/httpserver.h \
    httpd/httpresponse.h \
    httpd/httprequest.h \
    httpd/httphandler.h \
    thread/blockingtimer.h \
    textview/htmltableview.h \
    textview/textview.h \
    httpd/filesystemhttphandler.h \
    util/ioutils.h \
    httpd/templatinghttphandler.h \
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
    textview/csvtableview.h \
    textview/texttableview.h \
    httpd/httpcommon.h \
    log/qterrorcodes.h \
    modelview/textmatrixmodel.h \
    util/timeformats.h \
    httpd/pipelinehttphandler.h \
    httpd/basicauthhttphandler.h \
    auth/authenticator.h \
    auth/inmemoryauthenticator.h \
    auth/usersdatabase.h \
    auth/inmemoryusersdatabase.h \
    auth/usersdatabase_spi.h \
    auth/authorizer.h \
    auth/inmemoryrulesauthorizer.h \
    util/htmlutils.h \
    httpd/imagehttphandler.h \
    httpd/graphvizimagehttphandler.h \
    textview/htmlitemdelegate.h \
    textview/textviewitemdelegate.h \
    modelview/shareduiitem.h \
    modelview/shareduiitemstablemodel.h \
    modelview/shareduiitemsmodel.h \
    log/qtloglogger.h \
    modelview/shareduiitemstreemodel.h \
    util/relativedatetime.h \
    log/loggerthread.h \
    log/multiplexerlogger.h \
    httpd/uploadhttphandler.h \
    csv/csvfile.h \
    csv/csvfilemodel.h \
    modelview/shareduiitemdocumentmanager.h \
    modelview/shareduiitemlist.h \
    util/characterseparatedexpression.h \
    util/paramsprovidermerger.h \
    thread/atomicvalue.h \
    thread/circularbuffer.h \
    modelview/shareduiitemslogmodel.h \
    util/stringsparamsprovider.h \
    util/regexpparamsprovider.h \
    modelview/genericshareduiitem.h \
    sql/inmemorydatabasedocumentmanager.h \
    modelview/inmemoryshareduiitemdocumentmanager.h \
    util/coreundocommand.h \
    modelview/shareduiitemdocumenttransaction.h \
    modelview/shareduiitemsmatrixmodel.h \
    ftp/ftpclient.h \
    ftp/ftpscript.h \
    util/stringutils.h \
    net/dummysocket.h \
    util/containerutils.h \
    util/radixtree.h \
    net/readonlyresourcescache.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
