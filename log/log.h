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

using namespace Qt::Literals::StringLiterals;

class Logger;
class LogHelper;

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
  /** flush remove any logger */
  static void shutdown();
  static void log(QByteArray message, Severity severity = Info,
                  QByteArray task = {}, QByteArray execId = {},
                  QByteArray sourceCode = {});
  static QByteArray severityToString(Severity severity);
  /** Very tolerant severity mnemonic reader.
   * Read first character of string in a case insensitive manner, e.g.
   * "W", "warn", "warning", and "war against terror" are all interpreted
   * as Log::Warning.
   * Unmatched letters ar interpreted as Log::Debug, therefore "D", "Debug",
   * or even "Global Warming" and "" are all interpreted as Log::Debug. */
  static Log::Severity severityFromString(QByteArray string);
  static inline LogHelper log(
      Log::Severity severity, QByteArray task = {}, QByteArray execId = {},
      QByteArray sourceCode = {});
  static inline LogHelper debug(
      QByteArray task = {}, QByteArray execId = {}, QByteArray sourceCode = {});
  static inline LogHelper info(
      QByteArray task = {}, QByteArray execId = {}, QByteArray sourceCode = {});
  static inline LogHelper warning(
      QByteArray task = {}, QByteArray execId = {}, QByteArray sourceCode = {});
  static inline LogHelper error(
      QByteArray task = {}, QByteArray execId = {}, QByteArray sourceCode = {});
  static inline LogHelper fatal(
      QByteArray task = {}, QByteArray execId = {}, QByteArray sourceCode = {});
  static inline LogHelper log(
      Log::Severity severity, QByteArray task, quint64 execId,
      QByteArray sourceCode = {});
  static inline LogHelper debug(
      QByteArray task, quint64 execId, QByteArray sourceCode = {});
  static inline LogHelper info(
      QByteArray task, quint64 execId, QByteArray sourceCode = {});
  static inline LogHelper warning(
      QByteArray task, quint64 execId, QByteArray sourceCode = {});
  static inline LogHelper error(
      QByteArray task, quint64 execId, QByteArray sourceCode = {});
  static inline LogHelper fatal(
      QByteArray task, quint64 execId, QByteArray sourceCode = {});
  static inline LogHelper debug(
      QByteArray task, qint64 execId, QByteArray sourceCode = {});
  static inline LogHelper info(
      QByteArray task, qint64 execId, QByteArray sourceCode = {});
  static inline LogHelper warning(
      QByteArray task, qint64 execId, QByteArray sourceCode = {});
  static inline LogHelper error(
      QByteArray task, qint64 execId, QByteArray sourceCode = {});
  static inline LogHelper fatal(
      QByteArray task, qint64 execId, QByteArray sourceCode = {});
  static inline LogHelper debug(quint64 execId, QByteArray sourceCode = {});
  static inline LogHelper info(quint64 execId, QByteArray sourceCode = {});
  static inline LogHelper warning(quint64 execId, QByteArray sourceCode = {});
  static inline LogHelper error(quint64 execId, QByteArray sourceCode = {});
  static inline LogHelper fatal(quint64 execId, QByteArray sourceCode = {});
  static inline LogHelper debug(qint64 execId, QByteArray sourceCode = {});
  static inline LogHelper info(qint64 execId, QByteArray sourceCode = {});
  static inline LogHelper warning(qint64 execId, QByteArray sourceCode = {});
  static inline LogHelper error(qint64 execId, QByteArray sourceCode = {});
  static inline LogHelper fatal(qint64 execId, QByteArray sourceCode = {});
  /* temporary backward compatibility with QString API */
  static inline LogHelper log(
      Log::Severity severity, QString task, QString execId = {},
      QString sourceCode = {});
  static inline LogHelper debug(
      QString task, QString execId = {}, QString sourceCode = {});
  static inline LogHelper info(
      QString task, QString execId = {}, QString sourceCode = {});
  static inline LogHelper warning(
      QString task, QString execId = {}, QString sourceCode = {});
  static inline LogHelper error(
      QString task, QString execId = {}, QString sourceCode = {});
  static inline LogHelper fatal(
      QString task, QString execId = {}, QString sourceCode = {});
  static inline LogHelper log(
      Log::Severity severity, QString task, quint64 execId,
      QString sourceCode = {});
  static inline LogHelper debug(
      QString task, quint64 execId, QString sourceCode = {});
  static inline LogHelper info(
      QString task, quint64 execId, QString sourceCode = {});
  static inline LogHelper warning(
      QString task, quint64 execId, QString sourceCode = {});
  static inline LogHelper error(
      QString task, quint64 execId, QString sourceCode = {});
  static inline LogHelper fatal(
      QString task, quint64 execId, QString sourceCode = {});
  static inline LogHelper log(
      Log::Severity severity, const char *task);
  static inline LogHelper debug(const char *task);
  static inline LogHelper info(const char *task);
  static inline LogHelper warning(const char *task);
  static inline LogHelper error(const char *task);
  static inline LogHelper fatal(const char *task);
  /* /temporary backward compatibility with QString API */
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
  QByteArray _message, _task, _execId, _sourceCode;

public:
  inline LogHelper(Log::Severity severity, QByteArray task, QByteArray execId,
                   QByteArray sourceCode)
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
    _message.append(o.toUtf8()); return *this; }
  inline LogHelper &operator<<(const QLatin1String &o) {
    _message.append(o.toString().toUtf8()); return *this; }
  inline LogHelper &operator<<(const QByteArray &o) {
    _message.append(o); return *this; }
  inline LogHelper &operator<<(const char *o) {
    _message.append(o); return *this; }
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  inline LogHelper &operator<<(T o) {
    _message.append(QByteArray::number(o)); return *this; }
  inline LogHelper &operator<<(bool o) {
    _message.append(o ? "true"_ba : "false"_ba);
    return *this; }
  inline LogHelper &operator<<(const QVariant &o) {
    _message.append(o.canConvert<QByteArray>()
                    ? o.toByteArray()
                    : o.toString().toUtf8());
    return *this; }
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

LogHelper Log::log(Log::Severity severity, QByteArray task,
                   QByteArray execId, QByteArray sourceCode) {
  return LogHelper(severity, task, execId, sourceCode);
}

LogHelper Log::debug(
    QByteArray task, QByteArray execId, QByteArray sourceCode) {
  return LogHelper(Log::Debug, task, execId, sourceCode);
}

LogHelper Log::info(QByteArray task, QByteArray execId, QByteArray sourceCode) {
  return LogHelper(Log::Info, task, execId, sourceCode);
}

LogHelper Log::warning(
    QByteArray task, QByteArray execId, QByteArray sourceCode) {
  return LogHelper(Log::Warning, task, execId, sourceCode);
}

LogHelper Log::error(
    QByteArray task, QByteArray execId, QByteArray sourceCode) {
  return LogHelper(Log::Error, task, execId, sourceCode);
}

LogHelper Log::fatal(
    QByteArray task, QByteArray execId, QByteArray sourceCode) {
  return LogHelper(Log::Fatal, task, execId, sourceCode);
}

LogHelper Log::log(Log::Severity severity, QByteArray task, quint64 execId,
                   QByteArray sourceCode) {
  return log(severity, task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::debug(QByteArray task, quint64 execId, QByteArray sourceCode) {
  return debug(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::info(QByteArray task, quint64 execId, QByteArray sourceCode) {
  return info(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::warning(QByteArray task, quint64 execId, QByteArray sourceCode) {
  return warning(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::error(QByteArray task, quint64 execId, QByteArray sourceCode) {
  return error(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::fatal(QByteArray task, quint64 execId, QByteArray sourceCode) {
  return fatal(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::debug(QByteArray task, qint64 execId, QByteArray sourceCode) {
  return debug(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::info(QByteArray task, qint64 execId, QByteArray sourceCode) {
  return info(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::warning(QByteArray task, qint64 execId, QByteArray sourceCode) {
  return warning(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::error(QByteArray task, qint64 execId, QByteArray sourceCode) {
  return error(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::fatal(QByteArray task, qint64 execId,QByteArray sourceCode) {
  return fatal(task, QByteArray::number(execId), sourceCode);
}

LogHelper Log::debug(quint64 execId, QByteArray sourceCode) {
  return debug(QByteArray(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::info(quint64 execId, QByteArray sourceCode) {
  return info(QByteArray(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::warning(quint64 execId, QByteArray sourceCode) {
  return warning(QByteArray(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::error(quint64 execId, QByteArray sourceCode) {
  return error(QByteArray(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::fatal(quint64 execId,QByteArray sourceCode) {
  return fatal(QByteArray(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::debug(qint64 execId, QByteArray sourceCode) {
  return debug(QByteArray(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::info(qint64 execId, QByteArray sourceCode) {
  return info(QByteArray(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::warning(qint64 execId, QByteArray sourceCode) {
  return warning(QByteArray(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::error(qint64 execId, QByteArray sourceCode) {
  return error(QByteArray(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::fatal(qint64 execId,QByteArray sourceCode) {
  return fatal(QByteArray(), QByteArray::number(execId), sourceCode);
}

LogHelper Log::log(
    Log::Severity severity, QString task, QString execId, QString sourceCode) {
  return log(severity, task.toUtf8(), execId.toUtf8(), sourceCode.toUtf8());
}
LogHelper Log::debug(
    QString task, QString execId, QString sourceCode) {
  return log(Log::Debug, task.toUtf8(), execId.toUtf8(), sourceCode.toUtf8());
}
LogHelper Log::info(
    QString task, QString execId, QString sourceCode) {
  return log(Log::Info, task.toUtf8(), execId.toUtf8(), sourceCode.toUtf8());
}
LogHelper Log::warning(
    QString task, QString execId, QString sourceCode) {
  return log(Log::Warning, task.toUtf8(), execId.toUtf8(),
             sourceCode.toUtf8());
}
LogHelper Log::error(
    QString task, QString execId, QString sourceCode) {
  return log(Log::Error, task.toUtf8(), execId.toUtf8(), sourceCode.toUtf8());
}
LogHelper Log::fatal(
    QString task, QString execId, QString sourceCode) {
  return log(Log::Fatal, task.toUtf8(), execId.toUtf8(), sourceCode.toUtf8());
}

LogHelper Log::log(
    Log::Severity severity, QString task, quint64 execId, QString sourceCode) {
  return log(severity, task.toUtf8(), QByteArray::number(execId),
             sourceCode.toUtf8());
}
LogHelper Log::debug(
    QString task, quint64 execId, QString sourceCode) {
  return log(Log::Debug, task.toUtf8(), QByteArray::number(execId),
             sourceCode.toUtf8());
}
LogHelper Log::info(
    QString task, quint64 execId, QString sourceCode) {
  return log(Log::Info, task.toUtf8(), QByteArray::number(execId),
             sourceCode.toUtf8());
}
LogHelper Log::warning(
    QString task, quint64 execId, QString sourceCode) {
  return log(Log::Warning, task.toUtf8(), QByteArray::number(execId),
             sourceCode.toUtf8());
}
LogHelper Log::error(
    QString task, quint64 execId, QString sourceCode) {
  return log(Log::Error, task.toUtf8(), QByteArray::number(execId),
             sourceCode.toUtf8());
}
LogHelper Log::fatal(
    QString task, quint64 execId, QString sourceCode) {
  return log(Log::Fatal, task.toUtf8(), QByteArray::number(execId),
             sourceCode.toUtf8());
}
LogHelper Log::log(Log::Severity severity, const char *task) {
  return log(severity, QByteArray(task));
}
LogHelper Log::debug(const char *task) {
  return debug(QByteArray(task));
}
LogHelper Log::info(const char *task) {
  return info(QByteArray(task));
}
LogHelper Log::warning(const char *task) {
  return warning(QByteArray(task));
}
LogHelper Log::error(const char *task) {
  return error(QByteArray(task));
}
LogHelper Log::fatal(const char *task) {
  return fatal(QByteArray(task));
}

#endif // LOG_H
