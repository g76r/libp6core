/* Copyright 2012-2014 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
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
#include "filelogger.h"
#include <QMetaObject>
#include <QFile>
#include <QtDebug>
#include <QThread>
#include "util/paramset.h"

FileLogger::FileLogger(QIODevice *device, Log::Severity minSeverity,
                       bool buffered)
  : Logger(minSeverity, Logger::DedicatedThread), _device(0),
    _buffered(buffered) {
  //qDebug() << "creating FileLogger from device" << device;
  _device = device;
  /*qDebug() << "FileLogger::FileLoger" << this->thread() << _device->thread()
           << QThread::currentThread();
  qDebug() << "/FileLogger::setParent";*/
  if (!_device->isOpen()) {
    if (!_device->open(_buffered ? QIODevice::WriteOnly|QIODevice::Append
                       : QIODevice::WriteOnly|QIODevice::Append
                       |QIODevice::Unbuffered)) {
      qWarning() << "cannot open log device" << _device << ":"
                 << _device->errorString();
      delete _device;
      _device = 0;
    }
  }
}

FileLogger::FileLogger(QString pathPattern, Log::Severity minSeverity,
                       int secondsReopenInterval, bool buffered)
  : Logger(minSeverity, Logger::DedicatedThread), _device(0),
    _pathPattern(pathPattern), _lastOpen(QDateTime::currentDateTime()),
    _secondsReopenInterval(secondsReopenInterval), _buffered(buffered) {
  //qDebug() << "creating FileLogger from path" << path << _currentPath;
}

FileLogger::~FileLogger() {
  if (_device)
    delete _device;
}

QString FileLogger::currentPath() const {
  return _currentPath;
}

QString FileLogger::pathPattern() const {
  return _pathPattern;
}

void FileLogger::doLog(const LogEntry entry) {
  if (!_pathPattern.isEmpty()
      && (_device == 0 || _lastOpen.secsTo(QDateTime::currentDateTime())
          > _secondsReopenInterval)) {
    //qDebug() << "*******************************************************"
    //         << _patternPath << _lastOpen << timestamp;
    if (_device)
      _lastOpen = _lastOpen.addSecs(_secondsReopenInterval);
    if (_device)
      delete _device;
    _currentPath = ParamSet().evaluate(_pathPattern);
    _device = new QFile(_currentPath, this);
    if (!_device->open(_buffered ? QIODevice::WriteOnly|QIODevice::Append
                       : QIODevice::WriteOnly|QIODevice::Append
                       |QIODevice::Unbuffered)) {
      //qWarning() << "cannot open log file" << _currentPath << ":"
      //           << _device->errorString();
      delete _device;
      _device = 0;
    } //else
      //qDebug() << "opened log file" << _currentPath;
  }
  if (_device) {
    QString line = QString("%1 %2/%3 %4 %5 %6")
        .arg(entry.timestamp().toString("yyyy-MM-ddThh:mm:ss,zzz"))
        .arg(entry.task()).arg(entry.execId()).arg(entry.sourceCode())
        .arg(entry.severityText()).arg(entry.message()).append('\n');
    //qDebug() << "***log" << line;
    QByteArray ba = line.toUtf8();
    if (_device->write(ba) != ba.size()) {
      //qWarning() << "error while writing log:" << _device
      //           << _device->errorString();
      //qWarning() << line;
    }
  } else {
    //qWarning() << "error while writing log: null log device";
    //qWarning() << line;
  }
}
