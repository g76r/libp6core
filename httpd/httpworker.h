/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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
#ifndef HTTPWORKER_H
#define HTTPWORKER_H

#include <QThread>
#include "httpserver.h"
#include "libp6core_global.h"

class QTcpSocket;

class LIBP6CORESHARED_EXPORT HttpWorker : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(HttpWorker)
  HttpServer *_server;
  QThread *_thread;
  QByteArray _defaultCacheControlHeader;

public:
  explicit HttpWorker(HttpServer *server);

public slots:
  void handleConnection(
      int socketDescriptor, std::function<void(void)> handledCallback);
};

#endif // HTTPWORKER_H
