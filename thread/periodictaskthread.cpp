/* Copyright 2012 Hallowyn and others.
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
#include "periodictaskthread.h"
#include "periodictask.h"
#include <QTimer>
#include <QCoreApplication>

PeriodicTaskThread::PeriodicTaskThread(PeriodicTask *task, int msec) :
  ThreadedTaskThread(task), _periodicTask(task), _timer(msec),
  _shutingDown(false) {
}

void PeriodicTaskThread::run() {
  while (!_shutingDown) {
    //qDebug("PeriodicTaskThread::run %p %p", this, QThread::currentThread());
    _periodicTask->run();
    _timer.wait();
  }
  _periodicTask->moveToThread(QCoreApplication::instance()->thread());
}

void PeriodicTaskThread::shutdown() {
  _shutingDown = true;
  _timer.stopWaiting();
}
