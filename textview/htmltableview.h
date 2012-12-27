#ifndef HTMLTABLEVIEW_H
#define HTMLTABLEVIEW_H

#include "asynctextview.h"

/** Display the model content as a HTML table which first column is
 * indented to reflect the tree of the model if any.
 */
// LATER add style options (headers or not, indentation string, columns selection, hide non-leaf rows...)
// LATER implement thClassRole and tdClassRole for real
class LIBQTSSUSHARED_EXPORT HtmlTableView : public AsyncTextView {
  Q_OBJECT
  QString _tableClass;
  int _thClassRole, _trClassRole, _tdClassRole, _linkRole, _linkClassRole;
  int _htmlPrefixRole;

public:
  explicit HtmlTableView(QObject *parent = 0);
  void setTableClass(const QString tableClass) { _tableClass = tableClass; }
  void setThClassRole(int role) { _thClassRole = role; }
  void setTrClassRole(int role) { _trClassRole = role; }
  void setTdClassRole(int role) { _tdClassRole = role; }
  /** Surround Qt::DisplayRole text with <a href="${linkRole}"> and </a>. */
  void setLinkRole(int role) { _linkRole = role; }
  void setLinkClassRole(int role) { _linkClassRole = role; }
  /** Prefix with unescaped HTML text, e.g. "<img src='icon/foo.png'/>". */
  void setHtmlPrefixRole(int role) { _htmlPrefixRole = role; }

protected:
  void updateText();

private:
  void writeHtmlTableTree(QAbstractItemModel *m, QString &v,
                          QModelIndex parent, int depth);
};

#endif // HTMLTABLEVIEW_H
