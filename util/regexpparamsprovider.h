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
#ifndef REGEXPPARAMSPROVIDER_H
#define REGEXPPARAMSPROVIDER_H

#include "paramsprovider.h"
#include <QRegularExpressionMatch>

/** ParamsProvider evaluating QRegularExpressionMatch's catpure groups, both
 * numerical and named ones, as params. */
class LIBP6CORESHARED_EXPORT RegexpParamsProvider : public ParamsProvider {
  QRegularExpressionMatch _match;

public:
  RegexpParamsProvider(
      QRegularExpressionMatch match = QRegularExpressionMatch())
    : _match(match) { }
  QRegularExpressionMatch match() const { return _match; }
  void setMatch(QRegularExpressionMatch  match) { _match = match; }
  using ParamsProvider::paramValue;
  const QVariant paramValue(
    const QString &key, const ParamsProvider *context,
    const QVariant &defaultValue,
    QSet<QString> *alreadyEvaluated) const override;
  const QSet<QString> keys() const override;
};

#endif // REGEXPPARAMSPROVIDER_H
