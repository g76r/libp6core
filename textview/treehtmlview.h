#ifndef TREEHTMLVIEW_H
#define TREEHTMLVIEW_H

#include "asynctextview.h"

class LIBQTSSUSHARED_EXPORT TreeHtmlView : public AsyncTextView {
  Q_OBJECT
public:
  explicit TreeHtmlView(QObject *parent = 0);

protected:
  void updateText();

private:
  void writeHtmlTableTree(QAbstractItemModel *m, QString &v,
                          QModelIndex parent, int depth);
};

#endif // TREEHTMLVIEW_H
