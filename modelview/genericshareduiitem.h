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
#ifndef GENERICSHAREDUIITEM_H
#define GENERICSHAREDUIITEM_H

#include "shareduiitem.h"

class GenericSharedUiItemData;
class CsvFile;

/** Util class to build a SharedUiItem from arbitrary ids and QVariant values.
 */
class LIBP6CORESHARED_EXPORT GenericSharedUiItem : public SharedUiItem {
public:
  GenericSharedUiItem();
  GenericSharedUiItem(const GenericSharedUiItem &other);
  GenericSharedUiItem(
      QByteArray idQualifier, QByteArray id, QVariantList headers,
      QVariantList values);
  /** Convenience constructor, with idQualifier="generic" */
  GenericSharedUiItem(QByteArray id, QVariantList headers, QVariantList values)
    : GenericSharedUiItem("generic"_ba, id, headers, values) { }
  /** Convenience constructor, with idQualifier="generic" and id=values[0] */
  GenericSharedUiItem(QVariantList headers, QVariantList values)
    : GenericSharedUiItem(
        "generic"_ba, values.size() > 0 ? values[0].toByteArray() :QByteArray(),
        headers, values) { }
  /** Create empty item, without data or headers. */
  GenericSharedUiItem(QByteArray idQualifier, QByteArray id);
  /** Create empty item, without data or headers, by parsing qualifiedId. */
  explicit GenericSharedUiItem(QByteArray qualifiedId);
  [[deprecated("Use QByteArray API instead of QString")]]
  GenericSharedUiItem(QString idQualifier, QString id, QVariantList headers,
                      QVariantList values)
    : GenericSharedUiItem(idQualifier.toUtf8(), id.toUtf8(), headers, values) {}
  [[deprecated("Use QByteArray API instead of QString")]]
  GenericSharedUiItem(QString id, QVariantList headers, QVariantList values)
    : GenericSharedUiItem("generic"_ba, id.toUtf8(), headers, values) { }
  [[deprecated("Use QByteArray API instead of QString")]]
  GenericSharedUiItem(QString idQualifier, QString id)
  : GenericSharedUiItem(idQualifier.toUtf8(), id.toUtf8()) { }
  [[deprecated("Use QByteArray API instead of QString")]]
  explicit GenericSharedUiItem(QString qualifiedId)
    : GenericSharedUiItem(qualifiedId.toUtf8()) { }
  GenericSharedUiItem &operator=(const GenericSharedUiItem &other) {
    SharedUiItem::operator=(other); return *this; }
  static QList<GenericSharedUiItem> fromCsv(CsvFile *csvFile, int idColumn = 0,
      QByteArray idQualifier = "generic"_ba);
  // LATER another fromCsv(), with idQualifierColumn
  /** Not only set ui data but also update id if updated section is id section.
   */
  template <int idSection>
  bool setUiData(int section, const QVariant &value, QString *errorString,
                 SharedUiItemDocumentTransaction *transaction,
                 int role = Qt::EditRole) {
    return setUiDataWithIdSection(section, value, errorString, transaction,
                                  role, idSection);
  }

private:
  bool setUiDataWithIdSection(
      int section, const QVariant &value, QString *errorString,
      SharedUiItemDocumentTransaction *transaction, int role, int idSection);
  const GenericSharedUiItemData *data() const {
    return specializedData<GenericSharedUiItemData>(); }
  GenericSharedUiItemData *data();
};

Q_DECLARE_TYPEINFO(GenericSharedUiItem, Q_MOVABLE_TYPE);

#endif // GENERICSHAREDUIITEM_H
