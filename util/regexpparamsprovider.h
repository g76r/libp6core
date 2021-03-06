/* Copyright 2015-2018 Hallowyn, Gregoire Barbier and others.
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
class LIBPUMPKINSHARED_EXPORT RegexpParamsProvider : public ParamsProvider {
  QRegularExpressionMatch _match;

public:
  RegexpParamsProvider(
      QRegularExpressionMatch match = QRegularExpressionMatch())
    : _match(match) { }
  QRegularExpressionMatch match() const { return _match; }
  void setMatch(QRegularExpressionMatch  match) { _match = match; }
  QVariant paramValue(QString key, const ParamsProvider *context = 0,
                      QVariant defaultValue = QVariant(),
                      QSet<QString> alreadyEvaluated = QSet<QString>()
          ) const override;
};

#endif // REGEXPPARAMSPROVIDER_H
