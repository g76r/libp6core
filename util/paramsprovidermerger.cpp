/* Copyright 2013-2015 Hallowyn and others.
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
#include "paramsprovidermerger.h"
#include <QtDebug>

QVariant ParamsProviderMerger::paramValue(
    QString key, QVariant defaultValue, QSet<QString> alreadyEvaluated) const {
  QVariant v = _overridingParams.paramValue(key, QVariant(), alreadyEvaluated);
  if (!v.isNull())
    return v;
  foreach (const Provider &provider, _providers) {
    if (provider.d->_paramsProvider) {
      QVariant v = provider.d->_paramsProvider
          ->paramValue(key, QVariant(), alreadyEvaluated);
      if (!v.isNull())
        return v;
    } else {
      // ParamSet is a special case since it is able to use the whole
      // ParamsProviderMerger as an evaluation context instead of staying local
      // to itself
      // e.g. %foo can be evaluated even if foo is defined in another params
      // provider of the ParamsProviderMerger than the current one
      QString s = provider.d->_paramset
          .value(key, true, this, alreadyEvaluated);
      if (!s.isNull())
        return s;
    }
  }
  return defaultValue;
}

void ParamsProviderMerger::save() {
  _providersStack.prepend(_providers);
  _overridingParamsStack.prepend(_overridingParams);
}

void ParamsProviderMerger::restore() {
  if (!_providersStack.isEmpty() && !_overridingParamsStack.isEmpty()) {
    _providers = _providersStack.first();
    _providersStack.removeFirst();
    _overridingParams = _overridingParamsStack.first();
    _overridingParamsStack.removeFirst();
  } else {
    qDebug() << "calling ParamsProviderMerger::restore() without previously "
                "calling ParamsProviderMerger::save()";
  }
}
