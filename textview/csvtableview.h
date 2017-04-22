/* Copyright 2012-2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef CSVTABLEVIEW_H
#define CSVTABLEVIEW_H

#include "texttableview.h"
#include "format/csvformatter.h"

/** Display the model content as a CSV table. Only rows of the root index
  * are displayed. */
// LATER add style options (separators, quotes, indentation string, columns selection, hide non-leaf rows...)
class LIBPUMPKINSHARED_EXPORT CsvTableView : public TextTableView,
    public CsvFormatter {
  Q_OBJECT
  QString _tableHeader;

public:
  /** Note that CsvTableView disables rowsPerPage by default, unlike
   * TextTableView general case. */
  explicit CsvTableView(QObject *parent = 0, QString objectName = QString(),
                        int cachedRows = defaultCachedRows,
                        int rowsPerPage = -1);

protected:
  void updateHeaderAndFooterCache();
  QString rowText(int row);

private:
  QString header(int currentPage, int lastPage, QString pageVariableName) const;
  Q_DISABLE_COPY(CsvTableView)
};

#endif // CSVTABLEVIEW_H
