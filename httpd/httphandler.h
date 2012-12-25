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
#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include "httprequest.h"
#include "httpresponse.h"

class LIBQTSSUSHARED_EXPORT HttpHandler {
public:
  inline HttpHandler() { }
  virtual ~HttpHandler();
  virtual QString name() const = 0;
  virtual void handleRequest(HttpRequest &req, HttpResponse &res) = 0;
  virtual bool acceptRequest(const HttpRequest &req) = 0;

signals:

public slots:

private:
  Q_DISABLE_COPY(HttpHandler)
  // LATER give handlers their own threads and make server and handler threads exchange signals
};

#endif // HTTPHANDLER_H
