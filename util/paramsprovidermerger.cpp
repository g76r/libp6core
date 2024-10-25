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

using Provider = ParamsProviderMerger::Provider;

class ParamsProviderMergerData : public QSharedData {
public:
  QList<Provider> _providers;
  ParamSet _overridingParams;
  Utf8String _scope;

  ParamsProviderMergerData() = default;
  ParamsProviderMergerData(const Utf8String &scope) : _scope(scope) {}
  ParamsProviderMergerData(const ParamsProviderMergerData &other) = default;
};

ParamsProviderMerger::ParamsProviderMerger()
  : _data(new ParamsProviderMergerData()) {
}

ParamsProviderMerger::ParamsProviderMerger(const ParamsProviderMerger &other)
  : _data(other._data) {
}

ParamsProviderMerger::ParamsProviderMerger(
    const ParamsProvider *provider, const Utf8String &scope)
  : _data(new ParamsProviderMergerData(scope)) {
  append(provider);
}

ParamsProviderMerger::ParamsProviderMerger(
    const ParamSet &provider, bool inherit, const Utf8String &scope)
  : _data(new ParamsProviderMergerData(scope)) {
  append(provider, inherit);
}

ParamsProviderMerger &ParamsProviderMerger::append(
    const ParamsProvider *provider) {
  auto d = _data.data();
  if (provider)
    d->_providers.append(provider);
  return *this;
}

ParamsProviderMerger &ParamsProviderMerger::append(
    const ParamSet &provider, bool inherit) {
  if (!provider)
    return *this;
  auto d = _data.data();
  if (!inherit) {
    auto orphan = provider;
    orphan.setParent({});
    d->_providers.append(orphan);
  } else {
    d->_providers.append(provider);
  }
  return *this;
}

ParamsProviderMerger &ParamsProviderMerger::prepend(
    const ParamsProvider *provider) {
  auto d = _data.data();
  if (provider)
    d->_providers.prepend(provider);
  return *this;
}

ParamsProviderMerger &ParamsProviderMerger::prepend(
    const ParamSet &provider, bool inherit) {
  if (!provider)
    return *this;
  auto d = _data.data();
  if (!inherit) {
    auto orphan = provider;
    orphan.setParent({});
    d->_providers.prepend(orphan);
  } else {
    d->_providers.prepend(provider);
  }
  return *this;
}

ParamsProviderMerger &ParamsProviderMerger::pop_front() {
  auto d = _data.data();
  if (!d->_providers.isEmpty())
    d->_providers.pop_front();
  return *this;
}

ParamsProviderMerger &ParamsProviderMerger::pop_back() {
  auto d = _data.data();
  if (!d->_providers.isEmpty())
    d->_providers.pop_back();
  return *this;
}

ParamsProviderMerger &ParamsProviderMerger::overrideParamValue(
    const Utf8String &key, const QVariant &value) {
  auto d = _data.data();
  d->_overridingParams.insert(key, value);
  return *this;
}

ParamsProviderMerger &ParamsProviderMerger::unoverrideParamValue(
    const Utf8String &key) {
  auto d = _data.data();
  d->_overridingParams.erase(key);
  return *this;
}

ParamsProviderMerger &ParamsProviderMerger::clear() {
  auto d = _data.data();
  d->_providers.clear();
  d->_overridingParams.clear();
  return *this;
}

QVariant ParamsProviderMerger::paramRawValue(
    const Utf8String &key, const QVariant &def,
    const EvalContext &original_context) const {
  auto d = _data.data();
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
  v = d->_overridingParams.paramRawValue(key, {}, context);
  if (v.isValid())
    return v;
  for (auto provider: d->_providers) {
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
  auto d = _data.data();
  auto merger_scope = paramScope();
  EvalContext context = original_context;
  if (!merger_scope.isEmpty()) {
    // if merger has a scope, behave like if merged providers have this scope
    if (!context.hasScopeOrNone(paramScope()))
      return {};
    context.setScopeFilter({});
  } // otherwise let merged providers filter themselves
  Utf8StringSet keys { d->_overridingParams.paramKeys() };
  for (auto provider: d->_providers) {
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

const ParamSet ParamsProviderMerger::overridingParams() const {
  return _data.data()->_overridingParams;
}

ParamsProviderMerger &ParamsProviderMerger::setScope(Utf8String scope) {
  _data.data()->_scope = scope;
  return *this;
}

Utf8String ParamsProviderMerger::paramScope() const {
  return _data.data()->_scope;
}

QDebug operator<<(QDebug dbg, const ParamsProviderMerger *merger) {
  if (!merger) {
    dbg.nospace() << "nullptr";
    return dbg.space();
  }
  auto d = merger->_data.data();
  dbg.nospace() << "{";
  for (auto provider: d->_providers) {
    dbg.space()
        << "provider: " << provider.d->_wild << " "
        << (provider.d->_wild ? provider.d->_wild->paramKeys().toSortedList()
                              : Utf8StringList{})
        << provider.d->_owned << " scope: "
        << (provider.d->_wild ? provider.d->_wild->paramScope()
                              : provider.d->_owned.paramScope())
        << ",";
  }
  dbg.space() << "overridingParams: " << d->_overridingParams << ",";
  //dbg.space() << "stack size: " << d->_providersStack.size() << " }";
  return dbg.space();
}

LogHelper operator<<(LogHelper lh, const ParamsProviderMerger *merger) {
  if (!merger)
    return lh << "nullptr";
  auto d = merger->_data.data();
  lh << "{ ";
  for (auto provider: d->_providers) {
    lh << " provider: " << provider.d->_wild << " "
       << (provider.d->_wild ? provider.d->_wild->paramKeys().toSortedList()
                             : Utf8StringList{})
       << provider.d->_owned << " scope: "
       << (provider.d->_wild ? provider.d->_wild->paramScope()
                             : provider.d->_owned.paramScope())
       << ",";
  }
  return lh << " overridingParams: " << d->_overridingParams << " }";
  // lh << " overridingParams: " << d->_overridingParams << ",";
  // return lh << " stack size: " << d->_providersStack.size() << " }";
}
