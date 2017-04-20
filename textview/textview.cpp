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
#include "textview.h"

TextView::TextView(QObject *parent)
  : QObject(parent), _model(0), _defaultDelegate(0) {
}

TextView::TextView(QObject *parent, QString objectName)
  : QObject(parent), _model(0), _defaultDelegate(0) {
  setObjectName(objectName);
}

void TextView::setModel(QAbstractItemModel *model) {
  QAbstractItemModel *m = this->model();
  if (m) {
    disconnect(m, 0, this, 0);
  }
  if (model) {
    m = model;
    connect(m, &QAbstractItemModel::dataChanged,
            this, &TextView::dataChanged);
    connect(m, &QAbstractItemModel::layoutChanged,
            this, &TextView::layoutChanged);
    connect(m, &QAbstractItemModel::headerDataChanged,
            this, &TextView::headerDataChanged);
    connect(m, &QAbstractItemModel::modelReset,
            this, &TextView::resetAll);
    connect(m, &QAbstractItemModel::rowsInserted,
            this, &TextView::rowsInserted);
    connect(m, &QAbstractItemModel::rowsRemoved,
            this, &TextView::rowsRemoved);
    connect(m, &QAbstractItemModel::rowsMoved,
            this, &TextView::rowsMoved);
    connect(m, &QAbstractItemModel::columnsInserted,
            this, &TextView::columnsInserted);
    connect(m, &QAbstractItemModel::columnsRemoved,
            this, &TextView::columnsRemoved);
    connect(m, &QAbstractItemModel::columnsMoved,
            this, &TextView::columnsMoved);
  }
  _model = model;
  emit modelChanged();
}

void TextView::invalidateCache() {
  resetAll();
}

void TextView::layoutChanged() {
  resetAll();
}

void TextView::headerDataChanged(Qt::Orientation, int, int) {
  resetAll();
}

void TextView::dataChanged(const QModelIndex &, const QModelIndex &) {
  resetAll();
}

void TextView::rowsRemoved(const QModelIndex &, int, int) {
  resetAll();
}

void TextView::rowsInserted(const QModelIndex &, int, int) {
  resetAll();
}

void TextView::rowsMoved(const QModelIndex &, int, int, const QModelIndex &,
                         int) {
  resetAll();
}

void TextView::columnsInserted(const QModelIndex &, int, int) {
  resetAll();
}

void TextView::columnsRemoved(const QModelIndex &, int, int) {
  resetAll();
}

void TextView::columnsMoved(const QModelIndex &, int, int, const QModelIndex &,
                            int) {
  resetAll();
}

void TextView::setItemDelegate(TextViewItemDelegate *delegate) {
  // LATER avoid double connections to the same delegate
  if (_defaultDelegate) {
    disconnect(_defaultDelegate, &TextViewItemDelegate::textChanged,
               this, &TextView::resetAll);
  }
  _defaultDelegate = delegate;
  if (delegate) {
    connect(delegate, &TextViewItemDelegate::textChanged,
            this, &TextView::resetAll);
  }
  resetAll();
}

TextViewItemDelegate *TextView::itemDelegate() const {
  return _defaultDelegate;
}

void TextView::setItemDelegateForColumn(
    int column, TextViewItemDelegate *delegate) {
  // LATER try to reset only affected column on textChanged()
  // LATER avoid double connections to the same delegate
  TextViewItemDelegate *old = _columnDelegates.take(column);
  if (old)
    disconnect(old, &TextViewItemDelegate::textChanged,
               this, &TextView::resetAll);
  if (delegate) {
    _columnDelegates.insert(column, delegate);
    connect(delegate, &TextViewItemDelegate::textChanged,
            this, &TextView::resetAll);
  }
  resetAll();
}

TextViewItemDelegate *TextView::itemDelegateForColumn(int column) const {
  return _columnDelegates.value(column, 0);
}

void TextView::setItemDelegateForRow(int row, TextViewItemDelegate *delegate) {
  // LATER try to reset only affected row on textChanged()
  // LATER avoid double connections to the same delegate
  TextViewItemDelegate *old = _rowDelegates.take(row);
  if (old)
    disconnect(old, &TextViewItemDelegate::textChanged,
               this, &TextView::resetAll);
  if (delegate) {
    _rowDelegates.insert(row, delegate);
    connect(delegate, &TextViewItemDelegate::textChanged,
            this, &TextView::resetAll);
  }
  resetAll();
}

TextViewItemDelegate *TextView::itemDelegateForRow(int row) const {
  return _rowDelegates.value(row, 0);
}
