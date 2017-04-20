/* Copyright 2016-2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef HIDEDELETEDSQLROWSPROXYMODEL_H
#define HIDEDELETEDSQLROWSPROXYMODEL_H

#include <QSortFilterProxyModel>
#include "libp6core_global.h"

/** Hide deleted rows in QSqlXxx models.
 *
 * This is usefull because when one calls QSqlTableModell::removeRows(), the
 * model doesn't remove the row but instead keep it and mark it with an
 * exclamation mark in its row header (execepted in OnManualSubmit edit strategy
 * where it provokes a select() calls and forces full refresh of every view when
 * submitAll() is called) while the row is actually deleted in the database
 * (once again, excepted in OnManualSubmit edit strategy). Therefore the more
 * user intuitive way to handle that is to filter out rows with "!" header,
 * which is what this proxy model does.
 *
 * @see QSqlTableModel
 */
class LIBPUMPKINSHARED_EXPORT HideDeletedSqlRowsProxyModel
    : public QSortFilterProxyModel {
  Q_OBJECT

public:
  explicit HideDeletedSqlRowsProxyModel(QObject *parent = nullptr)
    : QSortFilterProxyModel(parent) {
  }

protected:
  bool filterAcceptsRow(
      int source_row, const QModelIndex &source_parent) const override;
};

#endif // HIDEDELETEDSQLROWSPROXYMODEL_H
