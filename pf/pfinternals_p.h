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

#ifndef PFINTERNALS_P_H
#define PFINTERNALS_P_H

/* This file contain methods and defines that are only for internal use
 * of PF implementation and MUST not be used (or even #include'd) by
 * user code.
 */

#include "pfutils.h"
#include <QObject>
#include <QString>
#include <QChar>

#define PF_SPACES " \t\n\r"
#define PF_RESERVED "$~[]{}"
#define PF_SEPARATORS "()|#;"
#define PF_QUOTES "'\""
#define PF_ESCAPE "\\"
#define PF_XML_SPECIAL_CHARS "<>#&["

inline bool pfin(const char c, const char *s) {
  while (*s)
    if (c == *(s++))
      return true;
  return false;
}

inline bool pfisspace(char c) { return pfin(c, PF_SPACES); }

inline bool pfisquote(char c) { return pfin(c, PF_QUOTES); }

inline bool pfisspecial(char c) {
  return pfin(c, PF_SPACES PF_RESERVED PF_SEPARATORS PF_QUOTES PF_ESCAPE);
}

inline bool pfisendofname(char c) {
  return pfin(c, PF_SPACES PF_SEPARATORS);
}

inline QString tr(const char *s) { return QObject::tr(s); }

/** Return a C-style quoted char if char is a special char, e.g.
  * 97 (a)                          ->      a
  * 92 (\)                          ->      \\
  * 233 (é in ISO 8859-1)           ->      \xe9
  * 10 (a.k.a. \n or end of line)   ->      \x0a
  */
inline QString pfquotechar(unsigned char c) {
  if (c == '\\')
    return "\\\\";
  if (c > 32 && c < 128)
    return QString(QChar(c));
  return QString("\\x").append("0123456789abcdef"[(c&0xf0)>>4])
      .append("0123456789abcdef"[c&0xf]);
}

inline QString pfquotechar(signed char c) {
  return pfquotechar(static_cast<unsigned char>(c));
}

inline QString pfquotechar(char c) {
  return pfquotechar(static_cast<unsigned char>(c));
}

inline QString pftoxmlname(const QString &string) {
  QString s;
  for (int i = 0; i < string.size(); ++i) {
    QChar c = string.at(i);
    ushort u = c.unicode();
    if (i == 0 && (u == '-' || (u >= '0' && u <= '9')))
      s.append('_');
    if (((u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z'))
        || (u >= '0' && u <= '9') || u == '-' || u > 127)
      s.append(c);
    else
      s.append('_');
  }
  return s;
}

inline QString pftoxmltext(const QString &string) {
  QString s;
  for (int i = 0; i < string.size(); ++i) {
    QChar c = string.at(i);
    ushort u = c.unicode();
    if (u == 0)
      s.append('_'); // char 0 is not allowed in XML
    else if (u < 32 || pfin(c.toLatin1(), PF_XML_SPECIAL_CHARS))
      s.append(QString("&#x%1;").arg(u, 1, 16, QChar('0')));
    else
      s.append(c);
  }
  return s;
}

#endif // PFINTERNALS_P_H
