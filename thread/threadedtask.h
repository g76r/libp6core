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
#ifndef THREADEDTASK_H
#define THREADEDTASK_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include "libqtssu_global.h"

class ThreadedTaskThread;

// MAYDO add a QRunnable adapter to make it possible to enroll tasks in QThreadPool

/** ThreadedTask is a base class for any task that should be run in a dedicated
  * thread.
  *
  * To create a CPU-intensive or blocking task run only once, one should extends
  * this class and implement run() method, which will be run when the thread
  * will be started (through start() method). The task must be started only
  * once.
  * Keep in mind that if the task has to receive events (including queued
  * signals) it must call QCoreApplication::processEvents() on a regular basis.
  *
  * To create a periodic task, one should rather extend PeriodicTask instead.
  *
  * To read more about threads in Qt, this article is a very good information
  * source: http://qt-project.org/wiki/ThreadsEventsQObjects
  *
  * @see PeriodicTask
  */
class LIBQTSSUSHARED_EXPORT ThreadedTask : public QObject {
  Q_OBJECT
private:
  long _id;
  const QString _name;
  ThreadedTaskThread *_thread;
  QString _status;
  bool _isRunning, _isFinished;
  QMutex _startMutex;

protected:
  /** Constructor for subclasses that would need to use their own QThread
    * implementation.
    * Warning: The QThread implementation must have no QObject parent and
    * must call
    * ThreadedTask::moveToThread(QCoreApplication::instance()->thread()) at the
    * end of its run() method. It will be destroyed automatically after
    * ThreadTask destruction (through deleteLater()).
    */
  explicit ThreadedTask(ThreadedTaskThread *thread,
                        QString name = "ThreadedTask",
                        QString status = tr("unknown"));

public:
  explicit ThreadedTask(QString name = "ThreadedTask",
                        QString status = tr("unknown"));
  virtual ~ThreadedTask();
  virtual void run() = 0;
  virtual void start();
  // LATER try to make the task implictly shared, if possible to avoid
  // isRunning/isFinished segfault when called on a deleted ThreadedTask*
  // (QObject make it hard, maybe have a shared handle which would not be a
  // QObject but would contain data like status, isRunning, id...)
  // this would make it possible to keep a handle on a deleted finished task
  // this handle may for instance look like QFuture
  /** beware of not calling this method on a deleted task since a finished
    * task may be deleted soon after finishing
    */
  inline bool isRunning() { return _isRunning; }
  /** beware of not calling this method on a deleted task since a finished
    * task may be deleted soon after finishing
    */
  inline bool isFinished() const { return _isFinished; }
  inline QString name() const { return _name; }
  inline long id() const { return _id; }
  inline QString status() const { return _status; }

protected:
  /** This method is not thread safe and must only be called from the task's
    * thread.
    */
  void setStatus(const QString &status);
  /** Enter the thread event loop.
    * @see QThread::exec()
    */
  void exec();
  /** Stop the thread event loop. Only usefull if run() calls exec().
    * @see QThread::exit()
    */
  void exit(int retcode);

private slots:
  void starting();
  void finishing();

signals:
  void taskStarted(long id);
  void taskFinished(long id);
  void statusChanged(long id, const QString &status);

private:
  Q_DISABLE_COPY(ThreadedTask)
};

#endif // THREADEDTASK_H
