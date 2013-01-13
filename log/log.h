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
  static void addConsoleLogger();
  /** Remove all loggers. */
  static void clearLoggers();
  /** Remove all loggers and replace them with a new one. */
  static void replaceLoggers(Logger *newLogger);
  /** Remove all loggers and replace them with new ones. */
  static void replaceLoggers(QList<Logger*> newLoggers);
  static void log(const QString message, Severity severity = Info,
                  const QString task = QString(),
                  const QString execId = QString(),
                  const QString sourceCode = QString());
  static const QString severityToString(Severity severity);
  /** Very tolerant severity mnemonic reader.
   * Read first character of string in a case insensitive manner, e.g.
   * "W", "warn", "warning", and "war against terror" are all interpreted
   * as Log::Warning.
   * Unmatched letters ar interpreted as Log::Debug, therefore "D", "Debug",
   * or even "Global Warming" and "" are all interpreted as Log::Debug. */
  static Log::Severity severityFromString(const QString string);
  static void logMessageHandler(QtMsgType type, const char *msg);
  static inline LogHelper debug(const QString task = QString(),
                                const QString execId = QString(),
                                const QString sourceCode = QString());
  static inline LogHelper info(const QString task = QString(),
                               const QString execId = QString(),
                               const QString sourceCode = QString());
  static inline LogHelper warning(const QString task = QString(),
                                  const QString execId = QString(),
                                  const QString sourceCode = QString());
  static inline LogHelper error(const QString task = QString(),
                                const QString execId = QString(),
                                const QString sourceCode = QString());
  static inline LogHelper fatal(const QString task = QString(),
                                const QString execId = QString(),
                                const QString sourceCode = QString());
  static inline LogHelper debug(const QString task, quint64 execId,
                                const QString sourceCode = QString());
  static inline LogHelper info(const QString task, quint64 execId,
                               const QString sourceCode = QString());
  static inline LogHelper warning(const QString task, quint64 execId,
                                  const QString sourceCode = QString());
  static inline LogHelper error(const QString task, quint64 execId,
                                const QString sourceCode = QString());
  static inline LogHelper fatal(const QString task, quint64 execId,
                                const QString sourceCode = QString());
  static QString pathToFullestLog();

private:
  Log() { }
  static inline QString sanitize(const QString string);
};

class LIBQTSSUSHARED_EXPORT LogHelper {
  class LogHelperData {
  public:
    int _ref;
    Log::Severity _severity;
    QString _message, _task, _execId, _sourceCode;
    LogHelperData(Log::Severity severity, const QString task,
                  const QString execId, const QString sourceCode)
      : _ref(1), _severity(severity), _task(task), _execId(execId),
        _sourceCode(sourceCode) { }
  } *_data;
public:
  inline LogHelper(Log::Severity severity, const QString task,
                   const QString execId, const QString sourceCode)
    : _data(new LogHelperData(severity, task, execId, sourceCode)) {
  }
  inline LogHelper(const LogHelper &other) : _data(other._data){
    ++_data->_ref;
  }
  inline ~LogHelper() {
    if (!--_data->_ref) {
      //qDebug() << "***log" << _data->_message;
      Log::log(_data->_message, _data->_severity, _data->_task, _data->_execId,
               _data->_sourceCode);
      delete _data;
    }
  }
  inline LogHelper &operator <<(const QString &o) {
    _data->_message.append(o); return *this; }
  inline LogHelper &operator <<(const QLatin1String &o) {
    _data->_message.append(o); return *this; }
  inline LogHelper &operator <<(const QStringRef &o) {
    _data->_message.append(o); return *this; }
  inline LogHelper &operator <<(const QByteArray &o) {
    _data->_message.append(o); return *this; }
  inline LogHelper &operator <<(const QChar &o) {
    _data->_message.append(o); return *this; }
  inline LogHelper &operator <<(const char *o) {
    _data->_message.append(o); return *this; }
  inline LogHelper &operator <<(char o) {
    _data->_message.append(o); return *this; }
  inline LogHelper &operator <<(qint64 o) {
    _data->_message.append(QString::number(o)); return *this; }
  inline LogHelper &operator <<(quint64 o) {
    _data->_message.append(QString::number(o)); return *this; }
  inline LogHelper &operator <<(qint32 o) {
    _data->_message.append(QString::number(o)); return *this; }
  inline LogHelper &operator <<(quint32 o) {
    _data->_message.append(QString::number(o)); return *this; }
  inline LogHelper &operator <<(double o) {
    _data->_message.append(QString::number(o)); return *this; }
  inline LogHelper &operator <<(const QVariant &o) {
    _data->_message.append(o.toString()); return *this; }
  inline LogHelper &operator <<(const QStringList &o) {
    _data->_message.append("{ ");
    foreach (const QString &s, o)
      _data->_message.append("\"").append(s).append("\" ");
    _data->_message.append("}");
    return *this; }
  inline LogHelper &operator <<(const QObject *o) {
    if (o)
      _data->_message.append(o->objectName()).append("(0x")
          .append(QString::number((quint64)o, 16)).append(")");
    else
      _data->_message.append("null");
    return *this; }
  inline LogHelper &operator <<(const QObject &o) {
    return operator <<(&o); }
  inline LogHelper &operator <<(const void *o) {
    _data->_message.append("0x").append(QString::number((quint64)o, 16));
    return *this; }
};

LogHelper Log::debug(const QString task, const QString execId,
                     const QString sourceCode) {
  return LogHelper(Log::Debug, task, execId, sourceCode);
}

LogHelper Log::info(const QString task, const QString execId,
                     const QString sourceCode) {
  return LogHelper(Log::Info, task, execId, sourceCode);
}

LogHelper Log::warning(const QString task, const QString execId,
                     const QString sourceCode) {
  return LogHelper(Log::Warning, task, execId, sourceCode);
}

LogHelper Log::error(const QString task, const QString execId,
                     const QString sourceCode) {
  return LogHelper(Log::Error, task, execId, sourceCode);
}

LogHelper Log::fatal(const QString task, const QString execId,
                     const QString sourceCode) {
  return LogHelper(Log::Fatal, task, execId, sourceCode);
}

LogHelper Log::debug(const QString task, quint64 execId,
                     const QString sourceCode) {
  return debug(task, QString::number(execId), sourceCode);
}

LogHelper Log::info(const QString task, quint64 execId,
                    const QString sourceCode) {
  return info(task, QString::number(execId), sourceCode);
}

LogHelper Log::warning(const QString task, quint64 execId,
                       const QString sourceCode) {
  return warning(task, QString::number(execId), sourceCode);
}

LogHelper Log::error(const QString task, quint64 execId,
                     const QString sourceCode) {
  return error(task, QString::number(execId), sourceCode);
}

LogHelper Log::fatal(const QString task, quint64 execId,
                     const QString sourceCode) {
  return fatal(task, QString::number(execId), sourceCode);
}

#endif // LOG_H
