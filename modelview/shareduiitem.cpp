/* Copyright 2014-2015 Hallowyn and others.
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
#include "shareduiitem.h"
#include <QtDebug>

SharedUiItemData::~SharedUiItemData() {
}

QVariant SharedUiItemData::uiData(int section, int role) const {
  Q_UNUSED(section)
  Q_UNUSED(role)
  return QVariant();
}

bool SharedUiItemData::setUiData(int section, const QVariant &value, int role) {
  Q_UNUSED(section)
  Q_UNUSED(role)
  Q_UNUSED(value)
  return false;
}

Qt::ItemFlags SharedUiItemData::uiFlags(int section) const {
  Q_UNUSED(section)
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant SharedUiItemData::uiHeaderData(int section, int role) const {
  Q_UNUSED(section)
  Q_UNUSED(role)
  return QVariant();
}

QString SharedUiItemData::id() const {
  return uiData(0, Qt::DisplayRole).toString();
}

QString SharedUiItemData::idQualifier() const {
  return QString();
}

int SharedUiItemData::uiSectionCount() const {
  return 0;
}

QDebug operator<<(QDebug dbg, const SharedUiItem &i) {
  dbg.nospace() << i.id();
  return dbg.space();
}
