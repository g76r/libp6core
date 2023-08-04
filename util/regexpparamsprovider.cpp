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
#include "regexpparamsprovider.h"
#include <QRegularExpression>

static QRegularExpression integerRE("\\A\\d+\\z");

const QVariant RegexpParamsProvider::paramValue(
    const Utf8String &key, const ParamsProvider *,
    const QVariant &defaultValue, Utf8StringSet *) const {
  if (key.isEmpty())
    return defaultValue;
  auto value = _match.captured(key);
  if (!value.isNull())
    return value;
  QRegularExpressionMatch match = integerRE.match(key);
  if (match.hasMatch()) {
    value = _match.captured(match.captured().toInt());
    if (!value.isNull())
      return value;
  }
  return defaultValue;
}

const Utf8StringSet RegexpParamsProvider::keys() const {
  Utf8StringSet keys;
  qsizetype n = _match.capturedTexts().size();
  for (qsizetype i = 0; i < n; ++i)
    keys << Utf8String::number(i);
  for (auto key: _match.regularExpression().namedCaptureGroups())
    if (!key.isEmpty())
      keys << key;
  return keys;
}
