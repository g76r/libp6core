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
//#include "log/log.h"

TextTableView::TextTableView(QObject *parent, QString objectName,
                             int cachedRows, int rowsPerPage)
  : TextView(parent, objectName), _cachedRows(cachedRows),
    _rowsPerPage(rowsPerPage) {
}

void TextTableView::setEmptyPlaceholder(QString rawText) {
  _emptyPlaceholder = rawText;
}

void TextTableView::setEllipsePlaceholder(QString rawText) {
  _ellipsePlaceholder = rawText;
}

void TextTableView::setModel(QAbstractItemModel *model) {
  TextView::setModel(model);
  resetAll();
}

QString TextTableView::text(ParamsProvider *params, QString scope) const {
  Q_UNUSED(scope)
  QString v;
  QList<QString> rows = _rows;
  int rowsCount = rows.size();
  QString pageVariableName(objectName().isEmpty() ? "page"
                                                  : objectName()+"-page");
  QString pageVariableValue(
        params ? params->paramValue(pageVariableName).toString() : "disabled");
  int currentPage = qMax(1, pageVariableValue.toInt());
  int maxPage = currentPage;
  if (rowsCount == 0)
    v = _emptyPlaceholder;
  else {
    int min, max;
    // TODO read uncached data if needed
    if (_rowsPerPage > 0 && pageVariableValue != "disabled") {
      maxPage = rowsCount/_rowsPerPage
          + (rowsCount%_rowsPerPage || !rowsCount ? 1 : 0);
      if (currentPage > maxPage)
        currentPage = maxPage;
      min = _rowsPerPage*(currentPage-1);
      max = qMin(_rowsPerPage*currentPage, rowsCount)-1;
    } else {
      min = 0;
      max = rowsCount-1;
    }
    for (int row = min; row <= max; ++row)
      v.append(rows.at(row));
    if (maxPage > currentPage)
      v.append(_ellipsePlaceholder);
  }
  return header(currentPage, maxPage, pageVariableName)
      +v+footer(currentPage, maxPage, pageVariableName);
}

void TextTableView::resetAll() {
  //qDebug() << "TextTableView::resetAll";// << objectName();;// << metaObject()->className();
  layoutChanged();
  QAbstractItemModel *m = model();
  _rows.clear();
  if (m && m->rowCount())
    rowsInserted(QModelIndex(), 0, m->rowCount()-1);
}

void TextTableView::dataChanged(const QModelIndex &topLeft,
                                const QModelIndex &bottomRight) {
  //Log::fatal() << "TextTableView::dataChanged " << objectName() << " "
  //             << metaObject()->className() << " " << topLeft.row()
  //             << "," << topLeft.column() << " " << bottomRight.row()
  //             << "," << bottomRight.column();
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
}

void TextTableView::rowsInserted (const QModelIndex &parent, int start,
                                  int end) {
  //qDebug() << "TextTableView::rowsInserted" << start << end;
  QAbstractItemModel *m = model();
  if (parent.isValid() || !m)
    return;
  if (_cachedRows > 0 && end > _cachedRows)
    end = _cachedRows;
  for (int row = start; row <= end; ++row)
    _rows.insert(row, rowText(row));
}

void TextTableView::layoutChanged() {
  if (model()) {
    if (_columnIndexes.isEmpty()) {
      _effectiveColumnIndexes.clear();
      int columns = model()->columnCount();
      for (int i = 0; i < columns; ++i)
        _effectiveColumnIndexes.append(i);
    }
  } else
    _effectiveColumnIndexes.clear();
  updateHeaderAndFooterCache();
}

QString TextTableView::header(int currentPage, int lastPage,
                              QString pageVariableName) const {
  Q_UNUSED(currentPage)
  Q_UNUSED(lastPage)
  Q_UNUSED(pageVariableName)
  return QString();
}

QString TextTableView::footer(int currentPage, int lastPage,
                              QString pageVariableName) const {
  Q_UNUSED(currentPage)
  Q_UNUSED(lastPage)
  Q_UNUSED(pageVariableName)
  return QString();
}
