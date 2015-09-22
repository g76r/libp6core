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
#ifndef REGEXPPARAMSPROVIDER_H
#define REGEXPPARAMSPROVIDER_H

#include "paramsprovider.h"
#include <QRegularExpressionMatch>

/** ParamsProvider evaluating QRegularExpressionMatch's catpure groups, both
 * numerical and named ones, as params. */
class LIBQTSSUSHARED_EXPORT RegexpParamsProvider
    : public ParamsProvider {
  QRegularExpressionMatch _match;

public:
  RegexpParamsProvider(
      QRegularExpressionMatch match = QRegularExpressionMatch())
    : _match(match) { }
  QRegularExpressionMatch match() const { return _match; }
  void setMatch(QRegularExpressionMatch  match) { _match = match; }
  QVariant paramValue(QString key, QVariant defaultValue = QVariant(),
                      QSet<QString> alreadyEvaluated = QSet<QString>()) const;
};

#endif // REGEXPPARAMSPROVIDER_H
