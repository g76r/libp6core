/* Copyright 2013-2017 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
 * Libqtssu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libqtssu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "logger.h"
#include "loggerthread.h"
#include <QTimer>
#include "util/paramset.h"
#include "format/timeformats.h"

#define ISO8601 QStringLiteral("yyyy-MM-ddThh:mm:ss,zzz")

static QString _uiHeaderNames[] = {
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
  QString _id;
  QDateTime _timestamp;
  QString _message;
  Log::Severity _severity;
  QString _task, _execId, _sourceCode;
  LogEntryData(QDateTime timestamp, QString message, Log::Severity severity,
               QString task, QString execId, QString sourceCode)
    : _id(QString::number(_sequence.fetchAndAddOrdered(1))),
      _timestamp(timestamp), _message(message), _severity(severity),
      _task(task), _execId(execId), _sourceCode(sourceCode) { }
  QVariant uiData(int section, int role) const;
  QVariant uiHeaderData(int section, int role) const;
  int uiSectionCount() const;
  QString id() const { return _id; }
  QString idQualifier() const { return "logentry"; }
};

Logger::LogEntry::LogEntry(QDateTime timestamp, QString message,
                           Log::Severity severity, QString task,
                           QString execId, QString sourceCode)
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

QString Logger::LogEntry::message() const {
  return isNull() ? QString() : data()->_message;
}

Log::Severity Logger::LogEntry::severity() const {
  return isNull() ? Log::Debug : data()->_severity;
}

QString Logger::LogEntry::severityToString() const {
  return Log::severityToString(isNull() ? Log::Debug : data()->_severity);
}

QString Logger::LogEntry::task() const {
  return isNull() ? QString() : data()->_task;
}

QString Logger::LogEntry::execId() const {
  return isNull() ? QString() : data()->_execId;
}

QString Logger::LogEntry::sourceCode() const {
  return isNull() ? QString() : data()->_sourceCode;
}

QVariant Logger::LogEntryData::uiData(int section, int role) const {
  switch(role) {
  case Qt::DisplayRole:
  case Qt::EditRole:
    switch(section) {
    case 0:
      return _timestamp.toString("yyyy-MM-dd hh:mm:ss,zzz");
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
  return QVariant();
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
    _lastBufferOverflownWarning(0), _buffer(0) {
  // LATER make buffer size parametrable
  //qDebug() << "*** Logger::Logger " << this << " " << minSeverity
  //         << " " << threadModel;
  //qDebug() << "Logger" << QString::number((long)this, 16);
  switch(threadModel) {
  case DirectCall:
    break;
  case DedicatedThread:
    _thread = new LoggerThread(this);
    _thread->setObjectName("Logger-"+Log::severityToString(minSeverity)
                           +"-"+QString::number((long)this, 16));
    _buffer = new CircularBuffer<LogEntry>(12);
    break;
  case RootLogger:
    _thread = new LoggerThread(this);
    _thread->setObjectName("RootLogger-"+QString::number((long)this, 16));
    _buffer = new CircularBuffer<LogEntry>(12);
    break;
  }
  if (_thread) {
    _thread->start();
    moveToThread(_thread);
  }
}

Logger::~Logger() {
  //qDebug() << "~Logger" << QString::number((long)this, 16);
  if (_buffer)
    delete _buffer;
}

void Logger::deleteLater() {
  if (_thread) {
    _thread->requestInterruption();
    // cannot access or modify any member data after _thread->requestInterruption()
    // since *this can have been deleted meanwhile (it would create a race
    // condition)
  } else {
    QObject::deleteLater();
  }
}

void Logger::log(const LogEntry &entry) {
  // this method must be callable from any thread, whereas the logger
  // implementation may not be threadsafe and/or may need a protection
  // against i/o latency: slow disk, NFS stall (for those fool enough to
  // write logs over NFS), etc.
  if (entry.severity() >= _minSeverity) {
    if (_thread) {
      if (!_buffer->tryPut(entry)) {
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

QString Logger::currentPath() const {
  return QString();
}

QString Logger::pathPattern() const {
  return currentPath();
}

QString Logger::pathMatchingRegexp() const {
  return ParamSet::matchingRegexp(pathPattern());
}
