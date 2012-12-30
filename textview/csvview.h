/* Copyright 2012 Hallowyn and others.
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
#ifndef CSVVIEW_H
#define CSVVIEW_H

#include "asynctextview.h"

/** Display the model content as a CSV table which first column is
 * indented to reflect the tree of the model if any.
 */
// LATER add style options (separators, quotes, indentation string, columns selection, hide non-leaf rows...)
class LIBQTSSUSHARED_EXPORT CsvView : public AsyncTextView {
  Q_OBJECT
  QString _topLeftHeader;
  bool _columnHeaders, _rowHeaders;

public:
  explicit CsvView(QObject *parent = 0);
  void setTopLeftHeader(const QString rawHtml) { _topLeftHeader = rawHtml; }
  void setColumnHeaders(bool set = true) { _columnHeaders = set; }
  void setRowHeaders(bool set = true) { _rowHeaders = set; }

protected:
  void updateText();

private:
  void writeCsvTree(QAbstractItemModel *m, QString &v, QModelIndex parent,
                    int depth);
};

#endif // CSVVIEW_H
