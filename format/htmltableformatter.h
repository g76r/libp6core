/* Copyright 2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef HTMLTABLEFORMATTER_H
#define HTMLTABLEFORMATTER_H

#include "abstracttextformatter.h"
#include <QAbstractItemModel>

/** Formats various data types to HTML table row, whole table or table header.
 */
class LIBPUMPKINSHARED_EXPORT HtmlTableFormatter
    : public AbstractTextFormatter {
public:
  enum TextConversion { AsIs, HtmlEscaping, HtmlEscapingWithUrlAsLinks };

private:
  TextConversion _textConversion;
  static TextConversion _defaultTextConversion;

public:
  explicit HtmlTableFormatter(int maxCellContentLength);
  HtmlTableFormatter()
    : HtmlTableFormatter(AbstractTextFormatter::defaultMaxCellContentLength()) {
  }
  /** Set the way text data in the model is converted.
   * <ul>
   * <li> AsIs: no conversion at all, even leave HTML special chars as is
   * <li> HtmlEscaping: transform HTML special chars into HTML entities
   * <li> HtmlEscapingWithUrlAsLinks: URLs are surrounded with a href tags
   * </ul>
   * default: HtmlEscapingWithUrlAsLinks */
  void setTextConversion(
      TextConversion conversion = HtmlEscapingWithUrlAsLinks) {
    _textConversion = conversion; }
  TextConversion textConversion() const { return _textConversion; }
  /** @see setTextConversion(). */
  static void setDefaultTextConversion(
      TextConversion conversion = HtmlEscapingWithUrlAsLinks) {
    _defaultTextConversion = conversion; }
  using AbstractTextFormatter::formatCell;
  QString formatCell(QString data) const override;
  using AbstractTextFormatter::formatTableHeader;
  QString formatTableHeader(const QStringList &columnHeaders) const override;
  using AbstractTextFormatter::formatTableFooter;
  QString formatTableFooter(const QStringList &columnHeaders) const override;
  using AbstractTextFormatter::formatRow;
  QString formatRow(const QStringList &cells,
                    QString rowHeader = QString()) const override;
};

#endif // HTMLTABLEFORMATTER_H
