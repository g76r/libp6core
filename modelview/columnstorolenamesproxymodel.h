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
#ifndef COLUMNSTOROLENAMESPROXYMODEL_H
#define COLUMNSTOROLENAMESPROXYMODEL_H

#include <QIdentityProxyModel>
#include "libp6core_global.h"

/** Proxy model for mapping source model column names to rolenames. Which is
 * very convenient for using a QSqlXxxModel or custom model from QML ListView
 * since the delegate item can then access each row cell using the column header
 * name as a javascript variable.
 *
 * Also expose as slots some QAbstractItemModel methods to make them callable
 * from QML.
 */
class LIBPUMPKINSHARED_EXPORT ColumnsToRolenamesProxyModel
    : public QIdentityProxyModel {
  Q_OBJECT
  QHash<int, QByteArray> _roles;
  QHash<QString, int> _reverseRoles;
  int _firstMappedRole, _setDataRole;
  QString _rolenamesPrefix;

public:
  explicit ColumnsToRolenamesProxyModel(QObject *parent = 0)
    : QIdentityProxyModel(parent), _firstMappedRole(Qt::UserRole),
      _setDataRole(Qt::EditRole) { }
  int firstMappedRole() const { return _firstMappedRole; }
  /** Set role numbers range used for mapping columns at a higher value than
   * default Qt::UserRole.
   * Must not be called with a role < Qt::UserRole or after source model has
   * been set. */
  void setFirstMappedRole(int role);
  QString rolenamesPrefix() const { return _rolenamesPrefix; }
  /** Set a prefix to column names, e.g. with "db_" a column with vertical
   * header "id" will be mapped to rolename "db_id".
   * Empty by default. */
  void setRolenamesPrefix(QString prefix) { _rolenamesPrefix = prefix; }
  void setSourceModel(QAbstractItemModel *sourceModel) override;
  QHash<int, QByteArray> roleNames() const override;
  QVariant data(const QModelIndex &index, int role) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role) override;

public slots:
  bool setData(int row, const QVariant &value, const QString &roleName);
  bool insertRow(int row, const QModelIndex &parent = QModelIndex());
  bool insertRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  bool removeRow(int row, const QModelIndex &parent = QModelIndex());
  bool removeRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  bool insertColumn(int column, const QModelIndex &parent = QModelIndex());
  bool insertColumns(int column, int count,
                     const QModelIndex &parent = QModelIndex()) override;
  bool removeColumn(int column, const QModelIndex &parent = QModelIndex());
  bool removeColumns(int column, int count,
                     const QModelIndex &parent = QModelIndex()) override;

private slots:
  void refreshRolenamesFromColumnHeaders();
};

#endif // COLUMNSTOROLENAMESPROXYMODEL_H
