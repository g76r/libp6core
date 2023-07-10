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
#ifndef LOG_H
#define LOG_H

#include <type_traits>
#include <QString>
#include <QtDebug>
#include <QVariant>
#include <QStringList>
#include "libp6core_global.h"
#include "util/utf8string.h"

class Log;
class Logger;
class LogHelper;

class LIBP6CORESHARED_EXPORT LogContext {
  friend class Log;
  friend class Logger;
  friend class LogHelper;
  Utf8String _taskid, _execid, _sourcecode;
public:
  LogContext(Utf8String taskid = {}, Utf8String execid = {},
             Utf8String sourcecode = {})
    : _taskid(taskid), _execid(execid), _sourcecode(sourcecode) {}
  LogContext(Utf8String taskid, quint64 execid,
             Utf8String sourcecode = {})
    : _taskid(taskid), _execid(QByteArray::number(execid)),
      _sourcecode(sourcecode) {}
  LogContext(quint64 execid, Utf8String sourcecode = {})
    : _execid(QByteArray::number(execid)), _sourcecode(sourcecode) {}
};

Q_DECLARE_METATYPE(LogContext)

/** This class provides a server-side log facility with common server-side
  * severities (whereas QtDebug does not) and write timestamped log files.
  * All public methods are threadsafe.
  */
class LIBP6CORESHARED_EXPORT Log {
public:
  enum Severity { Debug, Info, Warning, Error, Fatal };
  /** Add a new logger.
   * Takes the ownership of the logger (= will delete it).
   *
   * Autoremovable loggers are loggers that will be automaticaly removed by
   * methods such replaceLoggers(). It is convenient to mark as autoremovable
   * the user-defined loggers and as non-autoremovable some hard-wired base
   * loggers such as a ConsoleLogger, that way replaceLoggers() is able to
   * change configuration on the fly without loosing any log entry on the
   * hard-wired loggers and the configuration code does not have to recreate/
   * remember the hard-wired loggers. */
  static void addLogger(Logger *logger, bool autoRemovable);
  /** Remove a logger (and delete it), even if it is not autoremovable. */
  static void removeLogger(Logger *logger);
  /** Wrap Qt's log framework to make its output look like Log one. */
  static void wrapQtLogToSamePattern(bool enable = true);
  /** Add a logger to stdout. */
  static void addConsoleLogger(Log::Severity severity = Log::Warning,
                               bool autoRemovable = false);
  /** Remove loggers that are autoremovable and replace them with a new one. */
  static void replaceLoggers(Logger *newLogger);
  /** Remove loggers that are autoremovable and replace them with new ones. */
  static void replaceLoggers(QList<Logger*> newLoggers);
  /** Remove loggers that are autoremovable and replace them with new ones
   * plus a console logger.*/
  static void replaceLoggersPlusConsole(
      Log::Severity consoleLoggerSeverity, QList<Logger*> newLoggers);
  /** init log engine */
  static void init();
  /** flush remove any logger */
  static void shutdown();
  static void log(Utf8String message, Severity severity = Info,
                  Utf8String taskid = {}, Utf8String execid = {},
                  Utf8String sourcecode = {});
  static Utf8String severityToString(Severity severity);
  /** Very tolerant severity mnemonic reader.
   * Read first character of string in a case insensitive manner, e.g.
   * "W", "warn", "warning", and "war against terror" are all interpreted
   * as Log::Warning.
   * Unmatched letters ar interpreted as Log::Debug, therefore "D", "Debug",
   * or even "Global Warming" and "" are all interpreted as Log::Debug. */
  static Log::Severity severityFromString(Utf8String string);
  static inline LogHelper log(
      Log::Severity severity, Utf8String task = {}, Utf8String execId = {},
      Utf8String sourceCode = {});
  static inline LogHelper debug(
      Utf8String task = {}, Utf8String execId = {}, Utf8String sourceCode = {});
  static inline LogHelper info(
      Utf8String task = {}, Utf8String execId = {}, Utf8String sourceCode = {});
  static inline LogHelper warning(
      Utf8String task = {}, Utf8String execId = {}, Utf8String sourceCode = {});
  static inline LogHelper error(
      Utf8String task = {}, Utf8String execId = {}, Utf8String sourceCode = {});
  static inline LogHelper fatal(
      Utf8String task = {}, Utf8String execId = {}, Utf8String sourceCode = {});
  static inline LogHelper log(
      Log::Severity severity, Utf8String task, quint64 execId,
      Utf8String sourceCode = {});
  static inline LogHelper debug(
      Utf8String task, quint64 execId, Utf8String sourceCode = {});
  static inline LogHelper info(
      Utf8String task, quint64 execId, Utf8String sourceCode = {});
  static inline LogHelper warning(
      Utf8String task, quint64 execId, Utf8String sourceCode = {});
  static inline LogHelper error(
      Utf8String task, quint64 execId, Utf8String sourceCode = {});
  static inline LogHelper fatal(
      Utf8String task, quint64 execId, Utf8String sourceCode = {});
  static inline LogHelper debug(
      Utf8String task, qint64 execId, Utf8String sourceCode = {});
  static inline LogHelper info(
      Utf8String task, qint64 execId, Utf8String sourceCode = {});
  static inline LogHelper warning(
      Utf8String task, qint64 execId, Utf8String sourceCode = {});
  static inline LogHelper error(
      Utf8String task, qint64 execId, Utf8String sourceCode = {});
  static inline LogHelper fatal(
      Utf8String task, qint64 execId, Utf8String sourceCode = {});
  static inline LogHelper debug(quint64 execId, Utf8String sourceCode = {});
  static inline LogHelper info(quint64 execId, Utf8String sourceCode = {});
  static inline LogHelper warning(quint64 execId, Utf8String sourceCode = {});
  static inline LogHelper error(quint64 execId, Utf8String sourceCode = {});
  static inline LogHelper fatal(quint64 execId, Utf8String sourceCode = {});
  static inline LogHelper debug(qint64 execId, Utf8String sourceCode = {});
  static inline LogHelper info(qint64 execId, Utf8String sourceCode = {});
  static inline LogHelper warning(qint64 execId, Utf8String sourceCode = {});
  static inline LogHelper error(qint64 execId, Utf8String sourceCode = {});
  static inline LogHelper fatal(qint64 execId, Utf8String sourceCode = {});
  static QString pathToLastFullestLog();
  static QStringList pathsToFullestLogs();
  static QStringList pathsToAllLogs();

private:
  Log() { }
};

Q_DECLARE_METATYPE(Log::Severity)

class LIBP6CORESHARED_EXPORT LogHelper {
  mutable bool _logOnDestroy;
  Log::Severity _severity;
  Utf8String _message, _taskid, _execid, _sourceCode;

public:
  inline LogHelper(Log::Severity severity, Utf8String taskid, Utf8String execid,
                   Utf8String sourceCode)
    : _logOnDestroy(true), _severity(severity), _taskid(taskid),
      _execid(execid), _sourceCode(sourceCode) { }
  // The following copy constructor is needed because static Log::*() methods
  // return LogHelper by value. It must never be called in another context,
  // especially because it is not thread-safe.
  // Compilers are likely not to use the copy constructor at all, for instance
  // GCC won't use it but if it is called with -fno-elide-constructors option.
  inline LogHelper(const LogHelper &other)
    : _logOnDestroy(true), _severity(other._severity), _message(other._message),
      _taskid(other._taskid), _execid(other._execid),
      _sourceCode(other._sourceCode) {
    other._logOnDestroy = false;
    //qDebug() << "### copying LogHelper" << _message;
  }
  inline ~LogHelper() {
    if (_logOnDestroy) {
      //qDebug() << "***log" << _message;
      Log::log(_message, _severity, _taskid, _execid, _sourceCode);
    }
  }
  inline LogHelper &operator<<(const Utf8String &o) {
    _message.append(o); return *this; }
  inline LogHelper &operator<<(const QByteArray &o) { // disambiguation
    _message.append(Utf8String(o)); return *this; }
  inline LogHelper &operator<<(const QString &o) { // disambiguation
    _message.append(Utf8String(o)); return *this; }
  inline LogHelper &operator<<(const QLatin1StringView &o) { // disambiguation
    _message.append(Utf8String(o)); return *this; }
  inline LogHelper &operator<<(const char *o) { // disambiguation
    _message.append(Utf8String(o)); return *this; }
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  inline LogHelper &operator<<(T o) { // making cstr explicit
    _message.append(Utf8String(o)); return *this; }
  inline LogHelper &operator<<(bool o) { // making cstr explicit
    _message.append(Utf8String(o)); return *this; }
  inline LogHelper &operator<<(const QVariant &o) { // making cstr explicit
    _message.append(Utf8String(o)); return *this; }
  inline LogHelper &operator<<(const QList<QByteArray> &o) {
    _message += "{ "_ba;
    for (auto ba: o)
      _message += '"' + ba.replace('\\', "\\\\"_ba)
          .replace('"', "\\\""_ba) + "\" "_ba;
    _message += "}"_ba;
    return *this; }
  inline LogHelper &operator<<(const QList<QString> &o) {
    _message += "{ "_ba;
    for (auto s: o)
      _message += '"' + s.toUtf8().replace('\\', "\\\\"_ba)
          .replace('"', "\\\""_ba) + "\" "_ba;
    _message += "}"_ba;
    return *this; }
  inline LogHelper &operator<<(const QList<bool> &o) {
    _message += "{ "_ba;
    for (auto b: o)
      _message += b ? "true "_ba : "false "_ba;
    _message += "}"_ba;
    return *this; }
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  inline LogHelper &operator<<(const QList<T> &o) {
    _message += "{ "_ba;
    for (auto i: o)
      _message += QByteArray::number(i) + ' ';
    _message += "}"_ba;
    return *this; }
  inline LogHelper &operator<<(const QSet<QByteArray> &o) {
    _message += "{ "_ba;
    for (auto ba: o)
      _message += '"' + ba.replace('\\', "\\\\"_ba)
          .replace('"', "\\\""_ba) + "\" "_ba;
    _message += "}"_ba;
    return *this; }
  inline LogHelper &operator<<(const QSet<QString> &o) {
    _message += "{ "_ba;
    for (auto s: o)
      _message += '"' + s.toUtf8().replace('\\', "\\\\"_ba)
          .replace('"', "\\\""_ba) + "\" "_ba;
    _message += "}"_ba;
    return *this; }
  inline LogHelper &operator<<(const QSet<bool> &o) {
    _message += "{ "_ba;
    for (auto b: o)
      _message += b ? "true "_ba : "false "_ba;
    _message += "}"_ba;
    return *this; }
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  inline LogHelper &operator<<(const QSet<T> &o) {
    _message += "{ "_ba;
    for (auto i: o)
      _message += QByteArray::number(i) + ' ';
    _message += "}"_ba;
    return *this; }
  inline LogHelper &operator<<(const QObject *o) {
    const QMetaObject *mo = o ? o->metaObject() : 0;
    if (mo)
      _message += mo->className() + "(0x"_ba
          + QByteArray::number(reinterpret_cast<qintptr>(o), 16) + ", \""_ba
          + o->objectName().toUtf8() + "\")"_ba;
    else
      _message += "QObject(0x0)"_ba;
    return *this; }
  inline LogHelper &operator<<(const QObject &o) {
    return operator<<(&o); }
  inline LogHelper &operator<<(const void *o) {
    _message += "0x"_ba + QByteArray::number((qintptr)o, 16);
    return *this; }
};

LogHelper Log::log(Log::Severity severity, Utf8String task,
                   Utf8String execId, Utf8String sourceCode) {
  return LogHelper(severity, task, execId, sourceCode);
}

LogHelper Log::debug(Utf8String task, Utf8String execId, Utf8String sourceCode) {
  return LogHelper(Log::Debug, task, execId, sourceCode);
}

LogHelper Log::info(Utf8String task, Utf8String execId, Utf8String sourceCode) {
  return LogHelper(Log::Info, task, execId, sourceCode);
}

LogHelper Log::warning(
    Utf8String task, Utf8String execId, Utf8String sourceCode) {
  return LogHelper(Log::Warning, task, execId, sourceCode);
}

LogHelper Log::error(Utf8String task, Utf8String execId, Utf8String sourceCode) {
  return LogHelper(Log::Error, task, execId, sourceCode);
}

LogHelper Log::fatal(
    Utf8String task, Utf8String execId, Utf8String sourceCode) {
  return LogHelper(Log::Fatal, task, execId, sourceCode);
}

LogHelper Log::log(Log::Severity severity, Utf8String task, quint64 execId,
                   Utf8String sourceCode) {
  return log(severity, task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::debug(Utf8String task, quint64 execId, Utf8String sourceCode) {
  return debug(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::info(Utf8String task, quint64 execId, Utf8String sourceCode) {
  return info(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::warning(Utf8String task, quint64 execId, Utf8String sourceCode) {
  return warning(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::error(Utf8String task, quint64 execId, Utf8String sourceCode) {
  return error(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::fatal(Utf8String task, quint64 execId, Utf8String sourceCode) {
  return fatal(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::debug(Utf8String task, qint64 execId, Utf8String sourceCode) {
  return debug(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::info(Utf8String task, qint64 execId, Utf8String sourceCode) {
  return info(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::warning(Utf8String task, qint64 execId, Utf8String sourceCode) {
  return warning(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::error(Utf8String task, qint64 execId, Utf8String sourceCode) {
  return error(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::fatal(Utf8String task, qint64 execId, Utf8String sourceCode) {
  return fatal(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::debug(quint64 execId, Utf8String sourceCode) {
  return debug(Utf8String(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::info(quint64 execId, Utf8String sourceCode) {
  return info(Utf8String(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::warning(quint64 execId, Utf8String sourceCode) {
  return warning(Utf8String(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::error(quint64 execId, Utf8String sourceCode) {
  return error(Utf8String(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::fatal(quint64 execId,Utf8String sourceCode) {
  return fatal(Utf8String(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::debug(qint64 execId, Utf8String sourceCode) {
  return debug(Utf8String(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::info(qint64 execId, Utf8String sourceCode) {
  return info(Utf8String(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::warning(qint64 execId, Utf8String sourceCode) {
  return warning(Utf8String(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::error(qint64 execId, Utf8String sourceCode) {
  return error(Utf8String(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::fatal(qint64 execId,Utf8String sourceCode) {
  return fatal(Utf8String(), QByteArray::number(execId), sourceCode);
}

#endif // LOG_H
