/* Copyright 2012-2021 Hallowyn, Gregoire Barbier and others.
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
#include "csvtableview.h"

CsvTableView::CsvTableView(QObject *parent, QString objectName, int cachedRows,
                           int rowsPerPage)
  : TextTableView(parent, objectName, cachedRows, rowsPerPage) {
}

void CsvTableView::updateHeaderAndFooterCache() {
  QStringList headers;
  if (columnHeadersEnabled()) {
    fetchHeaderList(&headers, model());
    _tableHeader = formatTableHeader(headers);
  } else {
    _tableHeader.clear();
  }
}

QString CsvTableView::header(int currentPage, int lastPage,
                             QString pageVariableName) const {
  Q_UNUSED(currentPage)
  Q_UNUSED(lastPage)
  Q_UNUSED(pageVariableName)
  return _tableHeader;
}

QString CsvTableView::rowText(int row) {
  return formatRow(model(), row);
}
