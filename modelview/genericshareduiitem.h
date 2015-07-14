/* Copyright 2015 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
 * Libqtssu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libqtssu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef GENERICSHAREDUIITEM_H
#define GENERICSHAREDUIITEM_H

#include "shareduiitem.h"
#include "csv/csvfile.h"

/** Util class to build a SharedUiItem from arbitrary ids and QVariant values.
 */
class LIBQTSSUSHARED_EXPORT GenericSharedUiItem : public SharedUiItem {
public:
  GenericSharedUiItem();
  GenericSharedUiItem(const GenericSharedUiItem &other);
  GenericSharedUiItem(QString idQualifier, QString id, QVariantList headers,
                      QVariantList values);
  /** Convenience constructor, with idQualifier="generic" */
  GenericSharedUiItem(QString id, QVariantList headers, QVariantList values)
    : GenericSharedUiItem(QStringLiteral("generic"), id, headers, values) { }
  /** Convenience constructor, with idQualifier="generic" and id=values[0] */
  GenericSharedUiItem(QVariantList headers, QVariantList values)
    : GenericSharedUiItem(QStringLiteral("generic"),
                          values.size() > 0 ? values[0].toString() : QString(),
                          headers, values) { }
  GenericSharedUiItem &operator=(const GenericSharedUiItem &other) {
    SharedUiItem::operator=(other); return *this; }
  static QList<GenericSharedUiItem> fromCsv(
      CsvFile *csvFile, int idColumn = 0,
      QString idQualifier = QStringLiteral("generic"));
  // LATER another fromCsv(), with idQualifierColumn
  // LATER make it editable
};

#endif // GENERICSHAREDUIITEM_H
