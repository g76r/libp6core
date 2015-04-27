/* Copyright 2014 Hallowyn and others.
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
#ifndef UPLOADHTTPHANDLER_H
#define UPLOADHTTPHANDLER_H

#include "httphandler.h"
#include <QTemporaryFile>
#include <QSemaphore>

/** HttpHandler to deal with uploading files or data. */
class LIBQTSSUSHARED_EXPORT UploadHttpHandler : public HttpHandler {
  Q_OBJECT
  QString _urlPathPrefix;
  QString _tempFileTemplate;
  quint64 _maxBytesPerUpload;
  QSemaphore _maxSimultaneousUploads;

public:
  explicit UploadHttpHandler(QObject *parent = 0)
    : HttpHandler(parent), _maxBytesPerUpload(2L*1024*1024),
      _maxSimultaneousUploads(1) { }
  explicit UploadHttpHandler(QString urlPathPrefix, QObject *parent = 0)
    : HttpHandler(parent), _urlPathPrefix(urlPathPrefix),
      _maxBytesPerUpload(2*1024*1024), _maxSimultaneousUploads(1) { }
  explicit UploadHttpHandler(QString urlPathPrefix, int maxSimultaneousUploads,
                             QObject *parent = 0)
    : HttpHandler(parent), _urlPathPrefix(urlPathPrefix),
      _maxBytesPerUpload(2*1024*1024),
      _maxSimultaneousUploads(maxSimultaneousUploads) { }
  QString urlPathPrefix() const;
  void setUrlPathPrefix(const QString &urlPathPrefix);
  QString tempFileTemplate() const;
  void setTempFileTemplate(const QString &tempFileTemplate);
  int maxBytesPerUpload() const;
  void setMaxBytesPerUpload(quint64 maxBytesPerUpload);
  bool acceptRequest(HttpRequest req);
  bool handleRequest(HttpRequest req, HttpResponse res,
                     ParamsProviderMerger *processingContext);
  /** Perform processing of file after upload succeeded.
   * As long as maxSimultaneousUploads is set to 1 (which is the default),
   * this method implementation is not required to be thread-safe. However, if
   * several simultaneous uploads are enabled, this method can be called by
   * several httpd worker threads at the same time.
   * @param file opened, seeked at begin temporary file, caller will close and
   *   delete the object (thus removing the file) */
  virtual void processUploadedFile(
      HttpRequest req, HttpResponse res,
      ParamsProviderMerger *processingContext, QFile *file) = 0;
};

#endif // UPLOADHTTPHANDLER_H
