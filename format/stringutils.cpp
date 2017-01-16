/* Copyright 2016-2017 Hallowyn and others.
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
#include "stringutils.h"
#include <QRegularExpression>
#include <QHash>

const QString StringUtils::_ellipsis { "..." };
static QRegularExpression linkRe{"http(s?)://\\S+"};

QString StringUtils::htmlEncode(
    QString text, bool urlAsLinks, bool newlineAsBr) {
  QString s;
  QHash<int,int> linksIndexes; // start of link index -> length of link
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
