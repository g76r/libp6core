/* Copyright 2014-2025 Hallowyn, Gregoire Barbier and others.
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
#ifndef UPLOADHTTPHANDLER_H
#define UPLOADHTTPHANDLER_H

#include "httphandler.h"
#include <QSemaphore>
#include <QFile>

/** HttpHandler to deal with uploading files or data. */
class LIBP6CORESHARED_EXPORT UploadHttpHandler : public HttpHandler {
  Q_OBJECT
  Q_DISABLE_COPY(UploadHttpHandler)
  Utf8String _urlPathPrefix;
  Utf8String _tempFileTemplate;
  quint64 _maxBytesPerUpload = 2L*1024*1024;
  QSemaphore _maxSimultaneousUploads;

protected:
  explicit UploadHttpHandler(QObject *parent = 0)
    : HttpHandler(parent), _maxSimultaneousUploads(1) { }
  explicit UploadHttpHandler(
      const Utf8String &urlPathPrefix, QObject *parent = 0)
    : HttpHandler(parent), _urlPathPrefix(urlPathPrefix),
      _maxSimultaneousUploads(1) { }
  explicit UploadHttpHandler(
      const Utf8String &urlPathPrefix, int maxSimultaneousUploads,
      QObject *parent = 0)
    : HttpHandler(parent), _urlPathPrefix(urlPathPrefix),
      _maxSimultaneousUploads(maxSimultaneousUploads) { }

public:
  inline Utf8String urlPathPrefix() const { return _urlPathPrefix; }
  inline void setUrlPathPrefix(const QString &urlPathPrefix) {
    _urlPathPrefix = urlPathPrefix; }
  inline Utf8String tempFileTemplate() const { return _tempFileTemplate; }
  inline void setTempFileTemplate(const Utf8String &tempFileTemplate) {
    _tempFileTemplate = tempFileTemplate; }
  inline int maxBytesPerUpload() const { return _maxBytesPerUpload; }
  inline void setMaxBytesPerUpload(quint64 maxBytesPerUpload) {
    _maxBytesPerUpload = maxBytesPerUpload; }
  bool acceptRequest(HttpRequest &req) override;
  bool handleRequest(HttpRequest &req, HttpResponse &res,
                     ParamsProviderMerger &context) override;
  /** Perform processing of file after upload succeeded.
   * As long as maxSimultaneousUploads is set to 1 (which is the default),
   * this method implementation is not required to be thread-safe. However, if
   * several simultaneous uploads are enabled, this method can be called by
   * several httpd worker threads at the same time.
   * @param file opened, seeked at begin temporary file, caller will close and
   *   delete the object (thus removing the file) */
  virtual void processUploadedFile(
      HttpRequest &req, HttpResponse &res,
      ParamsProviderMerger &context, QFile *file) = 0;
};

#endif // UPLOADHTTPHANDLER_H
