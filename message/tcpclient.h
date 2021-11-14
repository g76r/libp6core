/* Copyright 2016-2021 Hallowyn, Gregoire Barbier and others.
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
#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include "incomingmessagedispatcher.h"
#include <QHostAddress>

class TcpConnectionHandler;
class Session;

/** Object responsible for (re)connecting to the server via TCP.
 * Create a TcpConnectionHandler that processes established connections
 * and call MessageDispatcher when needed. */
class LIBP6CORESHARED_EXPORT TcpClient : public QObject {
  Q_OBJECT
  QThread *_thread;
  IncomingMessageDispatcher *_dispatcher;
  TcpConnectionHandler* _handler;
  QHostAddress _address;
  quint16 _port;
  Session _session;

public:
  explicit TcpClient(IncomingMessageDispatcher *dispatcher);
  /** thread-safe */
  void connectToHost(const QHostAddress &address, quint16 port = 0);

signals:
  void connecting();
  void connected();

private:
  void tryConnect(TcpConnectionHandler*);
};

#endif // TCPCLIENT_H
