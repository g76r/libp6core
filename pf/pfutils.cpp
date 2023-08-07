/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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

#include "pfutils.h"
#include "pfinternals_p.h"
#include <QRegularExpression>

using namespace Qt::Literals::StringLiterals;

static QRegularExpression _whitespace { "\\s+" };

QString PfUtils::escape(const QString &string, const PfOptions &options,
                        bool escapeEvenSingleSpaces) {
  QString s;
  qsizetype imax = string.size()-1;
  PfPreferedCharactersProtection protection
      = options.preferedCharactersProtection();
  bool protectionUsed = false;
  for (int i = 0; i <= imax; ++i) {
    QChar c = string.at(i);
    if (pfisspecial(c.toLatin1())
        && (escapeEvenSingleSpaces || c != ' ' || i == 0 || i == imax
            || string.at(i+1) == ' ')) {
      switch (protection) {
      case PfBackslashProtection:
        s.append(PF_ESCAPE);
        s.append(c);
        break;
      case PfDoubleQuoteProtection:
        if (c == '\\')
          s.append("\\\\");
        else if (c == '"')
          s.append("\\\"");
        else
          s.append(c);
        protectionUsed = true;
        break;
      case PfSimpleQuoteProtection:
        if (c == '\'')
          s.append("'\\''");
        else
          s.append(c);
        protectionUsed = true;
        break;
      }
    } else {
      s.append(c);
    }
  }
  switch (protection) {
  case PfBackslashProtection:
    return s;
  case PfDoubleQuoteProtection:
    return protectionUsed ? "\""+s+"\"" : s;
  case PfSimpleQuoteProtection:
    return protectionUsed ? "'"+s+"'" : s;
  }
  return s; // should never happen
}

const QStringList PfUtils::stringSplittedOnFirstWhitespace(QStringView v) {
  v = v.trimmed();
  if (v.isEmpty())
    return { };
  int i = v.indexOf(_whitespace);
  if (i <= 0)
    return { v.toString() };
  return { v.left(i).toString(), v.mid(i+1).toString() };
}
