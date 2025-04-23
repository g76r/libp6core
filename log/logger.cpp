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
#include "log/logger.h"
#include "log/loggerthread.h"
#include "util/paramset.h"
#include "format/timeformats.h"
#include <QMutexLocker>

#define ISO8601 u"yyyy-MM-ddThh:mm:ss,zzz"_s

namespace p6::log {

Logger::Logger(Severity minSeverity, ThreadModel threadModel)
  : QObject(0), _min_severity(minSeverity), _auto_removable(true),
    _lastBufferOverflownWarning(0), _buffer(0), _thread_model(threadModel) {
  //qDebug() << "*** Logger::Logger " << this << " " << minSeverity
  //         << " " << threadModel << " " << QThread::currentThread();
  //qDebug() << "Logger" << QString::number((qlonglong)this, 16);
  int logBufferSizeLog2 =
    Utf8String(qEnvironmentVariable("LOG_BUFFER_SIZE_LOG2")).toInt();
  logBufferSizeLog2 = std::clamp(logBufferSizeLog2, 12, 27);
  QString name = "Logger-"+severity_as_text(minSeverity)
                 +"-"+QString::number((long long)this, 16);
  switch(threadModel) {
    case DirectCall: {
        break;
      }
    case RootLogger: {
        name = "Root"+name;
        [[fallthrough]];
      }
    case DedicatedThread: {
        _buffer = new CircularBuffer<Record>(logBufferSizeLog2);
        auto thread = new LoggerThread(this); // this is not used as parent
        thread->setObjectName(name);
        thread->start();
        moveToThread(thread);
      }
  }
  setObjectName(name);
}

Logger::~Logger() {
  if (_buffer)
    delete _buffer;
}

void Logger::log(const Record &record) {
  // this method must be callable from any thread, whereas the logger
  // implementation may not be threadsafe and/or may need a protection
  // against i/o latency: slow disk, NFS stall (for those fool enough to
  // write logs over NFS), etc.
  if (record.severity() >= _min_severity) {
    if (_thread_model & DedicatedThread) {
      if (!_buffer->tryPut(record)) {
        [[unlikely]];
        // warn only if not warned recently
        QMutexLocker ml(&_bufferOverflownMutex);
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - _lastBufferOverflownWarning > _bufferOverflownWarningIntervalMs) {
          _lastBufferOverflownWarning = now;
          qWarning().noquote()
              << QDateTime::currentDateTime().toString(ISO8601) << this
              << "Logger::log discarded at less one log record due to "
                 "thread buffer full" << record.message()
              << "this warning occurs at most every"
              << TimeFormats::toCoarseHumanReadableTimeInterval(
                   _bufferOverflownWarningIntervalMs)
              << "for every logger";
        }
      }
    } else {
      do_log(record);
    }
  }
}

void Logger::shutdown() {
  //qDebug() << "Logger::shutdown" << this << _thread << _buffer->used() << "---";
  if (_thread_model & DedicatedThread) {
    _buffer->tryPut({});
  } else {
    do_shutdown();
    deleteLater();
  }
}

Utf8String Logger::current_path() const {
  return {};
}

Utf8String Logger::path_pattern() const {
  return current_path();
}

QString Logger::path_matching_regexp() const {
  return PercentEvaluator::matching_regexp(path_pattern());
}

void Logger::do_log(const Record &) {
}

void Logger::do_shutdown() {
}

} // ns p6::log
