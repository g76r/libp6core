/*
Copyright 2012 Hallowyn and others.
See the NOTICE file distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file to you under
the Apache License, Version 2.0 (the "License"); you may not use this
file except in compliance with the License. You may obtain a copy of the
License at http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations
under the License.
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
