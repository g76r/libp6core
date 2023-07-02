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
#include "message.h"

class MessageData : public QSharedData {
public:
  Session _session;
  PfNode _node;
  MessageData(const Session session = Session(), PfNode node = PfNode())
    : _session(session), _node(node) { }
};

Message::Message(const Session &session, PfNode node)
  : d(new MessageData(session, node)) {
}

Message::Message() {
}


Message::Message(const Message &other) : d(other.d) {
}

Message &Message::operator=(const Message &other) {
  if (this != &other)
    d.operator=(other.d);
  return *this;
}

Message::~Message() {
}

Session Message::session() const {
  return d->_session;
}

PfNode Message::node() const {
  return d ? d->_node : PfNode();
}

QString Message::debugString() const {
  return d ? QStringLiteral("Message(clientaddr: %1, login: %2, name: %3)")
             .arg(d->_session.string("clientaddr"))
             .arg(d->_session.string("login"))
             .arg(d->_node.name())
           : QString();
}

QDebug operator<<(QDebug dbg, const Message &message) {
  dbg.nospace() << message.debugString();
  return dbg.space();
}

LogHelper operator<<(LogHelper lh, const Message &message) {
  return lh << message.debugString();
}
