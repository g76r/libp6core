/* Copyright 2013 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
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
#include "htmlutils.h"
#include <QRegExp>
#include <QHash>

QString HtmlUtils::htmlEncode(QString text, bool urlAsLinks) {
  QString s;
  static QRegExp link("http(s?)://\\S+");
  QHash<int,int> linksIndexes;
  if (urlAsLinks) {
    QRegExp re = link;
    for (int i = 0; (i = re.indexIn(text, i)) != -1; ) {
      int l = re.matchedLength();
      linksIndexes.insert(i, l);
      i += l;
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
      switch(c.toAscii()) {
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
        s.append("&quot;");
        break;
      case '\'':
        s.append("&apos;");
        break;
      default:
        s.append(c);
      }
      ++i;
    }
  }
  return s;
}
