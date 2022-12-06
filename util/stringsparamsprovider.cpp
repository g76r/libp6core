/* Copyright 2015-2022 Hallowyn, Gregoire Barbier and others.
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
#include "stringsparamsprovider.h"

const QVariant StringsParamsProvider::paramValue(
  const QString &key, const ParamsProvider *, const QVariant &defaultValue,
  QSet<QString> *) const {
  bool ok;
  int i = key.toInt(&ok);
  return (ok && i >= 1 && _strings.size() <= i) ? _strings[i-1] : defaultValue;
}

const QSet<QString> StringsParamsProvider::keys() const {
  QSet<QString> keys;
  for (int i = 0; i < _strings.size(); ++i)
    keys << QString::number(i+1);
  return keys;
}
