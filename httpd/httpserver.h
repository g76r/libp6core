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
#include <QThread>

class HttpWorker;

class LIBQTSSUSHARED_EXPORT HttpServer : public QTcpServer {
  Q_OBJECT
  QMutex _handlersMutex;
  QList<HttpHandler *> _handlers;
  HttpHandler *_defaultHandler;
  QList<HttpWorker*> _workersPool;
  QList<int> _queuedSockets;
  int _maxQueuedSockets;
  QThread *_thread;

public:
  explicit HttpServer(int workersPoolSize = 16, int maxQueuedSockets = 32,
                      QObject *parent = 0);
  virtual ~HttpServer();
  /** The handler does not become a child of HttpServer but its deleteLater()
   * method is called by ~HttpServer(). */
  void appendHandler(HttpHandler *handler);
  /** The handler does not become a child of HttpServer but its deleteLater()
   * method is called by ~HttpServer(). */
  void prependHandler(HttpHandler *handler);
  HttpHandler *chooseHandler(HttpRequest req);
  bool listen(QHostAddress address = QHostAddress::Any, quint16 port = 0);
  bool listen(quint16 port) { return listen (QHostAddress::Any, port); }

protected:
  void incomingConnection(qintptr handle);

private slots:
  void connectionHandled(HttpWorker *worker);

private:
  Q_INVOKABLE bool doListen(QHostAddress address, quint16 port);
  Q_DISABLE_COPY(HttpServer)
};

#endif // HTTPSERVER_H
