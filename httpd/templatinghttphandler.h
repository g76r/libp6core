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
#ifndef TEMPLATINGHTTPHANDLER_H
#define TEMPLATINGHTTPHANDLER_H

#include "filesystemhttphandler.h"
#include "textview/textview.h"
#include <QWeakPointer>

class TemplatingHttpHandler : public FilesystemHttpHandler {
  Q_OBJECT
  QHash<QString,QWeakPointer<TextView> > _views;
  QSet<QString> _filters;
public:
  explicit TemplatingHttpHandler(QObject *parent = 0,
                                 const QString urlPrefix = "",
                                 const QString documentRoot = ":docroot/");
  void addView(const QString label, TextView *view) {
    _views.insert(label, QWeakPointer<TextView>(view)); }
  void addFilter(const QString regexp) { _filters.insert(regexp); }

protected:
  void sendLocalResource(HttpRequest &req, HttpResponse &res, QFile &file);
};

#endif // TEMPLATINGHTTPHANDLER_H
