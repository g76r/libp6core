/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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
#include "paramsetmodel.h"
#include <QRandomGenerator>

#define COLUMNS 4

ParamSetModel::ParamSetModel(
    QObject *parent, bool inherit, bool evaluate, bool displayOverriden,
    bool trimOnEdit)
  : QAbstractListModel(parent), _inherit(inherit), _evaluate(evaluate),
    _displayOverriden(displayOverriden), _trimOnEdit(trimOnEdit) {
}

int ParamSetModel::rowCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  return parent.isValid() ? 0 : _rows.size();
}

int ParamSetModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  return COLUMNS;
}

QVariant ParamSetModel::data(const QModelIndex &index, int role) const {
  if (index.isValid() && index.row() >= 0 && index.row() < _rows.size()) {
    switch(role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      switch(index.column()) {
      case 0:
        return _rows.at(index.row())._key;
      case 1:
        return _rows.at(index.row())._value;
      case 2:
        return _rows.at(index.row())._scope;
      case 3:
        return _rows.at(index.row())._overriden;
      }
      break;
    case Qt::DecorationRole:
      switch(index.column()) {
      case 0:
        if (_rows.at(index.row())._overriden)
          return _overridenDecoration.isNull()
              ? _inheritedDecoration : _overridenDecoration;
        if (_rows.at(index.row())._inherited)
          return _inheritedDecoration;
        return _localDecoration;
      }
      break;
    case SharedUiItem::QualifiedIdRole:
      return _qualifier+':'+_rows.at(index.row())._key;
    case SharedUiItem::IdRole:
      return _rows.at(index.row())._key;
    case SharedUiItem::QualifierRole:
      return _qualifier;
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
        return QStringLiteral("Key");
      case 1:
        return QStringLiteral("Value");
      case 2:
        return QStringLiteral("Scope");
      case 3:
        return QStringLiteral("Overriden");
      }
    } else {
      return QString::number(section);
    }
  }
  return QVariant();
}

bool ParamSetModel::setData(const QModelIndex &index, const QVariant &value,
                            int role) {
  Q_UNUSED(role)
  if (!index.isValid())
    return false;
  Q_ASSERT(index.model() == this);
  if (_rows[index.row()]._inherited)
    return false; // cannot modify inherited rows (such rows are not selectable)
  ParamSet newParams = _params, oldParams = _params;
  auto s = _trimOnEdit ? value.toString().trimmed() : value.toString();
  switch (index.column()) {
  case 0:
    if (s.isEmpty())
      return false;
    if (_rows[index.row()]._key == s)
      return true; // nothing changed
    newParams.removeValue(_rows[index.row()]._key);
    newParams.setValue(s, _rows[index.row()]._value);
    break;
  case 1:
    if (_rows[index.row()]._value == s)
      return true; // nothing changed
    newParams.setValue(_rows[index.row()]._key, s);
    break;
  default:
    return false;
  }
  changeParams(newParams, oldParams, _paramsetId);
  emit paramsChanged(newParams, oldParams, _paramsetId);
  return true;
}

static QString genererateNewKey(ParamSet params) {
  QString key, prefix = u"key"_s;
  params.setParent({}); // don't inherit
  for (int i = 1; i < 100; ++i) {
    key = prefix+QString::number(i);
    if (!params.paramContains(key))
      return key;
  }
  forever {
    key = prefix+QString::number(QRandomGenerator::global()->generate());
    if (!params.paramContains(key))
      return key;
  }
}

QString ParamSetModel::createNewParam() {
  QString key = genererateNewKey(_params), value = u"value"_s;
  ParamSet newParams = _params, oldParams = _params;
  newParams.setValue(key, value);
  changeParams(newParams, oldParams, _paramsetId);
  emit paramsChanged(newParams, oldParams, _paramsetId);
  return key;
}

QModelIndex ParamSetModel::indexOf(QString key, bool allowInherited) const {
  for (int i = _rows.size()-1; i >= 0; --i) {
    if (_rows[i]._inherited && !allowInherited)
      break;
    if (_rows[i]._key == key)
      return index(i, 0);
  }
  return QModelIndex();
}

bool ParamSetModel::removeRows(
    int row, int count, const QModelIndex &parent) {
  if (row < 0 || count <= 0 || row+count > _rows.size() || parent.isValid())
    return false; // at less one row is out of range
  if (_rows[row]._inherited)
    return false; // cannot remove inherited rows (such rows are not selectable)
  ParamSet newParams = _params, oldParams = _params;
  for (int i = row; i < row+count; ++i)
    newParams.removeValue(_rows[i]._key);
  changeParams(newParams, oldParams, _paramsetId);
  emit paramsChanged(newParams, oldParams, _paramsetId);
  return true;
}

void ParamSetModel::fillRows(
    QList<ParamSetRow> *rows, ParamSet params, int depth,
    QSet<QString> *allKeys) {
  if (_inherit) {
    ParamSet parent = params.parent();
    if (!parent.isNull())
      fillRows(rows, parent, depth+1, allKeys);
  }
  params.setParent({}); // don't inherit
  auto localKeys = params.paramKeys().toSortedList();
  QString scope = _scopes.value(depth);
  if (scope.isEmpty() && depth)
    scope = _defaultScopeForInheritedParams;
  foreach (const QString &key, localKeys) {
    if (allKeys->contains(key)) {
      for (int i = 0; i < rows->size(); ++i)
        if ((*rows)[i]._key == key) {
          if (_displayOverriden) {
             (*rows)[i]._overriden = true;
          } else {
            rows->remove(i);
            break;
          }
        }
    }
    QString value = _evaluate ? params.paramValue(key).toString()
                              : params.paramRawValue(key).toString();
    rows->append(ParamSetRow(key, value, scope, false, depth));
    allKeys->insert(key);
  }
}

void ParamSetModel::changeParams(ParamSet newParams, ParamSet oldParams,
                                 QByteArray paramsetId) {
  Q_UNUSED(oldParams)
  if (!_changeParamsIdFilter.isEmpty() && _changeParamsIdFilter != paramsetId)
    return; // ignore filtered out paramsets
  if (!_rows.isEmpty()) {
    beginRemoveRows(QModelIndex(), 0, _rows.size()-1);
    _rows.clear();
    _params = ParamSet();
    endRemoveRows();
  }
  QList<ParamSetRow> rows;
  QSet<QString> allKeys;
  fillRows(&rows, newParams, 0, &allKeys);
  if (!newParams.isEmpty()) {
    beginInsertRows(QModelIndex(), 0, rows.size()-1);
    _rows = rows;
    _paramsetId = paramsetId;
    _params = newParams;
    endInsertRows();
  }
}

Qt::ItemFlags ParamSetModel::flags(const QModelIndex &index) const {
  //qDebug() << "ParamSetModel::flags" << index << index.isValid()
  //         << _rows[index.row()]._overriden << _rows[index.row()]._key;
  Qt::ItemFlags flags = Qt::NoItemFlags;
  if (index.isValid()) {
    flags |= Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    if (!_rows[index.row()]._inherited) {
      flags |= Qt::ItemIsSelectable;
      if (index.column() < 2)
        flags |= Qt::ItemIsEditable;
    }
  }
  return flags;
}
