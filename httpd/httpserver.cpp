/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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
#include "log/log.h"
#include "pipelinehttphandler.h"
#include "util/radixtree.h"
#include <QMutexLocker>
#include <QThread>
#include <QMetaObject>
#include <unistd.h>

#define DEFAULT_LOG_FORMAT "HTTP %[http]url %[http]method %[http]status %[http]servicems %[http]clientaddresses"_u8

HttpServer::HttpServer(int workersPoolSize, int maxQueuedSockets,
                       QObject *parent)
  : QTcpServer(parent), _defaultHandler(0), _maxQueuedSockets(maxQueuedSockets),
    _thread(new QThread()),
    _logPolicy(HttpServer::logPolicyFromText(
                 ParamsProvider::environment()->paramUtf8(
                   "HTTPD_LOG_POLICY", "LogErrorHits"))),
    _logFormat(ParamsProvider::environment()->paramRawUtf8(
                 "HTTPD_LOG_FORMAT", DEFAULT_LOG_FORMAT)) {
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
    connect(worker, &HttpWorker::connectionHandled,
            this, &HttpServer::connectionHandled);
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
    QMetaObject::invokeMethod(worker, [worker,fd](){
      worker->handleConnection(fd);
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
    QMetaObject::invokeMethod(worker, [worker,fd](){
      worker->handleConnection(fd);
    });
  }
}

HttpServer &HttpServer::appendHandler(HttpHandler *handler) {
  QMutexLocker ml(&_handlersMutex);
  _handlers.append(handler);
  // cannot make handlers become children, hence connecting
  // server's destroyed() to handlers' deleteLater()
  connect(this, &HttpServer::destroyed, handler, &HttpHandler::deleteLater);
  return *this;
}

HttpServer &HttpServer::prependHandler(HttpHandler *handler) {
  QMutexLocker ml(&_handlersMutex);
  _handlers.prepend(handler);
  // cannot make handlers become children, hence connecting
  // server's destroyed() to handlers' deleteLater()
  connect(this, &HttpServer::destroyed, handler, &HttpHandler::deleteLater);
  return *this;
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

static RadixTree<HttpServer::LogPolicy> _logPoliciesFromText {
  { "LogDisabled", HttpServer::LogDisabled },
  { "LogErrorHits", HttpServer::LogErrorHits },
  { "LogAllHits", HttpServer::LogAllHits },
};

static auto _logPoliciesToText = _logPoliciesFromText.toReversedUtf8Map();

Utf8String HttpServer::logPolicyAsText(LogPolicy policy) {
  return _logPoliciesToText.value(policy, "LogDisabled"_u8);
}

HttpServer::LogPolicy HttpServer::logPolicyFromText(const Utf8String &text) {
  return _logPoliciesFromText.value(text, LogDisabled);
}

void HttpServer::close() {
  auto shutdown = [this]() {
    QTcpServer::close();
  };
  if (this->thread() == QThread::currentThread())
    shutdown();
  else
    QMetaObject::invokeMethod(this, [shutdown](){
      shutdown();
    }, Qt::BlockingQueuedConnection);
}
