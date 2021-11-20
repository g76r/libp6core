/* Copyright 2017-2021 Hallowyn, Gregoire Barbier and others.
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
#include <QJsonObject>

/** Utilites to handle JSON formats. */
class LIBP6CORESHARED_EXPORT JsonFormats {
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
  /** insert value in a descendant object using a dot separated path
   *  e.g. recursive_insert(o, "foo.bar", "a") sets bar=a in foo child
   *  actually: creates foo if neeeded then calls QJsonObject::insert("bar","a")
   *  on it
   *  if path is empty (or only contains dots), do nothing */
  static void recursive_insert(
      QJsonObject &target, QString path, const QJsonValue &value) {
    recursive_insert(target, path.split('.', Qt::SkipEmptyParts), value); }
  static void recursive_insert(
      QJsonObject &target, QStringList path, const QJsonValue &value);
};

#endif // JSONFORMATS_H
