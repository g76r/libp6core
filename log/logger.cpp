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

static Utf8StringList _uiHeaderNames {
  "Timestamp", // 0
  "Task id",
  "Execution id",
  "Location",
  "Severity",
  "Message" // 5
};

static Utf8StringList _uiSectionNames {
  "timestamp", // 0
  "taskid",
  "execid",
  "location",
  "severity",
  "message" // 5
};

static auto _uiSectionIndex = ContainerUtils::index(_uiSectionNames);

static QAtomicInt _sequence;

class Logger::LogEntryData : public SharedUiItemData {
public:
  Utf8String _id;
  QDateTime _timestamp;
  Utf8String _message;
  Log::Severity _severity;
  Utf8String _taskid, _execid, _location;
  LogEntryData(QDateTime timestamp, Utf8String message, Log::Severity severity,
               LogContext context)
    : _id(QByteArray::number(_sequence.fetchAndAddOrdered(1))),
      _timestamp(timestamp), _message(message), _severity(severity),
      _taskid(context.taskid()), _execid(context.execid()),
      _location(context.location()) { }
  QVariant uiData(int section, int role) const override;
  QVariant uiHeaderData(int section, int role) const override {
    switch (role) {
      case Qt::DisplayRole:
      case Qt::EditRole:
      case SharedUiItem::ExternalDataRole:
        return uiSectionName(section);
    }
    return {};
  }
  int uiSectionCount() const override { return _uiSectionNames.size(); }
  Utf8String uiSectionName(int section) const override {
    return _uiSectionNames.value(section); }
  int uiSectionByName(Utf8String sectionName) const override {
    return _uiSectionIndex.value(sectionName, -1); }
  Utf8String id() const override { return _id; }
  Utf8String qualifier() const override { return "logentry"_u8; }
};

Logger::LogEntry::LogEntry(QDateTime timestamp, Utf8String message,
                           Log::Severity severity, LogContext context)
  : SharedUiItem(new LogEntryData(timestamp, message, severity, context)) {
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

Utf8String Logger::LogEntry::taskid() const {
  return isNull() ? Utf8String() : data()->_taskid;
}

Utf8String Logger::LogEntry::execid() const {
  return isNull() ? Utf8String() : data()->_execid;
}

Utf8String Logger::LogEntry::location() const {
  return isNull() ? Utf8String() : data()->_location;
}

QVariant Logger::LogEntryData::uiData(int section, int role) const {
  switch(role) {
  case Qt::DisplayRole:
  case Qt::EditRole:
    switch(section) {
    case 0:
      return _timestamp.toString(u"yyyy-MM-dd hh:mm:ss,zzz"_s);
    case 1:
      return _taskid;
    case 2:
      return _execid;
    case 3:
      return _location;
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

Logger::Logger(Log::Severity minSeverity, ThreadModel threadModel)
  : QObject(0), _thread(0), _minSeverity(minSeverity), _autoRemovable(true),
    _lastBufferOverflownWarning(0), _buffer(0), _threadModel(threadModel) {
  //qDebug() << "*** Logger::Logger " << this << " " << minSeverity
  //         << " " << threadModel << " " << QThread::currentThread();
  //qDebug() << "Logger" << QString::number((qlonglong)this, 16);
  int logBufferSizeLog2 =
    Utf8String(qEnvironmentVariable("LOG_BUFFER_SIZE_LOG2")).toInt();
  if (logBufferSizeLog2 < 6)
    logBufferSizeLog2 = 12;
  if (logBufferSizeLog2 > 27)
    logBufferSizeLog2 = 27;
  QString name = "Logger-"+Log::severityToString(minSeverity)
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

QString Logger::pathMatchingRegexp() const {
  return PercentEvaluator::matching_regexp(pathPattern());
}

void Logger::doLog(const LogEntry &) {
}

void Logger::doShutdown() {
}

const Logger::LogEntryData *Logger::LogEntry::data() const {
  return specializedData<Logger::LogEntryData>();
}
