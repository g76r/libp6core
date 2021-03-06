/* Copyright 2014-2017 Hallowyn, Gregoire Barbier and others.
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
#include "qtloglogger.h"
#include <QFile>
#include "util/ioutils.h"

MultiplexerLogger::MultiplexerLogger(
    Log::Severity minSeverity, bool isRootLogger)
  : Logger(minSeverity, isRootLogger ? Logger::RootLogger
                                     : Logger::DirectCall) {
}

void MultiplexerLogger::addLogger(
    Logger *logger, bool autoRemovable, bool takeOwnership) {
  QMutexLocker locker(&_loggersMutex);
  if (logger) {
    // LATER provide an option to enable Qt's standard log interception
    // drawbacks:
    // - Qt's log is synchronous (no writer thread) and thus intercepting it
    //   would change the behavior (log order, even missing log entries on
    //   crash)
    // - qFatal() expect the program to write a log and shutdown, which is not
    //   easy to reproduce here
    //if (_loggers.isEmpty())
    //  qInstallMsgHandler(Log::logMessageHandler);
    logger->_autoRemovable = autoRemovable;
    _loggers.append(logger);
    if (takeOwnership)
      _ownedLoggers.insert(logger);
  }
}

void MultiplexerLogger::removeLogger(Logger *logger) {
  QMutexLocker locker(&_loggersMutex);
  foreach(Logger *l, _loggers)
    if (l == logger) {
      if (_ownedLoggers.contains(logger)) {
        logger->deleteLater();
        _ownedLoggers.remove(logger);
      }
      _loggers.removeAll(logger);
      break;
    }
}

void MultiplexerLogger::addConsoleLogger(Log::Severity severity,
                                         bool autoRemovable) {
  QFile *console = new QFile;
  console->open(1, QIODevice::WriteOnly|QIODevice::Unbuffered);
  FileLogger *logger = new FileLogger(console, severity);
  addLogger(logger, autoRemovable, true);
}

void MultiplexerLogger::addQtLogger(Log::Severity severity,
                                    bool autoRemovable) {
  QtLogLogger *logger = new QtLogLogger(severity);
  addLogger(logger, autoRemovable, true);
}

void MultiplexerLogger::replaceLoggers(Logger *newLogger, bool takeOwnership) {
  QList<Logger*> newLoggers;
  if (newLogger)
    newLoggers.append(newLogger);
  replaceLoggers(newLoggers, takeOwnership);
}

void MultiplexerLogger::replaceLoggers(
    QList<Logger*> newLoggers, bool takeOwnership) {
  QMutexLocker locker(&_loggersMutex);
  doReplaceLoggers(newLoggers, takeOwnership);
}

void MultiplexerLogger::replaceLoggersPlusConsole(
    Log::Severity consoleLoggerSeverity, QList<Logger*> newLoggers,
    bool takeOwnership) {
  QFile *console = new QFile;
  console->open(1, QIODevice::WriteOnly|QIODevice::Unbuffered);
  FileLogger *consoleLogger = new FileLogger(console, consoleLoggerSeverity);
  newLoggers.prepend(consoleLogger);
  QMutexLocker locker(&_loggersMutex);
  // own console logger regardless taking ownership of other loggers
  _ownedLoggers.insert(consoleLogger);
  doReplaceLoggers(newLoggers, takeOwnership);
}

void MultiplexerLogger::doReplaceLoggers(
    QList<Logger*> newLoggers, bool takeOwnership) {
  foreach(Logger *logger, _loggers)
    if (logger->_autoRemovable) {
      if (!newLoggers.contains(logger) && _ownedLoggers.contains(logger)) {
        logger->deleteLater();
        _ownedLoggers.remove(logger);
      }
      _loggers.removeAll(logger);
    }
  foreach (Logger *logger, newLoggers) {
    _loggers.append(logger);
    if (takeOwnership)
      _ownedLoggers.insert(logger);
  }
}

QString MultiplexerLogger::pathToLastFullestLog() {
  // LATER avoid locking here whereas right logger won't change often
  QMutexLocker locker(&_loggersMutex);
  int severity = Log::Fatal+1;
  QString path;
  foreach(Logger *logger, _loggers) {
    if (logger->minSeverity() < severity) {
      QString p = logger->currentPath();
      if (!p.isEmpty()) {
        if (severity == Log::Debug)
          return p;
        path = p;
        severity = logger->minSeverity();
      }
    }
  }
  return path;
}

QStringList MultiplexerLogger::pathsToFullestLogs() {
  // LATER avoid locking here whereas right logger won't change often
  QMutexLocker locker(&_loggersMutex);
  int severity = Log::Fatal+1;
  QString path;
  foreach(Logger *logger, _loggers) {
    if (logger->minSeverity() < severity) {
      QString p = logger->pathMatchingRegexp();
      if (!p.isEmpty()) {
        if (severity == Log::Debug) {
          locker.unlock();
          return IOUtils::findFiles(p);
        }
        path = p;
        severity = logger->minSeverity();
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
  foreach(Logger *logger, _loggers) {
    QString p = logger->pathMatchingRegexp();
    if (!p.isEmpty())
      paths.append(p);
  }
  locker.unlock();
  return IOUtils::findFiles(paths);
}

void MultiplexerLogger::doLog(const LogEntry &entry) {
  QMutexLocker locker(&_loggersMutex);
  foreach (Logger *logger, _loggers)
    logger->log(entry);
}
