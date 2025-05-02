/* Copyright 2014-2025 Hallowyn, Gregoire Barbier and others.
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
#include "multiplexerlogger.h"
#include "filelogger.h"
#include "io/ioutils.h"
#include <QFile>
#include <QThread>

namespace p6::log {

MultiplexerLogger::MultiplexerLogger(
    Log::Severity minSeverity, bool isRootLogger)
  : Logger(minSeverity, isRootLogger ? Logger::RootLogger
                                     : Logger::DirectCall) {
}

MultiplexerLogger::~MultiplexerLogger() {
  QMutexLocker locker(&_loggersMutex);
  for (auto logger : _loggers)
    logger->shutdown();
}

void MultiplexerLogger::addLogger(Logger *logger, bool autoRemovable) {
  QMutexLocker locker(&_loggersMutex);
  if (logger) {
    // LATER provide an option to enable Qt's standard log interception
    // drawbacks:
    // - Qt's log is synchronous (no writer thread) and thus intercepting it
    //   would change the behavior (log order, even missing log records on
    //   crash)
    // - qFatal() expect the program to write a log and shutdown, which is not
    //   easy to reproduce here
    //if (_loggers.isEmpty())
    //  qInstallMsgHandler(Log::logMessageHandler);
    logger->_auto_removable = autoRemovable;
    _loggers.append(logger);
  }
}

void MultiplexerLogger::removeLogger(Logger *logger) {
  QMutexLocker locker(&_loggersMutex);
  QList<Logger*> old_loggers(_loggers);
  old_loggers.detach();
  for (auto l: old_loggers)
    if (l == logger) {
      logger->shutdown();
      _loggers.removeAll(logger);
      break;
    }
}

void MultiplexerLogger::addConsoleLogger(
  Log::Severity severity, bool autoRemovable, FILE *stream) {
  QFile *console = new QFile;
  console->open(stream, QIODevice::WriteOnly|QIODevice::Unbuffered);
  FileLogger *logger = new FileLogger(console, severity, false);
  connect(logger, &QObject::destroyed, console, &QObject::deleteLater);
  auto name = "Console"+logger->objectName();
  logger->setObjectName(name);
  if (auto t = logger->thread(); t) // should always be true
    t->setObjectName(name);
  addLogger(logger, autoRemovable);
}

void MultiplexerLogger::replace_loggers(
    QList<Logger*> &new_loggers, bool prepend_console,
    Severity console_min_severity) {
  if (prepend_console) {
    QFile *console = new QFile;
    console->open(1, QIODevice::WriteOnly|QIODevice::Unbuffered);
    FileLogger *consoleLogger = new FileLogger(console, console_min_severity);
    connect(consoleLogger, &QObject::destroyed, console, &QObject::deleteLater);
    auto name = "Console"+consoleLogger->objectName();
    consoleLogger->setObjectName(name);
    new_loggers.prepend(consoleLogger);
  }
  QMutexLocker locker(&_loggersMutex);
  QList<Logger*> old_loggers(_loggers);
  old_loggers.detach();
  for (auto logger: old_loggers)
    if (logger->_auto_removable) {
      if (!new_loggers.contains(logger))
        logger->shutdown();
      _loggers.removeAll(logger);
    }
  for (auto logger: new_loggers) {
    _loggers.append(logger);
  }
}

QString MultiplexerLogger::pathToLastFullestLog() {
  // LATER avoid locking here whereas right logger won't change often
  QMutexLocker locker(&_loggersMutex);
  int severity = Fatal+1;
  QString path;
  for (auto logger: _loggers) {
    if (logger->min_severity() < severity) {
      QString p = logger->current_path();
      if (!p.isEmpty()) {
        if (severity == Debug)
          return p;
        path = p;
        severity = logger->min_severity();
      }
    }
  }
  return path;
}

QStringList MultiplexerLogger::pathsToFullestLogs() {
  // LATER avoid locking here whereas right logger won't change often
  QMutexLocker locker(&_loggersMutex);
  int severity = Fatal+1;
  QString path;
  for (auto logger: _loggers) {
    if (logger->min_severity() < severity) {
      QString p = logger->path_matching_regexp();
      if (!p.isEmpty()) {
        if (severity == Debug) {
          locker.unlock();
          return IOUtils::findFiles(p);
        }
        path = p;
        severity = logger->min_severity();
      }
    }
  }
  locker.unlock();
  return IOUtils::findFiles(path);
}

QStringList MultiplexerLogger::pathsToAllLogs() {
  // LATER avoid locking here whereas loggers list won't change often
  QMutexLocker locker(&_loggersMutex);
  QStringList paths;
  for (auto logger: _loggers) {
    QString p = logger->path_matching_regexp();
    if (!p.isEmpty())
      paths.append(p);
  }
  locker.unlock();
  return IOUtils::findFiles(paths);
}

void MultiplexerLogger::do_log(const Record &record) {
  QMutexLocker locker(&_loggersMutex);
  for (auto logger : _loggers)
    logger->log(record);
  if (_thread_model & RootLogger && _loggers.isEmpty() && !!record) {
    switch(record.severity()) {
      case Debug:
        qDebug() << record.formated_message() << "(no logger configured)";
        break;
      case Info:
        qInfo() << record.formated_message() << "(no logger configured)";
        break;
      case Warning:
      case Error:
      case Fatal:
        qWarning() << record.formated_message() << "(no logger configured)";
    }
  }
}

void MultiplexerLogger::do_shutdown() {
  QMutexLocker locker(&_loggersMutex);
  for (auto logger : _loggers)
    logger->shutdown();
  _loggers.clear();
}

} // ns p6::log
