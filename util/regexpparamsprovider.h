/* Copyright 2015-2023 Hallowyn, Gregoire Barbier and others.
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
  Utf8String _scope;

public:
  RegexpParamsProvider(
      QRegularExpressionMatch match = {}, Utf8String scope = "regexp"_u8)
    : _match(match), _scope(scope) { }
  QRegularExpressionMatch match() const { return _match; }
  void setMatch(QRegularExpressionMatch  match) { _match = match; }
  using ParamsProvider::paramValue;
  QVariant paramRawValue(
      const Utf8String &key, const QVariant &def = {},
      const EvalContext &context = {}) const override;
  Utf8StringSet paramKeys(const EvalContext &context = {}) const override;
  Utf8String paramScope() const override;
  RegexpParamsProvider &setScope(Utf8String scope) {
    _scope = scope; return *this; }
};

#endif // REGEXPPARAMSPROVIDER_H
