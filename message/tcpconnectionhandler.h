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
#ifndef TCPCONNECTIONHANDLER_H
#define TCPCONNECTIONHANDLER_H

#include "messagesender.h"
#include "incomingmessagedispatcher.h"
#include <QMutex>

class QTcpSocket;
class QThread;

/** Object responsible for processing an established TCP connection.
 * Managed by TcpListener on the server side and TcpClient on the the
 * client side. */
class LIBP6CORESHARED_EXPORT TcpConnectionHandler : public MessageSender {
  Q_OBJECT
  QThread *_thread;
  QTcpSocket *_socket;
  Session _session;
  IncomingMessageDispatcher *_dispatcher;
  QMutex _mutex;

public:
  static const int ACTIVITY_TIMEOUT = 60000; // ms

  explicit TcpConnectionHandler(IncomingMessageDispatcher *dispatcher);
  /** thread-safe, can be called by any thread */
  void processConnection(QTcpSocket *socket, const Session &session);
  /** not-thread safe: must only be called by the thread handling the
   * connection, or through signals
   * however currently processConnection is blocking and this method cannot be
   * exposed through slot or Q_INVOKABLE for queued calls */
  void sendOutgoingMessage(Message message) override;

signals:
  void handlerReleased(TcpConnectionHandler *handler);

private:
  void releaseHandler();
};

#endif // TCPCONNECTIONHANDLER_H
