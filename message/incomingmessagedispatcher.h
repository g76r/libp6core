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
#ifndef INCOMINGMESSAGEDISPATCHER_H
#define INCOMINGMESSAGEDISPATCHER_H

#include "message.h"
#include "util/radixtree.h"

/** Dispatch incoming messages among registred handlers, depending on their
 * root node name. */
class LIBP6CORESHARED_EXPORT IncomingMessageDispatcher {

public:
  using MessageHandler = std::function<void(Message)>;

private:
  RadixTree<MessageHandler> _handlers;

public:
  /** not thread-safe, must only be called once at process initialization */
  void setHandlers(const RadixTree<MessageHandler> &handlers) {
    _handlers = handlers; }
  /** not thread-safe, must only be called by connection handler thread:
   * actually thread-safe per se (provided setHandlers() is not called
   * meanwhile) but the called handlers are not thread-safe */
  bool dispatch(Message message);
};

#endif // INCOMINGMESSAGEDISPATCHER_H

