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
#include "tcpclient.h"
#include "tcpconnectionhandler.h"
#include "log/log.h"
#include "session.h"
#include "sessionmanager.h"
#include <QThread>
#include <QTcpSocket>

TcpClient::TcpClient(IncomingMessageDispatcher *dispatcher)
  : QObject(), _thread(new QThread), _dispatcher(dispatcher), _handler(0),
    _port(0) {
  _thread->setObjectName("TcpClient");
  connect(this, &TcpConnectionHandler::destroyed, _thread, &QThread::quit);
  connect(_thread, &QThread::finished, _thread, &QThread::deleteLater);
  _thread->start();
  moveToThread(_thread);
  _handler = new TcpConnectionHandler(dispatcher);
  connect(_handler, &TcpConnectionHandler::handlerReleased,
          this, &TcpClient::tryConnect);
  // TODO may need to connect to some socket events too
}

void TcpClient::connectToHost(const QHostAddress &address, quint16 port) {
  QMetaObject::invokeMethod(this, [this,address,port]() {
    _address = address;
    _port = port;
    tryConnect(0);
  }, Qt::QueuedConnection);
}

void TcpClient::tryConnect(TcpConnectionHandler*) {
  qDebug() << "connecting";
  emit connecting();
  qDebug() << "connecting signal emitted";
  QTcpSocket *socket = new QTcpSocket;
  socket->connectToHost(_address, _port);
  bool result = socket->waitForConnected(3000);
  qDebug() << "waitForConnected" << result;
  if (!result) {
    Log::warning() << "cannot connect to server: " << socket->errorString();
    QMetaObject::invokeMethod(this, [this](){
      tryConnect(0);
    }, Qt::QueuedConnection);
    // FIXME need to delete again socket after solving multithread issue
    //delete socket;
    return;
  }
  Log::debug() << "successfuly connected to server";
  _session = SessionManager::createSession();
  socket->moveToThread(_handler->thread());
  _handler->processConnection(socket, _session);
  emit connected();
  qDebug() << "connected";
}
