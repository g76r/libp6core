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
#include "uploadhttphandler.h"
#include "util/ioutils.h"

QString UploadHttpHandler::urlPathPrefix() const {
  return _urlPathPrefix;
}

void UploadHttpHandler::setUrlPathPrefix(const QString &urlPathPrefix) {
  _urlPathPrefix = urlPathPrefix;
}
QString UploadHttpHandler::tempFileTemplate() const {
  return _tempFileTemplate;
}

void UploadHttpHandler::setTempFileTemplate(const QString &tempFilePrefix) {
  _tempFileTemplate = tempFilePrefix;
}

int UploadHttpHandler::maxBytesPerUpload() const {
  return _maxBytesPerUpload;
}

void UploadHttpHandler::setMaxBytesPerUpload(quint64 maxBytesPerUpload) {
  _maxBytesPerUpload = maxBytesPerUpload;
}

bool UploadHttpHandler::acceptRequest(HttpRequest req) {
  // LATER parametrize accepted methods
  return (_urlPathPrefix.isEmpty()
      || req.url().path().startsWith(_urlPathPrefix))
      && (req.method() == HttpRequest::POST
          || req.method() == HttpRequest::PUT);
}

bool UploadHttpHandler::handleRequest(HttpRequest req, HttpResponse res,
                                      HttpRequestContext ctxt) {
  _maxSimultaneousUploads.acquire(1);
  if (req.header("Content-Length").toULongLong() > _maxBytesPerUpload) {
    Log::warning() << "data too large when uploading data at "
                   << req.url().toString(QUrl::RemovePassword)
                   << " maximum is " << _maxBytesPerUpload;
    res.setStatus(413); // Request entity too large
    return true;
  }
  QTemporaryFile *file = _tempFileTemplate.isEmpty()
      ? new QTemporaryFile : new QTemporaryFile(_tempFileTemplate);
  if (!file->open()) {
    Log::warning() << "failed to create temporary file "
                   << file->fileTemplate() << " : " << file->errorString();
    res.setStatus(500);
  }
  qint64 result = IOUtils::copy(file, req.input(), _maxBytesPerUpload,
                                65536, 100, 100);
  if (result < 0) {
    Log::warning() << "failed uploading data at "
                   << req.url().toString(QUrl::RemovePassword)
                   << " - socket error : " << req.input()->errorString()
                   << " - temporary file error : " << file->errorString();
    res.setStatus(500);
  } else if (req.input()->waitForBytesWritten(100)) {
    Log::warning() << "data too large when uploading data at "
                   << req.url().toString(QUrl::RemovePassword)
                   << " maximum is " << _maxBytesPerUpload;
    res.setStatus(413); // Request entity too large
  } else {
    file->seek(0);
    processUploadedFile(req, res, ctxt, file);
  }
  delete file;
  _maxSimultaneousUploads.release(1);
  return true;
}
