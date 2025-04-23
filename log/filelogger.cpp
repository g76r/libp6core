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
#include "filelogger.h"
#include "util/paramset.h"
#include <QFile>
#include <QDateTime>

#define ISO8601 u"yyyy-MM-ddThh:mm:ss,zzz"_s

namespace p6::log {

FileLogger::FileLogger(QIODevice *device, Severity minSeverity,
                       bool buffered)
  : Logger(minSeverity, Logger::DedicatedThread), _device(device),
    _buffered(buffered) {
  //qDebug() << "creating FileLogger from device" << device;
  /*qDebug() << "FileLogger::FileLoger" << this->thread() << _device->thread()
           << QThread::currentThread();
  qDebug() << "/FileLogger::setParent";*/
  if (!_device->isOpen()) {
    if (!_device->open(_buffered ? QIODevice::WriteOnly|QIODevice::Append
                       : QIODevice::WriteOnly|QIODevice::Append
                       |QIODevice::Unbuffered)) {
      [[unlikely]];
      qWarning() << "cannot open log device" << _device << ":"
                 << _device->errorString();
      _device->deleteLater();
      _device = 0;
    }
  }
}

FileLogger::FileLogger(QString pathPattern, Severity minSeverity,
                       int secondsReopenInterval, bool buffered)
  : Logger(minSeverity, Logger::DedicatedThread), _device(0),
    _pathPattern(pathPattern), _lastOpen(QDateTime::currentDateTime()),
    _secondsReopenInterval(secondsReopenInterval), _buffered(buffered) {
  //qDebug() << "creating FileLogger from path" << pathPattern << this;
}

FileLogger::~FileLogger() {
  if (_device)
    _device->deleteLater();
}

Utf8String FileLogger::current_path() const {
  return _currentPath;
}

Utf8String FileLogger::path_pattern() const {
  return _pathPattern;
}

void FileLogger::do_log(const Record &record) {
  QDateTime now = QDateTime::currentDateTime();
  if (!_pathPattern.isEmpty()
      && (_device == 0
          || (_secondsReopenInterval >= 0
              && _lastOpen.secsTo(now) > _secondsReopenInterval))) {
    [[unlikely]];
    //qDebug() << "*******************************************************"
    //         << _pathPattern << _lastOpen << now << _secondsReopenInterval;
    if (_device)
      _device->deleteLater();
    _currentPath = Utf8String(PercentEvaluator::eval(_pathPattern));
    _device = new QFile(_currentPath);
    if (!_device->open(_buffered ? QIODevice::WriteOnly|QIODevice::Append
                                 : QIODevice::WriteOnly|QIODevice::Append
                                     |QIODevice::Unbuffered)) [[unlikely]] {
      // TODO warn, but only once
      //qWarning() << "cannot open log file" << _currentPath << ":"
      //           << _device->errorString();
      _device->deleteLater();
      _device = 0;
    } else {
      _lastOpen = QDateTime::currentDateTime();
      //qDebug() << "opened log file" << _currentPath;
    }
  }
  if (_device) {
    [[likely]];
    auto line = record.formated_message();
    if (_device->write(line) != line.size()) {
      // TODO warn, but only once
      //qWarning() << "error while writing log:" << _device
      //           << _device->errorString();
      //qWarning() << line;
    }
  } else {
    // TODO warn, but only once
    //qWarning() << "error while writing log: null log device";
    //qWarning() << line;
  }
}

void FileLogger::do_shutdown() {
  // TODO only if buffered ?
  _buffered = false;
  if (_device && !_pathPattern.isEmpty()) {
    _device->close();
    _device->open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Unbuffered);
  }
}

} // ns p6::log
