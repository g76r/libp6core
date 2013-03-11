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
#ifndef FILESYSTEMHTTPHANDLER_H
#define FILESYSTEMHTTPHANDLER_H

#include "httphandler.h"
#include <QStringList>
#include <QPair>

class QFile;

/** Simple static web resource handler.
 * Can serve files from local filesystem or Qt resources pseudo-filesystem.
 */
class LIBQTSSUSHARED_EXPORT FilesystemHttpHandler : public HttpHandler {
  Q_OBJECT
  QString _urlPrefix, _documentRoot;
  QStringList _directoryIndex;
  QList<QPair<QString,QString> > _mimeTypes;

public:
  explicit FilesystemHttpHandler(QObject *parent = 0,
                                 const QString urlPrefix = "",
                                 const QString documentRoot = ":docroot/");
  QString urlPrefix() const { return _urlPrefix; }
  void setUrlPrefix(const QString urlPrefix){ _urlPrefix = urlPrefix; }
  QString documentRoot() const { return _documentRoot; }
  void setDocumentRoot(const QString documentRoot) {
    _documentRoot = documentRoot; }
  QStringList directoryIndex() const { return _directoryIndex; }
  void appendDirectoryIndex(const QString index) {
    _directoryIndex.append(index); }
  void prependDirectoryIndex(const QString index) {
    _directoryIndex.prepend(index); }
  void clearDirectoryIndex() { _directoryIndex.clear(); }
  QString name() const;
  void appendMimeTypes(const QString pattern, const QString contentType) {
    _mimeTypes.append(qMakePair(pattern, contentType)); }
  void prependMimeTypes(const QString pattern, const QString contentType) {
    _mimeTypes.prepend(qMakePair(pattern, contentType)); }
  void clearMimeTypes() { _mimeTypes.clear(); }
  bool acceptRequest(HttpRequest req);
  void handleRequest(HttpRequest req, HttpResponse res);
  void handleRequestWithContext(HttpRequest req, HttpResponse res,
                                QHash<QString,QVariant> values);

protected:
  virtual void sendLocalResource(HttpRequest req, HttpResponse res, QFile *file,
                                 QHash<QString,QVariant> values);

protected:
  void setMimeTypeByName(QString name, HttpResponse res);
};

#endif // FILESYSTEMHTTPHANDLER_H
