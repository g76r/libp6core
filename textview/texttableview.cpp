/* Copyright 2013-2022 Hallowyn, Gregoire Barbier and others.
 * This file is part of libpumpkin, see <http://libpumpkin.g76r.eu/>.
 * Libpumpkin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libpumpkin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libpumpkin.  If not, see <http://www.gnu.org/licenses/>.
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
  auto locked_rows = _rows.constLockedData();
  const QStringList &rows = *locked_rows;
  int rowsCount = rows.size();
  QString pageVariableName =
      objectName().isEmpty() ? u"page"_s : objectName()+u"-page"_s;
  QString pageVariableValue;
  if (params)
    pageVariableValue = params->paramUtf16(
                          "value:"_u8+pageVariableName,
                          ParamsProvider::EvalContext{"http"_u8});
  int currentPage = qMax(1, pageVariableValue.toInt());
  int maxPage = currentPage;
  if (rowsCount == 0)
    v = _emptyPlaceholder;
  else {
    int min, max;
    // TODO read uncached data if needed
    if (_rowsPerPage > 0) {
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

void TextTableView::invalidateCache() {
  QAbstractItemModel *m = model();
  if (!m)
    return;
  int rows = m->rowCount(), columns = m->columnCount();
  if (rows < 1 || columns < 1)
    return;
  dataChanged(m->index(0, 0), m->index(rows-1, columns-1));
}

void TextTableView::resetAll() {
  layoutChanged();
  QAbstractItemModel *m = model();
  auto rows = _rows.lockedData();
  rows->clear();
  if (m) {
    auto size = m->rowCount();
    if (size)
      doRowsInserted(rows, {}, 0, size-1);
  }
}

void TextTableView::dataChanged(const QModelIndex &topLeft,
                                const QModelIndex &bottomRight) {
  QAbstractItemModel *m = model();
  int start = topLeft.row(), end = bottomRight.row();
  auto rows = _rows.lockedData();
  int size = rows->size(); // overflows if > 2G
  if (!topLeft.isValid() || !bottomRight.isValid() || topLeft.parent().isValid()
      || bottomRight.parent().isValid() || !m || size == 0)
    return;
  if (start >= size) {
    //qDebug() << "dataChanged start >= size:" << start << typeid(this).name()
    //         << objectName();
    start = size - 1;
  }
  if (start < 0) {
    //qDebug() << "dataChanged start < 0:" << start << typeid(this).name()
    //         << objectName();
    start = 0;
  }
  if (end >= size) {
    /*if (size) {
      qDebug() << "dataChanged end >= size:" << end << size
               << typeid(this).name() << objectName() << rows->at(0);
    if (size > 10)
      qDebug() << rows->at(0) << rows->at(1) << rows->at(2) << rows->at(3)
               << rows->at(4) << rows->at(5) << rows->at(6) << rows->at(7)
               << rows->at(8) << rows->at(9);
    } else
      qDebug() << "dataChanged end >= size:" << end << size
               << typeid(this).name() << objectName();*/
    end = size-1;
  }
  rows->remove(start, end-start+1);
  doRowsInserted(rows, {}, start, end);
}

void TextTableView::rowsRemoved(const QModelIndex &parent, int start, int end) {
  QAbstractItemModel *m = model();
  auto rows = _rows.lockedData();
  int size = rows->size(); // overflows if > 2G
  if (parent.isValid() || !m)
    return;
  if (size <= 0 || start < 0 || start >= size || end < 0) {
    //qDebug() << "rowRemoved size <= 0 || start < 0 || start >= size || end < 0:"
    //         << start << end << size << typeid(this).name() << objectName();
    return;
  }
  if (end > size) {
    //qDebug() << "rowRemoved end > size:" << end << size << typeid(this).name()
    //         << objectName();
    end = size-1;
  }
  rows->remove(start, end-start+1);
}

void TextTableView::rowsInserted (const QModelIndex &parent, int start,
                                  int end) {
  auto rows = _rows.lockedData();
  doRowsInserted(rows, parent, start, end);
}

void TextTableView::doRowsInserted(
    AtomicValue<QStringList>::LockedData &rows,
    const QModelIndex &parent, int start, int end) {
  QAbstractItemModel *m = model();
  if (parent.isValid() || !m)
    return;
  int size = rows->size(); // overflows if > 2G
  if (start > size) {
    //qDebug() << "doRowsInserted start > size:" << start << size
    //         << std::max(size-1, 0) << typeid(this).name() << objectName();
    start = std::max(size-1, 0);
  }
  if (_cachedRows > 0 && end > _cachedRows)
    end = _cachedRows;
  for (int row = start; row <= end; ++row)
    rows->insert(row, rowText(row));
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

void TextTableView::updateHeaderAndFooterCache() {
}

QString TextTableView::rowText(int) {
  return QString();
}
