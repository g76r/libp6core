/* Copyright 2015 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
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
#include "shareduiitemlist.h"
#include "shareduiitem.h"

template <class S>
inline static QString generic_join(
    const QList<SharedUiItem> *list, const S &separator, bool qualified) {
  QString s;
  bool first = true;
  foreach (const SharedUiItem &item, *list) {
    if (first)
      first = false;
    else
      s += separator;
    if (qualified) {
      s += item.idQualifier();
      s += ':';
      s += item.id();
    } else
      s += item.id();
  }
  return s;
}

template<>
QString SharedUiItemList<SharedUiItem>::join(
    const QString &separator, bool qualified) const {
  return generic_join(this, separator, qualified);
}

template<>
QString SharedUiItemList<SharedUiItem>::join(
    const QChar separator, bool qualified) const {
  return generic_join(this, separator, qualified);
}
