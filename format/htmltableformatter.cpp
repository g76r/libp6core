/* Copyright 2017 Hallowyn and others.
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
#include "htmltableformatter.h"
#include "util/htmlutils.h"
#include "util/stringutils.h"

HtmlTableFormatter::TextConversion
HtmlTableFormatter::_defaultTextConversion(HtmlEscapingWithUrlAsLinks);

HtmlTableFormatter::HtmlTableFormatter(int maxCellContentLength)
  : AbstractTextFormatter(maxCellContentLength),
    _textConversion(_defaultTextConversion) {
}

QString HtmlTableFormatter::formatCell(QString data) const {
  data = StringUtils::elideMiddle(data, maxCellContentLength());
  switch (_textConversion) {
  case HtmlEscaping:
    return HtmlUtils::htmlEncode(data, false, false);
    break;
  case HtmlEscapingWithUrlAsLinks:
    return HtmlUtils::htmlEncode(data, true, true);
    break;
  case AsIs:
    ;
  }
  return data;
}

QString HtmlTableFormatter::formatTableHeader(
    const QStringList &columnHeaders) const {
  QString s;
  s.append("<table>\n");
  if (columnHeadersEnabled()) {
    s.append("<thead><tr>");
    if (rowHeadersEnabled())
      s.append("<th>").append(formatCell(topLeftHeader())).append("</th>");
    for (const QString &header: columnHeaders)
      s.append("<th>").append(formatCell(header)).append("</th>");
    s.append("</tr></thead>");
  }
  s.append("<tbody>\n");
  return s;
}

QString HtmlTableFormatter::formatTableFooter(
    const QStringList &columnHeaders) const {
  Q_UNUSED(columnHeaders)
  return QStringLiteral("</tbody>\n</table>\n");
}

QString HtmlTableFormatter::formatRow(const QStringList &cells,
                                     QString rowHeader) const {
  QString s;
  s.append("<tr>");
  if (rowHeadersEnabled())
    s.append("<th>").append(formatCell(rowHeader)).append("</th>");
  for (const QString &cell: cells)
    s.append("<td>").append(formatCell(cell)).append("</td>");
  s.append("</tr>\n");
  return s;
}
