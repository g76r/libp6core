/*
Copyright 2012 Hallowyn and others.
See the NOTICE file distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file to you under
the Apache License, Version 2.0 (the "License"); you may not use this
file except in compliance with the License. You may obtain a copy of the
License at http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations
under the License.
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
