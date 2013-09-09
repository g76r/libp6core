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
#ifndef CSVTREEVIEW_H
#define CSVTREEVIEW_H

#include "asynctextview.h"

/** Display the model content as a CSV table which first column is
 * indented to reflect the tree of the model if any. */
// LATER add style options (separators, quotes, indentation string, columns selection, hide non-leaf rows...)
class LIBQTSSUSHARED_EXPORT CsvTreeView : public AsyncTextView {
  Q_OBJECT
  QString _topLeftHeader;
  bool _columnHeaders, _rowHeaders;

public:
  explicit CsvTreeView(QObject *parent = 0);
  void setTopLeftHeader(QString rawHtml) { _topLeftHeader = rawHtml; }
  void setColumnHeaders(bool set = true) { _columnHeaders = set; }
  void setRowHeaders(bool set = true) { _rowHeaders = set; }

protected:
  void resetAll();

private:
  void writeCsvTree(QAbstractItemModel *m, QString &v, QModelIndex parent,
                    int depth);
};

#endif // CSVTREEVIEW_H
