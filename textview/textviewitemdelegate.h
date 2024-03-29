/* Copyright 2013-2023 Hallowyn, Gregoire Barbier and others.
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
#ifndef TEXTVIEWITEMDELEGATE_H
#define TEXTVIEWITEMDELEGATE_H

#include "libp6core_global.h"
#include <QAbstractItemModel>

/** Delegate text rendering of TextView data or headers, the same way
 * QAbstractItemDelegate does for QAbstractItemView.
 * Default implementation only get string data from model through
 * data().toString() and headerData().toString() */
class LIBP6CORESHARED_EXPORT TextViewItemDelegate : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(TextViewItemDelegate)
public:
  explicit TextViewItemDelegate(QObject *parent = 0);
  /** default: return index.data().toString() */
  virtual QString text(const QModelIndex &index) const;
  /** default: return model->headerData(section, orientation).toString() */
  virtual QString headerText(int section, Qt::Orientation orientation,
                             const QAbstractItemModel* model) const;
signals:
  /** An event (e.g. settings change) occured and any data previously returned
   * by text() or headerText() is no longer valid, these methods should be
   * called again for any data or header this delegate is responsible for. */
  void textChanged();
};

#endif // TEXTVIEWITEMDELEGATE_H
