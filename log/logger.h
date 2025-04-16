/* Copyright 2013-2025 Hallowyn, Gregoire Barbier and others.
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

class QMutex;

namespace p6::log {

class MultiplexerLogger;
class LoggerThread;

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
  enum ThreadModel {
    DirectCall = 0, // the logger is already thread-safe and cannot block
    DedicatedThread = 1, // the logger needs a dedicated thread in case it blocks
    // TODO next comment seems out of date (every multiplexer earned a mutex)
    RootLogger = 3 // root logger needs a dedicated thread and an input mutex
  };

private:
  Q_OBJECT
  Q_DISABLE_COPY(Logger)
  Severity _min_severity;
  bool _auto_removable;
  QMutex _bufferOverflownMutex;
  qint64 _lastBufferOverflownWarning;
  // LATER make _bufferOverflownWarningIntervalMs configurable
  qint64 _bufferOverflownWarningIntervalMs = 10*60*1000; // 10'
  CircularBuffer<Record> *_buffer;
  ThreadModel _thread_model;

protected:
  // Loggers never have a parent (since they are owned and destroyed by Log
  // static methods)
  Logger(Severity minSeverity, ThreadModel threadModel);

public:
  /** This method is thread-safe. */
  void log(const Record &record);
  /** This method is thread-safe. */
  void shutdown();
  ~Logger();
  /** Return current logging path, e.g. "/var/log/qron-20181231.log"
   * To be used by implementation only when relevant.
   * Default: {} */
  virtual Utf8String current_path() const;
  /** Return the path pattern, e.g. "/var/log/qron-%!yyyy%!mm%!dd.log"
   * To be used by implementation only when relevant.
   * Default: same as currentPath() */
  virtual Utf8String path_pattern() const;
  /** Return the path regexp pattern, e.g. "/var/log/qron-.*\\.log" */
  QString path_matching_regexp() const;
  Severity min_severity() const { return _min_severity; }
  ThreadModel thread_model() const { return _thread_model; }

protected:
  /** Method to be implemented by the actual logger.
   * Either the Logger must be created with dedicatedThread = true or this
   * method must be threadsafe (= able to handle calls from any thread at any
   * time). */
  virtual void do_log(const Record &record) = 0;
  /** Perform shutdown tasks, such as flushing. */
  virtual void do_shutdown();

private:
  /** Should not call deleteLater() from elsewhere, rather call shutdown(). */
  using QObject::deleteLater;
};

} // ns p6::log

#endif // LOGGER_H
