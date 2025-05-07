/* Copyright 2016-2025 Hallowyn, Gregoire Barbier and others.
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
#include "stringutils.h"
#include <QRegularExpressionMatchIterator>

static QRegularExpression linkRe{"http(s?)://\\S+"};

QString StringUtils::htmlEncode(
    const QString &text, bool urlAsLinks, bool newlineAsBr) {
  QString s;
  QMap<int,int> linksIndexes; // start of link index -> length of link
  if (urlAsLinks) {
    QRegularExpressionMatchIterator it = linkRe.globalMatch(text);
    while (it.hasNext()) {
      QRegularExpressionMatch match = it.next();
      linksIndexes.insert(match.capturedStart(0), match.capturedLength(0));
    }
  }
  for (int i = 0; i < text.length(); ) {
    if (urlAsLinks && linksIndexes.contains(i)) {
      int l = linksIndexes.value(i);
      QString html = htmlEncode(text.mid(i, l), false), href = html;
      href.replace("\"", "%22");
      s.append("<a href=\"").append(href).append("\">").append(html)
          .append("</a>");
      i += l;
    } else {
      const QChar c = text.at(i);
      switch(c.toLatin1()) {
      case '<':
        s.append("&lt;");
        break;
      case '>':
        s.append("&gt;");
        break;
      case '&':
        s.append("&amp;");
        break;
      case '"':
        s.append("&#34;");
        break;
      case '\'':
        s.append("&#39;");
        break;
      case '\n':
        if (newlineAsBr)
          s.append("<br/>\n");
        else
          s.append(c);
        break;
      default:
        s.append(c);
      }
      ++i;
    }
  }
  return s;
}

static QString toSnakeCase(const QString &anycase) {
  QString sc;
  bool leading = true;
  for (const QChar &c : anycase) {
    if (c.isSpace() || c == '-' || c == '_') {
      sc.append('_');
      leading = false;
    } else if (c.isUpper()) {
      if (leading) {
        if (!sc.isEmpty())
          sc.append('_');
        leading = false;
      }
      sc.append(c.toLower());
    } else {
      sc.append(c);
      leading = true;
    }
  }
  return sc;
}

Utf8String StringUtils::toSnakeCase(const Utf8String &anycase) {
    return ::toSnakeCase(QString::fromUtf8(anycase)).toUtf8();
}

QString StringUtils::toSnakeCase(const QString &anycase) {
    return ::toSnakeCase(anycase);
}

template <int DIR> // DIR: -1 left 0 middle +1 right
static inline QString elide(
    const QString &string, int maxsize, const QString &placeholder) {
  if (maxsize < 0 || string.size() < maxsize)
    return string;
  if (placeholder.size() > maxsize)
    return DIR >= 0 ? placeholder.left(maxsize) : placeholder.right(maxsize);
  if (DIR > 0)
    return string.left(maxsize-placeholder.size())+placeholder;
  if (DIR < 0)
    return placeholder+string.right(maxsize-placeholder.size());
  return string.left(maxsize/2-placeholder.size())+placeholder
      +string.right(maxsize-maxsize/2);
}

QString StringUtils::elideRight(const QString &string, int maxsize,
                                const QString &placeholder) {
  return elide<+1>(string, maxsize, placeholder);
}

QString StringUtils::elideLeft(const QString &string, int maxsize,
                               const QString &placeholder) {
  return elide<-1>(string, maxsize, placeholder);
}

QString StringUtils::elideMiddle(const QString &string, int maxsize,
                                 const QString &placeholder) {
  return elide<0>(string, maxsize, placeholder);
}
