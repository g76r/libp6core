/* Copyright 2012-2021 Hallowyn, Gregoire Barbier and others.
 * This file is part of libpumpkin, see <http://libpumpkin.g76r.eu/>.
 * Libpumpkin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libpumpkin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libpumpkin.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "httpserver.h"
#include "httpworker.h"
#include <QMutexLocker>
#include "log/log.h"
#include <unistd.h>
#include "pipelinehttphandler.h"

HttpServer::HttpServer(int workersPoolSize, int maxQueuedSockets,
                       QObject *parent)
  : QTcpServer(parent), _defaultHandler(0), _maxQueuedSockets(maxQueuedSockets),
    _thread(new QThread()) {
  _thread->setObjectName("HttpServer");
  connect(this, &HttpServer::destroyed, _thread, &QThread::quit);
  connect(_thread, &QThread::finished, _thread, &QThread::deleteLater);
  _thread->start();
  _defaultHandler = new PipelineHttpHandler(this);
  for (int i = 0; i < workersPoolSize; ++i) {
    HttpWorker *worker = new HttpWorker(this);
    // cannot make workers become children, and cannot rely on _workersPool to
    // remove them in ~HttpServer since some may be in use, hence connecting
    // server's destroyed() to workers' deleteLater()
    connect(this, &HttpServer::destroyed, worker, &HttpWorker::deleteLater);
    _workersPool.append(worker);
  }
  moveToThread(_thread);
}

HttpServer::~HttpServer() {
}

void HttpServer::incomingConnection(qintptr socketDescriptor)  {
  int fd = (int)socketDescriptor;
  if (!_workersPool.isEmpty()) {
    HttpWorker *worker = _workersPool.takeFirst();
    QMetaObject::invokeMethod(worker, [this,worker,fd](){
      worker->handleConnection(fd, [this,worker](){
        connectionHandled(worker);
      });
    });
  } else {
    if (_queuedSockets.size() < _maxQueuedSockets) {
      _queuedSockets.append(fd);
    } else {
      Log::error() << "no HttpWorker available in pool and maximum queue size"
                      "reached, rejecting incoming connection";
      ::close(fd);
    }
  }
}

void HttpServer::connectionHandled(HttpWorker *worker) {
  if (_queuedSockets.isEmpty()) {
    _workersPool.append(worker);
  } else {
    int fd = _queuedSockets.takeFirst();
    QMetaObject::invokeMethod(worker, [this,worker,fd](){
      worker->handleConnection(fd, [this,worker](){
        connectionHandled(worker);
      });
    });
  }
}

void HttpServer::appendHandler(HttpHandler *handler) {
  QMutexLocker ml(&_handlersMutex);
  _handlers.append(handler);
  // cannot make handlers become children, hence connecting
  // server's destroyed() to handlers' deleteLater()
  connect(this, &HttpServer::destroyed, handler, &HttpHandler::deleteLater);
}

void HttpServer::prependHandler(HttpHandler *handler) {
  QMutexLocker ml(&_handlersMutex);
  _handlers.prepend(handler);
  // cannot make handlers become children, hence connecting
  // server's destroyed() to handlers' deleteLater()
  connect(this, &HttpServer::destroyed, handler, &HttpHandler::deleteLater);
}

HttpHandler *HttpServer::chooseHandler(HttpRequest req) {
  QMutexLocker ml(&_handlersMutex);
  foreach (HttpHandler *h, _handlers) {
    if (h->acceptRequest(req))
      return h;
  }
  return _defaultHandler;
}

bool HttpServer::listen(QHostAddress address, quint16 port) {
  // the constructor calls moveToThread() and QTcpServer::listen must be called
  // by owner thread (at less because it creates QObjects using this as parent)
  bool success;
  if (_thread == QThread::currentThread())
    success = QTcpServer::listen(address, port);
  else
    QMetaObject::invokeMethod(this, [this,&success,address,port](){
      success = QTcpServer::listen(address, port);
    }, Qt::BlockingQueuedConnection);
  return success;
}
