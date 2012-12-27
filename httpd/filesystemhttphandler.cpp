#include "filesystemhttphandler.h"
#include <QFile>
#include <QDir>
#include "util/ioutils.h"
#include <QtDebug>

FilesystemHttpHandler::FilesystemHttpHandler(
    QObject *parent, const QString urlPrefix, const QString documentRoot) :
  HttpHandler(parent), _urlPrefix(urlPrefix), _documentRoot(documentRoot) {
  appendDirectoryIndex("index.html");
  appendMimeTypes("\\.html$", "text/html;charset=UTF-8");
  appendMimeTypes("\\.js$", "application/javascript");
  appendMimeTypes("\\.css$", "text/css");
  appendMimeTypes("\\.png$", "image/png");
  appendMimeTypes("\\.jpeg$", "image/jpeg");
  appendMimeTypes("\\.svg$", "image/svg+xml");
  appendMimeTypes("\\.tiff$", "image/tiff");
  appendMimeTypes("\\.csv$", "text/csv");
  appendMimeTypes("\\.pdf$", "application/pdf");
  appendMimeTypes("\\.json$", "application/json");
  appendMimeTypes("\\.xml$", "application/xml");
  appendMimeTypes("\\.zip$", "application/zip");
  appendMimeTypes("\\.gz$", "application/gzip");
  appendMimeTypes("\\.htm$", "text/html;charset=UTF-8");
  appendMimeTypes("\\.jpg$", "image/jpeg");
  appendMimeTypes("\\.gif$", "image/gif");
  appendMimeTypes("\\.ico$", "image/vnd.microsoft.icon");
}

QString FilesystemHttpHandler::name() const {
  return "FilesystemHttpHandler";
}

bool FilesystemHttpHandler::acceptRequest(const HttpRequest &req) {
  return _urlPrefix.isEmpty() || req.url().path().startsWith(_urlPrefix);
}

void FilesystemHttpHandler::handleRequest(HttpRequest &req, HttpResponse &res) {
  QString path = req.url().path();
  path.remove(0, _urlPrefix.length());
  while (path.size() && path.at(path.size()-1) == '/')
    path.chop(1);
  QFile file(_documentRoot+path);
  //qDebug() << "try file" << file.fileName();
  // Must try QDir::exists() before QFile::exists() because QFile::exists()
  // returns true for resources directories.
  QDir dir(_documentRoot+path);
  if (dir.exists()) {
    foreach (QString index, _directoryIndex) {
      file.setFileName(_documentRoot+path+"/"+index);
      //qDebug() << "try file" << file.fileName();
      if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QString location;
        if (!_urlPrefix.startsWith("/"))
          location = "/";
        location.append(_urlPrefix);
        if (!_urlPrefix.isEmpty() && !_urlPrefix.endsWith("/"))
          location.append("/");
        location.append(path);
        if (!path.isEmpty())
          location.append("/");
        location.append(index);
        res.redirect(location);
        return;
      }
    }
    res.setStatus(403);
    res.output()->write("Directory list denied.");
  }
  if (file.open(QIODevice::ReadOnly)) {
    sendLocalResource(req, res, file);
    return;
  }
  int status = file.error() == QFile::PermissionsError ? 403 : 404;
  //qDebug() << "failure" << status;
  res.setStatus(status);
  res.output()->write(status == 403 ? "Permission denied."
                                    : "Document not found.");
  //qDebug() << "Cannot serve HTTP static resource" << req.url()
  //         << file.fileName() << status;
}

void FilesystemHttpHandler::sendLocalResource(HttpRequest &req,
                                              HttpResponse &res, QFile &file) {
  Q_UNUSED(req)
  //qDebug() << "success";
  setMimeTypeByName(file.fileName(), res);
  res.setContentLength(file.size());
  IOUtils::copyAll(res.output(), file);
}

void FilesystemHttpHandler::setMimeTypeByName(const QString &name,
                                              HttpResponse &res) {
  // LATER check if performance can be enhanced (regexp)
  typedef QPair<QString,QString> QStringQString;
  //qDebug() << "setMimeTypeByName" << name;
  foreach (QStringQString pair, _mimeTypes) {
    QRegExp re(pair.first, Qt::CaseInsensitive);
    //qDebug() << "check" << pair.first;
    if (re.indexIn(name) >= 0) {
      //qDebug() << "match" << pair.second;
      res.setContentType(pair.second);
      return;
    }
  }
}
