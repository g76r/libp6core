/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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

#define ISO8601 QStringLiteral("yyyy-MM-ddThh:mm:ss,zzz")

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

static inline void sanitizeField(QByteArray *ba) {
  if (ba->isNull())
    return;
  for (char *s = ba->data(); *s; ++s)
    if (::isspace(*s))
      *s = '_';
}

static inline void sanitizeMessage(QByteArray *ba) {
  ba->replace("\n","\n ");
}

void Log::addLogger(Logger *logger, bool autoRemovable) {
  if (logger->threadModel() == Logger::DirectCall)
    logger->moveToThread(_rootLogger->thread());
  _rootLogger->addLogger(logger, autoRemovable);
}

void Log::removeLogger(Logger *logger) {
  _rootLogger->removeLogger(logger);
}

void Log::addConsoleLogger(Severity severity, bool autoRemovable) {
  _rootLogger->addConsoleLogger(severity, autoRemovable);
}

void Log::replaceLoggers(Logger *newLogger) {
  _rootLogger->replaceLoggers(newLogger);
}

void Log::replaceLoggers(QList<Logger*> newLoggers) {
  _rootLogger->replaceLoggers(newLoggers);
}

void Log::replaceLoggersPlusConsole(Log::Severity consoleLoggerSeverity,
                                    QList<Logger*> newLoggers) {
  _rootLogger->replaceLoggersPlusConsole(
        consoleLoggerSeverity, newLoggers);
}

void Log::log(
    QByteArray message, Severity severity, QByteArray task, QByteArray execId,
    QByteArray sourceCode) {
  QByteArray realTask = task;
  if (realTask.isNull()) {
    QThread *t = QThread::currentThread();
    if (t)
      realTask = t->objectName().toUtf8();
  }
  sanitizeField(&realTask);
  if (realTask.isEmpty())
    realTask = QByteArrayLiteral("?");
  sanitizeField(&execId);
  if (execId.isEmpty())
    execId = QByteArrayLiteral("0");
  sanitizeField(&sourceCode);
  if (sourceCode.isEmpty())
    sourceCode = QByteArrayLiteral(":");
  QDateTime now = QDateTime::currentDateTime();
  sanitizeMessage(&message);
  _rootLogger->log(Logger::LogEntry(now, message, severity, realTask,
                                    execId, sourceCode));
}

void Log::shutdown() {
  _rootLogger->shutdown();
}

QByteArray Log::severityToString(Severity severity) {
  switch (severity) {
  case Debug:
    return QByteArrayLiteral("DEBUG");
  case Info:
    return QByteArrayLiteral("INFO");
  case Warning:
    return QByteArrayLiteral("WARNING");
  case Error:
    return QByteArrayLiteral("ERROR");
  case Fatal:
    return QByteArrayLiteral("FATAL");
  }
  return QByteArrayLiteral("UNKNOWN");
}

Log::Severity Log::severityFromString(QByteArray string) {
  if (string.isEmpty())
    return Debug;
  switch (string.at(0)) {
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

static void qtLogSamePatternWrapper(
    QtMsgType type, const QMessageLogContext &context, const QString &msg) {
  QByteArray severity = QByteArrayLiteral("UNKNOWN");
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
  QByteArray taskName = QThread::currentThread()->objectName().toUtf8();
  sanitizeField(&taskName);
  if (taskName.isEmpty())
    taskName = QByteArrayLiteral("?");
  QByteArray realMsg = msg.toUtf8();
  sanitizeMessage(&realMsg);
  QByteArray source =
      context.file ? QByteArray(context.file).append(":")
                     .append(QByteArray::number(context.line))
                   : QByteArrayLiteral(":");
  QByteArray localMsg =
    QDateTime::currentDateTime().toString(ISO8601).toUtf8()
      +" "+taskName+"/0 "+source+" "+severity+" qtdebug: "+realMsg+"\n";
  fputs(localMsg, stderr);
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
