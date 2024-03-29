/* Copyright 2015-2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef CHARACTERSEPARATEDEXPRESSION_H
#define CHARACTERSEPARATEDEXPRESSION_H

#include "libp6core_global.h"
#include "util/utf8stringlist.h"

/** Provide multi-field parsing using an arbitrary separator character.
 *
 * Works in two different modes:
 * - CSV-like with a given known separator:
 *      CharacterSeparatedExpression(',', "a,b,c")
 *      CharacterSeparatedExpression(';', "a;b;c")
 * - sed's s command-like with a leading input-defined separator:
 *      CharacterSeparatedExpression("/foo/bar/g")
 *      CharacterSeparatedExpression(",/,.,g")
 */
class LIBP6CORESHARED_EXPORT CharacterSeparatedExpression
    : public QStringList {
  QChar _separator;

public:
  CharacterSeparatedExpression() : _separator(0) { }
  CharacterSeparatedExpression(const CharacterSeparatedExpression &other)
    : QStringList(other), _separator(other._separator) { }
  CharacterSeparatedExpression(const QStringList &other)
    : QStringList(other), _separator(0) { }
  CharacterSeparatedExpression(QString inputWithLeadingSeparator,
                               int offset = 0);
  CharacterSeparatedExpression(
      QChar separator, QString inputWithoutLeadingSeparator, int offset = 0);
  void parse(QString inputWithLeadingSeparator, int offset = 0);
  void parse(QChar separator, QString inputWithoutLeadingSeparator,
             int offset = 0);
  inline void clear() { QStringList::clear(); _separator = QChar(0); }
  inline QChar separator() const { return _separator; }

private:
  inline void inner_parse(QString input, int pos);
};

#endif // CHARACTERSEPARATEDEXPRESSION_H
