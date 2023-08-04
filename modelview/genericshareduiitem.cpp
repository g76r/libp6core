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
#include "genericshareduiitem.h"
#include "shareduiitemdocumentmanager.h"
#include "shareduiitemdocumenttransaction.h"
#include "csv/csvfile.h"

class GenericSharedUiItemData : public SharedUiItemData {
public:
  Utf8String _idQualifier, _id;
  QVariantList _headers, _values;

  GenericSharedUiItemData() { }
  GenericSharedUiItemData(
      Utf8String idQualifier, Utf8String id, QVariantList headers,
      QVariantList values)
    : _idQualifier(idQualifier), _id(id), _headers(headers), _values(values) { }
  GenericSharedUiItemData(Utf8String idQualifier, Utf8String id)
    : _idQualifier(idQualifier), _id(id) { }
  explicit GenericSharedUiItemData(Utf8String qualifiedId) {
    int i = qualifiedId.indexOf(':');
    if (i < 0) {
      _idQualifier = "generic"_u8;
      _id = qualifiedId;
    } else {
      _idQualifier = qualifiedId.left(i);
      _id = qualifiedId.mid(i+1);
    }
  }
//  [[deprecated("Use Utf8String API instead of QString")]]
//  GenericSharedUiItemData(
//      QString idQualifier, QString id, QVariantList headers,
//      QVariantList values)
//    : GenericSharedUiItemData(idQualifier.toUtf8(), id.toUtf8(), headers,
//                              values) { }
//  [[deprecated("Use Utf8String API instead of QString")]]
//  GenericSharedUiItemData(QString idQualifier, QString id)
//    : GenericSharedUiItemData(idQualifier.toUtf8(), id.toUtf8()) { }
//  [[deprecated("Use Utf8String API instead of QString")]]
//  explicit GenericSharedUiItemData(QString qualifiedId)
//    : GenericSharedUiItemData(qualifiedId.toUtf8()) { }
  Utf8String id() const override { return _id; }
  Utf8String idQualifier() const override { return _idQualifier; }
  int uiSectionCount() const override {
    return qMax(_headers.size(), _values.size()); }
  Utf8String uiSectionName(int section) const override {
    return Utf8String(_headers.value(section)); // FIXME
  }
  int uiSectionByName(Utf8String sectionName) const override {
    return _headers.indexOf(sectionName); // FIXME
  }
  QVariant uiData(int section, int role) const override {
    if (role == Qt::DisplayRole || role == Qt::EditRole
        || role == SharedUiItem::ExternalDataRole)
      return _values.value(section);
    return QVariant();
  }
  QVariant uiHeaderData(int section, int role) const override {
    return role == Qt::DisplayRole ? _headers.value(section) : QVariant();
  }
  Qt::ItemFlags uiFlags(int section) const override;
  bool setUiData(
      int section, const QVariant &value, QString *errorString,
      SharedUiItemDocumentTransaction *transaction, int role) override;
};


GenericSharedUiItem::GenericSharedUiItem() {
}

GenericSharedUiItem::GenericSharedUiItem(const GenericSharedUiItem &other)
  : SharedUiItem(other) {
}

GenericSharedUiItem::GenericSharedUiItem(
    Utf8String idQualifier, Utf8String id, QVariantList headers,
    QVariantList values)
  : SharedUiItem(new GenericSharedUiItemData(idQualifier, id, headers,
                                             values)) {
}

GenericSharedUiItem::GenericSharedUiItem(
    Utf8String idQualifier, Utf8String id)
  : SharedUiItem(new GenericSharedUiItemData(idQualifier, id)) {
}

GenericSharedUiItem::GenericSharedUiItem(Utf8String qualifiedId)
  : SharedUiItem(new GenericSharedUiItemData(qualifiedId)) {
}


QList<GenericSharedUiItem> GenericSharedUiItem::fromCsv(
    CsvFile *csvFile, int idColumn, Utf8String idQualifier) {
  QList<GenericSharedUiItem> list;
  if (!csvFile)
    return list;
  QVariantList vHeaders;
  for (auto header: csvFile->headers())
    vHeaders.append(header);
  for (int i = 0; i < csvFile->rowCount(); ++i) {
    QStringList values = csvFile->row(i);
    QString id;
    if (values.size() > idColumn)
      id = values[idColumn];
    QVariantList vValues;
    for (auto value: values)
      vValues.append(value);
    list.append(GenericSharedUiItem(
                  idQualifier, id.toUtf8(), vHeaders, vValues));
  }
  return list;
}

bool GenericSharedUiItem::setUiDataWithIdSection(
    int section, const QVariant &value, QString *errorString,
    SharedUiItemDocumentTransaction *transaction, int role, int idSection) {
  auto *d = data();
  if (!d)
    setData(d = new GenericSharedUiItemData());
  bool success = d->setUiData(section, value, errorString, transaction, role);
  if (success && section == idSection)
    d->_id = d->_values.value(section).toString().toUtf8();
  return success;
}

Qt::ItemFlags GenericSharedUiItemData::uiFlags(int section) const {
  Qt::ItemFlags flags = SharedUiItemData::uiFlags(section);
  if (section >= 0 && (section < _values.size() || section < _headers.size())) {
    flags |= Qt::ItemIsEditable;
  }
  return flags;
}

bool GenericSharedUiItemData::setUiData(
    int section, const QVariant &value, QString *errorString,
    SharedUiItemDocumentTransaction *transaction, int role) {
  Q_ASSERT(transaction != 0);
  Q_ASSERT(errorString != 0);
  if (section >= 0 && (section < _values.size() || section < _headers.size())) {
    QString s = value.toString().trimmed();
    while (_values.size() < section+1)
      _values.append(QVariant());
    _values[section] = s;
    return true;
  }
  return SharedUiItemData::setUiData(section, value, errorString, transaction,
                                     role);
}

GenericSharedUiItemData *GenericSharedUiItem::data() {
  return SharedUiItem::detachedData<GenericSharedUiItemData>();
}
