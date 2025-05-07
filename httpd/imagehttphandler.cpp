/* Copyright 2013-2025 Hallowyn, Gregoire Barbier and others.
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
#include "imagehttphandler.h"

ImageHttpHandler::ImageHttpHandler(QObject *parent) : HttpHandler(parent) {
}

ImageHttpHandler::ImageHttpHandler(
    const QByteArray &urlPathPrefix, QObject *parent)
  : HttpHandler(parent), _urlPathPrefix(urlPathPrefix) {
}

bool ImageHttpHandler::acceptRequest(HttpRequest &req) {
  return _urlPathPrefix.isEmpty()
      || req.path().startsWith(_urlPathPrefix);
}

bool ImageHttpHandler::handleRequest(HttpRequest &req, HttpResponse &res,
                                     ParamsProviderMerger &request_context) {
  // TODO handle HTTP/304
  // LATER content type and content should be retrieve at once atomicaly
  // LATER pass params from request
  if (handleCORS(req, res))
    return true;
  res.set_content_type(contentType(req, request_context));
  auto contentEncoding = this->contentEncoding(req, request_context);
  if (!contentEncoding.isEmpty())
    res.set_header("Content-Encoding"_u8, contentEncoding);
  QByteArray data = imageData(req, request_context);
  res.set_content_length(data.size());
  if (req.method() != HttpRequest::HEAD)
    res.output()->write(data);
  return true;
}

QByteArray ImageHttpHandler::source(
    HttpRequest &, ParamsProviderMerger &) const {
  return {};
}

QByteArray ImageHttpHandler::contentEncoding(
  HttpRequest&, ParamsProviderMerger &) const {
  return {};
}

QByteArray ImageHttpHandler::imageData(
  HttpRequest&, ParamsProviderMerger &, int) {
  return {};
}

QByteArray ImageHttpHandler::contentType(
    HttpRequest &, ParamsProviderMerger &) const {
  return {};
}
