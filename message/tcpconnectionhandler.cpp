/* Copyright 2016-2017 Hallowyn, Gregoire Barbier and others.
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
#include "tcpconnectionhandler.h"
#include <QAtomicInt>
#include "log/log.h"
#include <QHostAddress>
#include "pf/pfparser.h"
#include "pf/pfdomhandler.h"
#include <QCoreApplication>
#include "outgoingmessagedispatcher.h"
#include "sessionmanager.h"

static QAtomicInt _handlersCounter;

TcpConnectionHandler::TcpConnectionHandler(IncomingMessageDispatcher *dispatcher)
  : _thread(new QThread()), _socket(0), _session(0),
    _dispatcher(dispatcher) {
  _thread->setObjectName(QString("TcpConnectionHandler-%1")
                         .arg(_handlersCounter.fetchAndAddOrdered(1)));
  connect(this, &TcpConnectionHandler::destroyed, _thread, &QThread::quit);
  connect(_thread, &QThread::finished, _thread, &QThread::deleteLater);
  _thread->start();
  moveToThread(_thread);
  qMetaTypeId<QTcpSocket*>();
}

void TcpConnectionHandler::processConnection(
    QTcpSocket *socket, const Session &session) {
  QMutexLocker ml(&_mutex);
  _socket = socket;
  _session = session;
  ml.unlock();
  OutgoingMessageDispatcher::setSessionSender(session.id(), this);
  // sending QTcpSocket* through queued connection is safe because it cannot be
  // deleted before processing the call otherwise it wouldn't
  // this is guaranted because the only way to delete it is calling
  // releaseHandler() which is only called by doProcessConnection
  QMetaObject::invokeMethod(this, "doProcessConnection",
                            Q_ARG(QTcpSocket*, socket),
                            Q_ARG(Session, session));
}

void TcpConnectionHandler::doProcessConnection(QTcpSocket *, const Session &) {
  QString clientaddr = _session.string("clientaddr");
  Log::debug(_session.id()) << "processing new connection " << clientaddr
                            << _socket << _session;
  PfDomHandler handler;
  PfParser parser(&handler);
  PfOptions options = PfOptions().stopAfterFirstRootNode()
      .setReadTimeout(ACTIVITY_TIMEOUT);
  forever {
    if (!parser.parse(_socket, options)) {
      Log::warning(_session.id()) << "cannot parse pf document: " << clientaddr
                                  << " : " << handler.errorString();
      releaseHandler();
      break;
    }
    if (handler.roots().isEmpty()) {
      Log::debug(_session.id()) << "peer disconnected or timed out: "
                                << clientaddr;
      releaseHandler();
      break;
    } else {
      Message message(_session, handler.roots().first());
      Log::debug(_session.id()) << "<<< "
                                << QString::fromUtf8(message.node().toPf());
      _dispatcher->dispatch(message);
    }
    handler.clear();
    QCoreApplication::processEvents();
  }
}

void TcpConnectionHandler::sendOutgoingMessage(Message message) {
  QByteArray ba = message.node().toPf();
  QMutexLocker ml(&_mutex);
  if (_socket) {
    if (_socket->write(ba) == -1) {
      Log::warning(_session.id()) << "cannot send outgoing message : "
                                  << _socket->errorString() << " : " << message;
    }
    _socket->write("\n");
    Log::debug(_session.id()) << ">>> " << QString::fromUtf8(ba);
    _socket->flush();
  } else {
    Log::warning() << "cannot send outgoing message : "
                      "connection disappeared : " << message;
  }
}

void TcpConnectionHandler::releaseHandler() {
  OutgoingMessageDispatcher::removeSessionSender(_session.id());
  QMutexLocker ml(&_mutex);
  //disconnect(_socket);
  //_socket->flush();
  //if (_socket->state() == QAbstractSocket::ConnectedState)
  //  _socket->waitForBytesWritten(FLUSH_TIMEOUT);
  qDebug() << "  before deleting" << _socket;
  // FIXME need to delete again socket after solving multithread issue
  // delete _socket;
  qDebug() << "  after";
  _socket = 0;
  SessionManager::closeSession(_session.id());
  _session = Session();
  ml.unlock();
  emit handlerReleased(this);
}

