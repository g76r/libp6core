/* Copyright 2012 Hallowyn and others.
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

