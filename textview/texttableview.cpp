/* Copyright 2013 Hallowyn and others.
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
#include "texttableview.h"

TextTableView::TextTableView(QObject *parent) : AsyncTextView(parent),
  _headersAndFootersAlreadyRead(false), _maxrows(100) {
}

void TextTableView::setEmptyPlaceholder(const QString rawText) {
  _emptyPlaceholder = rawText;
  update();
}

void TextTableView::setEllipsePlaceholder(const QString rawText) {
  _ellipsePlaceholder = rawText;
  update();
}

void TextTableView::setModel(QAbstractItemModel *model) {
  AsyncTextView::setModel(model);
  if (_columnIndexes.isEmpty()) {
    _effectiveColumnIndexes.clear();
    if (model) {
      int columns = model->columnCount();
      for (int i = 0; i < columns; ++i)
        _effectiveColumnIndexes.append(i);
    }
  }
}

void TextTableView::updateText() {
  //qDebug() << "TextTableView::updateText";
  QAbstractItemModel *m = model();
  QString v;
  if (!_headersAndFootersAlreadyRead) {
    _header = headerText();
    _footer = footerText();
  }
  v.append(_header);
  if (m) {
    if (m->rowCount(QModelIndex()) == 0)
      v.append(_emptyPlaceholder);
    else {
      int rows = _rows.size();
      if (_maxrows > 0 && rows > _maxrows)
        rows = _maxrows;
      for (int row = 0; row < rows; ++row)
        v.append(_rows.at(row));
      if (_maxrows > 0 && _rows.size() > _maxrows)
        v.append(_ellipsePlaceholder);
    }
  }
  v.append(_footer);
  _text = v;
}

void TextTableView::resetAll() {
  //qDebug() << "TextTableView::resetAll";// << objectName();;// << metaObject()->className();
  QAbstractItemModel *m = model();
  _header = headerText();
  _footer = footerText();
  _rows.clear();
  if (m)
    rowsInserted(QModelIndex(), 0, m->rowCount());
  else
    update();
}

void TextTableView::dataChanged(const QModelIndex &topLeft,
                                const QModelIndex &bottomRight) {
  //qDebug() << "TextTableView::dataChanged" << topLeft << bottomRight;
  QAbstractItemModel *m = model();
  int start = topLeft.row(), end = bottomRight.row();
  if (!topLeft.isValid() || !bottomRight.isValid() || topLeft.parent().isValid()
      || bottomRight.parent().isValid() || !m || start >= _rows.size())
    return;
  if (end >= _rows.size())
    end = _rows.size();
  for (int i = start; i <= end; ++i)
    _rows.removeAt(start);
  rowsInserted(QModelIndex(), start, end);
}

void TextTableView::rowsRemoved(const QModelIndex &parent, int start,
                                         int end) {
  //qDebug() << "TextTableView::rowsRemoved" << start << end;
  QAbstractItemModel *m = model();
  if (parent.isValid() || !m || start >= _rows.size())
    return;
  if (end >= _rows.size())
    end = _rows.size();
  for (int i = start; i <= end; ++i)
    _rows.removeAt(start);
  // TODO add more rows if maxrows (and insert one more than maxrows if available)
  update();
}

void TextTableView::rowsInserted (const QModelIndex &parent, int start,
                                  int end) {
  //qDebug() << "TextTableView::rowsInserted" << start << end;
  QAbstractItemModel *m = model();
  if (parent.isValid() || !m)
    return;
  if (_maxrows > 0 && end > _maxrows+1)
    end = _maxrows+1; // +1 to have a mean to detect that there are more
  for (int row = start; row <= end; ++row)
    _rows.insert(row, rowText(row));
  update();
}
