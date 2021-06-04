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
#ifndef MESSAGESENDER_H
#define MESSAGESENDER_H

#include <QObject>
#include "message.h"

/** Abstract class to be extended by network transport implementation, such
 * as TcpConnectionHandler. */
class LIBP6CORESHARED_EXPORT MessageSender : public QObject {
  Q_OBJECT

public:
  explicit MessageSender(QObject *parent = 0);
  virtual void sendOutgoingMessage(Message message);
};

#endif // MESSAGESENDER_H
