#ifndef ASYNCTEXTVIEW_H
#define ASYNCTEXTVIEW_H

#include "textview.h"

class LIBQTSSUSHARED_EXPORT AsyncTextView : public TextView {
  Q_OBJECT
protected:
  QString _text;

public:
  explicit AsyncTextView(QObject *parent = 0);
  QString text() const;
  void setModel(QAbstractItemModel *model);
  void customEvent(QEvent *event);

public slots:
  void update();

protected:
  virtual void updateText() = 0;
};

#endif // ASYNCTEXTVIEW_H
