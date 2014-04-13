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
#include "qtloglogger.h"

static QList<Logger*> _loggers;
static QMutex _loggersMutex;

void Log::addConsoleLogger(Severity severity, bool removable) {
  QFile *console = new QFile;
  console->open(1, QIODevice::WriteOnly|QIODevice::Unbuffered);
  FileLogger *logger = new FileLogger(console, severity);
  Log::addLogger(logger, removable);
}

void Log::addQtLogger(Severity severity, bool removable) {
  QtLogLogger *logger = new QtLogLogger(severity);
  Log::addLogger(logger, removable);
}

void Log::addLogger(Logger *logger, bool removable) {
  QMutexLocker locker(&_loggersMutex);
  if (logger) {
    logger->_removable = removable;
    // LATER provide an option to enable Qt's standard log interception
    // drawbacks:
    // - Qt's log is synchronous (no writer thread) and thus intercepting it
    //   would change the behavior (log order, even missing log entries on
    //   crash)
    // - qFatal() expect the program to write a log and shutdown, which is not
    //   easy to reproduce here
    //if (_loggers.isEmpty())
    //  qInstallMsgHandler(Log::logMessageHandler);
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
  //if (newLoggers.isEmpty())
  //  qInstallMsgHandler(0);
  foreach (Logger *logger, newLoggers)
    _loggers.append(logger);
}

void Log::replaceLoggersPlusConsole(Log::Severity consoleLoggerSeverity,
                                    QList<Logger*> newLoggers) {
  QMutexLocker locker(&_loggersMutex);
  foreach(Logger *logger, _loggers)
    if (logger->_removable) {
      logger->deleteLater();
      _loggers.removeOne(logger);
    }
  QFile *console = new QFile;
  console->open(1, QIODevice::WriteOnly|QIODevice::Unbuffered);
  FileLogger *logger = new FileLogger(console, consoleLoggerSeverity);
  logger->_removable = true;
  _loggers.append(logger);
  foreach (Logger *logger, newLoggers)
    _loggers.append(logger);
}

void Log::log(QString message, Severity severity, QString task, QString execId,
              QString sourceCode) {
  QDateTime now = QDateTime::currentDateTime();
  QString realTask(task);
  if (realTask.isNull()) {
    QThread *t(QThread::currentThread());
    if (t)
      realTask = t->objectName();
  }
  realTask = realTask.isEmpty() ? "?" : sanitize(realTask);
  QString realExecId = execId.isEmpty() ? "0" : sanitize(execId);
  QString realSourceCode = sourceCode.isEmpty() ? ":" : sanitize(sourceCode);
  QMutexLocker locker(&_loggersMutex);
  foreach (Logger *logger, _loggers)
    logger->log(now, message, severity, realTask, realExecId, realSourceCode);
}

QString Log::severityToString(Severity severity) {
  static const QString severityDebug("DEBUG");
  static const QString severityInfo("INFO");
  static const QString severityWarning("WARNING");
  static const QString severityError("ERROR");
  static const QString severityFatal("FATAL");
  static const QString severityUnknown("UNKNOWN");
  switch (severity) {
  case Debug:
    return severityDebug;
  case Info:
    return severityInfo;
  case Warning:
    return severityWarning;
  case Error:
    return severityError;
  case Fatal:
    return severityFatal;
  }
  return severityUnknown;
}

Log::Severity Log::severityFromString(QString string) {
  if (!string.isEmpty())
    switch (string.at(0).toLatin1()) {
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

QString Log::sanitize(QString string) {
  static const QRegExp whitespace("\\s");
  QString s(string);
  // LATER optimize: avoid using a regexp 3 times per log line
  s.replace(whitespace, "_");
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
  // LATER avoid locking here whereas right logger won't change often
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
  // LATER avoid locking here whereas right logger won't change often
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
  // LATER avoid locking here whereas loggers list won't change often
  QMutexLocker locker(&_loggersMutex);
  QStringList paths;
  foreach(Logger *logger, _loggers) {
    QString p = logger->pathMathchingPattern();
    if (!p.isEmpty())
      paths.append(p);
  }
  return IOUtils::findFiles(paths);
}
