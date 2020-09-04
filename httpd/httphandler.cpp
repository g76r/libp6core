/* Copyright 2012-2019 Hallowyn, Gregoire Barbier and others.
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
  QByteArray origins = qgetenv("CORS_ORIGINS");
  if (origins.isNull())
    origins = qgetenv("CORS_DOMAINS");
  for (const QString &origin : QString::fromUtf8(origins).split(';', Qt::SkipEmptyParts)) {
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
    reqPath.replace(_multipleSlashRE, QStringLiteral("/"));
    if (reqPath.startsWith('/')) {
      --depth;
      reqPath.remove(0, 1);
    }
    QUrl url;
    url.setPath(QStringLiteral("../").repeated(depth)+reqPath);
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

bool HttpHandler::handlePreflight(
    HttpRequest req, HttpResponse res, ParamsProviderMerger *processingContext,
    QSet<QString> methods) {
  Q_UNUSED(processingContext)
  QString origin = req.header("Origin");
  if (origin.isEmpty() && req.method() != HttpRequest::OPTIONS)
    return false;
  QString allowed;
  for (const QRegularExpression &re : _corsOrigins) {
    if (re.match(origin).hasMatch()) {
      allowed = origin;
      break;
    }
  }
  if (_corsOrigins.isEmpty())
    allowed = origin.isEmpty() ? "*" : origin;
  if (allowed.isNull())
    return false;
  methods.insert("OPTIONS");
  res.setHeader("Access-Control-Allow-Origin", allowed);
  res.setHeader("Access-Control-Allow-Methods", methods.values().join(", "));
  res.setHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept, Authorization");
  res.setHeader("Access-Control-Allow-Credentials", "true");
  if (req.method() == HttpRequest::OPTIONS) {
    res.setHeader("Access-Control-Max-Age", "86400");
    return true;
  }
  return false;
}
