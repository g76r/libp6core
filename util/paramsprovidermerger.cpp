/* Copyright 2013-2024 Hallowyn, Gregoire Barbier and others.
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

class Provider {
public:
  const ParamsProvider *_wild;
  ParamSet _owned;

  Provider(const ParamsProvider *wild = 0) : _wild(wild) { }
  Provider(const ParamSet &owned) : _wild(0), _owned(owned) { }
  [[nodiscard]] inline bool is_wild() const { return _wild; }
  [[nodiscard]] inline const ParamsProvider *provider() const {
    return _wild ? _wild : &_owned; }
  [[nodiscard]] inline const ParamsProvider *wild() const { return _wild; }
  [[nodiscard]] inline const ParamsProvider& operator*() const {
    return *provider(); }
  [[nodiscard]] inline const ParamsProvider* operator->() const {
    return provider(); }
};

} // anonymous namespace

class ParamsProviderMergerData : public QSharedData {
public:
  QList<Provider> _providers;
  ParamSet _overridingParams;
  Utf8String _scope;

  ParamsProviderMergerData() = default;
  ParamsProviderMergerData(const Utf8String &scope) : _scope(scope) {}
  ParamsProviderMergerData(const ParamsProviderMergerData &other) = default;
};

ParamsProviderMerger::ParamsProviderMerger() noexcept
  : _data(new ParamsProviderMergerData()) {
}

ParamsProviderMerger::ParamsProviderMerger(
    const ParamsProviderMerger &other) noexcept
  : _data(other._data) {
}

ParamsProviderMerger::ParamsProviderMerger(
    ParamsProviderMerger &&other) noexcept
  : _data(std::move(other._data)) {
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

ParamsProviderMerger::~ParamsProviderMerger() noexcept {
}

ParamsProviderMerger &ParamsProviderMerger::operator=(
    const ParamsProviderMerger &other) noexcept {
  if (this != &other)
    _data = other._data;
  return *this;
}

ParamsProviderMerger &ParamsProviderMerger::operator=(
    ParamsProviderMerger &&other) noexcept {
  if (this != &other)
    _data = std::move(other._data);
  return *this;
}

ParamsProviderMerger &ParamsProviderMerger::append(
    const ParamsProvider *provider) {
  if (provider)
    _data->_providers.append(provider);
  return *this;
}

ParamsProviderMerger &ParamsProviderMerger::append(
    const ParamSet &provider, bool inherit) {
  if (!provider)
    return *this;
  if (!inherit) {
    auto orphan = provider;
    orphan.setParent({});
    _data->_providers.append(orphan);
  } else {
    _data->_providers.append(provider);
  }
  return *this;
}

ParamsProviderMerger &ParamsProviderMerger::prepend(
    const ParamsProvider *provider) {
  if (provider)
    _data->_providers.prepend(provider);
  return *this;
}

ParamsProviderMerger &ParamsProviderMerger::prepend(
    const ParamSet &provider, bool inherit) {
  if (!provider)
    return *this;
  if (!inherit) {
    auto orphan = provider;
    orphan.setParent({});
    _data->_providers.prepend(orphan);
  } else {
    _data->_providers.prepend(provider);
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
  _data->_overridingParams.insert(key, value);
  return *this;
}

ParamsProviderMerger &ParamsProviderMerger::unoverrideParamValue(
    const Utf8String &key) {
  _data->_overridingParams.erase(key);
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
  for (auto &provider: d->_providers) {
    auto v = provider->paramRawValue(key, {}, context);
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
  for (auto &provider: d->_providers) {
    keys += provider->paramKeys(context);
  }
  return keys;
}

ParamSet ParamsProviderMerger::overridingParams() const {
  return _data->_overridingParams;
}

ParamsProviderMerger &ParamsProviderMerger::setScope(Utf8String scope) {
  _data->_scope = scope;
  return *this;
}

Utf8String ParamsProviderMerger::paramScope() const {
  return _data->_scope;
}

size_t ParamsProviderMerger::providers_count() const {
  return _data->_providers.count();
}

Utf8String ParamsProviderMerger::human_readable() const {
  Utf8String s = "{"_u8;
  for (auto &provider: _data->_providers) {
    s = s+" provider: "+Utf8String::number((qsizetype)provider.wild())+" "+
        provider->paramKeys().toSortedList().human_readable()+
        " scope: "+provider->paramScope()+",";
  }
  s = s+" overridingParams: "+overridingParams().toString()+" }";
  return s;
}

QDebug operator<<(QDebug dbg, const ParamsProviderMerger *merger) {
  if (merger)
    dbg.nospace() << merger->human_readable();
  return dbg;
}

LogHelper operator<<(LogHelper lh, const ParamsProviderMerger *merger) {
  if (merger)
    lh << merger->human_readable();
  return lh;
}
