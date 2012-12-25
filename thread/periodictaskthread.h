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
#ifndef PERIODICTASKTHREAD_H
#define PERIODICTASKTHREAD_H

#include <QThread>
#include "blockingtimer.h"
#include "threadedtaskthread.h"

class PeriodicTask;

/** PeriodicTaskThread is an helper class for PeriodicTask.
  * Do not directly instanciate it or extend it.
  * @see PeriodicTask
  */
class LIBQTSSUSHARED_EXPORT PeriodicTaskThread : public ThreadedTaskThread {
  Q_OBJECT
private:
  PeriodicTask *_periodicTask;
  BlockingTimer _timer;
  bool _shutingDown;

public:
  explicit PeriodicTaskThread(PeriodicTask *task, int msec);
  void run();
  //PeriodicTask *task() const { return _task; }
  void shutdown();

private:
  Q_DISABLE_COPY(PeriodicTaskThread)
};

#endif // PERIODICTASKTHREAD_H
