/* Copyright 2012-2013 Hallowyn and others.
 * This file is part of qron, see <http://qron.hallowyn.com/>.
 * Qron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Qron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with qron. If not, see <http://www.gnu.org/licenses/>.
 */
#include "filelogger.h"
#include <QMetaObject>
#include <QFile>
#include <QtDebug>
#include <QThread>
#include "util/paramset.h"

FileLogger::FileLogger(QIODevice *device, Log::Severity minSeverity)
  : Logger(0, minSeverity), _device(0), _thread(new QThread) {
  //qDebug() << "creating FileLogger from device" << device;
  _device = device;
  _device->setParent(this);
  connect(this, SIGNAL(destroyed(QObject*)), _thread, SLOT(quit()));
  connect(_thread, SIGNAL(finished()), _thread, SLOT(deleteLater()));
  _thread->start();
  moveToThread(_thread);
  if (!_device->isOpen()) {
    if (!_device->open(QIODevice::WriteOnly|QIODevice::Append
                       |QIODevice::Unbuffered)) {
      qWarning() << "cannot open log device" << _device << ":"
                 << _device->errorString();
      delete _device;
      _device = 0;
    }
  }
}

FileLogger::FileLogger(QString pathPattern, Log::Severity minSeverity,
                       int secondsReopenInterval)
  : Logger(0, minSeverity), _device(0), _thread(new QThread(0)),
    _pathPattern(pathPattern), _lastOpen(QDateTime::currentDateTime()),
    _secondsReopenInterval(secondsReopenInterval) {
  connect(this, SIGNAL(destroyed(QObject*)), _thread, SLOT(quit()));
  connect(_thread, SIGNAL(finished()), _thread, SLOT(deleteLater()));
  _thread->start();
  moveToThread(_thread);
  //qDebug() << "creating FileLogger from path" << path << _currentPath;
}

FileLogger::~FileLogger() {
}

QString FileLogger::currentPath() const {
  return _currentPath;
}

QString FileLogger::pathPattern() const {
  return _pathPattern;
}

void FileLogger::doLog(QDateTime timestamp, QString message,
                       Log::Severity severity,
                       QString task, QString execId,
                       QString sourceCode) {
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
    if (!_device->open(QIODevice::WriteOnly|QIODevice::Append
                       |QIODevice::Unbuffered)) {
      //qWarning() << "cannot open log file" << _currentPath << ":"
      //           << _device->errorString();
      delete _device;
      _device = 0;
    } //else
      //qDebug() << "opened log file" << _currentPath;
  }
  QString line = QString("%1 %2/%3 %4 %5 %6")
      .arg(timestamp.toString("yyyy-MM-ddThh:mm:ss,zzz")).arg(task).arg(execId)
      .arg(sourceCode).arg(Log::severityToString(severity)).arg(message);
  //qDebug() << "***log" << line;
  if (_device) {
    QByteArray ba = line.toUtf8();
    ba.append("\n");
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
