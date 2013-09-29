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
#include "csvtreeview.h"

CsvTreeView::CsvTreeView(QObject *parent) : AsyncTextView(parent),
  _columnHeaders(true), _rowHeaders(false) {
}

void CsvTreeView::writeCsvTree(QAbstractItemModel *m, QString &v,
                           QModelIndex parent, int depth) {
  int rows = m->rowCount(parent);
  int columns = m->columnCount(parent);
  //qDebug() << "CsvView::writeCsvTree()" << depth << parent << rows << columns;
  for (int row = 0; row < rows; ++row) {
    if (_rowHeaders)
      v.append(m->headerData(row, Qt::Vertical).toString()).append(";");
    for (int column = 0; column < columns; ++column) {
      if (!column) {
        for (int i = 0; i < depth; ++i)
          v.append(" ");
      }
      QModelIndex index = m->index(row, column, parent);
      v.append(m->data(index).toString());
      if (column < columns-1)
        v.append(";");
    }
    v.append("\n");
    //QModelIndex index = m->index(row, 0, parent);
    //writeCsvTree(m, v, index, depth+1); //FIXME
  }
}

void CsvTreeView::resetAll() {
  QAbstractItemModel *m = model();
  QString v;
  if (m) {
    if (_columnHeaders) {
      if (_rowHeaders)
        v.append(_topLeftHeader).append(";");
      int columns = m->columnCount(QModelIndex());
      for (int i = 0; i < columns; ++i) {
        v.append(m->headerData(i, Qt::Horizontal).toString());
        if (i < columns-1)
          v.append(";");
      }
      v.append("\n");
    }
    writeCsvTree(m, v, QModelIndex(), 0);
  }
  //qDebug() << "CsvView::updateText()" << m << v << (m?m->rowCount():-1);
  _text = v; // this operation is atomic, therefore htmlPart is thread-safe
}
