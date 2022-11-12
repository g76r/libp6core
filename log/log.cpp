/* Copyright 2012-2022 Hallowyn, Gregoire Barbier and others.
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
#include <QList>
#include <QString>
#include <QDateTime>
#include <QRegularExpression>
#include <QThread>
#include <time.h>

static MultiplexerLogger *_rootLogger = nullptr;
static QRegularExpression *_whitespaceRE = nullptr;
static QMutex *_qtHandlerMutex = nullptr;
static QtMessageHandler _qtOriginalHandler = nullptr;

/******************************************************************
  /!\ there must be no global variables with a destructor here /!\
  /!\ because Log::log() must not crash when called during the /!\
  /!\ program shutdown                                         /!\
 ******************************************************************/

static int staticInit() {
  _rootLogger = new MultiplexerLogger(Log::Debug, true);
  _whitespaceRE = new QRegularExpression{ "\\s+" };
  _qtHandlerMutex = new QMutex;
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

static inline QString sanitizeField(QString string) {
  QString s(string);
  // LATER optimize: avoid using a regexp 3 times per log line
  s.replace(*_whitespaceRE, QStringLiteral("_"));
  return s;
}

static inline QString sanitizeMessage(QString string) {
  QString s(string);
  s.replace(QChar('\n'), QStringLiteral("\n "));
  return s;
}

void Log::addLogger(Logger *logger, bool autoRemovable, bool takeOwnership) {
  if (logger->threadModel() == Logger::DirectCall)
    logger->moveToThread(_rootLogger->thread());
  _rootLogger->addLogger(logger, autoRemovable, takeOwnership);
}

void Log::removeLogger(Logger *logger) {
  _rootLogger->removeLogger(logger);
}

void Log::addConsoleLogger(Severity severity, bool autoRemovable) {
  _rootLogger->addConsoleLogger(severity, autoRemovable);
}

void Log::addQtLogger(Severity severity, bool autoRemovable) {
  _rootLogger->addQtLogger(severity, autoRemovable);
}

void Log::replaceLoggers(Logger *newLogger, bool takeOwnership) {
  _rootLogger->replaceLoggers(newLogger, takeOwnership);
}

void Log::replaceLoggers(QList<Logger*> newLoggers, bool takeOwnership) {
  _rootLogger->replaceLoggers(newLoggers, takeOwnership);
}

void Log::replaceLoggersPlusConsole(Log::Severity consoleLoggerSeverity,
                                    QList<Logger*> newLoggers, bool takeOwnership) {
  _rootLogger->replaceLoggersPlusConsole(
        consoleLoggerSeverity, newLoggers, takeOwnership);
}

void Log::log(QString message, Severity severity, QString task, QString execId,
              QString sourceCode) {
  QString realTask(task);
  if (realTask.isNull()) {
    QThread *t(QThread::currentThread());
    if (t)
      realTask = t->objectName();
  }
  realTask = sanitizeField(realTask);
  if (realTask.isEmpty())
    realTask = QStringLiteral("?");
  QString realExecId = sanitizeField(execId);
  if (realExecId.isEmpty())
    realExecId = QStringLiteral("0");
  QString realSourceCode = sanitizeField(sourceCode);
  if (sourceCode.isEmpty())
    sourceCode = QStringLiteral(":");
  QDateTime now = QDateTime::currentDateTime();
  message = sanitizeMessage(message);
  _rootLogger->log(Logger::LogEntry(now, message, severity, realTask,
                                     realExecId, realSourceCode));
}

QString Log::severityToString(Severity severity) {
  switch (severity) {
  case Debug:
    return QStringLiteral("DEBUG");
  case Info:
    return QStringLiteral("INFO");
  case Warning:
    return QStringLiteral("WARNING");
  case Error:
    return QStringLiteral("ERROR");
  case Fatal:
    return QStringLiteral("FATAL");
  }
  return QStringLiteral("UNKNOWN");
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

QString Log::pathToLastFullestLog() {
  return _rootLogger->pathToLastFullestLog();
}

QStringList Log::pathsToFullestLogs() {
  return _rootLogger->pathsToFullestLogs();
}

QStringList Log::pathsToAllLogs() {
  return _rootLogger->pathsToAllLogs();
}

static void qtLogSamePatternWrapper(QtMsgType type, const QMessageLogContext &,
                                    const QString &msg) {
  QString severity = QStringLiteral("UNKNOWN");
  switch (type) {
    case QtDebugMsg:
      severity = Log::severityToString(Log::Debug);
      break;
    case QtInfoMsg:
      severity = Log::severityToString(Log::Info);
      break;
    case QtWarningMsg:
      severity = Log::severityToString(Log::Warning);
      break;
    case QtCriticalMsg:
      severity = Log::severityToString(Log::Error);
      break;
    case QtFatalMsg:
      severity = Log::severityToString(Log::Fatal);
      break;
  }
  QString localMsg =
    QDateTime::currentDateTime().toString(QStringLiteral(
      "yyyy-MM-ddThh:mm:ss,zzz"))+" ?/0 : "+severity+" "+sanitizeMessage(msg)
    +"\n";
  fputs(localMsg.toLocal8Bit(), stderr);
  if (type == QtFatalMsg)
    abort();
}

void Log::wrapQtLogToSamePattern(bool enable) {
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
