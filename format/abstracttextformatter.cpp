/* Copyright 2017-2021 Hallowyn, Gregoire Barbier and others.
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
#include "abstracttextformatter.h"

int AbstractTextFormatter::_defaultMaxCellContentLength = 200;

AbstractTextFormatter::~AbstractTextFormatter() {
}

QString AbstractTextFormatter::formatTable(
    const SharedUiItemList<> &list, int role) const {
  QString s;
  const SharedUiItem first = list.isEmpty() ? SharedUiItem() : list.first();
  QStringList headers;
  if (_columnHeadersEnabled)
    fetchHeaderList(&headers, first);
  s.append(formatTableHeader(headers));
  if (_rowHeadersEnabled) {
    int row = 0;
    for (const SharedUiItem &item : list) {
      ++row;
      s.append(formatRowInternal(item, role, QString::number(row)));
    }
  } else {
    for (const SharedUiItem &item : list)
      s.append(formatRow(item, role));
  }
  s.append(formatTableFooter(headers));
  return s;
}

QString AbstractTextFormatter::formatTable(
    const QAbstractItemModel *model, int firstRow, int lastRow,
    const QModelIndex &parent, int role) const {
  QString s;
  QStringList headers;
  if (_columnHeadersEnabled)
    fetchHeaderList(&headers, model, parent, role);
  s.append(formatTableHeader(headers));
  if (model) {
    if (firstRow < 0)
      firstRow = 0;
    if (lastRow == -1 || lastRow >= model->rowCount(parent))
      lastRow = model->rowCount(parent)-1;
    if (_rowHeadersEnabled) {
      for (int row = firstRow; row <= lastRow; ++row)
        s.append(formatRowInternal(model, row, parent, role));
    } else {
      for (int row = firstRow; row <= lastRow; ++row)
        s.append(formatRow(model, row, parent, role));
    }
  }
  s.append(formatTableFooter(headers));
  return s;
}

void AbstractTextFormatter::fetchHeaderList(
    QStringList *headers, const SharedUiItem &item) const {
  Q_ASSERT(headers);
  int n = item.uiSectionCount();
  for (int i = 0; i < n; ++i)
    headers->append(item.uiHeaderString(i));
}

void AbstractTextFormatter::fetchHeaderList(
    QStringList *headers, const QAbstractItemModel *model,
    const QModelIndex &parent, int role) const {
  Q_ASSERT(headers);
  if (model) {
    int columns = model->columnCount(parent);
    for (int column = 0; column < columns; ++column) {
      headers->append(model->headerData(column, Qt::Horizontal, role)
                      .toString());
    }
  }
}

QString AbstractTextFormatter::formatRowInternal(
    const QAbstractItemModel *model, int row,
    const QModelIndex &parent, int role) const {
  if (!model)
    return QString();
  QStringList cells;
  int columns = model->columnCount(parent);
  for (int column = 0; column < columns; ++column)
    cells.append(
          model->index(row, column, parent).data(role).toString());
  QString rowHeader = model->headerData(row, Qt::Vertical, role).toString();
  if (rowHeader.isNull())
    rowHeader = QString::number(row);
  return formatRow(cells, rowHeader);
}

QString AbstractTextFormatter::formatRowInternal(
    const SharedUiItem &item, int role, QString rowHeader) const {
  QStringList cells;
  int n = item.uiSectionCount();
  for (int i = 0; i < n; ++i)
    cells.append(item.uiString(i, role));
  return formatRow(cells, rowHeader.isNull() ? item.qualifiedId() : rowHeader);
}
