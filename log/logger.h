/* Copyright 2013-2024 Hallowyn, Gregoire Barbier and others.
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
#ifndef LOGGER_H
#define LOGGER_H

#include "log/log.h"
#include "thread/circularbuffer.h"
#include "modelview/shareduiitem.h"

class MultiplexerLogger;
class LoggerThread;
class QMutex;

/** Base class to be extended by logger implementations.
 * Handle common behavior to all loggers, including optionnaly a
 * queuing mechanism using a dedicated thread intended for loggers that
 * may block (filesystem, network...).
 *
 * One must never delete a Logger nor call Logger::deleteLater(), but rather
 * call Logger::shutdown() instead, which will handle logger thread (when
 * applicable) graceful shutdown and deleteLater() in addition to logger
 * deleteLater().
 */
class LIBP6CORESHARED_EXPORT Logger : public QObject {
  friend class MultiplexerLogger;
  friend class LoggerThread;

public:
  class LogEntryData;
  class LogEntry : public SharedUiItem {
  public:
    LogEntry() noexcept {}
    LogEntry(QDateTime timestamp, Utf8String message, Log::Severity severity,
             LogContext context);
    LogEntry(const LogEntry &other) noexcept : SharedUiItem(other) {}
    LogEntry(LogEntry &&other) noexcept : SharedUiItem(std::move(other)) {}
    LogEntry &operator=(const LogEntry &other) {
      SharedUiItem::operator=(other); return *this; }
    LogEntry &operator=(LogEntry &&other) {
      SharedUiItem::operator=(std::move(other)); return *this; }
    QDateTime timestamp() const;
    Utf8String message() const;
    Log::Severity severity() const;
    Utf8String severityToString() const;
    Utf8String taskid() const;
    Utf8String execid() const;
    Utf8String location() const;

  private:
    inline const LogEntryData *data() const;
  };
  enum ThreadModel {
    DirectCall, // the logger is already thread-safe and cannot block
    DedicatedThread, // the logger needs a dedicated thread in case it blocks
    RootLogger // root logger needs a dedicated thread and an input mutex
  };

private:
  Q_OBJECT
  Q_DISABLE_COPY(Logger)
  QThread *_thread;
  Log::Severity _minSeverity;
  bool _autoRemovable;
  QMutex _bufferOverflownMutex;
  qint64 _lastBufferOverflownWarning;
  // LATER make _bufferOverflownWarningIntervalMs configurable
  qint64 _bufferOverflownWarningIntervalMs = 10*60*1000; // 10'
  CircularBuffer<LogEntry> *_buffer;
  ThreadModel _threadModel;

protected:
  // Loggers never have a parent (since they are owned and destroyed by Log
  // static methods)
  Logger(Log::Severity minSeverity, ThreadModel threadModel);

public:
  /** This method is thread-safe. */
  void log(const LogEntry &entry);
  /** This method is thread-safe. */
  void shutdown();
  ~Logger();
  /** Return current logging path, e.g. "/var/log/qron-20181231.log"
   * To be used by implementation only when relevant.
   * Default: QString() */
  virtual Utf8String currentPath() const;
  /** Return the path pattern, e.g. "/var/log/qron-%!yyyy%!mm%!dd.log"
   * To be used by implementation only when relevant.
   * Default: same as currentPath() */
  virtual Utf8String pathPattern() const;
  /** Return the path regexp pattern, e.g. "/var/log/qron-.*\\.log" */
  QString pathMatchingRegexp() const;
  Log::Severity minSeverity() const { return _minSeverity; }
  ThreadModel threadModel() const { return _threadModel; }

protected:
  /** Method to be implemented by the actual logger.
   * Either the Logger must be created with dedicatedThread = true or this
   * method must be threadsafe (= able to handle calls from any thread at any
   * time). */
  virtual void doLog(const LogEntry &entry) = 0;
  /** Perform shutdown tasks, such as flushing. */
  virtual void doShutdown();

private:
  /** Should not call deleteLater() from elsewhere, rather call shutdown(). */
  using QObject::deleteLater;
};

Q_DECLARE_METATYPE(Logger::LogEntry)
Q_DECLARE_TYPEINFO(Logger::LogEntry, Q_RELOCATABLE_TYPE);

#endif // LOGGER_H
