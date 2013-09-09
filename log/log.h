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
#ifndef LOG_H
#define LOG_H

#include <QString>
#include <QtDebug>
#include <QVariant>
#include <QStringList>
#include "libqtssu_global.h"

class Logger;
class LogHelper;

/** This class provides a server-side log facility with common server-side
  * severities (whereas QtDebug does not) and write timestamped log files.
  * All public methods are threadsafe.
  */
class LIBQTSSUSHARED_EXPORT Log {
public:
  enum Severity { Debug, Info, Warning, Error, Fatal };
  /** Add a new logger. Takes the ownership of the logger (= will delete it). */
  static void addLogger(Logger *logger, bool removable = true);
  /** Add a logger to stdout. */
  static void addConsoleLogger(Log::Severity severity = Log::Warning,
                               bool removable = false);
  /** Remove all loggers. */
  static void clearLoggers();
  /** Remove all loggers and replace them with a new one. */
  static void replaceLoggers(Logger *newLogger);
  /** Remove all loggers and replace them with new ones. */
  static void replaceLoggers(QList<Logger*> newLoggers);
  /** Remove all loggers and replace them with new ones plus a console logger.*/
  static void replaceLoggersPlusConsole(Log::Severity consoleLoggerSeverity,
                                        QList<Logger*> newLoggers);
  static void log(QString message, Severity severity = Info,
                  QString task = QString(), QString execId = QString(),
                  QString sourceCode = QString());
  static QString severityToString(Severity severity);
  /** Very tolerant severity mnemonic reader.
   * Read first character of string in a case insensitive manner, e.g.
   * "W", "warn", "warning", and "war against terror" are all interpreted
   * as Log::Warning.
   * Unmatched letters ar interpreted as Log::Debug, therefore "D", "Debug",
   * or even "Global Warming" and "" are all interpreted as Log::Debug. */
  static Log::Severity severityFromString(QString string);
  static void logMessageHandler(QtMsgType type, const char *msg);
  static inline LogHelper log(Log::Severity severity,
                              QString task = QString(),
                              QString execId = QString(),
                              QString sourceCode = QString());
  static inline LogHelper debug(QString task = QString(),
                                QString execId = QString(),
                                QString sourceCode = QString());
  static inline LogHelper info(QString task = QString(),
                               QString execId = QString(),
                               QString sourceCode = QString());
  static inline LogHelper warning(QString task = QString(),
                                  QString execId = QString(),
                                  QString sourceCode = QString());
  static inline LogHelper error(QString task = QString(),
                                QString execId = QString(),
                                QString sourceCode = QString());
  static inline LogHelper fatal(QString task = QString(),
                                QString execId = QString(),
                                QString sourceCode = QString());
  static inline LogHelper log(Log::Severity severity, QString task,
                              quint64 execId,
                              QString sourceCode = QString());
  static inline LogHelper debug(QString task, quint64 execId,
                                QString sourceCode = QString());
  static inline LogHelper info(QString task, quint64 execId,
                               QString sourceCode = QString());
  static inline LogHelper warning(QString task, quint64 execId,
                                  QString sourceCode = QString());
  static inline LogHelper error(QString task, quint64 execId,
                                QString sourceCode = QString());
  static inline LogHelper fatal(QString task, quint64 execId,
                                QString sourceCode = QString());
  static QString pathToLastFullestLog();
  static QStringList pathsToFullestLogs();
  static QStringList pathsToAllLogs();

private:
  Log() { }
  static inline QString sanitize(QString string);
};

class LIBQTSSUSHARED_EXPORT LogHelper {
  mutable bool _logOnDestroy;
  Log::Severity _severity;
  QString _message, _task, _execId, _sourceCode;

public:
  inline LogHelper(Log::Severity severity, QString task, QString execId,
                   QString sourceCode)
    : _logOnDestroy(true), _severity(severity), _task(task), _execId(execId),
      _sourceCode(sourceCode) {
  }
  // The following copy constructor is needed because static Log::*() methods
  // return LogHelper by value. It must never be called in another context,
  // especially because it is not thread-safe.
  // Compilers are likely not to use the copy constructor at all, for instance
  // GCC won't use it but if it is called with -fno-elide-constructors option.
  inline LogHelper(const LogHelper &other)
    : _logOnDestroy(true), _severity(other._severity), _message(other._message),
      _task(other._task), _execId(other._execId),
      _sourceCode(other._sourceCode) {
    other._logOnDestroy = false;
    //qDebug() << "### copying LogHelper" << _message;
  }
  inline ~LogHelper() {
    if (_logOnDestroy) {
      //qDebug() << "***log" << _message;
      Log::log(_message, _severity, _task, _execId, _sourceCode);
    }
  }
  inline LogHelper &operator<<(const QString &o) {
    _message.append(o); return *this; }
  inline LogHelper &operator<<(const QLatin1String &o) {
    _message.append(o); return *this; }
  inline LogHelper &operator<<(const QStringRef &o) {
    _message.append(o); return *this; }
  inline LogHelper &operator<<(const QByteArray &o) {
    _message.append(o); return *this; }
  inline LogHelper &operator<<(const QChar &o) {
    _message.append(o); return *this; }
  inline LogHelper &operator<<(const char *o) {
    _message.append(o); return *this; }
  inline LogHelper &operator<<(char o) {
    _message.append(o); return *this; }
  inline LogHelper &operator<<(qint64 o) {
    _message.append(QString::number(o)); return *this; }
  inline LogHelper &operator<<(quint64 o) {
    _message.append(QString::number(o)); return *this; }
  inline LogHelper &operator<<(qint32 o) {
    _message.append(QString::number(o)); return *this; }
  inline LogHelper &operator<<(quint32 o) {
    _message.append(QString::number(o)); return *this; }
  inline LogHelper &operator<<(double o) {
    _message.append(QString::number(o)); return *this; }
  inline LogHelper &operator<<(const QVariant &o) {
    _message.append(o.toString()); return *this; }
  inline LogHelper &operator<<(const QList<QString> &o) {
    _message.append("{ ");
    foreach (const QString &s, o)
      _message.append("\"").append(s).append("\" ");
    _message.append("}");
    return *this; }
  inline LogHelper &operator<<(const QSet<QString> &o) {
    _message.append("{ ");
    foreach (const QString &s, o)
      _message.append("\"").append(s).append("\" ");
    _message.append("}");
    return *this; }
  inline LogHelper &operator<<(const QObject *o) {
    if (o)
      _message.append(o->objectName()).append("(0x")
          .append(QString::number((quint64)o, 16)).append(")");
    else
      _message.append("null");
    return *this; }
  inline LogHelper &operator<<(const QObject &o) {
    return operator<<(&o); }
  inline LogHelper &operator<<(const void *o) {
    _message.append("0x").append(QString::number((quint64)o, 16));
    return *this; }
};


LogHelper Log::log(Log::Severity severity, QString task,
                   QString execId, QString sourceCode) {
  return LogHelper(severity, task, execId, sourceCode);
}

LogHelper Log::debug(QString task, QString execId, QString sourceCode) {
  return LogHelper(Log::Debug, task, execId, sourceCode);
}

LogHelper Log::info(QString task, QString execId, QString sourceCode) {
  return LogHelper(Log::Info, task, execId, sourceCode);
}

LogHelper Log::warning(QString task, QString execId, QString sourceCode) {
  return LogHelper(Log::Warning, task, execId, sourceCode);
}

LogHelper Log::error(QString task, QString execId, QString sourceCode) {
  return LogHelper(Log::Error, task, execId, sourceCode);
}

LogHelper Log::fatal(QString task, QString execId, QString sourceCode) {
  return LogHelper(Log::Fatal, task, execId, sourceCode);
}

LogHelper Log::log(Log::Severity severity, QString task, quint64 execId,
                   QString sourceCode) {
  return log(severity, task, QString::number(execId), sourceCode);
}

LogHelper Log::debug(QString task, quint64 execId, QString sourceCode) {
  return debug(task, QString::number(execId), sourceCode);
}

LogHelper Log::info(QString task, quint64 execId, QString sourceCode) {
  return info(task, QString::number(execId), sourceCode);
}

LogHelper Log::warning(QString task, quint64 execId, QString sourceCode) {
  return warning(task, QString::number(execId), sourceCode);
}

LogHelper Log::error(QString task, quint64 execId, QString sourceCode) {
  return error(task, QString::number(execId), sourceCode);
}

LogHelper Log::fatal(QString task, quint64 execId,QString sourceCode) {
  return fatal(task, QString::number(execId), sourceCode);
}

#endif // LOG_H
