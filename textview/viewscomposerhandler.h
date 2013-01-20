/* Copyright 2012-2013 Hallowyn and others.
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
#ifndef VIEWSCOMPOSERHANDLER_H
#define VIEWSCOMPOSERHANDLER_H

#include "httpd/uriprefixhandler.h"
#include <QList>
#include <QString>
#include "textview.h"
#include <QWeakPointer>

class LIBQTSSUSHARED_EXPORT ViewsComposerHandler : public UriPrefixHandler {
  Q_OBJECT
  QString _pageTemplate;
  QList<QWeakPointer<TextView> > _views;

public:
  ViewsComposerHandler(const QString &prefix, int allowedMethods,
                       QObject *parent);
  void handleRequest(HttpRequest &req, HttpResponse &res);
  QString pageTemplate() const { return _pageTemplate; }
  void setPageTemplate(QString pageTemplate) { _pageTemplate = pageTemplate; }
  const QList<QWeakPointer<TextView> > views() const { return _views; }
  void clearViews() { _views.clear(); }
  void appendView(TextView *view) {
    _views.append(QWeakPointer<TextView>(view)); }
};

#endif // VIEWSCOMPOSERHANDLER_H
