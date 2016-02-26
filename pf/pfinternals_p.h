/* Copyright 2012-2016 Hallowyn and others.
See the NOTICE file distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file to you under
the Apache License, Version 2.0 (the "License"); you may not use this
file except in compliance with the License. You may obtain a copy of the
License at http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations
under the License.
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
    return QString(c);
  return QString("\\x").append("0123456789abcdef"[(c&0xf0)>>4])
      .append("0123456789abcdef"[c&0xf]);
}

inline QString pftoxmlname(QString string) {
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

inline QString pftoxmltext(QString string) {
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
