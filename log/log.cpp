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
#include <unistd.h>

namespace p6::log {

static MultiplexerLogger *_rootLogger = nullptr;
static QMutex *_qt_handler_mutex = nullptr;
static QtMessageHandler _qt_original_handler = nullptr;

/******************************************************************
  /!\ there must be no global variables with a destructor here /!\
  /!\ because p6::log::log() must not crash when called during /!\
  /!\ the program shutdown                                     /!\
 ******************************************************************/

void add_logger(Logger *logger, bool auto_removable) {
  if (!_rootLogger)
    return;
  if (logger->thread_model() == Logger::DirectCall)
    logger->moveToThread(_rootLogger->thread());
  _rootLogger->addLogger(logger, auto_removable);
}

void remove_logger(Logger *logger) {
  if (!_rootLogger)
    return;
  _rootLogger->removeLogger(logger);
}

void add_console_logger(Severity severity, bool auto_removable, FILE *stream) {
  if (!_rootLogger)
    return;
  _rootLogger->addConsoleLogger(severity, auto_removable, stream);
}

void replace_loggers(
    QList<Logger*> &new_loggers, bool prepend_console,
    Severity console_min_severity) {
  if (!_rootLogger)
    return;
  _rootLogger->replace_loggers(new_loggers, prepend_console,
                               console_min_severity);
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
  _qt_handler_mutex = new QMutex;
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

static void same_format_qt_log_handler(
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

static void p6_log_wrapper_qt_log_handler(
    QtMsgType type, const QMessageLogContext &qtcontext, const QString &msg) {
  auto severity = severity_from_qttype(type);
  Utf8String location;
  if (qtcontext.file)
    [[likely]] location = qtcontext.file+":"_u8
                          +Utf8String::number(qtcontext.line);
  log(Record(severity, {}, {}, location).set_message(msg));
  if (type == QtFatalMsg) {
    shutdown();
    ::usleep(100'000);
    [[unlikely]] abort();
  }
}

static inline void set_qt_log_handler(bool enable, QtMessageHandler handler) {
  if (!_rootLogger)
    return;
  QMutexLocker ml(_qt_handler_mutex);
  if (enable) {
    QtMessageHandler previous = qInstallMessageHandler(handler);
    if (!_qt_original_handler)
      _qt_original_handler = previous;
  } else if (_qt_original_handler) {
    qInstallMessageHandler(_qt_original_handler);
    _qt_original_handler = 0;
  }
}

void use_same_format_for_qt_log(bool enable) {
  set_qt_log_handler(enable, same_format_qt_log_handler);
}

void wrap_qt_log(bool enable) {
  set_qt_log_handler(enable, p6_log_wrapper_qt_log_handler);
}

void stderr_direct_log(const Record &record) {
  fputs(record.formated_message(), stderr);
}

} // ns p6::log
