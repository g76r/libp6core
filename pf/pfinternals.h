#ifndef PFINTERNALS_H
#define PFINTERNALS_H

/* This file contain methods and defines that are only for internal use
 * of PF implementation and MUST not be used (or even #include'd) by
 * user code.
 */

#include <QObject>
#include <QString>

#define PF_SPACES " \t\n\r"
#define PF_RESERVED "$&~`^[]{}"
#define PF_SEPARATORS "()|#;"
#define PF_QUOTES "'\""
#define PF_ESCAPE "\\"
#define PF_XML_SPECIAL_CHARS "<>#&["

inline bool pfin(const char &c, const char *s) {
  while (*s)
    if (c == *(s++))
      return true;
  return false;
}

inline bool pfisnewline(const char &c) { return c == '\n'; }

inline bool pfisspace(const char &c) { return pfin(c, PF_SPACES); }

inline bool pfisquote(const char &c) { return pfin(c, PF_QUOTES); }

inline bool pfisspecial(const char &c) {
  return pfin(c, PF_SPACES PF_RESERVED PF_SEPARATORS PF_QUOTES PF_ESCAPE);
}

inline bool pfisendofname(const char &c) {
  return pfin(c, PF_SPACES PF_SEPARATORS);
}

inline QString tr(const char *s) { return QObject::tr(s); }

/** Return a C-style quoted char if char is a special char, e.g.
  * 97 (a)                          ->      a
  * 92 (\)                          ->      \\
  * 233 (é in ISO 8859-1)           ->      \xe9
  * 10 (a.k.a. \n or end of line)   ->      \x0a
  */
inline QString pfquotechar(const unsigned char &c) {
  if (c == '\\')
    return "\\\\";
  if (c > 32 && c < 128)
    return QString(c);
  return QString("\\x").append("0123456789abcdef"[(c&0xf0)>>4])
      .append("0123456789abcdef"[c&0xf]);
}

/** Return a string with all PF special chars escaped, e.g.
  * foo 'bar      ->      foo\ \'bar
  * foo\\bar      ->      foo\\\\bar
  * "foo"(|       ->      \"foo\"\(\|
  */
inline QString pfescape(const QString &string) {
  QString s;
  for (int i = 0; i < string.size(); ++i) {
    QChar c = string.at(i);
    if (pfisspecial(c.toAscii()))
      s.append(PF_ESCAPE);
    s.append(c);
  }
  return s;
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
    else if (u < 32 || pfin(c.toAscii(), PF_XML_SPECIAL_CHARS))
      s.append(QString("&#x%1;").arg(u, 1, 16, QChar('0')));
    else
      s.append(c);
  }
  return s;
}

#endif // PFINTERNALS_H
