/* Copyright 2023-2025 Gregoire Barbier and others.
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
#ifndef TEMPLATEDSHAREDUIITEMDATA_H
#define TEMPLATEDSHAREDUIITEMDATA_H

#include "modelview/shareduiitem.h"
#include "thread/atomicvalue.h"
#include "util/paramset.h"

using SharedUiItemDataFunction = std::function<
TypedValue(const SharedUiItemData *data, const Utf8String &key,
const PercentEvaluator::EvalContext &context, int matched_length)>;

using SharedUiItemDataFunctions = RadixTree<SharedUiItemDataFunction>;

template<class T>
class SharedUiItemDynamicData : public SharedUiItemData {
public:
  // SharedUiItemData interface
  [[nodiscard]] int uiSectionCount() const final {
    return T::_sectionNames.size(); }
  [[nodiscard]] Utf8String uiSectionName(int section) const final {
    return T::_sectionNames.value(section); }
  [[nodiscard]] int uiSectionByName(Utf8String sectionName) const final {
    return T::_sectionNames.toIndex().value(sectionName, -1); }
  [[nodiscard]] QVariant uiHeaderData(int section, int role) const final {
    switch (role) {
      case Qt::DisplayRole:
      case Qt::EditRole:
      case SharedUiItem::ExternalDataRole:
        return T::_headerNames.value(section);
    }
    return {};
  }

  // ParamsProvider interface
  Utf8String paramScope() const override { return qualifier(); }
};

/** Helper class template to implement SharedUiItemData.
 *
 *  Actual SharedUiItemData implementation, e.g. FooData, can inherit from
 *  SharedUiItemDataBase<FooData> and won't have to reimplement some part of
 *  the boiler plate provided it have the folowing static members defined:
 *  - const static Utf8String _qualifier e.g. = "foo"
 *     (but with XxxDynamicXxx variants, which is useful in complex cases
 *     where you implement several qualifiers with the same class or with a
 *     class hierarchy)
 *  - const static Utf8StringIndexedConstList _sectionNames
 *      e.g. = { "id", "parent", "name"}
 *  - const static Utf8StringIndexedConstList _headerNames
 *      e.g. = { "Id", "Parent", "Name"}
 *    (_headerNames can be = to _sectionNames if its convenient for you)
 *
 *  Some more specialized templates with more features exist, to use
 *  SharedUiItemDataWithFunction<FooData> you also need
 *  - const static SharedUiItemDataFunctions _paramFunctions
 *
 *  When using XxxDataWithImmutableParams or XxxDataWithMutableParams you'll
 *  have a ParamSet or AtomicValue<ParamSet> _params non-static member
 *  (you don't have to define it it will be inherited from the template).
 *
 *  @see SharedUiItemDataWithFunction
 *  @see SharedUiItemDataWithImmutableParams
 *  @see SharedUiItemDataWithMutableParams
 *  @see SharedUiItemDynamicData
 *  @see SharedUiItemDynamicDataWithFunction
 *  @see SharedUiItemDynamicDataWithImmutableParams
 *  @see SharedUiItemDynamicDataWithMutableParams
 *  @see SharedUiItemData
 *  @see SharedUiItem
 */
template<class T>
class SharedUiItemDataBase : public SharedUiItemDynamicData<T> {
public:
  // SharedUiItemData interface
  [[gnu::const]] virtual Utf8String qualifier() const final {
    return T::_qualifier; }

  // ParamsProvider interface
  [[gnu::const]] Utf8String paramScope() const final {
    return T::_qualifier; }
};

template<class T>
class SharedUiItemDataWithFunctions : public SharedUiItemDataBase<T> {
public:
  // ParamsProvider interface
  TypedValue paramRawValue(
      const Utf8String &key, const TypedValue &def,
      const PercentEvaluator::EvalContext &context) const override {
    if (!context.hasScopeOrNone(SharedUiItemDataBase<T>::paramScope()))
      return def;
    int ml;
    auto f = T::_paramFunctions.value(key, &ml);
    if (f)
      return f(this, key, context, ml);
    return SharedUiItemDataBase<T>::paramRawValue(key, def, context);
  }
  Utf8StringSet paramKeys(
      const PercentEvaluator::EvalContext &context) const override {
    Utf8StringSet keys = T::_paramFunctions.keys();
    keys |= SharedUiItemDataBase<T>::paramKeys(context);
    return keys;
  }
};

template<class T>
class SharedUiItemDynamicDataWithFunctions : public SharedUiItemDynamicData<T> {
public:
  // ParamsProvider interface
  TypedValue paramRawValue(
      const Utf8String &key, const TypedValue &def,
      const PercentEvaluator::EvalContext &context) const override {
    if (!context.hasScopeOrNone(SharedUiItemDynamicData<T>::paramScope()))
      return def;
    int ml;
    auto f = T::_paramFunctions.value(key, &ml);
    if (f)
      return f(this, key, context, ml);
    return SharedUiItemDynamicData<T>::paramRawValue(key, def, context);
  }
  Utf8StringSet paramKeys(
      const PercentEvaluator::EvalContext &context) const override {
    Utf8StringSet keys = T::_paramFunctions.keys();
    keys |= SharedUiItemDynamicData<T>::paramKeys(context);
    return keys;
  }
};

template<class T, bool INCLUDE_UI_DATA_AS_PARAM = false>
class SharedUiItemDataWithMutableParams
    : public SharedUiItemDataWithFunctions<T> {
public:
  mutable AtomicValue<ParamSet> _params;

  SharedUiItemDataWithMutableParams(
      const ParamSet &params, const Utf8String &scope)
    : _params(params) {
    _params.lockedData()->setScope(scope);
  }
  SharedUiItemDataWithMutableParams(const ParamSet &params = {})
    : SharedUiItemDataWithMutableParams(params, T::_qualifier) {}

  // ParamsProvider interface
  TypedValue paramRawValue(
      const Utf8String &key, const TypedValue &def,
      const PercentEvaluator::EvalContext &context) const override {
    if (!context.hasScopeOrNone(
          SharedUiItemDataWithFunctions<T>::paramScope()))
      return def;
    int ml;
    auto f = T::_paramFunctions.value(key, &ml);
    if (f)
      return f(this, key, context, ml);
    auto v = _params.lockedData()->paramRawValue(key, {}, context);
    if (!!v)
      return v;
    if constexpr (!INCLUDE_UI_DATA_AS_PARAM)
      return def;
    return SharedUiItemDataBase<T>::paramRawValue(key, def, context);
  }
  Utf8StringSet paramKeys(
      const PercentEvaluator::EvalContext &context) const override {
    Utf8StringSet keys = _params.lockedData()->paramKeys(context);
    keys |= T::_paramFunctions.keys();
    if constexpr (INCLUDE_UI_DATA_AS_PARAM)
      keys |= SharedUiItemDataBase<T>::paramKeys(context);
    return keys;
  }
};

template<class T, bool INCLUDE_UI_DATA_AS_PARAM = false>
class SharedUiItemDynamicDataWithMutableParams
    : public SharedUiItemDynamicDataWithFunctions<T> {
public:
  mutable AtomicValue<ParamSet> _params;

  SharedUiItemDynamicDataWithMutableParams(
      const ParamSet &params, const Utf8String &scope)
    : _params(params) {
    _params.lockedData()->setScope(scope);
  }
  SharedUiItemDynamicDataWithMutableParams(const ParamSet &params = {})
    : SharedUiItemDynamicDataWithMutableParams(params, {}) { }

  // ParamsProvider interface
  TypedValue paramRawValue(
      const Utf8String &key, const TypedValue &def,
      const PercentEvaluator::EvalContext &context) const override {
    if (!context.hasScopeOrNone(
          SharedUiItemDynamicDataWithFunctions<T>::paramScope()))
      return def;
    int ml;
    auto f = T::_paramFunctions.value(key, &ml);
    if (f)
      return f(this, key, context, ml);
    auto v = _params.lockedData()->paramRawValue(key, {}, context);
    if (!!v)
      return v;
    if constexpr (!INCLUDE_UI_DATA_AS_PARAM)
      return def;
    return SharedUiItemDynamicDataWithFunctions<T>
        ::paramRawValue(key, def, context);
  }
  Utf8StringSet paramKeys(
      const PercentEvaluator::EvalContext &context) const override {
    Utf8StringSet keys = _params.lockedData()->paramKeys(context);
    keys |= T::_paramFunctions.keys();
    if constexpr (INCLUDE_UI_DATA_AS_PARAM)
      keys |= SharedUiItemDynamicDataWithFunctions<T>::paramKeys(context);
    return keys;
  }
};

template<class T, bool INCLUDE_UI_DATA_AS_PARAM = false>
class SharedUiItemDataWithImmutableParams
    : public SharedUiItemDataWithFunctions<T> {
public:
  ParamSet _params;

  SharedUiItemDataWithImmutableParams(
      const ParamSet &params, const Utf8String &scope)
    : _params(params) {
    _params.setScope(scope);
  }
  SharedUiItemDataWithImmutableParams(const ParamSet &params = {})
    : SharedUiItemDataWithImmutableParams(params, T::_qualifier) { }

  // ParamsProvider interface
  TypedValue paramRawValue(
      const Utf8String &key, const TypedValue &def,
      const PercentEvaluator::EvalContext &context) const override {
    if (!context.hasScopeOrNone(SharedUiItemDataWithFunctions<T>::paramScope()))
      return def;
    int ml;
    auto f = T::_paramFunctions.value(key, &ml);
    if (f)
      return f(this, key, context, ml);
    auto v = _params.paramRawValue(key, {}, context);
    if (!!v)
      return v;
    if constexpr (!INCLUDE_UI_DATA_AS_PARAM)
      return def;
    return SharedUiItemDataWithFunctions<T>::paramRawValue(key, def, context);
  }
  Utf8StringSet paramKeys(
      const PercentEvaluator::EvalContext &context) const override {
    Utf8StringSet keys = _params.paramKeys(context);
    keys |= T::_paramFunctions.keys();
    if constexpr (INCLUDE_UI_DATA_AS_PARAM)
      keys += SharedUiItemDataWithFunctions<T>::paramKeys(context);
    return keys;
  }
};

template<class T, bool INCLUDE_UI_DATA_AS_PARAM = false>
class SharedUiItemDynamicDataWithImmutableParams
    : public SharedUiItemDynamicDataWithFunctions<T> {
public:
  ParamSet _params;

  SharedUiItemDynamicDataWithImmutableParams(
      const ParamSet &params, const Utf8String &scope)
    : _params(params) {
    _params.setScope(scope);
  }
  SharedUiItemDynamicDataWithImmutableParams(const ParamSet &params = {})
    : SharedUiItemDynamicDataWithImmutableParams(params, {}) { }

  // ParamsProvider interface
  TypedValue paramRawValue(
      const Utf8String &key, const TypedValue &def,
      const PercentEvaluator::EvalContext &context) const override {
    if (!context.hasScopeOrNone(
          SharedUiItemDynamicDataWithFunctions<T>::paramScope()))
      return def;
    int ml;
    auto f = T::_paramFunctions.value(key, &ml);
    if (f)
      return f(this, key, context, ml);
    auto v = _params.paramRawValue(key, {}, context);
    if (!!v)
      return v;
    if constexpr (!INCLUDE_UI_DATA_AS_PARAM)
      return def;
    return SharedUiItemDynamicDataWithFunctions<T>
        ::paramRawValue(key, def, context);
  }
  Utf8StringSet paramKeys(
      const PercentEvaluator::EvalContext &context) const override {
    Utf8StringSet keys = _params.paramKeys(context);
    keys |= T::_paramFunctions.keys();
    if constexpr (INCLUDE_UI_DATA_AS_PARAM)
      keys += SharedUiItemDynamicDataWithFunctions<T>::paramKeys(context);
    return keys;
  }
};

#endif // TEMPLATEDSHAREDUIITEMDATA_H
