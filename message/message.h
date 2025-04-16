/* Copyright 2016-2025 Hallowyn, Gregoire Barbier and others.
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
#ifndef MESSAGE_H
#define MESSAGE_H

#include "session.h"
#include "pf/pfnode.h"

class MessageData;

/** Data object representing a message, regardless the network transport. */
class LIBP6CORESHARED_EXPORT Message {
  friend QDebug operator<<(QDebug dbg, const Message &message);
  friend p6::log::LogHelper operator<<(
      p6::log::LogHelper lh, const Message &message);
  QSharedDataPointer<MessageData> d;

public:
  Message(const Session &session, PfNode node);
  Message();
  Message(const Message &);
  Message &operator=(const Message &);
  ~Message();
  [[nodiscard]] inline bool isNull() const { return !d; }
  [[nodiscard]] inline bool operator!() const { return isNull(); }
  Session session() const;
  PfNode node() const;

private:
  QString debugString() const;
};

Q_DECLARE_METATYPE(Message)
Q_DECLARE_TYPEINFO(Message, Q_RELOCATABLE_TYPE);

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const Message &message);

p6::log::LogHelper LIBP6CORESHARED_EXPORT operator<<(
    p6::log::LogHelper lh, const Message &message);

#endif // MESSAGE_H
