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
#include "uploadhttphandler.h"
#include "io/ioutils.h"
#include <QTemporaryFile>

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
      || req.path().startsWith(_urlPathPrefix.toUtf8()))
      && (req.method() == HttpRequest::POST
          || req.method() == HttpRequest::PUT);
}

bool UploadHttpHandler::handleRequest(
    HttpRequest req, HttpResponse res, ParamsProviderMerger *processingContext) {
  _maxSimultaneousUploads.acquire(1);
  QSemaphoreReleaser releaser(&_maxSimultaneousUploads, 1);
  if (handleCORS(req, res))
    return true;
  if (req.header("Content-Length"_u8).toULongLong() > _maxBytesPerUpload) {
    Log::warning() << "data too large when uploading data at "
                   << req.url() << " maximum is " << _maxBytesPerUpload;
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
  // LATER avoid DoS by setting a maximum *total* read time out
  // LATER also stop copying or waiting when Content-Length is reached
  qint64 result = IOUtils::copy(file, req.input(), _maxBytesPerUpload,
                                65536, 1000, 100);
  if (result < 0) {
    Log::warning() << "failed uploading data at "
                   << req.url()
                   << " - socket error : " << req.input()->errorString()
                   << " - temporary file error : " << file->errorString();
    res.setStatus(500);
  } else if (req.input()->waitForBytesWritten(100)) {
    Log::warning() << "data too large when uploading data at "
                   << req.url()
                   << " maximum is " << _maxBytesPerUpload;
    res.setStatus(413); // Request entity too large
  } else {
    file->seek(0);
    processUploadedFile(req, res, processingContext, file);
  }
  delete file;
  return true;
}

void UploadHttpHandler::processUploadedFile(
  HttpRequest, HttpResponse, ParamsProviderMerger *, QFile *) {
}
