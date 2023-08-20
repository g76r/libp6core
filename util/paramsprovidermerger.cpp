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

static inline ParamsProvider::ScopedValue cookScopedValue(
    const ParamsProvider::ScopedValue &sv, const Utf8String &forced_scope) {
  if (!forced_scope.isEmpty())
    return { forced_scope, sv.value };
  return sv;
}

const QVariant ParamsProviderMerger::paramRawValue(
    const Utf8String &key, const QVariant &def) const {
  return paramScopedRawValue(key, def);
}

const ParamsProvider::ScopedValue ParamsProviderMerger::paramScopedRawValue(
    const Utf8String &key, const QVariant &def) const {
  int depth = 0;
  auto sv = _overridingParams.paramScopedRawValue(key);
  if (sv.isValid())
    return cookScopedValue(sv, _scope);
  for (auto provider: _providers) {
    ++depth;
    if (provider.d->_wild)
      sv = provider.d->_wild->paramScopedRawValue(key);
    else
      sv = provider.d->_owned.paramScopedRawValue(key);
    if (sv.isValid())
      return cookScopedValue(sv, _scope);
  }
  sv = { {}, def };
  return cookScopedValue(sv, _scope);
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
    if (provider.d->_wild)
      keys += provider.d->_wild->paramKeys();
    else
      keys += provider.d->_owned.paramKeys();
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
    dbg.space() << "provider: " << provider.d->_wild << " " << provider.d->_owned << ",";
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
    lh << " provider: " << provider.d->_wild << " " << provider.d->_owned << ",";
  }
  lh << " overridingParams: " << merger->overridingParams() << ",";
  return lh << " stack size: " << merger->_providersStack.size() << " }";
}
