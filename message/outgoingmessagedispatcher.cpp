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
#include "outgoingmessagedispatcher.h"
#include "log/log.h"
#include <QMetaObject>
#include "messagesender.h"
#include <QtDebug>

OutgoingMessageDispatcher *OutgoingMessageDispatcher::_instance = 0;
static QMutex _instanceMutex;

OutgoingMessageDispatcher::OutgoingMessageDispatcher(Behavior behavior)
  : _behavior(behavior), _lastInserted(0) {
  QMutexLocker ml(&_instanceMutex);
  Q_ASSERT(!_instance);
  _instance = this;
}

void OutgoingMessageDispatcher::doDispatch(Message message) {
  // TODO use queued calls, especialy because the mutex is still locked
  qint64 sessionid = message.session().id();
  QMutexLocker ml(&_mutex);
  if (_behavior == SendToLastRecordedSender) {
    //qDebug() << "OutgoingMessageDispatcher::doDispatch"
    //         << sessionid << message.node().name();
    if (_lastInserted)
      _lastInserted->sendOutgoingMessage(message);
    else
      Log::warning(sessionid)
          << "cannot dispatch outgoing message without a current sender "
          << message.node().name();
  } else {
    MessageSender *sender = _sessionSenders.value(sessionid);
    //qDebug() << "OutgoingMessageDispatcher::doDispatch"
    //         << sessionid << message.node().name() << sender;
    if (sender) {
      sender->sendOutgoingMessage(message);
    } else {
      Log::debug(sessionid)
          << "cannot dispatch outgoing message without a sender associated with "
             "the session: " << message.node().name();
    }
  }
}

void OutgoingMessageDispatcher::doSetSessionSender(
    qint64 sessionid, MessageSender *sender) {
  QMutexLocker ml(&_mutex);
  _sessionSenders.insert(sessionid, sender);
  _lastInserted = sender;
}

void OutgoingMessageDispatcher::doRemoveSessionSender(qint64 sessionid) {
  QMutexLocker ml(&_mutex);
  MessageSender *sender = _sessionSenders.take(sessionid);
  if (sender == _lastInserted)
    _lastInserted = 0;
}
