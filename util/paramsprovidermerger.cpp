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

QVariant ParamsProviderMerger::paramRawValue(
    const Utf8String &key, const QVariant &def,
    const EvalContext &original_context) const {
  auto merger_scope = paramScope();
  EvalContext context = original_context;
  QVariant v;
  if (!context.functionsEvaluated()) {
    bool is_function;
    v = PercentEvaluator::eval_function(key, context, &is_function);
    if (is_function)
      return v;
    // don't call context.setFunctionsEvaluated() because ParamsProvider
    // implementation may include custom functions or other complex features
    // implying %-evaluation re-entrance needing functions evaluation again
    // (may do it for paramsets)
  }
  if (!merger_scope.isEmpty()) {
    // if merger has a scope, pretend that all merged providers have this scope
    if (!context.hasScopeOrNone(paramScope()))
      return def;
    context.setScopeFilter({});
  } // otherwise let merged providers filter themselves
  v = _overridingParams.paramRawValue(key, {}, context);
  if (v.isValid())
    return v;
  for (auto provider: _providers) {
    const ParamsProvider *pp = provider.d->_wild;
    if (!pp)
      pp = &provider.d->_owned;
    auto v = pp->paramRawValue(key, {}, context);
    if (v.isValid())
      return v;
  }
  return def;
}

Utf8StringSet ParamsProviderMerger::paramKeys(
    const EvalContext &original_context) const {
  auto merger_scope = paramScope();
  EvalContext context = original_context;
  if (!merger_scope.isEmpty()) {
    // if merger has a scope, behave like if merged providers have this scope
    if (!context.hasScopeOrNone(paramScope()))
      return {};
    context.setScopeFilter({});
  } // otherwise let merged providers filter themselves
  Utf8StringSet keys { _overridingParams.paramKeys() };
  for (auto provider: _providers) {
    if (provider.d->_wild) {
      //keys += "["_u8+provider.d->_wild->paramScope()+"]";
      keys += provider.d->_wild->paramKeys(context);
    }else {
      //keys += "["_u8+provider.d->_owned.paramScope()+"]";
      keys += provider.d->_owned.paramKeys(context);
    }
  }
  return keys;
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

Utf8String ParamsProviderMerger::paramScope() const {
  return _scope;
}

QDebug operator<<(QDebug dbg, const ParamsProviderMerger *merger) {
  if (!merger) {
    dbg.nospace() << "nullptr";
    return dbg.space();
  }
  dbg.nospace() << "{";
  for (auto provider: merger->_providers) {
    dbg.space()
        << "provider: " << provider.d->_wild << " "
        << (provider.d->_wild ? provider.d->_wild->paramKeys().toSortedList()
                              : Utf8StringList{})
        << provider.d->_owned << " scope: "
        << (provider.d->_wild ? provider.d->_wild->paramScope()
                              : provider.d->_owned.paramScope())
        << ",";
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
    lh << " provider: " << provider.d->_wild << " "
       << (provider.d->_wild ? provider.d->_wild->paramKeys().toSortedList()
                             : Utf8StringList{})
       << provider.d->_owned << " scope: "
       << (provider.d->_wild ? provider.d->_wild->paramScope()
                             : provider.d->_owned.paramScope())
       << ",";
  }
  lh << " overridingParams: " << merger->overridingParams() << ",";
  return lh << " stack size: " << merger->_providersStack.size() << " }";
}
