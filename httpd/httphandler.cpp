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
#include "httphandler.h"
#include <QRegularExpression>

static const QRegularExpression _multipleSlashRE("//+");

HttpHandler::HttpHandler(QString name, QObject *parent)
  : QObject(parent), _name(name) {
  QByteArray origins = qgetenv("HTTP_ALLOWED_CORS_ORIGINS");
#if QT_VERSION >= 0x050f00
  for (const QString &origin : QString::fromUtf8(origins).split(';', Qt::SkipEmptyParts)) {
#else
  for (const QString &origin : QString::fromUtf8(origins).split(';', QString::SkipEmptyParts)) {
#endif
    if (origin == "*") {
      _corsOrigins.clear();
      return;
    }
    _corsOrigins.append(QRegularExpression(origin));
  }
}

QString HttpHandler::name() const {
  if (!_name.isEmpty())
      return _name;
  if (!objectName().isEmpty())
    return objectName();
  return metaObject()->className();
}

bool HttpHandler::redirectForUrlCleanup(
    HttpRequest req, HttpResponse res, ParamsProviderMerger *) {
  QString reqPath = req.url().path();
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
    HttpRequest req, HttpResponse res, Utf8StringSet methods) {
  if (!methods.contains("OPTIONS"_u8))
    qWarning() << "HttpHandler::handleCORS(): OPTIONS method should be included"
                  " in methods set whereas it was not: " << methods;
  res.appendValueToHeader("Vary", "Origin");
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
  res.setHeader("Access-Control-Allow-Origin", origin);
  if (req.method() == HttpRequest::OPTIONS) {
    res.setHeader("Access-Control-Allow-Methods", methods.sortedJoin(", "));
    res.setHeader("Access-Control-Allow-Headers", "X-Requested-With, Content-Type"); // Origin, Accept, Authorization ?
    res.setHeader("Access-Control-Allow-Credentials", "true");
    res.setHeader("Access-Control-Max-Age", "86400");
  }
ignored:
denied:
  return req.method() == HttpRequest::OPTIONS;
}

bool HttpHandler::acceptRequest(HttpRequest) {
  return false;
}

bool HttpHandler::handleRequest(
  HttpRequest, HttpResponse, ParamsProviderMerger *) {
  return false;
}
