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
#ifndef PARAMSETMODEL_H
#define PARAMSETMODEL_H

#include <QAbstractListModel>
#include "util/paramset.h"

/** Model to display a ParamSet into a 2-columns (key, value) View.
 * @see ParamSet
 * @see QAbstractItemModel
 */
class LIBQTSSUSHARED_EXPORT ParamSetModel : public QAbstractListModel {
  Q_OBJECT
  Q_DISABLE_COPY(ParamSetModel)

  ParamSet _params;
  QStringList _keys;
  bool _inherit, _evaluate;

public:
  explicit ParamSetModel(QObject *parent = 0, bool inherit = false,
                         bool evaluate = false);
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

public slots:
  /** Must be signaled each time the ParamSet data changes. */
  void paramsChanged(ParamSet params);
};

#endif // PARAMSETMODEL_H
