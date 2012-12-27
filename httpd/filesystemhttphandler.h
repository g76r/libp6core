#ifndef FILESYSTEMHTTPHANDLER_H
#define FILESYSTEMHTTPHANDLER_H

#include "httphandler.h"
#include <QStringList>
#include <QPair>

class QFile;

/** Simple static web resource handler.
 * Can serve files from local filesystem or Qt resources pseudo-filesystem.
 */
class FilesystemHttpHandler : public HttpHandler {
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
  bool acceptRequest(const HttpRequest &req);
  void handleRequest(HttpRequest &req, HttpResponse &res);

protected:
  virtual void sendLocalResource(HttpRequest &req, HttpResponse &res,
                                 QFile &file);

protected:
  void setMimeTypeByName(const QString &name, HttpResponse &res);
};

#endif // FILESYSTEMHTTPHANDLER_H
