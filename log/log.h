/* Copyright 2012-2025 Hallowyn, Gregoire Barbier and others.
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

namespace p6::log {

class Logger;
class LogHelper;

enum Severity : signed char {
  Debug = 0,
  Info = 1,
  Warning = 2,
  Error = 4,
  Fatal = 8,
};

Utf8String LIBP6CORESHARED_EXPORT severity_as_text(Severity severity);

inline Severity severity_from_text(char first_char) {
  switch (first_char) {
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

/** Very tolerant severity mnemonic reader.
 *  Read first character of string in a case insensitive manner, e.g.
 *  "W", "warn", "warning", and "war against terror" are all interpreted
 *  as Log::Warning.
 *  Unmatched letters ar interpreted as Log::Debug, therefore "D", "Debug",
 *  or even "Global Warming" and "" are all interpreted as Log::Debug.
 */
inline Severity severity_from_text(const Utf8String &text) {
  return severity_from_text(text.value(0));
}

inline Severity severity_from_text(const QString &text) {
  return severity_from_text(!text.isEmpty() ? text.at(0).toLatin1() : '\0');
}

inline Severity severity_from_text(const char *text) {
  return severity_from_text(text ? *text : '\0');
}

[[deprecated("use severity_as_text instead")]]
inline Utf8String severityToString(Severity severity) {
  return severity_as_text(severity);
}

[[deprecated("use severity_from_text instead")]]
inline Severity severityFromString(const Utf8String &text) {
  return severity_from_text(text);
}

class LIBP6CORESHARED_EXPORT Record {
protected:
  qint64 _timestamp = 0;
  Severity _severity;
  Utf8String _taskid, _execid, _location, _message;

private:
  inline static Utf8String sanitized_field(const Utf8String &input, const Utf8String &def) {
    if (input.isEmpty())
      return def;
    auto output = input;
    for (char *s = output.data(); *s; ++s)
      if (::isspace(*s))
        [[unlikely]] *s = '_';
    return output;
  }
  static Utf8String current_thread_name();
  static qint64 now();

public:
#if LOG_LOCATION_ENABLED
  Record(Severity severity, const Utf8String &taskid = {},
             const Utf8String &execid = {}, const source_location &location = {})
    : _timestamp(now()), _severity(severity),
      _taskid(sanitized_field(
                taskid.isEmpty() ? current_thread_name() : taskid, "?"_u8)),
      _execid(sanitized_field(execid, "0"_u8)),
#if LOG_LOCATION_WITH_FUNCTION_ENABLED
      _location(location.file_name()+":"_u8+Utf8String::number(location.line())
                  +":"_u8+location.function_name()) { }
#else
      _location(location.file_name()+":"_u8+Utf8String::number(
                  location.line())) { }
#endif
  Record(Severity severity, const Utf8String &taskid, quint64 execid,
             const source_location &location = {})
    : Record(severity, taskid, Utf8String::number(execid), location) {}
  Record(Severity severity, quint64 execid, const source_location &location = {})
    : Record(severity, {}, Utf8String::number(execid), location) {}
  Record(Severity severity, const Utf8String &taskid, qint64 execid,
             const source_location &location = {})
    : Record(severity, taskid, Utf8String::number(execid), location) {}
  Record(Severity severity, qint64 execid, const source_location &location = {})
    : Record(severity, {}, Utf8String::number(execid), location) {}
  Record(Severity severity, const source_location &location = {})
    : Record(severity, {}, 0ULL, location) {}
#else
  Record(Severity severity, const Utf8String &taskid = {},
             const Utf8String &execid = {})
    : _timestamp(now()), _severity(severity),
      _taskid(sanitized_field(
                taskid.isEmpty() ? current_thread_name() : taskid, "?"_u8)),
      _execid(sanitized_field(execid, "0"_u8)),
      _location(":"_u8) { }
  Record(Severity severity, const Utf8String &taskid, quint64 execid)
    : Record(severity, taskid, Utf8String::number(execid)) {}
  Record(Severity severity, quint64 execid)
    : Record(severity, {}, Utf8String::number(execid)) {}
  Record(Severity severity, const Utf8String &taskid, qint64 execid)
    : Record(severity, taskid, Utf8String::number(execid)) {}
  Record(Severity severity, qint64 execid)
    : Record(severity, {}, Utf8String::number(execid)) {}
  Record(Severity severity) : Record(severity, {}, 0ULL) {}
#endif
  Record(Severity severity, const Utf8String &taskid,
         const Utf8String &execid, const Utf8String &location)
    : _timestamp(now()), _severity(severity),
      _taskid(sanitized_field(
                taskid.isEmpty() ? current_thread_name() : taskid, "?"_u8)),
      _execid(sanitized_field(execid, "0"_u8)),
      _location(sanitized_field(location, ":"_u8)) { }
  Record() {}
  Record(const Record &other) = default;
  Record(Record &&other) = default;
  Record &operator =(const Record &other) = default;
  Record &operator =(Record &&other) = default;
  inline bool operator !() const { return !_timestamp; }
  inline qint64 timestamp() const { return _timestamp; }
  inline Utf8String taskid() const { return _taskid; }
  inline Utf8String execid() const { return _execid; }
  inline Utf8String location() const { return _location; }
  inline Severity severity() const { return _severity; }
  inline Utf8String message() const { return _message; }
  Utf8String formated_message() const;
  inline Record &set_message(const Utf8String &message) {
    _message = message; return *this; }
  inline Record &append_message(const Utf8String &suffix) {
    _message.append(suffix); return *this; }
};

/** Add a new logger.
   * Takes the ownership of the logger (= will delete it).
   *
   * Autoremovable loggers are loggers that will be automaticaly removed by
   * methods such replaceLoggers(). It is convenient to mark as autoremovable
   * the user-defined loggers and as non-autoremovable some hard-wired base
   * loggers such as a ConsoleLogger, that way replaceLoggers() is able to
   * change configuration on the fly without loosing any log record on the
   * hard-wired loggers and the configuration code does not have to recreate/
   * remember the hard-wired loggers. */
void LIBP6CORESHARED_EXPORT add_logger(Logger *logger, bool auto_removable);
/** Remove a logger (and delete it), even if it is not autoremovable. */
void LIBP6CORESHARED_EXPORT remove_logger(Logger *logger);
/** Format Qt's log framework to make its output look like p6::log one, but keep
 *  them wrote on stderr as Qt does. */
void LIBP6CORESHARED_EXPORT use_same_format_for_qt_log(bool enable = true);
/** Wrap Qt's log framework into p6::log, which make them asynchronous. */
void LIBP6CORESHARED_EXPORT wrap_qt_log(bool enable = true);
/** Add a logger to stderr (or stdout). */
void LIBP6CORESHARED_EXPORT add_console_logger(
    Severity severity = Warning, bool auto_removable = false,
    FILE *stream = stderr);
/** Remove loggers that are autoremovable and replace them with new ones.
 *  Optionaly prepend a console logger. */
void LIBP6CORESHARED_EXPORT replace_loggers(
    QList<Logger*> &new_loggers, bool prepend_console = false,
    Severity console_min_severity = Fatal);

[[deprecated("use replace_loggers instead")]]
inline void replaceLoggers(Logger *newLogger) {
  QList<Logger*> list{newLogger};
  replace_loggers(list);
}

[[deprecated("use replace_loggers instead")]]
inline void replaceLoggers(QList<Logger*> &newLoggers) {
  replace_loggers(newLoggers);
}

[[deprecated("use replace_loggers instead")]]
inline void replaceLoggersPlusConsole(
    Severity consoleLoggerSeverity, QList<Logger *> &newLoggers) {
  return replace_loggers(newLoggers, true, consoleLoggerSeverity);
}

/** init log engine */
void LIBP6CORESHARED_EXPORT init();
/** flush remove any logger */
void LIBP6CORESHARED_EXPORT shutdown();
void LIBP6CORESHARED_EXPORT log(const Record &record);
#if LOG_LOCATION_ENABLED
inline void log(
    const Utf8String &message, Severity severity, const Utf8String &taskid = {},
    const Utf8String &execid = {},
    source_location location = source_location::current()) {
  log(Record{severity, taskid, execid, location}.set_message(message));
}
#else
inline void log(
    const Utf8String &message, Severity severity, const Utf8String &taskid = {},
    const Utf8String &execid = {}) {
  log(Record{severity, taskid, execid}.set_message(message));
}
#endif // LOG_LOCATION_ENABLED

QString LIBP6CORESHARED_EXPORT pathToLastFullestLog();
QStringList LIBP6CORESHARED_EXPORT pathsToFullestLogs();
QStringList LIBP6CORESHARED_EXPORT pathsToAllLogs();

class LIBP6CORESHARED_EXPORT LogHelper : public Record {
  mutable bool _log_on_destroy;

public:
  inline LogHelper(const Record &record)
    : Record(record), _log_on_destroy(true) { }
  // The following copy constructor is needed because log,debug...() methods
  // return LogHelper by value. It must never be called in another record,
  // especially because it is not thread-safe.
  // Compilers are likely not to use the copy constructor at all, for instance
  // GCC won't use it but if it is called with -fno-elide-constructors option.
  inline LogHelper(const LogHelper &other)
    : Record(other), _log_on_destroy(true) {
    other._log_on_destroy = false;
  }
  inline ~LogHelper() {
    if (_log_on_destroy)
      log(*this);
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
#ifdef __cpp_concepts
  template <p6::arithmetic T>
#else
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
#endif
  inline LogHelper &operator<<(T o) { // making cstr explicit
    _message += Utf8String(o); return *this; }
#ifdef __cpp_concepts
  template <p6::enumeration T>
#else
  template <typename T, typename = std::enable_if_t<std::is_enum_v<T>::value>>
#endif
  inline LogHelper &operator<<(T o) { // making cstr explicit
    return operator<<((std::underlying_type_t<T>)o); }
  inline LogHelper &operator<<(const QVariant &o) { // making cstr explicit
    _message += Utf8String(o); return *this; }
  inline LogHelper &operator<<(const QList<QByteArray> &o) {
    _message += "{ "_ba;
    for (auto ba: o)
      _message += '"' + ba.replace('\\', "\\\\"_ba)
          .replace('"', "\\\""_ba) + "\" "_ba;
    _message += "}"_ba;
    return *this; }
  inline LogHelper &operator<<(const QList<Utf8String> &o) {
    return operator<<((const QList<QByteArray>&)o);
  }
  inline LogHelper &operator<<(const QList<QString> &o) {
    _message += "{ "_ba;
    for (const auto &s: o)
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
#ifdef __cpp_concepts
  template <p6::arithmetic T>
#else
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
#endif
  inline LogHelper &operator<<(const QList<T> &o) {
    _message += "{ "_ba;
    for (auto i: o)
      _message += Utf8String::number(i) + ' ';
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
    for (const auto &s: o)
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
#ifdef __cpp_concepts
  template <p6::arithmetic T>
#else
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
#endif
  inline LogHelper &operator<<(const QSet<T> &o) {
    _message += "{ "_ba;
    for (auto i: o)
      _message += Utf8String::number(i) + ' ';
    _message += "}"_ba;
    return *this; }
  inline LogHelper &operator<<(const QObject *o) {
    const QMetaObject *mo = o ? o->metaObject() : 0;
    if (mo)
      _message += mo->className() + "(0x"_u8
          + Utf8String::number(reinterpret_cast<qintptr>(o), 16) + ", \""_ba
          + o->objectName().toUtf8() + "\")"_ba;
    else
      _message += "QObject(0x0)"_ba;
    return *this; }
  inline LogHelper &operator<<(const QObject &o) {
    return operator<<(&o); }
  inline LogHelper &operator<<(const void *o) {
    _message += "0x"_u8 + Utf8String::number((qintptr)o, 16);
    return *this; }
};

inline LogHelper log(Severity severity, const Utf8String &taskid,
                     const Utf8String &execid, const Utf8String &location) {
  return LogHelper({severity, taskid, execid, location});
}

inline LogHelper log(Severity severity, const Utf8String &taskid,
                     quint64 execid, const Utf8String &location) {
  return LogHelper({severity, taskid, Utf8String::number(execid), location});
}

inline LogHelper log(Severity severity, const Utf8String &taskid,
                     qint64 execid, const Utf8String &location) {
  return LogHelper({severity, taskid, Utf8String::number(execid), location});
}

#if LOG_LOCATION_ENABLED

inline LogHelper log(Severity severity, const Utf8String &taskid = {},
                     const Utf8String &execid = {},
                     source_location location = source_location::current()) {
  return LogHelper({severity, taskid, execid, location});
}

inline LogHelper debug(const Utf8String &taskid = {},
                       const Utf8String &execid = {},
                       source_location location = source_location::current()) {
  return LogHelper({Debug, taskid, execid, location});
}

inline LogHelper info(const Utf8String &taskid = {},
                      const Utf8String &execid = {},
                      source_location location = source_location::current()) {
  return LogHelper({Info, taskid, execid, location});
}

inline LogHelper warning(const Utf8String &taskid = {},
                         const Utf8String &execid = {},
                         source_location location = source_location::current()) {
  return LogHelper({Warning, taskid, execid, location});
}

inline LogHelper error(const Utf8String &taskid = {},
                       const Utf8String &execid = {},
                       source_location location = source_location::current()) {
  return LogHelper({Error, taskid, execid, location});
}

inline LogHelper fatal(const Utf8String &taskid = {},
                       const Utf8String &execid = {},
                       source_location location = source_location::current()) {
  return LogHelper({Fatal, taskid, execid, location});
}

inline LogHelper log(Severity severity, const Utf8String &taskid, qint64 execid,
                     source_location location = source_location::current()) {
  return LogHelper({severity, taskid, execid, location});
}

inline LogHelper debug(const Utf8String &taskid, qint64 execid,
                       source_location location = source_location::current()) {
  return LogHelper({Debug, taskid, execid, location});
}

inline LogHelper info(const Utf8String &taskid, qint64 execid,
                      source_location location = source_location::current()) {
  return LogHelper({Info, taskid, execid, location});
}

inline LogHelper warning(const Utf8String &taskid, qint64 execid,
                         source_location location = source_location::current()) {
  return LogHelper({Warning, taskid, execid, location});
}

inline LogHelper error(const Utf8String &taskid, qint64 execid,
                       source_location location = source_location::current()) {
  return LogHelper({Error, taskid, execid, location});
}

inline LogHelper fatal(const Utf8String &taskid, qint64 execid,
                       source_location location = source_location::current()) {
  return LogHelper({Fatal, taskid, execid, location});
}

inline LogHelper log(Severity severity, const Utf8String &taskid, quint64 execid,
                     source_location location = source_location::current()) {
  return LogHelper({severity, taskid, execid, location});
}

inline LogHelper debug(const Utf8String &taskid, quint64 execid,
                       source_location location = source_location::current()) {
  return LogHelper({Debug, taskid, execid, location});
}

inline LogHelper info(const Utf8String &taskid, quint64 execid,
                      source_location location = source_location::current()) {
  return LogHelper({Info, taskid, execid, location});
}

inline LogHelper warning(const Utf8String &taskid, quint64 execid,
                         source_location location = source_location::current()) {
  return LogHelper({Warning, taskid, execid, location});
}

inline LogHelper error(const Utf8String &taskid, quint64 execid,
                       source_location location = source_location::current()) {
  return LogHelper({Error, taskid, execid, location});
}

inline LogHelper fatal(const Utf8String &taskid, quint64 execid,
                       source_location location = source_location::current()) {
  return LogHelper({Fatal, taskid, execid, location});
}

inline LogHelper debug(qint64 execid,
                       source_location location = source_location::current()) {
  return LogHelper({Debug, {}, execid, location});
}

inline LogHelper info(qint64 execid,
                      source_location location = source_location::current()) {
  return LogHelper({Info, {}, execid, location});
}

inline LogHelper warning(qint64 execid,
                         source_location location = source_location::current()) {
  return LogHelper({Warning, {}, execid, location});
}

inline LogHelper error(qint64 execid,
                       source_location location = source_location::current()) {
  return LogHelper({Error, {}, execid, location});
}

inline LogHelper fatal(qint64 execid,
                       source_location location = source_location::current()) {
  return LogHelper({Fatal, {}, execid, location});
}

inline LogHelper debug(quint64 execid,
                       source_location location = source_location::current()) {
  return LogHelper({Debug, {}, execid, location});
}

inline LogHelper info(quint64 execid,
                      source_location location = source_location::current()) {
  return LogHelper({Info, {}, execid, location});
}

inline LogHelper warning(quint64 execid,
                         source_location location = source_location::current()) {
  return LogHelper({Warning, {}, execid, location});
}

inline LogHelper error(quint64 execid,
                       source_location location = source_location::current()) {
  return LogHelper({Error, {}, execid, location});
}

inline LogHelper fatal(quint64 execid,
                       source_location location = source_location::current()) {
  return LogHelper({Fatal, {}, execid, location});
}

#else


inline LogHelper log(Severity severity, const Utf8String &taskid = {},
                     const Utf8String &execid = {}) {
  return LogHelper({severity, taskid, execid});
}

inline LogHelper debug(const Utf8String &taskid = {},
                       const Utf8String &execid = {}) {
  return LogHelper({Debug, taskid, execid});
}

inline LogHelper info(const Utf8String &taskid = {},
                      const Utf8String &execid = {}) {
  return LogHelper({Info, taskid, execid});
}

inline LogHelper warning(const Utf8String &taskid = {},
                         const Utf8String &execid = {}) {
  return LogHelper({Warning, taskid, execid});
}

inline LogHelper error(const Utf8String &taskid = {},
                       const Utf8String &execid = {}) {
  return LogHelper({Error, taskid, execid});
}

inline LogHelper fatal(const Utf8String &taskid = {},
                       const Utf8String &execid = {}) {
  return LogHelper({Fatal, taskid, execid});
}

inline LogHelper log(Severity severity, const Utf8String &taskid, qint64 execid) {
  return LogHelper({severity, taskid, execid});
}

inline LogHelper debug(const Utf8String &taskid, qint64 execid) {
  return LogHelper({Debug, taskid, execid});
}

inline LogHelper info(const Utf8String &taskid, qint64 execid) {
  return LogHelper({Info, taskid, execid});
}

inline LogHelper warning(const Utf8String &taskid, qint64 execid) {
  return LogHelper({Warning, taskid, execid});
}

inline LogHelper error(const Utf8String &taskid, qint64 execid) {
  return LogHelper({Error, taskid, execid});
}

inline LogHelper fatal(const Utf8String &taskid, qint64 execid) {
  return LogHelper({Fatal, taskid, execid});
}

inline LogHelper log(Severity severity, const Utf8String &taskid, quint64 execid) {
  return LogHelper({severity, taskid, execid});
}

inline LogHelper debug(const Utf8String &taskid, quint64 execid) {
  return LogHelper({Debug, taskid, execid});
}

inline LogHelper info(const Utf8String &taskid, quint64 execid) {
  return LogHelper({Info, taskid, execid});
}

inline LogHelper warning(const Utf8String &taskid, quint64 execid) {
  return LogHelper({Warning, taskid, execid});
}

inline LogHelper error(const Utf8String &taskid, quint64 execid) {
  return LogHelper({Error, taskid, execid});
}

inline LogHelper fatal(const Utf8String &taskid, quint64 execid) {
  return LogHelper({Fatal, taskid, execid});
}

inline LogHelper debug(qint64 execid) {
  return LogHelper({Debug, {}, execid});
}

inline LogHelper info(qint64 execid) {
  return LogHelper({Info, {}, execid});
}

inline LogHelper warning(qint64 execid) {
  return LogHelper({Warning, {}, execid});
}

inline LogHelper error(qint64 execid) {
  return LogHelper({Error, {}, execid});
}

inline LogHelper fatal(qint64 execid) {
  return LogHelper({Fatal, {}, execid});
}

inline LogHelper debug(quint64 execid) {
  return LogHelper({Debug, {}, execid});
}

inline LogHelper info(quint64 execid) {
  return LogHelper({Info, {}, execid});
}

inline LogHelper warning(quint64 execid) {
  return LogHelper({Warning, {}, execid});
}

inline LogHelper error(quint64 execid) {
  return LogHelper({Error, {}, execid});
}

inline LogHelper fatal(quint64 execid) {
  return LogHelper({Fatal, {}, execid});
}

#endif // LOG_LOCATION_ENABLED

} // ns p6::log

namespace Log { // backward compat with old ::Log class

using p6::log::Severity;
using enum p6::log::Severity;
using p6::log::log;
using p6::log::debug;
using p6::log::info;
using p6::log::warning;
using p6::log::error;
using p6::log::fatal;
[[deprecated("use p6::log::use_same_format_for_qt_log or "
             "p6::log::wrap_qt_log instead")]]
inline void qtLogSamePatternWrapper(bool enable = true) {
  p6::log::use_same_format_for_qt_log(enable);
}
[[deprecated("use p6::log::add_console_logger instead")]]
inline void addConsoleLogger(
    Severity severity = Warning, bool autoRemovable = false,
    FILE *stream = stderr) {
  p6::log::add_console_logger(severity, autoRemovable, stream);
}
[[deprecated("use p6::log::add_logger instead")]]
inline void addLogger(
    p6::log::Logger *logger, bool autoRemovable) {
  p6::log::add_logger(logger, autoRemovable);
}

} // ns Log

//using p6::log::LogHelper;

Q_DECLARE_METATYPE(p6::log::Record)
Q_DECLARE_TYPEINFO(p6::log::Record, Q_RELOCATABLE_TYPE);
Q_DECLARE_METATYPE(p6::log::Severity)


#endif // LOG_H
