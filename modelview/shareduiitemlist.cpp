/* Copyright 2015-2023 Hallowyn, Gregoire Barbier and others.
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
#include "shareduiitem.h"
#include "util/utf8stringset.h"

const QVariant SharedUiItemListParamsProvider::paramRawValue(
    const Utf8String &key, const QVariant &defaultValue) const {
  int colon = key.indexOf(':');
  Utf8String idQualifier = colon >= 0 ? key.left(colon) : Utf8String{};
  Utf8String sectionName = key.mid(colon+1);// works even with colon=-1
  for (auto item : _list) {
    if (!idQualifier.isEmpty() && item.idQualifier() != idQualifier)
      continue;
    bool ok;
    int section = sectionName.toInt(&ok);
    if (!ok) {
      if (sectionName == "id"_u8)
        return item.id();
      if (sectionName == "id_qualifier"_u8)
        return item.idQualifier();
      if (sectionName == "qualified_id"_u8)
        return item.qualifiedId();
      section = item.uiSectionByName(sectionName);
      ok = section >= 0;
    }
    if (ok) {
      QVariant value = item.uiData(section, _role);
      if (value.isValid())
        return value;
    }
  }
  return defaultValue;
}

const Utf8StringSet SharedUiItemListParamsProvider::paramKeys() const {
  Utf8StringSet keys, qualifiers;
  for (auto item: _list) {
    auto q = item.idQualifier();
    if (qualifiers.contains(q))
      continue;
    qualifiers << q;
    keys << q+":id";
    keys << q+":id_qualifier";
    keys << q+":qualified_id";
    for (int i = 0; i < item.uiSectionCount(); ++i) {
      keys << q+":"+QString::number(i);
      keys << QString::number(i);
      auto name = item.uiSectionName(i);
      if (name.isEmpty())
        continue;
      keys << q+":"+name;
      keys << name;
    }
  }
  return keys;
}

const Utf8String SharedUiItemListParamsProvider::paramScope() const {
  return _scope;
}
