/* Copyright 2013-2018 Hallowyn, Gregoire Barbier and others.
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
#include "paramsprovidermerger.h"
#include "log/log.h"

namespace {

/** Two providers merger used internaly as an helper during evaluation. */
class SimplerMerger : public ParamsProvider {
    const ParamsProvider *_first;
    const ParamsProvider *_second;
public:
    SimplerMerger(const ParamsProvider *first, const ParamsProvider *second)
        : _first(first), _second(second) { }
    /** Evaluate using second provider if first returns an invalid QVariant. */
    QVariant paramValue(QString key, const ParamsProvider *context,
                        QVariant defaultValue, QSet<QString> alreadyEvaluated
                        ) const override {
        QVariant v = _first->paramValue(key, context, defaultValue,
                                        alreadyEvaluated);
        if (!v.isValid() && _second)
            _second->paramValue(key, context, defaultValue, alreadyEvaluated);
        return v;
    }
};

} // unnamed namespace

QVariant ParamsProviderMerger::paramValue(
        QString key, const ParamsProvider *context, QVariant defaultValue,
        QSet<QString> alreadyEvaluated) const {
  SimplerMerger sm(this, context);
  QVariant v = _overridingParams.paramValue(key, &sm, QVariant(),
                                            alreadyEvaluated);
  if (!v.isNull())
    return v;
  foreach (const Provider &provider, _providers) {
    if (provider.d->_paramsProvider) {
      QVariant v = provider.d->_paramsProvider
          ->paramValue(key, &sm, QVariant(), alreadyEvaluated);
      if (!v.isNull())
        return v;
    } else {
      QString s = provider.d->_paramset.value(key, true, &sm, alreadyEvaluated);
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
    Log::warning() << "calling ParamsProviderMerger::restore() without "
                      "previously calling ParamsProviderMerger::save()";
  }
}
