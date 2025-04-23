/* Copyright 2012-2025 Hallowyn, Gregoire Barbier and others.
 * This file is part of libpumpkin, see <http://libpumpkin.g76r.eu/>.
 * Libpumpkin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libpumpkin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libpumpkin.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "log.h"
#include "multiplexerlogger.h"
#include <QThread>
#include <QDateTime>

namespace p6::log {

static MultiplexerLogger *_rootLogger = nullptr;
static QMutex *_qtHandlerMutex = nullptr;
static QtMessageHandler _qtOriginalHandler = nullptr;

/******************************************************************
  /!\ there must be no global variables with a destructor here /!\
  /!\ because p6::log::log() must not crash when called during /!\
  /!\ the program shutdown                                     /!\
 ******************************************************************/

void addLogger(Logger *logger, bool autoRemovable) {
  if (!_rootLogger)
    return;
  if (logger->thread_model() == Logger::DirectCall)
    logger->moveToThread(_rootLogger->thread());
  _rootLogger->addLogger(logger, autoRemovable);
}

void removeLogger(Logger *logger) {
  if (!_rootLogger)
    return;
  _rootLogger->removeLogger(logger);
}

void addConsoleLogger(
    Severity severity, bool autoRemovable, FILE *stream) {
  if (!_rootLogger)
    return;
  _rootLogger->addConsoleLogger(severity, autoRemovable, stream);
}

void replaceLoggers(Logger *newLogger) {
  if (!_rootLogger)
    return;
  _rootLogger->replaceLoggers(newLogger);
}

void replaceLoggers(QList<Logger*> newLoggers) {
  if (!_rootLogger)
    return;
  _rootLogger->replaceLoggers(newLoggers);
}

void replaceLoggersPlusConsole(
    Severity consoleLoggerSeverity, QList<Logger*> newLoggers) {
  if (!_rootLogger)
    return;
  _rootLogger->replaceLoggersPlusConsole(
        consoleLoggerSeverity, newLoggers);
}

Utf8String Record::current_thread_name() {
  if (QThread *t = QThread::currentThread(); t)
    return t->objectName();
  return {};
}

qint64 Record::now() {
  return QDateTime::currentMSecsSinceEpoch();
}

inline static Utf8String sanitized_message(const Utf8String &input) {
  const char *begin = input.constData(), *s = begin;
  for (; *s; ++s)
    if (*s == '\n')
      [[unlikely]] goto eol_found;
  return input; // short path: copy nothing because there was no \n
eol_found:;
  Utf8String output = input.sliced(0, s-begin);
  for (; *s; ++s)
    if (*s == '\n' && s[1])
      output += "\n "_u8;
    else
      output += *s;
  return output;
}

Utf8String Record::formated_message() const {
  const static QString _timestamp_format = "yyyy-MM-ddThh:mm:ss,zzz ";
  Utf8String ts{QDateTime::fromMSecsSinceEpoch(_timestamp)
        .toString(_timestamp_format)};
  if (ts.isEmpty()) // on late shutdown QDateTime ceases to work
    [[unlikely]] ts = Utf8String::number(_timestamp/1e3, 'f', 3)+" "_u8;
  return ts+_taskid+"/"_u8+_execid+" "_u8+_location+" "_u8
      +severity_as_text(_severity)+" "_u8+sanitized_message(_message)+"\n"_u8;
}

void log(const Record &record) {
  if (!_rootLogger)
    return;
  _rootLogger->log(record);
}

void init() {
  if (_rootLogger)
    return;
  _rootLogger = new MultiplexerLogger(Debug, true);
  _qtHandlerMutex = new QMutex;
}

void shutdown() {
  if (!_rootLogger)
    return;
  auto rl = _rootLogger;
  _rootLogger = nullptr;
  rl->shutdown();
}

Utf8String severity_as_text(Severity severity) {
  switch (severity) {
  case Debug:
    return "DEBUG"_u8;
  case Info:
    return "INFO"_u8;
  case Warning:
    return "WARNING"_u8;
  case Error:
    return "ERROR"_u8;
  case Fatal:
    return "FATAL"_u8;
  }
  return "UNKNOWN"_u8;
}

inline static Severity severity_from_qttype(QtMsgType type) {
  switch (type) {
    case QtInfoMsg:
      return Info;
    case QtWarningMsg:
      return Warning;
    case QtCriticalMsg:
      return Error;
    case QtFatalMsg:
      return Fatal;
    case QtDebugMsg:
      ;
  }
  return Debug;
}

QString pathToLastFullestLog() {
  if (!_rootLogger)
    return {};
  return _rootLogger->pathToLastFullestLog();
}

QStringList pathsToFullestLogs() {
  if (!_rootLogger)
    return {};
  return _rootLogger->pathsToFullestLogs();
}

QStringList pathsToAllLogs() {
  if (!_rootLogger)
    return {};
  return _rootLogger->pathsToAllLogs();
}

static void qtLogSamePatternWrapper(
    QtMsgType type, const QMessageLogContext &qtcontext, const QString &msg) {
  auto severity = severity_from_qttype(type);
  Utf8String location;
  if (qtcontext.file)
    [[likely]] location = qtcontext.file+":"_u8
                          +Utf8String::number(qtcontext.line);
  fputs(Record(severity, {}, {}, location).set_message(msg)
        .formated_message(), stderr);
  if (type == QtFatalMsg)
    [[unlikely]] abort();
}

void wrapQtLogToSamePattern(bool enable) {
  if (!_rootLogger)
    return;
  QMutexLocker ml(_qtHandlerMutex);
  if (enable) {
    QtMessageHandler previous = qInstallMessageHandler(qtLogSamePatternWrapper);
    if (!_qtOriginalHandler)
      _qtOriginalHandler = previous;
  } else if (_qtOriginalHandler) {
    qInstallMessageHandler(_qtOriginalHandler);
    _qtOriginalHandler = 0;
  }
}

} // ns p6::log
