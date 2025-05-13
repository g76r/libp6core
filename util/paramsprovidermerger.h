/* Copyright 2013-2025 Hallowyn, Gregoire Barbier and others.
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
#ifndef PARAMSPROVIDERMERGER_H
#define PARAMSPROVIDERMERGER_H

#include "paramset.h"

class ParamsProviderMergerData;

/** This class builds up several ParamsProvider into only one, chaining
 * calls to paramRawValue().
 *
 * Does not take ownership on referenced ParamsProvider, these objects must
 * not be deleted before the last call to ParamsProviderList::paramValue()
 * or must be removed from the ParamsProviderMerger before their destruction.
 *
 * Therefore ParamsProviderMerger should only be used as a temporary object
 * around a call to some method taking a ParamsProvider as a parameter.
 *
 * When a scope is set, behave like if all merged providers have this scope
 * rather than their own, otherwise let every merged provider filtering using
 * its own scope, which is a way to choose one of their merged providers rather
 * than previous one. */
class LIBP6CORESHARED_EXPORT ParamsProviderMerger : public ParamsProvider {
  friend QDebug LIBP6CORESHARED_EXPORT operator<<(
      QDebug dbg, const ParamsProviderMerger *params);
  friend p6::log::LogHelper LIBP6CORESHARED_EXPORT operator<<(
      p6::log::LogHelper lh, const ParamsProviderMerger *merger);
  QSharedDataPointer<ParamsProviderMergerData> _data;

public:
  ParamsProviderMerger() noexcept;
  ~ParamsProviderMerger() noexcept;
  ParamsProviderMerger(const ParamsProviderMerger &other) noexcept;
  ParamsProviderMerger(ParamsProviderMerger &&other) noexcept;
  explicit ParamsProviderMerger(
      const ParamsProvider *provider, const Utf8String &scope = {});
  explicit ParamsProviderMerger(
      const ParamSet &provider, bool inherit = true,
      const Utf8String &scope = {});
  ParamsProviderMerger(const ParamSet &provider, const Utf8String &scope)
    : ParamsProviderMerger(provider, true, scope) { }
  ParamsProviderMerger &operator=(const ParamsProviderMerger &other) noexcept;
  ParamsProviderMerger &operator=(ParamsProviderMerger &&other) noexcept;
  /** Add a ParamsProvider that will be evaluated after those already added. */
  ParamsProviderMerger &append(const ParamsProvider *provider);
  /** Add a ParamsProvider that will be evaluated after those already added. */
  ParamsProviderMerger &append(const ParamSet &provider, bool inherit = true);
  /** Add a ParamsProvider that will be evaluated before those already added but
   * after parameters set with overrideParamValue(). */
  ParamsProviderMerger &prepend(const ParamsProvider *provider);
  /** Add a ParamsProvider that will be evaluated before those already added but
   * after parameters set with overrideParamValue(). */
  ParamsProviderMerger &prepend(const ParamSet &provider, bool inherit = true);
  ParamsProviderMerger &pop_front();
  ParamsProviderMerger &pop_back();
  /** Convenience operator for append() */
  inline ParamsProviderMerger &operator()(const ParamsProvider *provider) {
    return append(provider);
  }
  /** Convenience operator for append() */
  inline ParamsProviderMerger &operator()(
      const ParamSet &provider, bool inherit = true) {
    return append(provider, inherit);
  }
  /** Parameters set through overrideParamValue() will override any
   * ParamsProvider, even those prepended. */
  ParamsProviderMerger &overrideParamValue(
      const Utf8String &key, const TypedValue &value);
  /** Remove an override set using overrideParamValue(). */
  ParamsProviderMerger &unoverrideParamValue(const Utf8String &key);
  /** Remove all ParamsProvider and overriding params. */
  ParamsProviderMerger &clear();
  using ParamsProvider::paramRawValue;
  [[nodiscard]] TypedValue paramRawValue(
      const Utf8String &key, const TypedValue &def = {},
      const EvalContext &context = {}) const override;
  [[nodiscard]] Utf8StringSet paramKeys(
      const EvalContext &context = {}) const override;
  [[nodiscard]] Utf8String paramScope() const override;
  ParamsProviderMerger &setScope(Utf8String scope);
  /** Give access to currently overriding params. */
  [[nodiscard]] ParamSet overridingParams() const;
  [[nodiscard]] size_t providers_count() const;
  [[nodiscard]] Utf8String human_readable() const;
};

/** RAII helper for ParamsProviderMerger save/restore.
 *
 * Can be used that way:
 * void myfunc(ParamsProviderMerger *merger) {
 *   ParamsProviderMergerRestorer restorer(merger);
 *   // modify merger the way you want, it'll be restored when myfunc() exits
 * }
 */
class LIBP6CORESHARED_EXPORT ParamsProviderMergerRestorer {
  ParamsProviderMerger *_merger, _backup;
  ParamsProviderMergerRestorer() = delete;
  ParamsProviderMergerRestorer(const ParamsProviderMergerRestorer &) = delete;
public:
  ParamsProviderMergerRestorer(ParamsProviderMerger *merger)
    : _merger(merger) {
    if (_merger)
      _backup = *merger;
  }
  ParamsProviderMergerRestorer(ParamsProviderMerger &merger)
    : ParamsProviderMergerRestorer(&merger) { }
  ~ParamsProviderMergerRestorer() {
    if (_merger)
      *_merger = _backup;
  }
};

QDebug LIBP6CORESHARED_EXPORT operator<<(
    QDebug dbg, const ParamsProviderMerger *params);

p6::log::LogHelper LIBP6CORESHARED_EXPORT operator<<(
    p6::log::LogHelper lh, const ParamsProviderMerger *merger);

#endif // PARAMSPROVIDERMERGER_H
