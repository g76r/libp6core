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
#include "shareduiitemslogmodel.h"
#include "modelview/shareduiitem.h"

static QAtomicInt _sequence;

class LIBP6CORESHARED_EXPORT SharedUiItemLogWrapperData
    : public SharedUiItemData {
public:
  QByteArray _id;
  SharedUiItem _wrapped;
  QDateTime _timestamp;
  int _timestampSection;

  SharedUiItemLogWrapperData(SharedUiItem wrapped, QDateTime timestamp)
    : _id(QByteArray::number(_sequence.fetchAndAddOrdered(1))),
      _wrapped(wrapped), _timestamp(timestamp),
      _timestampSection(wrapped.uiSectionCount()) { }
  SharedUiItemLogWrapperData() : _timestampSection(0) { }
  Utf8String id() const override { return _id; }
  Utf8String idQualifier() const override { return "suilogwrapper"_u8; }
  int uiSectionCount() const override { return _timestampSection+1; }
  Utf8String uiSectionName(int section) const override {
    return section == _timestampSection
        ? "timestamp"_u8 : _wrapped.uiSectionName(section);
  }
  int uiSectionByName(Utf8String sectionName) const override {
    return sectionName == "timestamp"_u8
        ? _timestampSection : _wrapped.uiSectionByName(sectionName);
  }
  QVariant uiData(int section, int role) const override {
    return (role == Qt::DisplayRole && section == _timestampSection)
        ? _timestamp : _wrapped.uiData(section, role); }
  QVariant uiHeaderData(int section, int role) const override {
    return (role == Qt::DisplayRole && section == _timestampSection)
        ? "Timestamp"_u8 : _wrapped.uiHeaderData(section, role); }
  Qt::ItemFlags uiFlags(int section) const override {
  return (section == _timestampSection)
      ? Qt::ItemIsSelectable|Qt::ItemIsEnabled : _wrapped.uiFlags(section); }
  // LATER also wrap setUiData, or warn it is not wrapped
  //bool setUiData(int section, const QVariant &value, QString *errorString,
  //               int role, const SharedUiItemDocumentManager *dm) {
  //  return _wrapped.setUiData(section, value, errorString, role, dm); }
  };

class LIBP6CORESHARED_EXPORT SharedUiItemLogWrapper : public SharedUiItem {
public:
  SharedUiItemLogWrapper(SharedUiItem wrapped)
    : SharedUiItem(new SharedUiItemLogWrapperData(
                     wrapped, QDateTime::currentDateTime())) { }
};

Q_DECLARE_TYPEINFO(SharedUiItemLogWrapper, Q_MOVABLE_TYPE);

SharedUiItemsLogModel::SharedUiItemsLogModel(QObject *parent, int maxrows)
  : SharedUiItemsTableModel(parent), _timestampColumn(0) {
  setDefaultInsertionPoint(FirstItem);
  setMaxrows(maxrows);
}

void SharedUiItemsLogModel::setHeaderDataFromTemplate(
    SharedUiItem templateItem, int role) {
  _timestampColumn = templateItem.uiSectionCount();
  SharedUiItemsTableModel::setHeaderDataFromTemplate(
        SharedUiItemLogWrapper(templateItem), role);
}

void SharedUiItemsLogModel::logItem(SharedUiItem newItem) {
  if (!newItem.isNull())
    SharedUiItemsTableModel::changeItem(
          SharedUiItemLogWrapper(newItem), SharedUiItem(),
          newItem.idQualifier());
}
