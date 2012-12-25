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
