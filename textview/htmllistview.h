#ifndef HTMLLISTVIEW_H
#define HTMLLISTVIEW_H

#include "asynctextview.h"

/** Display the model content as a HTML list, or list of lists to reflect the
 * tree of the model if any.
 */
// LATER add style options (html classes, ul or ol, icons, columns selection...)
class HtmlListView : public AsyncTextView {
  Q_OBJECT
public:
  explicit HtmlListView(QObject *parent = 0);

protected:
  void updateText();

private:
  void writeHtmlListTree(QAbstractItemModel *m, QString &v,
                         QModelIndex parent, int depth);
};

#endif // HTMLLISTVIEW_H
