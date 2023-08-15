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
#ifndef TCPLISTENER_H
#define TCPLISTENER_H

#include "incomingmessagedispatcher.h"
#include <QObject>
#include <QHostAddress>

class TcpConnectionHandler;
class QTcpServer;

/** Object responsible for listening and accepting new TCP connections.
 * Manage a pool of TcpConnectionHandlers that process established connections
 * and call IncomingMessageDispatcher when needed. */
class LIBP6CORESHARED_EXPORT TcpListener : public QObject {
  Q_OBJECT
  QThread *_thread;
  QTcpServer *_server;
  IncomingMessageDispatcher *_dispatcher;
  QList<TcpConnectionHandler*> _idleHandlers, _allHandlers;

public:
  explicit TcpListener(IncomingMessageDispatcher *dispatcher);
  /** thread-safe */
  bool listen(const QHostAddress &address, quint16 port = 0);
  /** thread-safe */
  bool listen(quint16 port) { return listen(QHostAddress::Any, port); }

private:
  void newConnection();
  void handlerReleased(TcpConnectionHandler *handler);
};

#endif // TCPLISTENER_H
