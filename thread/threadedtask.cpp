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
#include "threadedtask.h"
#include "threadedtaskthread.h"
#include <QtDebug>
#include <QAtomicInt>

static QAtomicInt _taskCounter = 1;

ThreadedTask::ThreadedTask(QString name, QString status)
  : _id(_taskCounter.fetchAndAddRelaxed(1)), _name(name), _status(status),
    _isRunning(false), _isFinished(false) {
  if (parent()) {
    // having a parent make moveToThread impossible
    qWarning() << "ThreadedTask must not have a parent (removing it)";
    setParent(0);
  }
  moveToThread(_thread = new ThreadedTaskThread(this));
  connect(_thread, SIGNAL(started()), this, SLOT(starting()));
  connect(_thread, SIGNAL(finished()), this, SLOT(finishing()));
}

ThreadedTask::ThreadedTask(ThreadedTaskThread *thread, QString name,
                           QString status)
  : _id(_taskCounter.fetchAndAddRelaxed(1)), _name(name), _status(status),
    _isRunning(false), _isFinished(false) {
  if (parent()) {
    // having a parent make moveToThread impossible
    qWarning() << "ThreadedTask must not have a parent (removing it)";
    setParent(0);
  }
  moveToThread(_thread = thread);
  connect(_thread, SIGNAL(started()), this, SLOT(starting()));
  connect(_thread, SIGNAL(finished()), this, SLOT(finishing()));
}

ThreadedTask::~ThreadedTask() {
  // testing _thread->isRunning() is useless since QThread emits finished()
  // before setting running to false, hence _thread->isRunning() can be true
  // on a finished() thread
  // this is the reason why we need to manage our own isRunning state
  if (_isRunning) {
    qWarning() << "ThreadedTask destroyed while thread still running"
               << this << _id << _name << _thread << _status;
    //           << ". Trying to terminate thread (this may lead to data "
    //              "corruption or strange behaviour";
    //_thread->terminate();
  }
  _thread->deleteLater();
}

void ThreadedTask::start() {
  _startMutex.lock();
  if (_isRunning || _isFinished) {
    qWarning() << "ThreadedTask::start called twice on same task" << _id
               << _name << _status;
    _startMutex.unlock();
    return;
  }
  _isRunning = true;
  _startMutex.unlock();
  _thread->start();
  emit taskStarted(_id);
}

void ThreadedTask::setStatus(const QString &status) {
  if (_status != status) {
    _status = status;
    emit statusChanged(id(), status);
  }
}

void ThreadedTask::starting() {
  emit taskStarted(_id);
}

void ThreadedTask::finishing() {
  _isRunning = false;
  _isFinished = true;
  emit taskFinished(_id);
}

void ThreadedTask::exec() {
  _thread->exec();
}

void ThreadedTask::exit(int retcode) {
  _thread->exit(retcode);
}
