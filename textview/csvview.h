#ifndef CSVVIEW_H
#define CSVVIEW_H

#include "asynctextview.h"

/** Display the model content as a CSV table which first column is
 * indented to reflect the tree of the model if any.
 */
// LATER add style options (separators, quotes, indentation string, columns selection, hide non-leaf rows...)
class LIBQTSSUSHARED_EXPORT CsvView : public AsyncTextView {
  Q_OBJECT
public:
  explicit CsvView(QObject *parent = 0);
protected:
  void updateText();

private:
  void writeCsvTree(QAbstractItemModel *m, QString &v,
                    QModelIndex parent, int depth);
};

#endif // CSVVIEW_H
