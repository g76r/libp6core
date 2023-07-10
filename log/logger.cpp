/* Copyright 2013-2023 Hallowyn, Gregoire Barbier and others.
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
#include "logger.h"
#include "loggerthread.h"
#include <QTimer>
#include "util/paramset.h"
#include "format/timeformats.h"

#define ISO8601 u"yyyy-MM-ddThh:mm:ss,zzz"_s

static Utf8String _uiHeaderNames[] = {
  "Timestamp", // 0
  "Task",
  "Execution id",
  "Source",
  "Severity",
  "Message" // 5
};

static QAtomicInt _sequence;

class Logger::LogEntryData : public SharedUiItemData {
public:
  Utf8String _id;
  QDateTime _timestamp;
  Utf8String _message;
  Log::Severity _severity;
  Utf8String _task, _execId, _sourceCode;
  LogEntryData(QDateTime timestamp, Utf8String message, Log::Severity severity,
               Utf8String task, Utf8String execId, Utf8String sourceCode)
    : _id(QByteArray::number(_sequence.fetchAndAddOrdered(1))),
      _timestamp(timestamp), _message(message), _severity(severity),
      _task(task), _execId(execId), _sourceCode(sourceCode) { }
  QVariant uiData(int section, int role) const override;
  QVariant uiHeaderData(int section, int role) const override;
  int uiSectionCount() const override;
  QByteArray id() const override { return _id; }
  QByteArray idQualifier() const override { return "logentry"_ba; }
};

Logger::LogEntry::LogEntry(QDateTime timestamp, Utf8String message,
                           Log::Severity severity, Utf8String task,
                           Utf8String execId, Utf8String sourceCode)
  : SharedUiItem(new LogEntryData(timestamp, message, severity, task, execId,
                                  sourceCode)) {

}

Logger::LogEntry::LogEntry() {
}

Logger::LogEntry::LogEntry(const Logger::LogEntry &other)
  : SharedUiItem(other) {
}

QDateTime Logger::LogEntry::timestamp() const {
  return isNull() ? QDateTime() : data()->_timestamp;
}

Utf8String Logger::LogEntry::message() const {
  return isNull() ? Utf8String() : data()->_message;
}

Log::Severity Logger::LogEntry::severity() const {
  return isNull() ? Log::Debug : data()->_severity;
}

Utf8String Logger::LogEntry::severityToString() const {
  return Log::severityToString(isNull() ? Log::Debug : data()->_severity);
}

Utf8String Logger::LogEntry::task() const {
  return isNull() ? Utf8String() : data()->_task;
}

Utf8String Logger::LogEntry::execId() const {
  return isNull() ? Utf8String() : data()->_execId;
}

Utf8String Logger::LogEntry::sourceCode() const {
  return isNull() ? Utf8String() : data()->_sourceCode;
}

QVariant Logger::LogEntryData::uiData(int section, int role) const {
  switch(role) {
  case Qt::DisplayRole:
  case Qt::EditRole:
    switch(section) {
    case 0:
      return _timestamp.toString(u"yyyy-MM-dd hh:mm:ss,zzz"_s);
    case 1:
      return _task;
    case 2:
      return _execId;
    case 3:
      return _sourceCode;
    case 4:
      return Log::severityToString(_severity);
    case 5:
      return _message;
    }
    break;
  default:
    ;
  }
  return {};
}

QVariant Logger::LogEntryData::uiHeaderData(int section, int role) const {
  return role == Qt::DisplayRole && section >= 0
      && (unsigned)section < sizeof _uiHeaderNames
      ? _uiHeaderNames[section] : QVariant();
}

int Logger::LogEntryData::uiSectionCount() const {
  return sizeof _uiHeaderNames / sizeof *_uiHeaderNames;
}

Logger::Logger(Log::Severity minSeverity, ThreadModel threadModel)
  : QObject(0), _thread(0), _minSeverity(minSeverity), _autoRemovable(true),
    _lastBufferOverflownWarning(0), _buffer(0), _threadModel(threadModel) {
  //qDebug() << "*** Logger::Logger " << this << " " << minSeverity
  //         << " " << threadModel << " " << QThread::currentThread();
  //qDebug() << "Logger" << QString::number((qlonglong)this, 16);
  int logBufferSizeLog2 =
    QString::fromLocal8Bit(qgetenv("LOG_BUFFER_SIZE_LOG2")).toInt(0, 0);
  if (logBufferSizeLog2 < 6)
    logBufferSizeLog2 = 12;
  if (logBufferSizeLog2 > 27)
    logBufferSizeLog2 = 27;
  auto name = "Logger-"+Log::severityToString(minSeverity)
              +"-"+QString::number((long long)this, 16);
  switch(threadModel) {
  case DirectCall:
    break;
  case DedicatedThread:
    _thread = new LoggerThread(this);
    _buffer = new CircularBuffer<LogEntry>(logBufferSizeLog2);
    break;
  case RootLogger:
    name = "Root"+name;
    _thread = new LoggerThread(this);
    _buffer = new CircularBuffer<LogEntry>(logBufferSizeLog2);
    break;
  }
  setObjectName(name);
  if (_thread) {
    _thread->setObjectName(name);
    _thread->start();
    moveToThread(_thread);
  }
}

Logger::~Logger() {
  if (_buffer)
    delete _buffer;
}

void Logger::log(const LogEntry &entry) {
  // this method must be callable from any thread, whereas the logger
  // implementation may not be threadsafe and/or may need a protection
  // against i/o latency: slow disk, NFS stall (for those fool enough to
  // write logs over NFS), etc.
  if (entry.severity() >= _minSeverity) {
    if (_thread) {
      if (!_buffer->tryPut(entry)) [[unlikely]] {
        // warn only if not warned recently
        QMutexLocker ml(&_bufferOverflownMutex);
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - _lastBufferOverflownWarning > _bufferOverflownWarningIntervalMs) {
          _lastBufferOverflownWarning = now;
          qWarning().noquote()
              << QDateTime::currentDateTime().toString(ISO8601) << this
              << "Logger::log discarded at less one log entry due to "
                 "thread buffer full" << entry.message()
              << "this warning occurs at most every"
              << TimeFormats::toCoarseHumanReadableTimeInterval(
                   _bufferOverflownWarningIntervalMs)
              << "for every logger";
        }
      }
    } else {
      doLog(entry);
    }
  }
}

void Logger::shutdown() {
  //qDebug() << "Logger::shutdown" << this << _thread << _buffer->used() << "---";
  if (_thread) {
    _buffer->tryPut(LogEntry());
  } else {
    doShutdown();
    QObject::deleteLater();
  }
}

Utf8String Logger::currentPath() const {
  return QString();
}

Utf8String Logger::pathPattern() const {
  return currentPath();
}

Utf8String Logger::pathMatchingRegexp() const {
  return ParamSet::matchingRegexp(pathPattern());
}

void Logger::doLog(const LogEntry &) {
}

void Logger::doShutdown() {
}
