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
#ifndef OUTGOINGMESSAGEDISPATCHER_H
#define OUTGOINGMESSAGEDISPATCHER_H

#include "message.h"
#include <QMutex>

class MessageSender;

/** Dispatch outgoing messages among registred senders, depending on their
 * session id. */
class LIBP6CORESHARED_EXPORT OutgoingMessageDispatcher {
public:
  enum Behavior {
    DispatchAmongSessions, // intended for multiple peers (server)
    SendToLastRecordedSender, // intended for auto-reconnection (client)
  };

private:
  Behavior _behavior;
  QHash<qint64,MessageSender*> _sessionSenders; // sesionid -> messagesender
  MessageSender *_lastInserted;
  QMutex _mutex;
  static OutgoingMessageDispatcher *_instance;

public:
  OutgoingMessageDispatcher(Behavior behavior);
  /** thread-safe */
  static void dispatch(Message message) { instance()->doDispatch(message); }
  /** thread-safe */
  static void setSessionSender(qint64 sessionid, MessageSender *sender) {
    instance()->doSetSessionSender(sessionid, sender); }
  /** thread-safe */
  static void removeSessionSender(qint64 sessionid) {
    instance()->doRemoveSessionSender(sessionid); }

private:
  static OutgoingMessageDispatcher *instance() {
    Q_ASSERT(_instance);
    return _instance;
  }
  /** thread-safe */
  void doDispatch(Message message);
  /** thread-safe */
  void doSetSessionSender(qint64 sessionid, MessageSender *sender);
  /** thread-safe */
  void doRemoveSessionSender(qint64 sessionid);
};

#endif // OUTGOINGMESSAGEDISPATCHER_H
