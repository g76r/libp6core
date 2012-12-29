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
