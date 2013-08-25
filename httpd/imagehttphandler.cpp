/* Copyright 2013 Hallowyn and others.
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
#include "imagehttphandler.h"

ImageHttpHandler::ImageHttpHandler(QObject *parent) : HttpHandler(parent) {
}

ImageHttpHandler::ImageHttpHandler(QString urlPathPrefix, QObject *parent)
  : HttpHandler(parent), _urlPathPrefix(urlPathPrefix) {
}

bool ImageHttpHandler::acceptRequest(HttpRequest req) {
  return _urlPathPrefix.isEmpty()
      || req.url().path().startsWith(_urlPathPrefix);
}

bool ImageHttpHandler::handleRequest(HttpRequest req, HttpResponse res,
                                     HttpRequestContext ctxt) {
  Q_UNUSED(req)
  Q_UNUSED(ctxt)
  // TODO handle HTTP/304
  // LATER content type and content should be retrieve at once atomicaly
  // LATER pass params from request
  res.setContentType(contentType(0));
  QByteArray data = imageData(0);
  res.setContentLength(data.size());
  res.output()->write(data);
  return true;
}

QString ImageHttpHandler::source(ParamsProvider *params) const {
  Q_UNUSED(params);
  return QString();
}