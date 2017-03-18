/* Copyright 2012-2016 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
 * Libqtssu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libqtssu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "log.h"
#include "multiplexerlogger.h"
#include <QList>
#include <QString>
#include <QDateTime>
#include <QRegularExpression>
#include <QThread>
#include <time.h>

Q_GLOBAL_STATIC_WITH_ARGS(MultiplexerLogger, _rootLogger, (Log::Debug, true))

static inline MultiplexerLogger *rootLogger() { return _rootLogger(); }

static QtMessageHandler _originalHandler = 0;
static QMutex _qtHandlerMutex;
static const QRegularExpression whitespace { "\\s+" };
static const QString lineContinuation = "\n  ";
static const QString defaultTask = "?";
static const QString defaultExecid = "0";
static const QString defaultSourceCode = ":";
static const QString defaultTaskAndSourceCode = " ?/0 : ";
static const QString eol = "\n";
static const QString timestampFormat = "yyyy-MM-ddThh:mm:ss,zzz";
static const QString severityDebug("DEBUG");
static const QString severityInfo("INFO");
static const QString severityWarning("WARNING");
static const QString severityError("ERROR");
static const QString severityFatal("FATAL");
static const QString severityUnknown("UNKNOWN");

static inline QString sanitizeField(QString string) {
  QString s(string);
  // LATER optimize: avoid using a regexp 3 times per log line
  s.replace(whitespace, "_");
  return s;
}

static inline QString sanitizeMessage(QString string) {
  QString s(string);
  s.replace('\n', lineContinuation);
  return s;
}

void Log::addLogger(Logger *logger, bool autoRemovable, bool takeOwnership) {
  rootLogger()->addLogger(logger, autoRemovable, takeOwnership);
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

void Log::replaceLoggers(Logger *newLogger, bool takeOwnership) {
  rootLogger()->replaceLoggers(newLogger, takeOwnership);
}

void Log::replaceLoggers(QList<Logger*> newLoggers, bool takeOwnership) {
  rootLogger()->replaceLoggers(newLoggers, takeOwnership);
}

void Log::replaceLoggersPlusConsole(Log::Severity consoleLoggerSeverity,
                                    QList<Logger*> newLoggers, bool takeOwnership) {
  rootLogger()->replaceLoggersPlusConsole(
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
  realTask = realTask.isEmpty() ? defaultTask : sanitizeField(realTask);
  QString realExecId = execId.isEmpty() ? defaultExecid : sanitizeField(execId);
  QString realSourceCode
      = sourceCode.isEmpty() ? defaultSourceCode : sanitizeField(sourceCode);
  QDateTime now = QDateTime::currentDateTime();
  message = sanitizeMessage(message);
  rootLogger()->log(Logger::LogEntry(now, message, severity, realTask,
                                     realExecId, realSourceCode));
}

QString Log::severityToString(Severity severity) {
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

QString Log::pathToLastFullestLog() {
  return rootLogger()->pathToLastFullestLog();
}

QStringList Log::pathsToFullestLogs() {
  return rootLogger()->pathsToFullestLogs();
}

QStringList Log::pathsToAllLogs() {
  return rootLogger()->pathsToAllLogs();
}

static void qtLogSamePatternWrapper(QtMsgType type, const QMessageLogContext &,
                                    const QString &msg) {
  QString severity;
  switch (type) {
  case QtDebugMsg:
    severity = severityDebug;
    break;
#if QT_VERSION >= 0x050500
  case QtInfoMsg:
    severity = severityInfo;
    break;
#endif
  case QtWarningMsg:
    severity = severityWarning;
    break;
  case QtCriticalMsg:
    severity = severityError;
    break;
  case QtFatalMsg:
    severity = severityFatal;
    break;
  default: // should never occur
    severity = severityUnknown;
  }
  QByteArray localMsg =
      (QDateTime::currentDateTime().toString(timestampFormat)
      +defaultTaskAndSourceCode+severity+QStringLiteral(" ")
      +sanitizeMessage(msg)+eol).toLocal8Bit();
  /*int localLen = strlen(localMsg);
  char buf[100+localLen] { 0 }; // actually, 100 is too much
  time_t t = time(NULL);
  struct tm tmp;
  if (localtime_r(&t, &tmp))
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S     unknown/0 :", &tmp);
  .toLocal8Bit()
  int i = strlen(buf);
  for (int j = 0; severity[j]; ++i, ++j)
    buf[i] = severity[j];
  for (int j = 0; j < localLen; ++i, ++j)
    buf[i] = localMsg[j];
  buf[i++] = '\n';
  buf[i] = '\0';
  fputs(buf, stderr);*/
  fputs(localMsg, stderr);
  if (type == QtFatalMsg)
    abort();
}

void Log::wrapQtLogToSamePattern(bool enable) {
  QMutexLocker ml(&_qtHandlerMutex);
  if (enable) {
    QtMessageHandler previous = qInstallMessageHandler(qtLogSamePatternWrapper);
    if (!_originalHandler)
      _originalHandler = previous;
  } else if (_originalHandler) {
    qInstallMessageHandler(_originalHandler);
    _originalHandler = 0;
  }
}
