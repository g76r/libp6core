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

SharedUiItemData::~SharedUiItemData() {
}

QVariant SharedUiItemData::uiData(int section, int role) const {
  Q_UNUSED(section)
  Q_UNUSED(role)
  return QVariant();
}

bool SharedUiItemData::setUiData(
    int section, const QVariant &value, QString *errorString,
    SharedUiItemDocumentTransaction *transaction, int role) {
  Q_UNUSED(role)
  Q_UNUSED(value)
  Q_UNUSED(transaction)
  Q_ASSERT(errorString != 0);
  *errorString = "Field \""+uiHeaderData(section, Qt::DisplayRole).toString()
      +"\" is not ui-editable for item of type "+idQualifier()+"";
  return false;
}

Qt::ItemFlags SharedUiItemData::uiFlags(int section) const {
  Q_UNUSED(section)
  return Qt::ItemIsEnabled;
}

QVariant SharedUiItemData::uiHeaderData(int section, int role) const {
  Q_UNUSED(section)
  Q_UNUSED(role)
  return QVariant();
}

QByteArray SharedUiItemData::id() const {
  return uiData(0, Qt::DisplayRole).toByteArray();
}

int SharedUiItemData::uiSectionCount() const {
  return 0;
}

QByteArray SharedUiItemData::uiSectionName(int section) const {
  QVariant v = uiHeaderData(section, Qt::DisplayRole);
  QByteArray ba = v.canConvert<QByteArray>()
      ? v.toByteArray().toLower() : v.toString().toUtf8().toLower();
  for (char *s = ba.data(); *s; ++s)
    if (!::isalnum(*s))
      *s = '_';
  return ba;
}

QByteArray SharedUiItemData::idQualifier() const {
  return QByteArray();
}

QDebug operator<<(QDebug dbg, const SharedUiItem &i) {
  dbg.nospace() << i.qualifiedId();
  return dbg.space();
}

const QVariant SharedUiItemParamsProvider::paramValue(
  const QString &key, const ParamsProvider *, const QVariant &defaultValue,
  QSet<QString> *) const {
  bool ok;
  int section = key.toInt(&ok);
  if (!ok) {
    if (key == "id")
      return _item.id();
    if (key == "idQualifier")
      return _item.idQualifier();
    if (key == "qualifiedId")
      return _item.qualifiedId();
    section = _item.uiSectionByName(key.toUtf8());
    ok = section >= 0;
  }
  if (ok) {
    QVariant value = _item.uiData(section, _role);
    if (value.isValid())
      return value;
  }
  return defaultValue;
}

const QSet<QString> SharedUiItemParamsProvider::keys() const {
  QSet<QString> keys { "id", "idQualifier", "qualifiedId" };
  int count = _item.uiSectionCount();
  for (int section = 0; section < count; ++section) {
    keys << QString::number(section);
    auto name = _item.uiSectionName(section);
    if (name.isEmpty())
      continue;
    keys << name;
  }
  return keys;
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
    const QSet<QByteArray> &ignoredSections, int role) {
  int n = uiSectionCount();
  for (int i = 0; i < n; ++i) {
    auto name = uiSectionName(i);
    if (hash.contains(name) && !ignoredSections.contains(name))
      if (!setUiData(i, hash.value(name), errorString, transaction, role))
        return false;
  }
  return true;
}
