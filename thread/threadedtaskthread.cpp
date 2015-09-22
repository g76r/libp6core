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
#include "threadedtaskthread.h"
#include "threadedtask.h"
#include <QCoreApplication>
//#include <QDateTime>
//#include <QDebug>

ThreadedTaskThread::ThreadedTaskThread(ThreadedTask *task)
  : QThread(0), _task(task) {
  // ensure that this QThread object belongs to the main thread
  // it must not have a parent (hence QThread(0) above) to call moveToThread
  moveToThread(QCoreApplication::instance()->thread());
}

// this method is executed by the thread represented by this QThread
void ThreadedTaskThread::run() {
  if (_task) {
    //QDateTime t0 = QDateTime::currentDateTime();
    _task->run();
    //qDebug() << "Threaded task terminated in"
    //         << t0.msecsTo(QDateTime::currentDateTime()) << "ms for"
    //         << _task->metaObject()->className() << _task->name()
    //         << _task->id();
    // give the task back to main thread, otherwise remaining events (such
    // as delete event sent by deleteLater) would not be processed, this
    // thread being about to stop
    _task->moveToThread(QCoreApplication::instance()->thread());
  }
}
