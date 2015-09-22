/* Copyright 2015 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
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
#include "characterseparatedexpression.h"

CharacterSeparatedExpression::CharacterSeparatedExpression(
    QString inputWithLeadingSeparator, int offset) {
  parse(inputWithLeadingSeparator, offset);
}

CharacterSeparatedExpression::CharacterSeparatedExpression(
    QChar separator, QString inputWithoutLeadingSeparator, int offset) {
  parse(separator, inputWithoutLeadingSeparator, offset);
}

void CharacterSeparatedExpression::parse(
    QString inputWithLeadingSeparator, int offset) {
  QStringList::clear();
  if (inputWithLeadingSeparator.size() > offset) {
    _separator = inputWithLeadingSeparator[offset+0];
    inner_parse(inputWithLeadingSeparator, offset+1);
  } else {
    _separator = 0;
  }
}

void CharacterSeparatedExpression::parse(
    QChar separator, QString inputWithoutLeadingSeparator, int offset) {
  QStringList::clear();
  _separator = separator;
  inner_parse(inputWithoutLeadingSeparator, offset+0);
}

void CharacterSeparatedExpression::inner_parse(QString input, int pos) {
  int l = input.length();
  if (pos >= l)
    return;
  QString field;
  for (; pos < l; ++pos) {
    QChar c = input[pos];
    if (c == _separator) {
      append(field);
      field.clear();
    } else {
      field += c;
    }
  }
  append(field);
}
