#ifndef VIEWSCOMPOSERHANDLER_H
#define VIEWSCOMPOSERHANDLER_H

#include "httpd/uriprefixhandler.h"
#include <QList>
#include <QString>
#include "textview.h"
#include <QWeakPointer>

class LIBQTSSUSHARED_EXPORT ViewsComposerHandler : public UriPrefixHandler {
  Q_OBJECT
  QString _pageTemplate;
  QList<QWeakPointer<TextView> > _views;

public:
  ViewsComposerHandler(QObject *parent, const QString &prefix,
                       int allowedMethods);
  void handleRequest(HttpRequest &req, HttpResponse &res);
  QString pageTemplate() const { return _pageTemplate; }
  void setPageTemplate(QString pageTemplate) { _pageTemplate = pageTemplate; }
  const QList<QWeakPointer<TextView> > views() const { return _views; }
  void clearViews() { _views.clear(); }
  void appendView(TextView *view) {
    _views.append(QWeakPointer<TextView>(view)); }
};

#endif // VIEWSCOMPOSERHANDLER_H
