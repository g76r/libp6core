/* Copyright 2013-2016 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
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
                                     ParamsProviderMerger *processingContext) {
  Q_UNUSED(req)
  Q_UNUSED(processingContext)
  // TODO handle HTTP/304
  // LATER content type and content should be retrieve at once atomicaly
  // LATER pass params from request
  res.setContentType(contentType(0));
  QString contentEncoding = this->contentEncoding(0);
  if (!contentEncoding.isEmpty())
    res.setHeader("Content-Encoding", contentEncoding);
  QByteArray data = imageData(0);
  res.setContentLength(data.size());
  if (req.method() != HttpRequest::HEAD)
    res.output()->write(data);
  return true;
}

QString ImageHttpHandler::source(ParamsProvider *params) const {
  Q_UNUSED(params);
  return QString();
}

QString ImageHttpHandler::contentEncoding(ParamsProvider *params) const {
  Q_UNUSED(params)
  return QString();
}
