/* Copyright 2015-2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef SHAREDUIITEMSLOGMODEL_H
#define SHAREDUIITEMSLOGMODEL_H

#include "shareduiitemstablemodel.h"

/** Log model, appending a timestamp column after last column for each logged
 * SharedUiItem.
 *
 * This is convenient for keeping log of item changes.
 *
 * For instance, if logItem() is called with a "Customer" item with 14 sections,
 * SharedUiItemsLogModel will instead contain a special SharedUiItem containing
 * 15 sections, the 15th one being the timestamp when logItem() was called.
 *
 * Records are sorted in reverse chronological order (first row displays last
 * event).
 *
 * @brief The SharedUiItemsLogModel class
 */
class LIBP6CORESHARED_EXPORT SharedUiItemsLogModel
    : public SharedUiItemsTableModel {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemsLogModel)
  int _timestampColumn;

public:
  explicit SharedUiItemsLogModel(QObject *parent = 0, int maxrows = 500);
  void setHeaderDataFromTemplate(SharedUiItem templateItem, int role);
  int timestampColumn() const { return _timestampColumn; }

public slots:
  /** Can be called directly or connected to any
   * itemChanged(SharedUiItem newItem, SharedUiItem oldItem) signal from a
   * data-holding class (incl. a DocumentManager).
   */
  void logItem(SharedUiItem newItem);
};

#endif // SHAREDUIITEMSLOGMODEL_H
