/* Copyright 2013-2023 Hallowyn, Gregoire Barbier and others.
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
    const QVariant paramValue(
      const Utf8String &key, const ParamsProvider *context,
      const QVariant &defaultValue,
      Utf8StringSet *alreadyEvaluated) const override {
      QVariant v = _first->paramValue(key, context, defaultValue,
                                      alreadyEvaluated);
      if (!v.isValid() && _second)
        _second->paramValue(key, context, defaultValue, alreadyEvaluated);
      return v;
    }
    const Utf8StringSet paramKeys() const override {
      return {};
    }
};

} // unnamed namespace

const QVariant ParamsProviderMerger::paramValue(
    const Utf8String &key, const ParamsProvider *context,
    const QVariant &defaultValue, Utf8StringSet *alreadyEvaluated) const {
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
      auto s = provider.d->_paramset.value(key, true, &sm, alreadyEvaluated);
      if (!s.isNull())
        return s;
    }
  }
  bool functionFound = false;
  auto s = ParamSet::evaluateFunction(ParamSet(), key, false, &sm,
                                      alreadyEvaluated, &functionFound);
  if (functionFound)
    return s;
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

const Utf8StringSet ParamsProviderMerger::paramKeys() const {
  Utf8StringSet keys { _overridingParams.paramKeys() };
  for (auto provider: _providers) {
    if (provider.d->_paramsProvider) {
      keys += provider.d->_paramsProvider->paramKeys();
    } else {
      keys += provider.d->_paramset.paramKeys();
    }
  }
  return keys;
}

const Utf8String ParamsProviderMerger::paramScope() const {
  return _scope;
}

QDebug operator<<(QDebug dbg, const ParamsProviderMerger *merger) {
  if (!merger) {
    dbg.nospace() << "nullptr";
    return dbg.space();
  }
  dbg.nospace() << "{";
  for (auto provider: merger->_providers) {
    dbg.space() << "provider: " << provider.d->_paramsProvider << " " << provider.d->_paramset << ",";
  }
  dbg.space() << "overridingParams: " << merger->overridingParams() << ",";
  dbg.space() << "stack size: " << merger->_providersStack.size() << " }";
  return dbg.space();
}

LogHelper operator<<(LogHelper lh, const ParamsProviderMerger *merger) {
  if (!merger)
    return lh << "nullptr";
  lh << "{ ";
  for (auto provider: merger->_providers) {
    lh << " provider: " << provider.d->_paramsProvider << " " << provider.d->_paramset << ",";
  }
  lh << " overridingParams: " << merger->overridingParams() << ",";
  return lh << " stack size: " << merger->_providersStack.size() << " }";
}
