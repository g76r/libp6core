/* Copyright 2012-2013 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
 * Libqtssu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libqtssu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef HTMLTREEVIEW_H
#define HTMLTREEVIEW_H

#include "asynctextview.h"

/** Display the model content as a HTML table which first column is
 * indented to reflect the tree of the model if any. */
// LATER add style options (indentation string, hide non-leaf rows...)
// LATER implement thClassRole and tdClassRole for real
class LIBQTSSUSHARED_EXPORT HtmlTreeView : public AsyncTextView {
  Q_OBJECT
  QString _tableClass, _topLeftHeader, _emptyPlaceholder, _ellipsePlaceholder;
  int _thClassRole, _trClassRole, _tdClassRole, _linkRole, _linkClassRole;
  int _htmlPrefixRole;
  bool _columnHeaders, _rowHeaders;
  int _maxrows;

public:
  explicit HtmlTreeView(QObject *parent = 0);
  void setTableClass(const QString tableClass) { _tableClass = tableClass; }
  void setTopLeftHeader(const QString rawHtml) { _topLeftHeader = rawHtml; }
  void setThClassRole(int role) { _thClassRole = role; }
  void setTrClassRole(int role) { _trClassRole = role; }
  void setTdClassRole(int role) { _tdClassRole = role; }
  /** Surround Qt::DisplayRole text with <a href="${linkRole}"> and </a>. */
  void setLinkRole(int role) { _linkRole = role; }
  void setLinkClassRole(int role) { _linkClassRole = role; }
  /** Prefix with unescaped HTML text, e.g. "<img src='icon/foo.png'/>". */
  void setHtmlPrefixRole(int role) { _htmlPrefixRole = role; }
  void setColumnHeaders(bool set = true) { _columnHeaders = set; }
  void setRowHeaders(bool set = true) { _rowHeaders = set; }
  /** Text printed if the table is empty. Default is "(empty)". */
  void setEmptyPlaceholder(const QString rawHtml) {
    _emptyPlaceholder = rawHtml; }
  /** Text printed if the table is truncated to maxrows. Default is "...". */
  void setEllipsePlaceholder(const QString rawHtml) {
    _ellipsePlaceholder = rawHtml; }
  /** Max number of rows to display. Default is 100. Use INT_MAX if you want
    * no limit. */
  void setMaxrows(int maxrows) { _maxrows = maxrows; }

protected:
  void resetAll();

private:
  void writeHtmlTableTree(QAbstractItemModel *m, QString &v,
                          QModelIndex parent, int depth, int &totalRaws);
};

#endif // HTMLTREEVIEW_H
