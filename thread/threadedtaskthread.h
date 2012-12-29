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
#ifndef THREADEDTASKTHREAD_H
#define THREADEDTASKTHREAD_H

#include <QThread>
#include "libqtssu_global.h"

class ThreadedTask;

/** ThreadedTaskThread is an helper class for ThreadedTask.
  * Do not directly instanciate it or extend it.
  * @see ThreadedTask
  */
class LIBQTSSUSHARED_EXPORT ThreadedTaskThread : public QThread {
  Q_OBJECT
  friend class ThreadedTask;
private:
  ThreadedTask *_task;

public:
  explicit ThreadedTaskThread(ThreadedTask *task);
  void run();

private:
  Q_DISABLE_COPY(ThreadedTaskThread)
};

#endif // THREADEDTASKTHREAD_H
