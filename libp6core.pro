# Copyright 2012-2024 Hallowyn, Gregoire Barbier and others.
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
QT *= network sql
CONFIG *= largefile c++17 c++20 precompile_header force_debug_info

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

DEFINES += LIBP6CORE_LIBRARY
exists(/usr/bin/ccache):QMAKE_CXX = \
  CCACHE_SLOPPINESS=pch_defines,time_macros ccache $$QMAKE_CXX
exists(/usr/bin/ccache):QMAKE_CXXFLAGS += -fdiagnostics-color=always
QMAKE_CXXFLAGS += -Wextra -Woverloaded-virtual \
  -Wdouble-promotion -Wimplicit-fallthrough=5 -Wtrampolines \
  -Wduplicated-branches -Wduplicated-cond -Wlogical-op \
  -Wno-padded -Wno-deprecated-copy -Wsuggest-attribute=noreturn \
  -Wsuggest-override -DQT_NO_JAVA_STYLE_ITERATORS -DQT_NO_FOREACH
# LATER add -Wfloat-equal again when QVariant::value<double>() won't trigger it
QMAKE_CXXFLAGS_DEBUG += -ggdb
QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO += -ggdb
!isEmpty(OPTIMIZE_LEVEL):QMAKE_CXXFLAGS_DEBUG += -O$$OPTIMIZE_LEVEL
!isEmpty(OPTIMIZE_LEVEL):QMAKE_CXXFLAGS_RELEASE += -O$$OPTIMIZE_LEVEL
!isEmpty(OPTIMIZE_LEVEL):QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO += -O$$OPTIMIZE_LEVEL

OBJECTS_DIR = ../build-$$TARGET-$$TARGET_OS/$$BUILD_TYPE/obj
RCC_DIR = ../build-$$TARGET-$$TARGET_OS/$$BUILD_TYPE/rcc
MOC_DIR = ../build-$$TARGET-$$TARGET_OS/$$BUILD_TYPE/moc
DESTDIR = ../build-$$TARGET-$$TARGET_OS/$$BUILD_TYPE
#QMAKE_CXXFLAGS += -O0 -pg -fprofile-arcs -ftest-coverage
#QMAKE_LFLAGS += -pg -fprofile-arcs

unix {
    unicodedata.target = util/unicodedata.cpp
    unicodedata.depends = util/build_unicodedata.sh util/UnicodeData.txt
    unicodedata.commands = util/build_unicodedata.sh
    PRE_TARGETDEPS += util/unicodedata.cpp
    QMAKE_EXTRA_TARGETS += unicodedata
}

PRECOMPILED_HEADER *= \
    libp6core_stable.h

SOURCES *= \
    eg/entity.cpp \
    format/graphvizparser.cpp \
    format/graphvizrenderer.cpp \
    format/svgwriter.cpp \
    io/opensshcommand.cpp \
    log/logrecorditemlogger.cpp \
    log/logrecorditemmodel.cpp \
    pf/pfnode.cpp \
    pf/pfparser.cpp \
    util/paramsformula.cpp \
    util/percentevaluator.cpp \
    util/typedvalue.cpp \
    util/typedvaluelist.cpp \
    util/utf8string.cpp \
    util/utf8stringlist.cpp \
    util/paramsprovider.cpp \
    util/paramset.cpp \
    format/xlsxwriter.cpp \
    log/logger.cpp \
    log/log.cpp \
    log/filelogger.cpp \
    io/unixsignalmanager.cpp \
    mail/mailaddress.cpp \
    httpd/httpworker.cpp \
    httpd/httpserver.cpp \
    httpd/httpresponse.cpp \
    httpd/httprequest.cpp \
    httpd/httphandler.cpp \
    sql/sqlutils.cpp \
    thread/blockingtimer.cpp \
    textview/htmltableview.cpp \
    textview/textview.cpp \
    httpd/filesystemhttphandler.cpp \
    io/ioutils.cpp \
    httpd/templatinghttphandler.cpp \
    mail/mailsender.cpp \
    textview/clockview.cpp \
    util/mathutils.cpp \
    modelview/paramsetmodel.cpp \
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
    modelview/shareduiitemstreemodel.cpp \
    util/relativedatetime.cpp \
    log/loggerthread.cpp \
    log/multiplexerlogger.cpp \
    httpd/uploadhttphandler.cpp \
    csv/csvfile.cpp \
    csv/csvfilemodel.cpp \
    modelview/shareduiitemdocumentmanager.cpp \
    modelview/shareduiitemlist.cpp \
    util/paramsprovidermerger.cpp \
    modelview/shareduiitemslogmodel.cpp \
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
    format/jsonformats.cpp \
    modelview/stringlistdiffmodel.cpp

HEADERS *=\
    eg/entity.h \
    format/graphvizparser.h \
    format/graphvizrenderer.h \
    format/svgwriter.h \
    io/opensshcommand.h \
    log/log_p.h \
    log/logrecorditemlogger.h \
    log/logrecorditemmodel.h \
    modelview/templatedshareduiitemdata.h \
    pf/pfnode.h \
    pf/pfoptions.h \
    pf/pfparser.h \
    util/datacache.h \
    util/numberutils.h \
    util/paramsformula.h \
    util/percentevaluator.h \
    util/typedvalue.h \
    util/typedvaluelist.h \
    util/utf8string.h \
    util/utf8stringlist.h \
    util/utf8stringset.h \
    util/paramsprovider.h \
    util/paramset.h \
    util/paramsprovidermerger.h \
    util/regexpparamsprovider.h \
    format/xlsxwriter.h \
    log/logger.h \
    log/log.h \
    log/filelogger.h \
    log/loggerthread.h \
    log/multiplexerlogger.h \
    modelview/shareduiitem.h \
    io/unixsignalmanager.h \
    mail/mailaddress.h \
    httpd/httpworker.h \
    httpd/httpserver.h \
    httpd/httpresponse.h \
    httpd/httprequest.h \
    httpd/httphandler.h \
    sql/sqlutils.h \
    thread/blockingtimer.h \
    textview/htmltableview.h \
    textview/textview.h \
    httpd/filesystemhttphandler.h \
    io/ioutils.h \
    httpd/templatinghttphandler.h \
    mail/mailsender.h \
    textview/clockview.h \
    util/mathutils.h \
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
    modelview/shareduiitemstablemodel.h \
    modelview/shareduiitemsmodel.h \
    modelview/shareduiitemstreemodel.h \
    util/relativedatetime.h \
    httpd/uploadhttphandler.h \
    csv/csvfile.h \
    csv/csvfilemodel.h \
    modelview/shareduiitemdocumentmanager.h \
    modelview/shareduiitemlist.h \
    thread/atomicvalue.h \
    thread/circularbuffer.h \
    modelview/shareduiitemslogmodel.h \
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
    modelview/stringhashmodel.h \  \
    format/jsonformats.h \
    modelview/paramsetmodel.h \
    modelview/stringlistdiffmodel.h \
    util/utf8utils.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES *= \
    util/percent_evaluation.md
