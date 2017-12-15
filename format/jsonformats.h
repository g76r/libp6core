/* Copyright 2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef JSONFORMATS_H
#define JSONFORMATS_H

#include "libp6core_global.h"
#include <QStringList>
#include <QHash>
#include <QMap>

/** Utilites to handle JSON formats. */
class LIBPUMPKINSHARED_EXPORT JsonFormats {
  JsonFormats() = delete;

public:
  /** convert a QHash<QString,QString> to a JSON object formatted string */
  static QString hash2string(const QHash<QString,QString> &hash);
  /** convert a JSON object formatted string to a QHash<QString,QString> */
  static QHash<QString,QString> string2hash(const QString &string);
  /** convert a QMap<QString,QString> to a JSON object formatted string */
  static QString map2string(const QMap<QString,QString> &hash);
  /** convert a JSON object formatted string to a QMap<QString,QString> */
  static QMap<QString,QString> string2map(const QString &string);
  /** convert a QList<QString> to a JSON array formatted string */
  static QString list2string(const QList<QString> &list);
  /** convert a JSON array formatted string to a QList<QString> */
  static QStringList string2list(const QString &string);
};

#endif // JSONFORMATS_H