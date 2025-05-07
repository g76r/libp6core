/* Copyright 2015-2025 Hallowyn, Gregoire Barbier and others.
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
  Utf8String _qualifier, _id;
  Utf8StringList _section_names;
  QVariantList _values;

  GenericSharedUiItemData() { }
  GenericSharedUiItemData(
      Utf8String qualifier, Utf8String id, Utf8StringList section_names,
      QVariantList values)
    : _qualifier(qualifier), _id(id), _section_names(section_names),
      _values(values) { }
  GenericSharedUiItemData(Utf8String qualifier, Utf8String id)
    : _qualifier(qualifier), _id(id) { }
  explicit GenericSharedUiItemData(Utf8String qualifiedId) {
    int i = qualifiedId.indexOf(':');
    if (i < 0) {
      _qualifier = "generic"_u8;
      _id = qualifiedId;
    } else {
      _qualifier = qualifiedId.left(i);
      _id = qualifiedId.mid(i+1);
    }
  }
  Utf8String id() const override { return _id; }
  Utf8String qualifier() const override { return _qualifier; }
  int uiSectionCount() const override { return _values.size(); }
  Utf8String uiSectionName(int section) const override {
    return _section_names.value(section);
  }
  int uiSectionByName(Utf8String sectionName) const override {
    auto i = _section_names.indexOf(sectionName);
    if (i >= 0)
      return i;
    if (sectionName.value(0) == '_')
      return sectionName.mid(1).toNumber<int>(-1);
    return -1;
  }
  QVariant uiData(int section, int role) const override {
    if (role == Qt::DisplayRole || role == Qt::EditRole
        || role == SharedUiItem::ExternalDataRole)
      return _values.value(section);
    return QVariant();
  }
  QVariant uiHeaderData(int section, int role) const override {
    return role == Qt::DisplayRole ? uiSectionName(section) : QVariant();
  }
  Qt::ItemFlags uiFlags(int section) const override;
  bool setUiData(
      int section, const QVariant &value, QString *errorString,
      SharedUiItemDocumentTransaction *transaction, int role) override;
};

GenericSharedUiItem::GenericSharedUiItem(
    Utf8String qualifier, Utf8String id, Utf8StringList section_names,
    QVariantList values)
  : SharedUiItem(new GenericSharedUiItemData(qualifier, id, section_names,
                                             values)) {
}

GenericSharedUiItem::GenericSharedUiItem(
    Utf8String qualifier, Utf8String id)
  : SharedUiItem(new GenericSharedUiItemData(qualifier, id)) {
}

GenericSharedUiItem::GenericSharedUiItem(Utf8String qualifiedId)
  : SharedUiItem(new GenericSharedUiItemData(qualifiedId)) {
}

QList<GenericSharedUiItem> GenericSharedUiItem::fromCsv(
    CsvFile *csvFile, int idColumn, Utf8String qualifier) {
  QList<GenericSharedUiItem> list;
  if (!csvFile)
    return list;
  Utf8StringList section_names;
  for (Utf8String header: csvFile->headers())
    section_names.append(header.toIdentifier());
  for (int i = 0; i < csvFile->rowCount(); ++i) {
    Utf8StringList row = csvFile->row(i);
    auto id = row.value(idColumn);
    QVariantList values;
    for (const auto &value: row)
      values.append(value);
    list.append(GenericSharedUiItem(qualifier, id, section_names, values));
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
  if (section >= 0 && (section < _values.size()
                       || section < _section_names.size())) {
    flags |= Qt::ItemIsEditable;
  }
  return flags;
}

bool GenericSharedUiItemData::setUiData(
    int section, const QVariant &value, QString *errorString,
    SharedUiItemDocumentTransaction *transaction, int role) {
  Q_ASSERT(transaction != 0);
  Q_ASSERT(errorString != 0);
  if (section >= 0 && (section < _values.size()
                       || section < _section_names.size())) {
    QString s = value.toString().trimmed();
    while (_values.size() < section+1)
      _values.append(QVariant());
    _values[section] = s;
    return true;
  }
  return SharedUiItemData::setUiData(section, value, errorString, transaction,
                                     role);
}

const GenericSharedUiItemData *GenericSharedUiItem::data() const {
  return specializedData<GenericSharedUiItemData>();
}

GenericSharedUiItemData *GenericSharedUiItem::data() {
  return SharedUiItem::detachedData<GenericSharedUiItemData>();
}
