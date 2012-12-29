/* Copyright 2012 Hallowyn and others.
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

#include "thread/threadedtask.h"
#include "httpserver.h"
#include "libqtssu_global.h"

class QTcpSocket;

class LIBQTSSUSHARED_EXPORT HttpWorker : public ThreadedTask {
  Q_OBJECT
  HttpServer *_server;
  int _socketDescriptor;

public:
  explicit HttpWorker(int socketDescriptor, HttpServer *parent);
  // virtual ~HttpWorker() { qDebug("~HttpWorker"); }
  void run();

private:
  Q_DISABLE_COPY(HttpWorker)
};

#endif // HTTPWORKER_H
