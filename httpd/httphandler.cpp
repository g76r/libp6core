/* Copyright 2012-2017 Hallowyn, Gregoire Barbier and others.
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
