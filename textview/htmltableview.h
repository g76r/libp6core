#ifndef HTMLTABLEVIEW_H
#define HTMLTABLEVIEW_H

#include "asynctextview.h"

/** Display the model content as a HTML table which first column is
 * indented to reflect the tree of the model if any.
 */
// LATER add style options (headers or not, html classes, icons, indentation string, columns selection, hide non-leaf rows...)
class LIBQTSSUSHARED_EXPORT HtmlTableView : public AsyncTextView {
  Q_OBJECT
public:
  explicit HtmlTableView(QObject *parent = 0);

protected:
  void updateText();

private:
  void writeHtmlTableTree(QAbstractItemModel *m, QString &v,
                          QModelIndex parent, int depth);
};

#endif // HTMLTABLEVIEW_H
