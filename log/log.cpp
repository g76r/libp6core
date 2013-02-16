/* Copyright 2012-2013 Hallowyn and others.
 * This file is part of qron, see <http://qron.hallowyn.com/>.
 * Qron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Qron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with qron. If not, see <http://www.gnu.org/licenses/>.
 */
#include "log.h"
#include "filelogger.h"
#include <QList>
#include <QString>
#include <QDateTime>
#include <QRegExp>
#include <QThread>
#include <QMutex>
#include <QFile>
#include "util/ioutils.h"

static QList<Logger*> _loggers;
static QMutex _loggersMutex;

void Log::addConsoleLogger() {
  QFile *console = new QFile;
  console->open(1, QIODevice::WriteOnly|QIODevice::Unbuffered);
  FileLogger *logger = new FileLogger(console, Log::Debug);
  Log::addLogger(logger, false);
}

void Log::addLogger(Logger *logger, bool removable) {
  QMutexLocker locker(&_loggersMutex);
  if (logger) {
    logger->_removable = removable;
    if (_loggers.isEmpty())
      qInstallMsgHandler(Log::logMessageHandler);
    _loggers.append(logger);
  }
}

void Log::clearLoggers() {
  QMutexLocker locker(&_loggersMutex);
  foreach(Logger *logger, _loggers)
    if (logger->_removable) {
      logger->deleteLater();
      _loggers.removeOne(logger);
    }
}

void Log::replaceLoggers(Logger *newLogger) {
  QList<Logger*> newLoggers;
  if (newLogger)
    newLoggers.append(newLogger);
  replaceLoggers(newLoggers);
}

void Log::replaceLoggers(QList<Logger*> newLoggers) {
  QMutexLocker locker(&_loggersMutex);
  foreach(Logger *logger, _loggers)
    if (logger->_removable) {
      logger->deleteLater();
      _loggers.removeOne(logger);
    }
  if (newLoggers.isEmpty())
    qInstallMsgHandler(0);
  foreach (Logger *logger, newLoggers)
    _loggers.append(logger);
}

void Log::log(const QString message, Severity severity, const QString task,
              const QString execId, const QString sourceCode) {
  QDateTime now = QDateTime::currentDateTime();
  QString realTask(task);
  if (realTask.isNull())
    realTask = QThread::currentThread()->objectName();
  realTask = realTask.isEmpty() ? "?" : sanitize(realTask);
  QString realExecId = execId.isEmpty() ? "0" : sanitize(execId);
  QString realSourceCode = sourceCode.isEmpty() ? ":" : sanitize(sourceCode);
  QMutexLocker locker(&_loggersMutex);
  foreach (Logger *logger, _loggers)
    logger->log(now, message, severity, realTask, realExecId, realSourceCode);
}

const QString Log::severityToString(Severity severity) {
  switch (severity) {
  case Debug:
    return "DEBUG";
  case Info:
    return "INFO";
  case Warning:
    return "WARNING";
  case Error:
    return "ERROR";
  case Fatal:
    return "FATAL";
  }
  return "UNKNOWN";
}

Log::Severity Log::severityFromString(const QString string) {
  if (!string.isEmpty())
    switch (string.at(0).toAscii()) {
    case 'I':
    case 'i':
      return Info;
    case 'W':
    case 'w':
      return Warning;
    case 'E':
    case 'e':
      return Error;
    case 'F':
    case 'f':
      return Fatal;
    }
  return Debug;
}

QString Log::sanitize(const QString string) {
  QString s(string);
  // LATER optimize: avoid using a regexp 3 times per log line
  s.replace(QRegExp("\\s"), "_");
  return s;
}

void Log::logMessageHandler(QtMsgType type, const char *msg) {
  switch (type) {
  case QtDebugMsg:
    Log::log(msg, Log::Debug);
    break;
  case QtWarningMsg:
    Log::log(msg, Log::Warning);
    break;
  case QtCriticalMsg:
    Log::log(msg, Log::Error);
    break;
  case QtFatalMsg:
    Log::log(msg, Log::Fatal);
    // LATER shutdown process because default Qt message handler does shutdown
  }
}

QString Log::pathToLastFullestLog() {
  QMutexLocker locker(&_loggersMutex);
  int severity = Fatal+1;
  QString path;
  foreach(Logger *logger, _loggers) {
    if (logger->minSeverity() < severity) {
      QString p = logger->currentPath();
      if (!p.isEmpty()) {
        if (severity == Debug)
          return p;
        path = p;
        severity = logger->minSeverity();
      }
    }
  }
  return path;
}

QStringList Log::pathsToFullestLogs() {
  QMutexLocker locker(&_loggersMutex);
  int severity = Fatal+1;
  QString path;
  foreach(Logger *logger, _loggers) {
    if (logger->minSeverity() < severity) {
      QString p = logger->pathMathchingPattern();
      if (!p.isEmpty()) {
        if (severity == Debug) {
          return IOUtils::findFiles(p);
        }
        path = p;
        severity = logger->minSeverity();
      }
    }
  }
  return IOUtils::findFiles(path);
}

QStringList Log::pathsToAllLogs() {
  QMutexLocker locker(&_loggersMutex);
  QStringList paths;
  foreach(Logger *logger, _loggers) {
    QString p = logger->pathMathchingPattern();
    if (!p.isEmpty())
      paths.append(p);
  }
  return IOUtils::findFiles(paths);
}
