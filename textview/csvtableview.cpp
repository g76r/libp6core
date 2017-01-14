/* Copyright 2012-2017 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
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
#include "csvtableview.h"

CsvTableView::CsvTableView(QObject *parent, int cachedRows, int rowsPerPage)
  : TextTableView(parent, QString(), cachedRows, rowsPerPage) {
}

void CsvTableView::updateHeaderAndFooterCache() {
  _tableHeader = formatHeader(model());
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
