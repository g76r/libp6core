/* Copyright 2023 Gregoire Barbier and others.
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

using SharedUiItemDataFunction = std::function<
QVariant(const SharedUiItemData *data, const Utf8String &key,
const PercentEvaluator::EvalContext context, int matched_length)>;

using SharedUiItemDataFunctions = RadixTree<SharedUiItemDataFunction>;

/** Helper class template to implement SharedUiItemData.
 *
 *  Actual SharedUiItemData implementation, e.g. FooData, can inherit from
 *  SharedUiItemDataBase<FooData> and won't have to reimplement some part of
 *  the boiler plate provided it have the folowing static members defined:
 *  - const static Utf8String _idQualifier e.g. = "foo"
 *  - const static Utf8StringList _sectionNames e.g. = { "id", "parent", "name"}
 *  - const static Utf8StringList _headerNames e.g. = { "Id", "Parent", "Name"}
 *    (_headerNames can be = to _sectionNames if its convenient for you)
 *
 *  Some more specialized templates with more features exist, to use
 *  SharedUiItemDataWithFunction<FooData> you also need
 *  - const static SharedUiItemDataFunctions _paramFunctions
 *
 *  When using SharedUiItemDataWithImmutableParams or
 *  SharedUiItemDataWithMutableParams you'll have a _params non-static member
 *  (you don't have to define it it will be inherited from the template).
 *
 *  @see SharedUiItemDataWithFunction
 *  @see SharedUiItemDataWithImmutableParams
 *  @see SharedUiItemDataWithMutableParams
 *  @see SharedUiItemData
 *  @see SharedUiItem
 */
template<class T>
class SharedUiItemDataBase : public SharedUiItemData {
public:
  static const QMap<Utf8String,int> _sectionIndex;

  // SharedUiItemData interface
  virtual Utf8String idQualifier() const override { return T::_idQualifier; }
  int uiSectionCount() const override { return T::_sectionNames.size(); }
  Utf8String uiSectionName(int section) const override {
    return T::_sectionNames.value(section); }
  int uiSectionByName(Utf8String sectionName) const override {
    return _sectionIndex.value(sectionName, -1); }
  QVariant uiHeaderData(int section, int role) const override {
    switch (role) {
      case Qt::DisplayRole:
      case Qt::EditRole:
      case SharedUiItem::ExternalDataRole:
        return T::_headerNames.value(section);
    }
    return {};
  }

  // ParamsProvider interface
  const Utf8String paramScope() const override { return T::_idQualifier; }
};

template<class T>
const QMap<Utf8String,int> SharedUiItemDataBase<T>::_sectionIndex =
    ContainerUtils::index(T::_sectionNames);

template<class T>
class SharedUiItemDataWithFunctions : public SharedUiItemDataBase<T> {
public:
  // ParamsProvider interface
  const QVariant paramRawValue(
      const Utf8String &key, const QVariant &def,
      const PercentEvaluator::EvalContext &context) const override {
    int ml;
    auto f = T::_paramFunctions.value(key, &ml);
    if (f)
      return f(this, key, context, ml);
    return SharedUiItemDataBase<T>::paramRawValue(key, def, context);
  }
};

template<class T, bool _includeUiDataAsParam = false>
class SharedUiItemDataWithMutableParams
    : public SharedUiItemDataWithFunctions<T> {
public:
  mutable AtomicValue<ParamSet> _params;

  SharedUiItemDataWithMutableParams(const ParamSet &params = {})
    : _params(params) {}
  // ParamsProvider interface
  const QVariant paramRawValue(
      const Utf8String &key, const QVariant &def,
      const PercentEvaluator::EvalContext &context) const override {
    int ml;
    auto f = T::_paramFunctions.value(key, &ml);
    if (f)
      return f(this, key, context, ml);
    QVariant v = _params.lockedData()->paramRawValue(key, {}, context);
    if (v.isValid())
      return v;
    return SharedUiItemDataBase<T>::paramRawValue(key, def, context);
  }
  const Utf8StringSet paramKeys(
      const PercentEvaluator::EvalContext &context) const override {
    Utf8StringSet keys = _params.lockedData()->paramKeys(context);
    if (_includeUiDataAsParam)
      keys += SharedUiItemDataBase<T>::paramKeys(context);
    return keys;
  }
};

template<class T, bool _includeUiDataAsParam = false>
class SharedUiItemDataWithImmutableParams
    : public SharedUiItemDataWithFunctions<T> {
public:
  ParamSet _params;

  SharedUiItemDataWithImmutableParams(const ParamSet &params = {})
    : _params(params) {}
  // ParamsProvider interface
  const QVariant paramRawValue(
      const Utf8String &key, const QVariant &def,
      const PercentEvaluator::EvalContext &context) const override {
    int ml;
    auto f = T::_paramFunctions.value(key, &ml);
    if (f)
      return f(this, key, context, ml);
    QVariant v = _params.paramRawValue(key, {}, context);
    if (v.isValid())
      return v;
    return SharedUiItemDataBase<T>::paramRawValue(key, def, context);
  }
  const Utf8StringSet paramKeys(
      const PercentEvaluator::EvalContext &context) const override {
    Utf8StringSet keys = _params.paramKeys(context);
    if (_includeUiDataAsParam)
      keys += SharedUiItemDataBase<T>::paramKeys(context);
    return keys;
  }
};

#endif // TEMPLATEDSHAREDUIITEMDATA_H
