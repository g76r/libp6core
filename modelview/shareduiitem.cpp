/* Copyright 2014-2023 Hallowyn, Gregoire Barbier and others.
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
#include <QtDebug>
#include <QRegularExpression>
#include "util/paramset.h"

SharedUiItemData::~SharedUiItemData() {
}

Utf8String SharedUiItemData::id() const {
  return Utf8String(uiData(0, Qt::DisplayRole));
}

Utf8String SharedUiItemData::idQualifier() const {
  return {};
}

int SharedUiItemData::uiSectionCount() const {
  return {};
}

QVariant SharedUiItemData::uiData(int, int) const {
  return {};
}

QVariant SharedUiItemData::uiHeaderData(int section, int) const {
  return uiSectionName(section);
}

Utf8String SharedUiItemData::uiSectionName(int) const {
  return {};
}

int SharedUiItemData::uiSectionByName(Utf8String) const {
  return {};
}

Qt::ItemFlags SharedUiItemData::uiFlags(int section) const {
  Q_UNUSED(section)
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
      .arg(uiHeaderData(section, Qt::DisplayRole).toString())
      .arg(idQualifier());
  return false;
}

QDebug operator<<(QDebug dbg, const SharedUiItem &i) {
  dbg.nospace() << i.qualifiedId();
  return dbg.space();
}

const QVariant SharedUiItem::paramValue(
    const Utf8String &key, const ParamsProvider *, const QVariant &defaultValue,
    Utf8StringSet *) const {
  bool ok;
  int section = key.toInt(&ok);
  QVariant value = ok ? uiData(section) : uiDataBySectionName(key);
  return value.isValid() ? value : defaultValue;
}

const QVariant SharedUiItemParamsProvider::paramValue(
    const Utf8String &key, const ParamsProvider *, const QVariant &defaultValue,
    Utf8StringSet *) const {
  bool ok;
  int section = key.toInt(&ok);
  QVariant value = ok ? _item.uiData(section, _role)
                      : _item.uiDataBySectionName(key, _role);
  return value.isValid() ? value : defaultValue;
}

const Utf8StringSet SharedUiItem::keys() const {
  Utf8StringSet keys { "id", "idQualifier", "qualifiedId" };
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

const Utf8String SharedUiItem::paramScope() const {
  return qualifiedId();
}

const ParamSet SharedUiItem::snapshot() const {
  return ParamsProvider::snapshot();
}

const Utf8StringSet SharedUiItemParamsProvider::keys() const {
  return _item.keys();
}


bool SharedUiItemData::operator<(const SharedUiItemData &other) const {
  return idQualifier() < other.idQualifier() || id() < other.id();
}

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

SharedUiItem::~SharedUiItem() {
}
