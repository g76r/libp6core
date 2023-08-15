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
#include "filesystemhttphandler.h"
#include "io/ioutils.h"
#include "format/timeformats.h"
#include <QFileInfo>
#include <QDir>

FilesystemHttpHandler::FilesystemHttpHandler(
    QObject *parent, const QByteArray &urlPathPrefix,
    const QByteArray &documentRoot) :
  HttpHandler(parent), _urlPathPrefix(urlPathPrefix),
  _documentRoot(documentRoot.endsWith('/') ? documentRoot : documentRoot+"/") {
  appendDirectoryIndex("index.html");
  appendMimeType("\\.html$", "text/html;charset=UTF-8");
  appendMimeType("\\.js$", "application/javascript");
  appendMimeType("\\.css$", "text/css");
  appendMimeType("\\.png$", "image/png");
  appendMimeType("\\.jpeg$", "image/jpeg");
  appendMimeType("\\.svg$", "image/svg+xml");
  appendMimeType("\\.tiff$", "image/tiff");
  appendMimeType("\\.csv$", "text/csv");
  appendMimeType("\\.pdf$", "application/pdf");
  appendMimeType("\\.json$", "application/json");
  appendMimeType("\\.xml$", "application/xml");
  appendMimeType("\\.zip$", "application/zip");
  appendMimeType("\\.gz$", "application/gzip");
  appendMimeType("\\.htm$", "text/html;charset=UTF-8");
  appendMimeType("\\.jpg$", "image/jpeg");
  appendMimeType("\\.gif$", "image/gif");
  appendMimeType("\\.ico$", "image/vnd.microsoft.icon");
}

bool FilesystemHttpHandler::acceptRequest(HttpRequest req) {
  return _urlPathPrefix.isEmpty()
      || req.url().path().startsWith(_urlPathPrefix);
}

bool FilesystemHttpHandler::handleRequest(
    HttpRequest req, HttpResponse res,
    ParamsProviderMerger *processingContext) {
  if (_documentRoot.isEmpty()) { // should never happen (at less == "/")
    res.setStatus(500);
    res.output()->write("No document root.");
    return true;
  }
  if (handleCORS(req, res))
    return true;
  QByteArray path = req.url().path().toUtf8().mid(_urlPathPrefix.length());
  if (path.endsWith('/'))
    path.chop(1);
  if (path.startsWith('/'))
    path.remove(0, 1);
  QFile file(_documentRoot+path);
  //qDebug() << "try file" << file.fileName();
  // Must try QDir::exists() before QFile::exists() because QFile::exists()
  // returns true for resources directories.
  QDir dir(_documentRoot+path);
  if (dir.exists()) {
    for (auto index: _directoryIndex) {
      file.setFileName(_documentRoot+path+"/"+index);
      //qDebug() << "try file" << file.fileName();
      if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray location, reqPath = req.url().path().toUtf8();
        if (!reqPath.endsWith('/')) {
          int i = reqPath.lastIndexOf('/');
          location.append(reqPath.mid(i == -1 ? 0 : i+1));
          location.append('/');
        }
        location.append(index);
        res.redirect(location);
        return true;
      }
    }
    res.setStatus(403);
    res.output()->write("Directory list denied.");
  }
  sendFile(req, res, file.fileName().toUtf8(), processingContext);
  return true;
}

bool FilesystemHttpHandler::sendFile(
    HttpRequest req, HttpResponse res, const QByteArray &filename,
    ParamsProviderMerger *processingContext) {
  QFile file(filename);
  if (file.open(QIODevice::ReadOnly)) {
    sendLocalResource(req, res, &file, processingContext);
    return true;
  }
  if (file.error() == QFile::PermissionsError) {
    res.setStatus(403);
    res.output()->write("Permission denied.");
  } else {
    res.setStatus(404);
    res.output()->write("Document not found.");
  }
  return false;
}

void FilesystemHttpHandler::sendLocalResource(
    HttpRequest req, HttpResponse res, QFile *file,
    ParamsProviderMerger *processingContext) {
  Q_UNUSED(req)
  Q_UNUSED(processingContext)
  //qDebug() << "success";
  QByteArray filename = file->fileName().toUtf8();
  if (!handleCacheHeadersAndSend304(file, req, res)) {
    setMimeTypeByName(filename, res);
    res.setContentLength(file->size());
    if (req.method() != HttpRequest::HEAD)
      IOUtils::copy(res.output(), file);
  }
}

void FilesystemHttpHandler::setMimeTypeByName(
    const QByteArray &name, HttpResponse res) {
  // LATER check if performance can be enhanced (regexp)
  for (auto pair : _mimeTypes) {
    if (pair.first.match(name).hasMatch()) {
      res.setContentType(pair.second);
      return;
    }
  }
}

static QDateTime startTimeUTC(QDateTime::currentDateTimeUtc());

// LATER handle ETag / If-None-Match
bool FilesystemHttpHandler::handleCacheHeadersAndSend304(
    QFile *file, HttpRequest req, HttpResponse res) {
  if (file) {
    QByteArray filename = file->fileName().toUtf8();
    QFileInfo info(*file);
    QDateTime lastModified;
    if (filename.startsWith("qrc:") || filename.startsWith(":"))
      lastModified = startTimeUTC;
    else
      lastModified = info.lastModified().toUTC();
    if (lastModified.isValid())
      res.setHeader("Last-Modified", TimeFormats::toRfc2822DateTime(
                      lastModified).toUtf8());
    auto ifModifiedSinceString = req.header("If-Modified-Since");
    if (!ifModifiedSinceString.isEmpty() && lastModified.isValid()) {
      QString errorString;
      QDateTime ifModifiedSince(
            TimeFormats::fromRfc2822DateTime(ifModifiedSinceString,
                                             &errorString).toUTC());
      if (ifModifiedSince.isValid()) {
        // compare to If-Modified-Since +1" against rounding issues
        if (lastModified <= ifModifiedSince.addSecs(1)) {
          res.setStatus(304);
          return true;
        }
      } else {
        // LATER remove this debug trace
        qDebug() << "Cannot parse If-Modified-Since header timestamp:"
                 << ifModifiedSinceString << ":" << errorString;
      }
    }
    return false;
  }
  return false;
}
