/* Copyright 2016-2023 Hallowyn, Gregoire Barbier and others.
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
#include "tcplistener.h"
#include "log/log.h"
#include "sessionmanager.h"
#include "tcpconnectionhandler.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>

#define HANDLERS_POOL_SIZE 20

TcpListener::TcpListener(IncomingMessageDispatcher *dispatcher)
  : QObject(), _thread(new QThread), _server(new QTcpServer(this)),
    _dispatcher(dispatcher) {
  _thread->setObjectName("TcpListener");
  connect(this, &TcpConnectionHandler::destroyed, _thread, &QThread::quit);
  connect(_thread, &QThread::finished, _thread, &QThread::deleteLater);
  _thread->start();
  moveToThread(_thread);
  for (int i = 0; i < HANDLERS_POOL_SIZE; ++i) {
    TcpConnectionHandler *handler = new TcpConnectionHandler(dispatcher);
    _idleHandlers.append(handler);
    _allHandlers.append(handler);
    connect(handler, &TcpConnectionHandler::handlerReleased,
            this, &TcpListener::handlerReleased);
  }
  connect(_server, &QTcpServer::newConnection,
          this, &TcpListener::newConnection);
}

bool TcpListener::listen(const QHostAddress &address, quint16 port) {
  // the constructor calls moveToThread() and QTcpServer::listen must be called
  // by owner thread (at less because it creates QObjects using this as parent)
  bool success;
  if (!_server) // should never happen
    return false;
  if (_thread == QThread::currentThread())
    success = _server->listen(address, port);
  else
    QMetaObject::invokeMethod(this, [this,&success,address,port](){
      success = _server->listen(address, port);
    }, Qt::BlockingQueuedConnection);
  return success;
}

void TcpListener::newConnection() {
  QTcpSocket *socket = _server->nextPendingConnection();
  if (!socket) // should never happen
    return;
  Session session = SessionManager::createSession();
  QString clientaddr = "["+socket->peerAddress().toString()
      +"]:"+QString::number(socket->peerPort());
  session.setParam("clientaddr", clientaddr);
  if (_idleHandlers.size() == 0) {
    Log::warning(session.id()) << "cannot handle connection: no idle handler"
                               << clientaddr;
    // LATER leave a reply to client
    delete socket;
  } else {
    TcpConnectionHandler *handler = _idleHandlers.takeFirst();
    socket->setParent(0);
    socket->moveToThread(handler->thread());
    handler->processConnection(socket, session);
  }
}

void TcpListener::handlerReleased(TcpConnectionHandler *handler) {
  _idleHandlers.append(handler);
}
