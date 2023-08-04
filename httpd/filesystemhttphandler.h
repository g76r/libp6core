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
#ifndef FILESYSTEMHTTPHANDLER_H
#define FILESYSTEMHTTPHANDLER_H

#include "httphandler.h"
#include <QStringList>
#include <QPair>
#include "util/paramsprovider.h"

class QFile;

/** Simple static web resource handler.
 * Can serve files from local filesystem or Qt resources pseudo-filesystem.
 *
 * Will map an url path prefix (e.g. "/my/site", default "") to a files under
 * a given local document root (e.g. "/var/www", default ":docroot/" which are
 * embeded Qt resources). For instance it would handle "/my/site/foo/bar.png"
 * by sending "/var/www/foo/bar.png".
 *
 * Content type header is set based on file suffix, not content.
 *
 * Handle HTTP/304 through Last-Modified/If-Modified-Since, using local files
 * timestamps (or program start time for Qt resources since they don't have
 * timestamps). ETag handling is not implemented.
 */
// LATER accept several document roots to enable e.g. overriding embeded
// resources with real local files
class LIBP6CORESHARED_EXPORT FilesystemHttpHandler : public HttpHandler {
  Q_OBJECT
  Q_DISABLE_COPY(FilesystemHttpHandler)
  QByteArray _urlPathPrefix, _documentRoot;
  QByteArrayList _directoryIndex;
  QList<QPair<QRegularExpression,QByteArray>> _mimeTypes;

public:
  /** @param documentRoot will be appended a / if not present */
  explicit FilesystemHttpHandler(QObject *parent = 0,
                                 const QByteArray &urlPathPrefix = {},
                                 const QByteArray &documentRoot = ":docroot/");
  QByteArray urlPathPrefix() const { return _urlPathPrefix; }
  void setUrlPrefix(const QByteArray &urlPathPrefix){
    _urlPathPrefix = urlPathPrefix; }
  /** always ends with a / */
  QByteArray documentRoot() const { return _documentRoot; }
  /** @param documentRoot will be appended a / if not present */
  void setDocumentRoot(const QByteArray &documentRoot) {
    _documentRoot = documentRoot; }
  QByteArrayList directoryIndex() const { return _directoryIndex; }
  void appendDirectoryIndex(const QByteArray &index) {
    _directoryIndex.append(index); }
  void prependDirectoryIndex(const QByteArray &index) {
    _directoryIndex.prepend(index); }
  void clearDirectoryIndex() { _directoryIndex.clear(); }
  void appendMimeType(
      const QByteArray &pattern, const QByteArray &contentType) {
    _mimeTypes.append(qMakePair(QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption),
                                contentType)); }
  void prependMimeType(const QByteArray &pattern, const QByteArray &contentType) {
    _mimeTypes.prepend(qMakePair(QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption),
                                 contentType)); }
  void clearMimeTypes() { _mimeTypes.clear(); }
  bool acceptRequest(HttpRequest req) override;
  bool handleRequest(HttpRequest req, HttpResponse res,
                     ParamsProviderMerger *processingContext) override;
  bool sendFile(HttpRequest req, HttpResponse res, const QByteArray &filename,
                ParamsProviderMerger *processingContext);

protected:
  /** Thread-safe (called by several HttpWorker threads at the same time). */
  virtual void sendLocalResource(HttpRequest req, HttpResponse res, QFile *file,
                                 ParamsProviderMerger *processingContext);

protected:
  void setMimeTypeByName(const QByteArray &name, HttpResponse res);
  /** @return true iff 304 was sent */
  bool handleCacheHeadersAndSend304(QFile *file, HttpRequest req,
                                    HttpResponse res);
};

#endif // FILESYSTEMHTTPHANDLER_H
