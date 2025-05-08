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
#include "log/log_p.h"
#include <QFile>
#include <QDateTime>
#include <QCoreApplication>

#define ISO8601 u"yyyy-MM-ddThh:mm:ss,zzz"_s

namespace p6::log {

FileLogger::FileLogger(QIODevice *device, Severity minSeverity,
                       bool buffered)
  : Logger(minSeverity, Logger::DedicatedThread), _device(device),
    _buffered(buffered) {
  if (!_device->isOpen()) {
    if (!_device->open(_buffered ? QIODevice::WriteOnly|QIODevice::Append
                       : QIODevice::WriteOnly|QIODevice::Append
                       |QIODevice::Unbuffered)) {
      [[unlikely]];
      stderr_direct_log("cannot open log device"
                        +Utf8String::number_and_name(_device)+": "
                        +_device->errorString(), Warning);
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
    if (_device)
      _device->deleteLater();
    _currentPath = Utf8String(PercentEvaluator::eval(_pathPattern));
    _device = new QFile(_currentPath);
    _device->setObjectName(_device->objectName()+" from "+this->objectName());
    _device->moveToThread(QCoreApplication::instance()->thread());
    if (!_device->open(_buffered ? QIODevice::WriteOnly|QIODevice::Append
                                 : QIODevice::WriteOnly|QIODevice::Append
                                     |QIODevice::Unbuffered)) [[unlikely]] {
      // TODO warn, but only once
      _device->deleteLater();
      _device = 0;
    } else {
      _lastOpen = QDateTime::currentDateTime();
    }
  }
  if (_device) {
    [[likely]];
    auto line = record.formated_message();
    if (_device->write(line) != line.size()) {
      // TODO warn, but only once
    }
  } else {
    // TODO warn, but only once
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
