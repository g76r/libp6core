/* Copyright 2013 Hallowyn and others.
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
#ifndef TEXTVIEWITEMDELEGATE_H
#define TEXTVIEWITEMDELEGATE_H

#include "libqtssu_global.h"
#include <QObject>
#include <QModelIndex>

/** Delegate text rendering of TextView data or headers, the same way
 * QAbstractItemDelegate does for QAbstractItemView.
 * Default implementation only get string data from model through
 * data().toString() and headerData().toString() */
class LIBQTSSUSHARED_EXPORT TextViewItemDelegate : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(TextViewItemDelegate)
public:
  explicit TextViewItemDelegate(QObject *parent = 0);
  /** default: return index.data().toString() */
  virtual QString text(const QModelIndex &index) const;
  /** default: return model->headerData(section, orientation).toString() */
  virtual QString headerText(int section, Qt::Orientation orientation,
                             const QAbstractItemModel* model) const;
};

#endif // TEXTVIEWITEMDELEGATE_H
