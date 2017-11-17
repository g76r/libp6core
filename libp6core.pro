# Copyright 2012-2017 Hallowyn, Gregoire Barbier and others.
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
QT += network sql
CONFIG += largefile c++11

TARGET = p6core
TEMPLATE = lib

TARGET_OS=default
unix: TARGET_OS=unix
linux: TARGET_OS=linux
android: TARGET_OS=android
macx: TARGET_OS=macx
win32: TARGET_OS=win32
BUILD_TYPE=unknown
CONFIG(debug,debug|release): BUILD_TYPE=debug
CONFIG(release,debug|release): BUILD_TYPE=release

DEFINES += LIBPUMPKIN_LIBRARY

exists(/usr/bin/ccache):QMAKE_CXX = ccache $$QMAKE_CXX
exists(/usr/bin/ccache):QMAKE_CXXFLAGS += -fdiagnostics-color=always
QMAKE_CXXFLAGS += -Wextra -Woverloaded-virtual
CONFIG(debug,debug|release):QMAKE_CXXFLAGS += -ggdb

OBJECTS_DIR = ../build-$$TARGET-$$TARGET_OS/$$BUILD_TYPE/obj
RCC_DIR = ../build-$$TARGET-$$TARGET_OS/$$BUILD_TYPE/rcc
MOC_DIR = ../build-$$TARGET-$$TARGET_OS/$$BUILD_TYPE/moc
DESTDIR = ../build-$$TARGET-$$TARGET_OS/$$BUILD_TYPE
#QMAKE_CXXFLAGS += -O0 -pg -fprofile-arcs -ftest-coverage
#QMAKE_LFLAGS += -pg -fprofile-arcs

# dependency libs
INCLUDEPATH += ../libqtpf
LIBS += -L../build-qtpf-$$TARGET_OS/$$BUILD_TYPE
LIBS += -lqtpf

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
    format/timeformats.cpp \
    httpd/pipelinehttphandler.cpp \
    httpd/basicauthhttphandler.cpp \
    auth/authenticator.cpp \
    auth/inmemoryauthenticator.cpp \
    auth/usersdatabase.cpp \
    auth/inmemoryusersdatabase.cpp \
    auth/authorizer.cpp \
    auth/inmemoryrulesauthorizer.cpp \
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
    sql/hidedeletedsqlrowsproxymodel.cpp \
    util/coreundocommand.cpp \
    modelview/shareduiitemdocumenttransaction.cpp \
    modelview/shareduiitemsmatrixmodel.cpp \
    modelview/columnstorolenamesproxymodel.cpp \
    ftp/ftpclient.cpp \
    ftp/ftpscript.cpp \
    format/stringutils.cpp \
    io/dummysocket.cpp \
    io/readonlyresourcescache.cpp \
    format/csvformatter.cpp \
    format/abstracttextformatter.cpp \
    format/htmltableformatter.cpp \
    message/tcplistener.cpp \
    message/sessionmanager.cpp \
    message/session.cpp \
    message/message.cpp \
    message/messagesender.cpp \
    message/tcpconnectionhandler.cpp \
    message/tcpclient.cpp \
    message/incomingmessagedispatcher.cpp \
    message/outgoingmessagedispatcher.cpp \
    ostore/objectsstore.cpp \
    ostore/objectslistmodel.cpp \
    ostore/sqlobjectsstore.cpp \
    io/directorywatcher.cpp \
    modelview/stringhashmodel.cpp \
    format/jsonformats.cpp

HEADERS +=\
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
    format/timeformats.h \
    httpd/pipelinehttphandler.h \
    httpd/basicauthhttphandler.h \
    auth/authenticator.h \
    auth/inmemoryauthenticator.h \
    auth/usersdatabase.h \
    auth/inmemoryusersdatabase.h \
    auth/usersdatabase_spi.h \
    auth/authorizer.h \
    auth/inmemoryrulesauthorizer.h \
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
    sql/hidedeletedsqlrowsproxymodel.h \
    modelview/inmemoryshareduiitemdocumentmanager.h \
    util/coreundocommand.h \
    modelview/shareduiitemdocumenttransaction.h \
    modelview/shareduiitemsmatrixmodel.h \
    modelview/columnstorolenamesproxymodel.h \
    ftp/ftpclient.h \
    ftp/ftpscript.h \
    format/stringutils.h \
    io/dummysocket.h \
    util/containerutils.h \
    util/radixtree.h \
    io/readonlyresourcescache.h \
    format/csvformatter.h \
    format/abstracttextformatter.h \
    format/htmltableformatter.h \
    libp6core_global.h \
    message/tcplistener.h \
    message/sessionmanager.h \
    message/session.h \
    message/message.h \
    message/messagesender.h \
    message/tcpconnectionhandler.h \
    message/tcpclient.h \
    message/incomingmessagedispatcher.h \
    message/outgoingmessagedispatcher.h \
    ostore/objectsstore.h \
    ostore/objectslistmodel.h \
    ostore/sqlobjectsstore.h \
    io/directorywatcher.h \
    modelview/stringhashmodel.h \ 
    format/jsonformats.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
