/* Copyright 2012-2025 Hallowyn, Gregoire Barbier and others.
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
#include "httphandler.h"

static const QRegularExpression _multipleSlashRE("//+");

HttpHandler::HttpHandler(const Utf8String &name, QObject *parent)
  : QObject(parent), _name(name) {
  auto origins = qEnvironmentVariable("HTTP_ALLOWED_CORS_ORIGINS");
  for (const auto &origin : origins.split(';', Qt::SkipEmptyParts)) {
    if (origin == "*") {
      _corsOrigins.clear();
      return;
    }
    _corsOrigins.append(QRegularExpression(origin));
  }
}

Utf8String HttpHandler::name() const {
  if (!_name.isEmpty())
      return _name;
  if (!objectName().isEmpty())
    return objectName();
  return metaObject()->className();
}

bool HttpHandler::redirectForUrlCleanup(
    HttpRequest &req, HttpResponse &res, ParamsProviderMerger &) {
  QString reqPath = req.path();
  if (reqPath.contains(_multipleSlashRE)) {
    int depth = reqPath.count('/');
    reqPath.replace(_multipleSlashRE, u"/"_s);
    if (reqPath.startsWith('/')) {
      --depth;
      reqPath.remove(0, 1);
    }
    QUrl url;
    url.setPath(u"../"_s.repeated(depth)+reqPath);
    QUrlQuery query(req.url());
    if (query.isEmpty())
      res.redirect(url.path(QUrl::FullyEncoded));
    else
      res.redirect(url.path(QUrl::FullyEncoded)+"?"
                   +QUrlQuery(req.url()).toString(QUrl::FullyEncoded));
    return true;
  }
  return false;
}

bool HttpHandler::handleCORS(
    HttpRequest &req, HttpResponse &res, const Utf8StringSet &methods) {
  if (!methods.contains("OPTIONS"_u8))
    qWarning() << "HttpHandler::handleCORS(): OPTIONS method should be included"
                  " in methods set whereas it was not: " << methods;
  res.append_value_to_header("Vary", "Origin");
  auto origin = req.header("Origin");
  if (origin.isEmpty())
    goto ignored;
  if (_corsOrigins.isEmpty())
    goto granted;
  for (const QRegularExpression &re : _corsOrigins)
    if (re.match(origin).hasMatch())
      goto granted;
  goto denied;
granted:
  res.set_header("Access-Control-Allow-Origin", origin);
  if (req.method() == HttpRequest::OPTIONS) {
    res.set_header("Access-Control-Allow-Methods", methods.sorted_join(", "));
    res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type"); // Origin, Accept, Authorization ?
    res.set_header("Access-Control-Allow-Credentials", "true");
    res.set_header("Access-Control-Max-Age", "86400");
  }
ignored:
denied:
  return req.method() == HttpRequest::OPTIONS;
}

bool HttpHandler::acceptRequest(HttpRequest &) {
  return false;
}

bool HttpHandler::handleRequest(HttpRequest&, HttpResponse &,
                                ParamsProviderMerger &) {
  return false;
}
