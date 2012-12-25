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
#ifndef PERIODICTASK_H
#define PERIODICTASK_H

#include "threadedtask.h"

class PeriodicTaskThread;

/** PeriodicTask is the base class for any task that should be run every
  * msec milliseconds in a dedicated task.
  *
  * To create a CPU-intensive or blocking task run on a periodic basis, one
  * should extends this class and implement run() method. It will be run every
  * msec milliseconds if previous execution was shorter than msec milliseconds
  * or immediatly after finished otherwise.
  * Keep in mind that if the task has to receive events (including queued
  * signals) it must call QCoreApplication::processEvents() on a regular basis.
  * Events are automatically processed when waiting between two executions but
  * not when run() method is running.
  *
  * To create a task run only once, one should rather extend ThreadedTask
  * instead.
  *
  * To execute a non-blocking non-CPU-intensive task on a periodic basis, it is
  * most often a better idea to use QTimer.
  *
  * To read more about threads in Qt, this article is a very good information
  * source: http://qt-project.org/wiki/ThreadsEventsQObjects
  *
  * @see ThreadedTask
  * @see QTimer
  */
class LIBQTSSUSHARED_EXPORT PeriodicTask : public ThreadedTask {
  Q_OBJECT
private:
  PeriodicTaskThread *_periodicTaskThread;
public:
  explicit PeriodicTask(int msec);
  /** Schedule stop thread as soon as possible, after the end of the task
    * execution if it is currently running. Returns immediatly.
    */
  void shutdown();

private:
  Q_DISABLE_COPY(PeriodicTask)
};

#endif // PERIODICTASK_H

