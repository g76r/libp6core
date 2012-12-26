#include "viewscomposerhandler.h"

ViewsComposerHandler::ViewsComposerHandler(QObject *parent,
                                           const QString &prefix,
                                           int allowedMethods)
  : UriPrefixHandler(parent, prefix, allowedMethods) {
}

void ViewsComposerHandler::handleRequest(HttpRequest &req, HttpResponse &res) {
  Q_UNUSED(req)
  QString s = pageTemplate();
  foreach (QWeakPointer<TextView> view, views())
    if (view)
      s.arg(view.data()->text());
  res.output()->write(s.toUtf8().constData());
}
