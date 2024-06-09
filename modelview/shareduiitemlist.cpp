/* Copyright 2015-2024 Hallowyn, Gregoire Barbier and others.
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
#include "shareduiitemlist.h"

QVariant SharedUiItemList::paramRawValue(
    const Utf8String &key, const QVariant &def,
    const EvalContext &context) const {
  int colon = key.indexOf(':');
  Utf8String qualifier = colon >= 0 ? key.left(colon) : Utf8String{};
  Utf8String sectionName = key.mid(colon+1);// works even with colon=-1
  for (auto item : *this) {
    // ignore item if keys contains a qualifier and it does not match item
    // e.g. "employee:name" and current item is qualified as "building"
    if (!qualifier.isEmpty() && item.qualifier() != qualifier)
      continue;
    // ignore item if context contains a scope and it does not match item
    if (!context.hasScopeOrNone(item.paramScope()))
      continue;
    // special section names e.g. "id" or "employee:qualified_id"
    if (sectionName == "id"_u8)
      return item.id();
    if (sectionName == "qualifier"_u8)
      return item.qualifier();
    if (sectionName == "qualified_id"_u8)
      return item.qualifiedId();
    // section by name e.g. "name" or "employee:name"
    auto section = item.uiSectionByName(sectionName);
    // section by number e.g. "0" or "employee:12"
    if (section < 0)
      section = sectionName.toNumber<int>(-1);
    // ignore item for which the section can't be found
    if (section < 0)
      continue;
    QVariant value = item.uiData(section, context.role());
    // ignore item for which no valid data can be found
    if (!value.isValid())
      return value;
  }
  return def;
}

Utf8StringSet SharedUiItemList::paramKeys(
    const EvalContext &) const {
  Utf8StringSet keys, qualifiers;
  for (auto item: *this) {
    auto q = item.qualifier();
    if (qualifiers.contains(q))
      continue;
    qualifiers << q;
    keys << q+":id";
    keys << q+":qualifier";
    keys << q+":qualified_id";
    for (int i = 0; i < item.uiSectionCount(); ++i) {
      keys << q+":"+Utf8String::number(i);
      keys << Utf8String::number(i);
      auto name = item.uiSectionName(i);
      if (name.isEmpty())
        continue;
      keys << q+":"+name;
      keys << name;
    }
  }
  return keys;
}

template <class T, class S>
inline T generic_join(const SharedUiItemList &list, const S &separator,
                      bool qualified) {
  T s;
  bool first = true;
  for (auto item : list) {
    if (first)
      first = false;
    else
      s += separator;
    if (qualified) {
      s += item.qualifier();
      s += ':';
      s += item.id();
    } else
      s += item.id();
  }
  return s;
}

Utf8String SharedUiItemList::join(const QByteArray &separator) const {
  return generic_join<Utf8String>(*this, separator, false);
}

Utf8String SharedUiItemList::join(const char separator) const {
  return generic_join<Utf8String>(*this, separator, false);
}

Utf8String SharedUiItemList::join(const char32_t separator) const {
  return generic_join<Utf8String>(*this, separator, false);
}

QString SharedUiItemList::joinUtf16(const QString &separator) const {
  return generic_join<QString>(*this, separator, false);
}

QString SharedUiItemList::joinUtf16(const QChar separator) const {
  return generic_join<QString>(*this, separator, false);
}

template <class S>
inline Utf8String generic_join(
    const SharedUiItemList &list, const S &separator,
    const Utf8String &format, const Utf8StringSet &qualifiers) {
  Utf8String s;
  bool first = true, filtered = !qualifiers.isEmpty();
  for (auto item : list) {
    if (filtered && !qualifiers.contains(item.qualifier()))
      continue;
    if (first)
      first = false;
    else
      s += separator;
    s += Utf8String{format % item};
  }
  return s;
}

Utf8String SharedUiItemList::join(
    const Utf8String &separator, const Utf8String &format,
    const Utf8StringSet &qualifiers) const {
  return generic_join<>(*this, separator, format, qualifiers);
}

Utf8String SharedUiItemList::join(
    const char separator, const Utf8String &format,
    const Utf8StringSet &qualifiers) const {
  return generic_join<>(*this, separator, format, qualifiers);
}

Utf8String SharedUiItemList::join(
    const char32_t separator, const Utf8String &format,
    const Utf8StringSet &qualifiers) const {
  return generic_join<>(*this, separator, format, qualifiers);
}

QString SharedUiItemList::joinUtf16(
    const QString &separator, const Utf8String &format,
    const Utf8StringSet &qualifiers) const {
  return generic_join<>(*this, separator, format, qualifiers);
}

QString SharedUiItemList::joinUtf16(
    const QChar separator, const Utf8String &format,
    const Utf8StringSet &qualifiers) const {
  return generic_join<>(*this, (char32_t)separator.unicode(), format,
                        qualifiers);
}
