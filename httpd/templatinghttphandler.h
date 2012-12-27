#ifndef TEMPLATINGHTTPHANDLER_H
#define TEMPLATINGHTTPHANDLER_H

#include "filesystemhttphandler.h"
#include "textview/textview.h"
#include <QWeakPointer>

class TemplatingHttpHandler : public FilesystemHttpHandler {
  Q_OBJECT
  QHash<QString,QWeakPointer<TextView> > _views;
  QSet<QString> _filters;
public:
  explicit TemplatingHttpHandler(QObject *parent = 0,
                                 const QString urlPrefix = "",
                                 const QString documentRoot = ":docroot/");
  void addView(const QString label, TextView *view) {
    _views.insert(label, QWeakPointer<TextView>(view)); }
  void addFilter(const QString regexp) { _filters.insert(regexp); }

protected:
  void sendLocalResource(HttpRequest &req, HttpResponse &res, QFile &file);
};

#endif // TEMPLATINGHTTPHANDLER_H
