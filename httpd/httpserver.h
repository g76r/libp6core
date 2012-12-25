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
#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QTcpServer>
#include <QList>
#include "httphandler.h"
#include <QMutex>

class LIBQTSSUSHARED_EXPORT HttpServer : public QTcpServer {
  Q_OBJECT
private:
  QMutex _mutex;
  QList<HttpHandler *> _handlers;
  HttpHandler *_defaultHandler;

public:
  explicit HttpServer(QObject *parent = 0);
  virtual ~HttpServer();
  /** handlers are deleted by ~HttpServer */
  // LATER make handlers inherit from QObject and take ownership of them
  void appendHandler(HttpHandler *handler);
  void prependHandler(HttpHandler *handler);
  HttpHandler *chooseHandler(const HttpRequest &req);

protected:
  void incomingConnection(int handle);

signals:

public slots:

private:
  Q_DISABLE_COPY(HttpServer)
};

#endif // HTTPSERVER_H
