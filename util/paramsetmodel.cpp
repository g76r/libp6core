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
#include "paramsetmodel.h"
#include <QtDebug>
#include <QStringList>

#define COLUMNS 2

// LATER provide a 3-column mode with 3rd column displaying "", "inherited" or "overrided"
ParamSetModel::ParamSetModel(QObject *parent, bool inherit, bool evaluate)
  : QAbstractListModel(parent), _inherit(inherit), _evaluate(evaluate) {
}

int ParamSetModel::rowCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  return parent.isValid() ? 0 : _keys.size();
}

int ParamSetModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  return COLUMNS;
}

QVariant ParamSetModel::data(const QModelIndex &index, int role) const {
  if (index.isValid() && index.row() >= 0 && index.row() < _keys.size()) {
    switch(role) {
    case Qt::DisplayRole:
      switch(index.column()) {
      case 0:
        return _keys.at(index.row());
      case 1:
        return _evaluate ? _params.value(_keys.at(index.row()), _inherit)
                         : _params.rawValue(_keys.at(index.row()), _inherit);
      }
      break;
    default:
      ;
    }
  }
  return QVariant();
}

QVariant ParamSetModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const {
  if (role == Qt::DisplayRole) {
    if (orientation == Qt::Horizontal) {
      switch(section) {
      case 0:
        return "Key";
      case 1:
        return "Value";
      }
    } else {
      return QString::number(section);
    }
  }
  return QVariant();
}

void ParamSetModel::paramsChanged(ParamSet params) {
  if (!_keys.isEmpty()) {
    beginRemoveRows(QModelIndex(), 0, _keys.size()-1);
    _keys.clear();
    _params = ParamSet();
    endRemoveRows();
  }
  if (!params.isEmpty()) {
    beginInsertRows(QModelIndex(), 0, params.size()-1);
    _keys = params.keys(_inherit).toList();
    _keys.sort();
    _params = params;
    endInsertRows();
  }
}
