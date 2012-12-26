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
#include "httpserver.h"
#include "httpworker.h"
#include <QMutexLocker>

class DefaultHandler : public HttpHandler {
  virtual QString name() const {
    return "default";
  }

  virtual void handleRequest(HttpRequest &req, HttpResponse &res) {
    // TODO handle HEAD request (I don't know yet the most usefull way)
    res.setStatus(404);
    res.output()->write("Error 404 - Not found");
    //qDebug() << "serving with default handler" << req.methodName() << req.url();
  }

  virtual bool acceptRequest(const HttpRequest &req) {
    Q_UNUSED(req)
    return true;
  }
};

HttpServer::HttpServer(QObject *parent) : QTcpServer(parent),
_defaultHandler(new DefaultHandler()){
}

HttpServer::~HttpServer() {
  delete _defaultHandler;
  qDeleteAll(_handlers);
}

void HttpServer::incomingConnection(int socketDescriptor)  {
  HttpWorker *worker = new HttpWorker(socketDescriptor, this);
  connect(worker, SIGNAL(taskFinished(long)), worker, SLOT(deleteLater()));
  worker->start();
}

void HttpServer::appendHandler(HttpHandler *handler) {
  QMutexLocker ml(&_mutex);
  _handlers.append(handler);
}

void HttpServer::prependHandler(HttpHandler *handler) {
  QMutexLocker ml(&_mutex);
  _handlers.prepend(handler);
}

HttpHandler *HttpServer::chooseHandler(const HttpRequest &req) {
  QMutexLocker ml(&_mutex);
  foreach (HttpHandler *h, _handlers) {
    if (h->acceptRequest(req))
      return h;
  }
  return _defaultHandler;
}
