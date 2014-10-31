/* Copyright 2012-2014 Hallowyn and others.
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
#include "multiplexerlogger.h"
#include <QList>
#include <QString>
#include <QDateTime>
#include <QRegExp>
#include <QThread>

Q_GLOBAL_STATIC_WITH_ARGS(MultiplexerLogger, _rootLogger, (Log::Debug, true))

static inline MultiplexerLogger *rootLogger() { return _rootLogger(); }

void Log::addLogger(Logger *logger, bool autoRemovable) {
  rootLogger()->addLogger(logger, autoRemovable);
}

void Log::removeLogger(Logger *logger) {
  rootLogger()->removeLogger(logger);
}

void Log::addConsoleLogger(Severity severity, bool autoRemovable) {
  rootLogger()->addConsoleLogger(severity, autoRemovable);
}

void Log::addQtLogger(Severity severity, bool autoRemovable) {
  rootLogger()->addQtLogger(severity, autoRemovable);
}

/*void Log::removeLoggers() {
  QMutexLocker locker(&_loggersMutex);
  foreach(Logger *logger, _loggers)
    if (logger->_autoRemovable) {
      logger->deleteLater();
      _loggers.removeOne(logger);
    }
}*/

void Log::replaceLoggers(Logger *newLogger) {
  rootLogger()->replaceLoggers(newLogger);
}

void Log::replaceLoggers(QList<Logger*> newLoggers) {
  rootLogger()->replaceLoggers(newLoggers);
}

void Log::replaceLoggersPlusConsole(Log::Severity consoleLoggerSeverity,
                                    QList<Logger*> newLoggers) {
  rootLogger()->replaceLoggersPlusConsole(consoleLoggerSeverity, newLoggers);
}

void Log::log(QString message, Severity severity, QString task, QString execId,
              QString sourceCode) {
  QString realTask(task);
  if (realTask.isNull()) {
    QThread *t(QThread::currentThread());
    if (t)
      realTask = t->objectName();
  }
  realTask = realTask.isEmpty() ? "?" : sanitizeField(realTask);
  QString realExecId = execId.isEmpty() ? "0" : sanitizeField(execId);
  QString realSourceCode
      = sourceCode.isEmpty() ? ":" : sanitizeField(sourceCode);
  QDateTime now = QDateTime::currentDateTime();
  message = sanitizeMessage(message);
  rootLogger()->log(Logger::LogEntry(now, message, severity, realTask,
                                     realExecId, realSourceCode));
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

QString Log::sanitizeField(QString string) {
  static const QRegExp whitespace("\\s+");
  QString s(string);
  // LATER optimize: avoid using a regexp 3 times per log line
  s.replace(whitespace, "_");
  return s;
}

QString Log::sanitizeMessage(QString string) {
  QString s(string);
  s.replace('\n', "\n  ");
  return s;
}

/*void Log::logMessageHandler(QtMsgType type, const char *msg) {
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
}*/

QString Log::pathToLastFullestLog() {
  return rootLogger()->pathToLastFullestLog();
}

QStringList Log::pathsToFullestLogs() {
  return rootLogger()->pathsToFullestLogs();
}

QStringList Log::pathsToAllLogs() {
  return rootLogger()->pathsToAllLogs();
}
