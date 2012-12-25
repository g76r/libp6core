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
