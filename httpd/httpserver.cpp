/* Copyright 2012-2013 Hallowyn and others.
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
  connect(this, SIGNAL(destroyed(QObject*)), _thread, SLOT(quit()));
  connect(_thread, SIGNAL(finished()), _thread, SLOT(deleteLater()));
  _thread->start();
  _defaultHandler = new PipelineHttpHandler(this);
  for (int i = 0; i < workersPoolSize; ++i) {
    HttpWorker *worker = new HttpWorker(this);
    // cannot make workers become children, and cannot rely on _workersPool to
    // remove them in ~HttpServer since some may be in use, hence connecting
    // server's destroyed() to workers' deleteLater()
    connect(this, SIGNAL(destroyed()), worker, SLOT(deleteLater()));
    _workersPool.append(worker);
  }
  moveToThread(_thread);
}

HttpServer::~HttpServer() {
}

void HttpServer::incomingConnection(int socketDescriptor)  {
  if (_workersPool.size() > 0) {
    HttpWorker *worker = _workersPool.takeFirst();
    connect(worker, SIGNAL(connectionHandled(HttpWorker*)),
            this, SLOT(connectionHandled(HttpWorker*)));
    QMetaObject::invokeMethod(worker, "handleConnection",
                              Q_ARG(int, socketDescriptor));
  } else {
    if (_queuedSockets.size() < _maxQueuedSockets) {
      _queuedSockets.append(socketDescriptor);
    } else {
      Log::error() << "no HttpWorker available in pool and maximum queue size"
                      "reached, throwing incoming connection away";
      ::close(socketDescriptor);
    }
  }
}

void HttpServer::connectionHandled(HttpWorker *worker) {
  if (_queuedSockets.isEmpty()) {
    _workersPool.append(worker);
    disconnect(worker, SIGNAL(connectionHandled(HttpWorker*)),
               this, SLOT(connectionHandled(HttpWorker*)));
  } else {
    QMetaObject::invokeMethod(worker, "handleConnection",
                              Q_ARG(int, _queuedSockets.takeFirst()));
  }
}

void HttpServer::appendHandler(HttpHandler *handler) {
  QMutexLocker ml(&_handlersMutex);
  _handlers.append(handler);
  // cannot make handlers become children, hence connecting
  // server's destroyed() to handlers' deleteLater()
  connect(this, SIGNAL(destroyed()), handler, SLOT(deleteLater()));
}

void HttpServer::prependHandler(HttpHandler *handler) {
  QMutexLocker ml(&_handlersMutex);
  _handlers.prepend(handler);
  // cannot make handlers become children, hence connecting
  // server's destroyed() to handlers' deleteLater()
  connect(this, SIGNAL(destroyed()), handler, SLOT(deleteLater()));
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
    success = doListen(address, port);
  else
    QMetaObject::invokeMethod(this, "doListen", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, success),
                              Q_ARG(QHostAddress, address),
                              Q_ARG(quint16, port));
  return success;
}

bool HttpServer::doListen(QHostAddress address, quint16 port) {
  return QTcpServer::listen(address, port);
}
