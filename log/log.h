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

#include "util/utf8stringlist.h"

#ifndef LOG_LOCATION_ENABLED
#  if __has_include(<source_location>)
#    include <source_location>
using std::source_location;
#    define LOG_LOCATION_ENABLED 1
#  elif __has_include(<experimental/source_location>)
#    include <experimental/source_location>
using std::experimental::source_location;
#    define LOG_LOCATION_ENABLED 2
#  else
#    define LOG_LOCATION_ENABLED 0
#  endif
#endif // LOG_LOCATION_ENABLED

class Logger;
class LogHelper;

class LIBP6CORESHARED_EXPORT LogContext {
  Utf8String _taskid, _execid, _location;
  Utf8String field(Utf8String input, Utf8String def) {
    auto output = input;
    if (output.isEmpty())
      return def;
    for (char *s = output.data(); *s; ++s)
      if (::isspace(*s)) [[unlikely]]
        *s = '_';
    return output;
  }
  static Utf8String current_thread_name();

public:
#if LOG_LOCATION_ENABLED
  LogContext(Utf8String taskid = {}, Utf8String execid = {},
             source_location location = {})
    : _taskid(field(taskid.isEmpty() ? current_thread_name() : taskid, "?"_u8)),
      _execid(field(execid, "0"_u8)),
#if LOG_LOCATION_WITH_FUNCTION_ENABLED
      _location(location.file_name()+":"_u8+QByteArray::number(location.line())
                  +":"_u8+location.function_name()) { }
#else
      _location(location.file_name()+":"_u8+QByteArray::number(
                  location.line())) { }
#endif
  LogContext(Utf8String taskid, quint64 execid,
             source_location location = {})
    : LogContext(taskid, QByteArray::number(execid), location) {}
  LogContext(quint64 execid, source_location location = {})
    : LogContext({}, QByteArray::number(execid), location) {}
  LogContext(Utf8String taskid, qint64 execid,
             source_location location = {})
    : LogContext(taskid, QByteArray::number(execid), location) {}
  LogContext(qint64 execid, source_location location = {})
    : LogContext({}, QByteArray::number(execid), location) {}
#else
  LogContext(Utf8String taskid = {}, Utf8String execid = {})
    : _taskid(field(taskid.isEmpty() ? current_thread_name() : taskid, "?"_u8)),
      _execid(field(execid, "0"_u8)),
      _location(":"_u8) { }
  LogContext(Utf8String taskid, quint64 execid)
    : LogContext(taskid, QByteArray::number(execid)) {}
  LogContext(quint64 execid)
    : LogContext({}, QByteArray::number(execid)) {}
  LogContext(Utf8String taskid, qint64 execid)
    : LogContext(taskid, QByteArray::number(execid)) {}
  LogContext(qint64 execid)
    : LogContext({}, QByteArray::number(execid)) {}
#endif
  Utf8String taskid() const { return _taskid; }
  Utf8String execid() const { return _execid; }
  Utf8String location() const { return _location; }
};

Q_DECLARE_METATYPE(LogContext)
Q_DECLARE_TYPEINFO(LogContext, Q_MOVABLE_TYPE);

/** This class provides a server-side log facility with common server-side
  * severities (whereas QtDebug does not) and write timestamped log files.
  * All public methods are threadsafe.
  */
class LIBP6CORESHARED_EXPORT Log {
public:
  enum Severity : signed char { Debug, Info, Warning, Error, Fatal };
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
  static void log(Utf8String message, Severity severity, LogContext context);
#if LOG_LOCATION_ENABLED
  static inline void log(
      Utf8String message, Severity severity, Utf8String taskid = {},
      Utf8String execid = {},
      source_location location = source_location::current()) {
    log(message, severity, {taskid, execid, location});
  }
#else
  static inline void log(
      Utf8String message, Severity severity, Utf8String taskid = {},
      Utf8String execid = {}) {
    log(message, severity, {taskid, execid});
  }
#endif // LOG_LOCATION_ENABLED
  static Utf8String severityToString(Severity severity);
  /** Very tolerant severity mnemonic reader.
   * Read first character of string in a case insensitive manner, e.g.
   * "W", "warn", "warning", and "war against terror" are all interpreted
   * as Log::Warning.
   * Unmatched letters ar interpreted as Log::Debug, therefore "D", "Debug",
   * or even "Global Warming" and "" are all interpreted as Log::Debug. */
  static Log::Severity severityFromString(Utf8String string);
  static inline LogHelper log(Log::Severity severity, LogContext context);
  static inline LogHelper debug(LogContext context);
  static inline LogHelper info(LogContext context);
  static inline LogHelper warning(LogContext context);
  static inline LogHelper error(LogContext context);
  static inline LogHelper fatal(LogContext context);
#if LOG_LOCATION_ENABLED
  static inline LogHelper log(
      Log::Severity severity, Utf8String taskid = {}, Utf8String execid = {},
      source_location location = source_location::current());
  static inline LogHelper debug(
      Utf8String taskid = {}, Utf8String execid = {},
      source_location location = source_location::current());
  static inline LogHelper info(
      Utf8String taskid = {}, Utf8String execid = {},
      source_location location = source_location::current());
  static inline LogHelper warning(
      Utf8String taskid = {}, Utf8String execid = {},
      source_location location = source_location::current());
  static inline LogHelper error(
      Utf8String taskid = {}, Utf8String execid = {},
      source_location location = source_location::current());
  static inline LogHelper fatal(
      Utf8String taskid = {}, Utf8String execid = {},
      source_location location = source_location::current());
  static inline LogHelper log(
      Log::Severity severity, Utf8String taskid, quint64 execid,
      source_location location = source_location::current());
  static inline LogHelper debug(
      Utf8String taskid, quint64 execid,
      source_location location = source_location::current());
  static inline LogHelper info(
      Utf8String taskid, quint64 execid,
      source_location location = source_location::current());
  static inline LogHelper warning(
      Utf8String taskid, quint64 execid,
      source_location location = source_location::current());
  static inline LogHelper error(
      Utf8String taskid, quint64 execid,
      source_location location = source_location::current());
  static inline LogHelper fatal(
      Utf8String taskid, quint64 execid,
      source_location location = source_location::current());
  static inline LogHelper debug(
      Utf8String taskid, qint64 execid,
      source_location location = source_location::current());
  static inline LogHelper info(
      Utf8String taskid, qint64 execid,
      source_location location = source_location::current());
  static inline LogHelper warning(
      Utf8String taskid, qint64 execid,
      source_location location = source_location::current());
  static inline LogHelper error(
      Utf8String taskid, qint64 execid,
      source_location location = source_location::current());
  static inline LogHelper fatal(
      Utf8String taskid, qint64 execid,
      source_location location = source_location::current());
  static inline LogHelper debug(
      quint64 execid,
      source_location location = source_location::current());
  static inline LogHelper info(
      quint64 execid,
      source_location location = source_location::current());
  static inline LogHelper warning(
      quint64 execid,
      source_location location = source_location::current());
  static inline LogHelper error(
      quint64 execid,
      source_location location = source_location::current());
  static inline LogHelper fatal(
      quint64 execid,
      source_location location = source_location::current());
  static inline LogHelper debug(
      qint64 execid,
      source_location location = source_location::current());
  static inline LogHelper info(
      qint64 execid,
      source_location location = source_location::current());
  static inline LogHelper warning(
      qint64 execid,
      source_location location = source_location::current());
  static inline LogHelper error(
      qint64 execid,
      source_location location = source_location::current());
  static inline LogHelper fatal(
      qint64 execid,
      source_location location = source_location::current());
#else
  static inline LogHelper log(
      Log::Severity severity, Utf8String taskid = {}, Utf8String execid = {});
  static inline LogHelper debug(
      Utf8String taskid = {}, Utf8String execid = {});
  static inline LogHelper info(
      Utf8String taskid = {}, Utf8String execid = {});
  static inline LogHelper warning(
      Utf8String taskid = {}, Utf8String execid = {});
  static inline LogHelper error(
      Utf8String taskid = {}, Utf8String execid = {});
  static inline LogHelper fatal(
      Utf8String taskid = {}, Utf8String execid = {});
  static inline LogHelper log(
      Log::Severity severity, Utf8String taskid, quint64 execid);
  static inline LogHelper debug(Utf8String taskid, quint64 execid);
  static inline LogHelper info(Utf8String taskid, quint64 execid);
  static inline LogHelper warning(Utf8String taskid, quint64 execid);
  static inline LogHelper error(Utf8String taskid, quint64 execid);
  static inline LogHelper fatal(Utf8String taskid, quint64 execid);
  static inline LogHelper debug(Utf8String taskid, qint64 execid);
  static inline LogHelper info(Utf8String taskid, qint64 execid);
  static inline LogHelper warning(Utf8String taskid, qint64 execid);
  static inline LogHelper error(Utf8String taskid, qint64 execid);
  static inline LogHelper fatal(Utf8String taskid, qint64 execid);
  static inline LogHelper debug(quint64 execid);
  static inline LogHelper info(quint64 execid);
  static inline LogHelper warning(quint64 execid);
  static inline LogHelper error(quint64 execid);
  static inline LogHelper fatal(quint64 execid);
  static inline LogHelper debug(qint64 execid);
  static inline LogHelper info(qint64 execid);
  static inline LogHelper warning(qint64 execid);
  static inline LogHelper error(qint64 execid);
  static inline LogHelper fatal(qint64 execid);
#endif // LOG_LOCATION_ENABLED
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
  Utf8String _message;
  LogContext _context;

public:
  inline LogHelper(Log::Severity severity, LogContext context)
    : _logOnDestroy(true), _severity(severity), _context(context) { }
  // The following copy constructor is needed because static Log::*() methods
  // return LogHelper by value. It must never be called in another context,
  // especially because it is not thread-safe.
  // Compilers are likely not to use the copy constructor at all, for instance
  // GCC won't use it but if it is called with -fno-elide-constructors option.
  inline LogHelper(const LogHelper &other)
    : _logOnDestroy(true), _severity(other._severity), _message(other._message),
      _context(other._context) {
    other._logOnDestroy = false;
    //qDebug() << "### copying LogHelper" << _message;
  }
  inline ~LogHelper() {
    if (_logOnDestroy) {
      //qDebug() << "***log" << _message;
      Log::log(_message, _severity, _context);
    }
  }
  inline LogHelper &operator<<(const Utf8String &o) {
    _message += o; return *this; }
  inline LogHelper &operator<<(const QByteArray &o) { // disambiguation
    _message += o; return *this; }
  inline LogHelper &operator<<(const QString &o) { // disambiguation
    _message += o; return *this; }
  inline LogHelper &operator<<(const QLatin1StringView &o) { // disambiguation
    _message += QString(o); return *this; }
  inline LogHelper &operator<<(const char *o) { // disambiguation
    _message += o; return *this; }
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  inline LogHelper &operator<<(T o) { // making cstr explicit
    _message += Utf8String(o); return *this; }
  inline LogHelper &operator<<(bool o) { // making cstr explicit
    _message += Utf8String(o); return *this; }
  inline LogHelper &operator<<(const QVariant &o) { // making cstr explicit
    _message += Utf8String(o); return *this; }
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

LogHelper Log::log(Log::Severity severity, LogContext context) {
  return LogHelper(severity, context);
}

LogHelper Log::debug(LogContext context) {
  return LogHelper(Log::Debug, context);
}

LogHelper Log::info(LogContext context) {
  return LogHelper(Log::Info, context);
}

LogHelper Log::warning(LogContext context) {
  return LogHelper(Log::Warning, context);
}

LogHelper Log::error(LogContext context) {
  return LogHelper(Log::Error, context);
}

LogHelper Log::fatal(LogContext context) {
  return LogHelper(Log::Fatal, context);
}

#if LOG_LOCATION_ENABLED
LogHelper Log::log(Log::Severity severity, Utf8String taskid,
                   Utf8String execid, source_location location) {
  return LogHelper(severity, { taskid, execid, location });
}

LogHelper Log::debug(Utf8String taskid, Utf8String execid,
                     source_location location) {
  return LogHelper(Log::Debug, { taskid, execid, location });
}

LogHelper Log::info(Utf8String taskid, Utf8String execid,
                    source_location location) {
  return LogHelper(Log::Info, { taskid, execid, location });
}

LogHelper Log::warning(
    Utf8String taskid, Utf8String execid,
    source_location location) {
  return LogHelper(Log::Warning, { taskid, execid, location });
}

LogHelper Log::error(Utf8String taskid, Utf8String execid,
                     source_location location) {
  return LogHelper(Log::Error, { taskid, execid, location });
}

LogHelper Log::fatal(
    Utf8String taskid, Utf8String execid, source_location location) {
  return LogHelper(Log::Fatal, { taskid, execid, location });
}

LogHelper Log::log(Log::Severity severity, Utf8String taskid, quint64 execid,
                   source_location location) {
  return LogHelper(severity, { taskid, execid, location });
}

LogHelper Log::debug(Utf8String taskid, quint64 execid,
                     source_location location) {
  return LogHelper(Log::Debug, { taskid, execid, location });
}

LogHelper Log::info(Utf8String taskid, quint64 execid,
                    source_location location) {
  return LogHelper(Log::Info, { taskid, execid, location });
}

LogHelper Log::warning(Utf8String taskid, quint64 execid,
                       source_location location) {
  return LogHelper(Log::Warning, { taskid, execid, location });
}

LogHelper Log::error(Utf8String taskid, quint64 execid,
                     source_location location) {
  return LogHelper(Log::Error, { taskid, execid, location });
}

LogHelper Log::fatal(Utf8String taskid, quint64 execid,
                     source_location location) {
  return LogHelper(Log::Fatal, { taskid, execid, location });
}

LogHelper Log::debug(Utf8String taskid, qint64 execid,
                     source_location location) {
  return LogHelper(Log::Debug, { taskid, execid, location });
}

LogHelper Log::info(Utf8String taskid, qint64 execid,
                    source_location location) {
  return LogHelper(Log::Info, { taskid, execid, location });
}

LogHelper Log::warning(Utf8String taskid, qint64 execid,
                       source_location location) {
  return LogHelper(Log::Warning, { taskid, execid, location });
}

LogHelper Log::error(Utf8String taskid, qint64 execid,
                     source_location location) {
  return LogHelper(Log::Error, { taskid, execid, location });
}

LogHelper Log::fatal(Utf8String taskid, qint64 execid,
                     source_location location) {
  return LogHelper(Log::Fatal, { taskid, execid, location });
}

LogHelper Log::debug(quint64 execid, source_location location) {
  return LogHelper(Log::Debug, { execid, location });
}

LogHelper Log::info(quint64 execid, source_location location) {
  return LogHelper(Log::Info, { execid, location });
}

LogHelper Log::warning(quint64 execid, source_location location) {
  return LogHelper(Log::Warning, { execid, location });
}

LogHelper Log::error(quint64 execid, source_location location) {
  return LogHelper(Log::Error, { execid, location });
}

LogHelper Log::fatal(quint64 execid, source_location location) {
  return LogHelper(Log::Fatal, { execid, location });
}

LogHelper Log::debug(qint64 execid, source_location location) {
  return LogHelper(Log::Debug, { execid, location });
}

LogHelper Log::info(qint64 execid, source_location location) {
  return LogHelper(Log::Info, { execid, location });
}

LogHelper Log::warning(qint64 execid, source_location location) {
  return LogHelper(Log::Warning, { execid, location });
}

LogHelper Log::error(qint64 execid, source_location location) {
  return LogHelper(Log::Error, { execid, location });
}

LogHelper Log::fatal(qint64 execid, source_location location) {
  return LogHelper(Log::Fatal, { execid, location });
}

#else

LogHelper Log::log(Log::Severity severity, Utf8String taskid, Utf8String execid) {
  return LogHelper(severity, { taskid, execid });
}

LogHelper Log::debug(Utf8String taskid, Utf8String execid) {
  return LogHelper(Log::Debug, { taskid, execid });
}

LogHelper Log::info(Utf8String taskid, Utf8String execid) {
  return LogHelper(Log::Info, { taskid, execid });
}

LogHelper Log::warning(Utf8String taskid, Utf8String execid) {
  return LogHelper(Log::Warning, { taskid, execid });
}

LogHelper Log::error(Utf8String taskid, Utf8String execid) {
  return LogHelper(Log::Error, { taskid, execid });
}

LogHelper Log::fatal(Utf8String taskid, Utf8String execid) {
  return LogHelper(Log::Fatal, { taskid, execid });
}

LogHelper Log::log(Log::Severity severity, Utf8String taskid, quint64 execid) {
  return LogHelper(severity, { taskid, execid });
}

LogHelper Log::debug(Utf8String taskid, quint64 execid) {
  return LogHelper(Log::Debug, { taskid, execid });
}

LogHelper Log::info(Utf8String taskid, quint64 execid) {
  return LogHelper(Log::Info, { taskid, execid });
}

LogHelper Log::warning(Utf8String taskid, quint64 execid) {
  return LogHelper(Log::Warning, { taskid, execid });
}

LogHelper Log::error(Utf8String taskid, quint64 execid) {
  return LogHelper(Log::Error, { taskid, execid });
}

LogHelper Log::fatal(Utf8String taskid, quint64 execid) {
  return LogHelper(Log::Fatal, { taskid, execid });
}

LogHelper Log::debug(Utf8String taskid, qint64 execid) {
  return LogHelper(Log::Debug, { taskid, execid });
}

LogHelper Log::info(Utf8String taskid, qint64 execid) {
  return LogHelper(Log::Info, { taskid, execid });
}

LogHelper Log::warning(Utf8String taskid, qint64 execid) {
  return LogHelper(Log::Warning, { taskid, execid });
}

LogHelper Log::error(Utf8String taskid, qint64 execid) {
  return LogHelper(Log::Error, { taskid, execid });
}

LogHelper Log::fatal(Utf8String taskid, qint64 execid) {
  return LogHelper(Log::Fatal, { taskid, execid });
}

LogHelper Log::debug(quint64 execid) {
  return LogHelper(Log::Debug, { execid });
}

LogHelper Log::info(quint64 execid) {
  return LogHelper(Log::Info, { execid });
}

LogHelper Log::warning(quint64 execid) {
  return LogHelper(Log::Warning, { execid });
}

LogHelper Log::error(quint64 execid) {
  return LogHelper(Log::Error, { execid });
}

LogHelper Log::fatal(quint64 execid) {
  return LogHelper(Log::Fatal, { execid });
}

LogHelper Log::debug(qint64 execid) {
  return LogHelper(Log::Debug, { execid });
}

LogHelper Log::info(qint64 execid) {
  return LogHelper(Log::Info, { execid });
}

LogHelper Log::warning(qint64 execid) {
  return LogHelper(Log::Warning, { execid });
}

LogHelper Log::error(qint64 execid) {
  return LogHelper(Log::Error, { execid });
}

LogHelper Log::fatal(qint64 execid) {
  return LogHelper(Log::Fatal, { execid });
}

#endif // LOG_LOCATION_ENABLED

#endif // LOG_H
