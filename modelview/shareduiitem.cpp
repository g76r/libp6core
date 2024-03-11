/* Copyright 2014-2024 Hallowyn, Gregoire Barbier and others.
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
#include "shareduiitem.h"
#include "util/paramset.h"

Utf8String SharedUiItemData::id() const {
  return Utf8String(uiData(0, Qt::DisplayRole));
}

Utf8String SharedUiItemData::qualifier() const {
  return {};
}

int SharedUiItemData::uiSectionCount() const {
  return {};
}

QVariant SharedUiItemData::uiData(int, int) const {
  return {};
}

QVariant SharedUiItemData::uiHeaderData(int section, int role) const {
  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case SharedUiItem::ExternalDataRole:
      return uiSectionName(section);
  }
  return {};
}

Utf8String SharedUiItemData::uiSectionName(int) const {
  return {};
}

int SharedUiItemData::uiSectionByName(Utf8String) const {
  return -1;
}

Qt::ItemFlags SharedUiItemData::uiFlags(int) const {
  return Qt::ItemIsEnabled;
}

bool SharedUiItemData::setUiData(
    int section, const QVariant &value, QString *errorString,
    SharedUiItemDocumentTransaction *transaction, int role) {
  Q_UNUSED(role)
  Q_UNUSED(value)
  Q_UNUSED(transaction)
  Q_ASSERT(errorString != 0);
  *errorString =
      QObject::tr("Field \"%1\" is not ui-editable for item of type %2")
      .arg(uiHeaderData(section, Qt::DisplayRole) | Utf8String::number(section))
      .arg(qualifier());
  return false;
}

QDebug operator<<(QDebug dbg, const SharedUiItem &i) {
  dbg.nospace() << i.qualifiedId();
  return dbg.space();
}

QVariant SharedUiItemData::paramRawValue(
    const Utf8String &key, const QVariant &def,
    const EvalContext &context) const {
  if (!context.hasScopeOrNone(paramScope()))
    return def;
  auto section = uiSectionByName(key);
  if (section < 0)
    section = key.toNumber<int>(-1);
  if (section < 0)
    return def;
  auto value = uiData(section, context.role());
  if (!value.isValid())
    return def;
  return value;
}

QVariant SharedUiItem::paramRawValue(
    const Utf8String &key, const QVariant &def,
    const EvalContext &context) const {
  if (!_data)
    return {};
  return _data->paramRawValue(key, def, context);
}

Utf8StringSet SharedUiItemData::paramKeys(
    const EvalContext &) const {
  Utf8StringSet keys { "id"_u8, "qualifier"_u8, "qualified_id"_u8 };
  int count = uiSectionCount();
  for (int section = 0; section < count; ++section) {
    keys << Utf8String::number(section);
    auto name = uiSectionName(section);
    if (name.isEmpty())
      continue;
    keys << name;
  }
  return keys;
}

Utf8StringSet SharedUiItem::paramKeys(const EvalContext &context) const {
  if (!_data)
    return {};
  return _data->paramKeys(context);
}

bool SharedUiItem::paramContains(
    const Utf8String &key, const EvalContext &context) const {
  if (!_data)
    return {};
  return _data->paramContains(key, context);
}

Utf8String SharedUiItemData::paramScope() const {
  return qualifier();
}

Utf8String SharedUiItem::paramScope() const {
  if (!_data)
    return {};
  return _data->paramScope();
}

#if __cpp_impl_three_way_comparison >= 201711
std::strong_ordering SharedUiItemData::operator<=>(
    const SharedUiItemData &that) const {
  if (auto cmp = qualifier() <=> that.qualifier(); cmp != 0)
    return cmp;
  return id() <=> that.id();
}
#else
bool SharedUiItemData::operator<(const SharedUiItemData &that) const {
  return qualifier() < that.qualifier() || id() < that.id();
}
#endif // C++ 20: spaceship op

QVariantHash SharedUiItemData::toVariantHash(int role) const {
  QVariantHash hash;
  int n = uiSectionCount();
  for (int i = 0; i < n; ++i)
    hash.insert(uiSectionName(i), uiData(i, role));
  return hash;
}

bool SharedUiItemData::setFromVariantHash(
    const QVariantHash &hash, QString *errorString,
    SharedUiItemDocumentTransaction *transaction,
    const QSet<Utf8String> &ignoredSections, int role) {
  int n = uiSectionCount();
  for (int i = 0; i < n; ++i) {
    auto name = uiSectionName(i);
    if (hash.contains(name) && !ignoredSections.contains(name))
      if (!setUiData(i, hash.value(name), errorString, transaction, role))
        return false;
  }
  return true;
}
