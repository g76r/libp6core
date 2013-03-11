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
#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QTcpServer>
#include <QList>
#include "httphandler.h"
#include <QMutex>

class HttpWorker;

class LIBQTSSUSHARED_EXPORT HttpServer : public QTcpServer {
  Q_OBJECT
  QMutex _mutex;
  QList<HttpHandler *> _handlers;
  HttpHandler *_defaultHandler;
  QList<HttpWorker*> _workersPool;

public:
  explicit HttpServer(int workersPoolSize = 16, QObject *parent = 0);
  virtual ~HttpServer();
  /** Takes ownership of the handler */
  void appendHandler(HttpHandler *handler);
  /** Takes ownership of the handler */
  void prependHandler(HttpHandler *handler);
  HttpHandler *chooseHandler(HttpRequest req);

protected:
  void incomingConnection(int handle);

private slots:
  void connectionHandled(HttpWorker *worker);

private:
  Q_DISABLE_COPY(HttpServer)
};

#endif // HTTPSERVER_H
