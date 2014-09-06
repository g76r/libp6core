/* Copyright 2012-2014 Hallowyn and others.
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
    disconnect(m, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
               this, SLOT(dataChanged(QModelIndex,QModelIndex)));
    disconnect(m, SIGNAL(layoutChanged()),
               this, SLOT(layoutChanged()));
    disconnect(m, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
               this, SLOT(headerDataChanged(Qt::Orientation,int,int)));
    disconnect(m, SIGNAL(modelReset()), this, SLOT(resetAll()));
    disconnect(m, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
               this, SLOT(rowsInserted(QModelIndex,int,int)));
    disconnect(m, SIGNAL(rowsRemoved(QModelIndex,int,int)),
               this, SLOT(rowsRemoved(QModelIndex,int,int)));
    disconnect(m, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
               this, SLOT(rowsMoved(QModelIndex,int,int,QModelIndex,int)));
    disconnect(m, SIGNAL(columnsInserted(const QModelIndex&,int,int)),
               this, SLOT(columnsInserted(QModelIndex,int,int)));
    disconnect(m, SIGNAL(columnsRemoved(const QModelIndex&,int,int)),
               this, SLOT(columnsRemoved(QModelIndex,int,int)));
    disconnect(m, SIGNAL(columnsMoved(const QModelIndex&,int,int,const QModelIndex&,int)),
               this, SLOT(columnsMoved(QModelIndex,int,int,QModelIndex,int)));
  }
  if (model) {
    m = model;
    connect(m, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(dataChanged(QModelIndex,QModelIndex)));
    connect(m, SIGNAL(layoutChanged()),
            this, SLOT(layoutChanged()));
    connect(m, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
            this, SLOT(headerDataChanged(Qt::Orientation,int,int)));
    connect(m, SIGNAL(modelReset()), this, SLOT(resetAll()));
    connect(m, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
            this, SLOT(rowsInserted(QModelIndex,int,int)));
    connect(m, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(rowsRemoved(QModelIndex,int,int)));
    connect(m, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
            this, SLOT(rowsMoved(QModelIndex,int,int,QModelIndex,int)));
    connect(m, SIGNAL(columnsInserted(const QModelIndex&,int,int)),
            this, SLOT(columnsInserted(QModelIndex,int,int)));
    connect(m, SIGNAL(columnsRemoved(const QModelIndex&,int,int)),
            this, SLOT(columnsRemoved(QModelIndex,int,int)));
    connect(m, SIGNAL(columnsMoved(const QModelIndex&,int,int,const QModelIndex&,int)),
            this, SLOT(columnsMoved(QModelIndex,int,int,QModelIndex,int)));
  }
  _model = model;
  emit modelChanged();
}

void TextView::resetAll() {
}

void TextView::layoutChanged() {
  resetAll();
}

void TextView::headerDataChanged(Qt::Orientation orientation, int first,
                                 int last) {
  Q_UNUSED(orientation)
  Q_UNUSED(first)
  Q_UNUSED(last)
  resetAll();
}

void TextView::dataChanged(const QModelIndex &topLeft,
                           const QModelIndex &bottomRight) {
  Q_UNUSED(topLeft)
  Q_UNUSED(bottomRight)
  resetAll();
}

void TextView::rowsRemoved(const QModelIndex &parent, int start,
                           int end) {
  Q_UNUSED(parent)
  Q_UNUSED(start)
  Q_UNUSED(end)
  resetAll();
}

void TextView::rowsInserted(const QModelIndex &parent, int start,
                            int end) {
  Q_UNUSED(parent)
  Q_UNUSED(start)
  Q_UNUSED(end)
  resetAll();
}

void TextView::rowsMoved(const QModelIndex &sourceParent, int sourceStart,
                         int sourceEnd,
                         const QModelIndex &destinationParent,
                         int destinationRow) {
  Q_UNUSED(sourceParent)
  Q_UNUSED(sourceStart)
  Q_UNUSED(sourceEnd)
  Q_UNUSED(destinationParent)
  Q_UNUSED(destinationRow)
  resetAll();
}

void TextView::columnsInserted(const QModelIndex &parent, int start,
                               int end) {
  Q_UNUSED(parent)
  Q_UNUSED(start)
  Q_UNUSED(end)
  resetAll();
}

void TextView::columnsRemoved(const QModelIndex &parent, int start,
                              int end) {
  Q_UNUSED(parent)
  Q_UNUSED(start)
  Q_UNUSED(end)
  resetAll();
}

void TextView::columnsMoved(const QModelIndex &sourceParent,
                            int sourceStart, int sourceEnd,
                            const QModelIndex &destinationParent,
                            int destinationColumn) {
  Q_UNUSED(sourceParent)
  Q_UNUSED(sourceStart)
  Q_UNUSED(sourceEnd)
  Q_UNUSED(destinationParent)
  Q_UNUSED(destinationColumn)
  resetAll();
}

void TextView::setItemDelegate(TextViewItemDelegate *delegate) {
  if (_defaultDelegate) {
    disconnect(_defaultDelegate, SIGNAL(textChanged()), this, SLOT(resetAll()));
  }
  _defaultDelegate = delegate;
  if (delegate) {
    connect(delegate, SIGNAL(textChanged()), this, SLOT(resetAll()));
  }
  resetAll();
}

TextViewItemDelegate *TextView::itemDelegate() const {
  return _defaultDelegate;
}

void TextView::setItemDelegateForColumn(
    int column, TextViewItemDelegate *delegate) {
  // LATER try to reset only affected column on textChanged()
  TextViewItemDelegate *old = _columnDelegates.take(column);
  if (old)
    disconnect(old, SIGNAL(textChanged()), this, SLOT(resetAll()));
  if (delegate) {
    _columnDelegates.insert(column, delegate);
    connect(delegate, SIGNAL(textChanged()), this, SLOT(resetAll()));
  }
  resetAll();
}

TextViewItemDelegate *TextView::itemDelegateForColumn(int column) const {
  return _columnDelegates.value(column, 0);
}

void TextView::setItemDelegateForRow(int row, TextViewItemDelegate *delegate) {
  // LATER try to reset only affected row on textChanged()
  TextViewItemDelegate *old = _rowDelegates.take(row);
  if (old)
    disconnect(old, SIGNAL(textChanged()), this, SLOT(resetAll()));
  if (delegate) {
    _rowDelegates.insert(row, delegate);
    connect(delegate, SIGNAL(textChanged()), this, SLOT(resetAll()));
  }
  resetAll();
}

TextViewItemDelegate *TextView::itemDelegateForRow(int row) const {
  return _rowDelegates.value(row, 0);
}
