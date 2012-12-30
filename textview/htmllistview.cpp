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
#include "htmllistview.h"
#include <QtDebug>

HtmlListView::HtmlListView(QObject *parent) : AsyncTextView(parent) {
}

void HtmlListView::writeHtmlListTree(QAbstractItemModel *m, QString &v,
                            QModelIndex parent, int depth) {
  v.append("<ul>");
  int rows = m->rowCount(parent);
  int columns = m->columnCount(parent);
  qDebug() << "HtmlListView::writeHtmlTree()" << depth << parent << rows
           << columns;
  for (int row = 0; row < rows; ++row) {
    v.append("<li>");
    for (int column = 0; column < columns; ++column) {
      QModelIndex index = m->index(row, column, parent);
      v.append(m->data(index).toString()).append(" ");
    }
    QModelIndex index = m->index(row, 0, parent);
    writeHtmlListTree(m, v, index, depth+1);
  }
  v.append("</ul>");
}

void HtmlListView::updateText() {
  QAbstractItemModel *m = model();
  QString v;
  if (m) {
    writeHtmlListTree(m, v, QModelIndex(), 0);
    v.append("\n");
  }
  //qDebug() << "HtmlListView::updateText()" << m << v << (m?m->rowCount():-1);
  _text = v; // this operation is atomic, therefore htmlPart is thread-safe
}
