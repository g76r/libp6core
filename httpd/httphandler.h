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
#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include "httprequest.h"
#include "httpresponse.h"
#include <QObject>

class LIBQTSSUSHARED_EXPORT HttpHandler : public QObject {
  Q_OBJECT
public:
  inline HttpHandler(QObject *parent = 0) : QObject(parent) { }
  virtual QString name() const = 0;
  virtual bool acceptRequest(const HttpRequest &req) = 0;
  virtual void handleRequest(HttpRequest &req, HttpResponse &res) = 0;

private:
  Q_DISABLE_COPY(HttpHandler)
  // LATER give handlers their own threads and make server and handler threads exchange signals
};

#endif // HTTPHANDLER_H
