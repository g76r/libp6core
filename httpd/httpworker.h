/* Copyright 2012-2013 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
 * Libqtssu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libqtssu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef HTTPWORKER_H
#define HTTPWORKER_H

#include <QThread>
#include "httpserver.h"
#include "libqtssu_global.h"

class QTcpSocket;

class LIBQTSSUSHARED_EXPORT HttpWorker : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(HttpWorker)
  HttpServer *_server;
  QThread *_thread;

public:
  explicit HttpWorker(HttpServer *server);

public slots:
  void handleConnection(int socketDescriptor);

signals:
  void connectionHandled(HttpWorker *worker);
};

#endif // HTTPWORKER_H
