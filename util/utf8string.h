/* Copyright 2023 Gregoire Barbier and others.
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

#ifndef UTF8STRING_H
#define UTF8STRING_H

#include <QByteArray>
#include <QString>

/** Enhanced QByteArray with string methods, always assuming content is a
 * UTF-8 encoded string. */
class Utf8String : public QByteArray {
public:
  Utf8String(const QByteArray &ba = {}) : QByteArray(ba) {}
  Utf8String(const QString &s) : QByteArray(s.toUtf8()) {}
  char value(qsizetype i, char defaultValue = '\0') const {
    return size() < i+1 ? defaultValue : at(0);
  }
  char operator[](qsizetype i) const { return value(i, '\0'); }
};

#endif // UTF8STRING_H
