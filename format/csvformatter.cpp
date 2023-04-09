/* Copyright 2017-2023 Hallowyn, Gregoire Barbier and others.
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
#include "csvformatter.h"
#include "format/stringutils.h"

QChar CsvFormatter::_defaultFieldSeparator(',');
QString CsvFormatter::_defaultRecordSeparator("\n");
QChar CsvFormatter::_defaultFieldQuote;
QChar CsvFormatter::_defaultEscapeChar;
QChar CsvFormatter::_defaultReplacementChar;

CsvFormatter::CsvFormatter(
    QChar fieldSeparator, QString recordSeparator, QChar fieldQuote,
    QChar escapeChar, QChar replacementChar, int maxCellContentLength)
  : AbstractTextFormatter(maxCellContentLength),
    _recordSeparator(recordSeparator), _fieldSeparator(fieldSeparator),
    _fieldQuote(fieldQuote), _escapeChar(escapeChar),
    _replacementChar(replacementChar) {
  updateSpecialChars();
}

void CsvFormatter::setFieldSeparator(QChar c) {
  _fieldSeparator = c;
  updateSpecialChars();
}

void CsvFormatter::setRecordSeparator(QString string) {
  _recordSeparator = string;
  updateSpecialChars();
}

void CsvFormatter::setFieldQuote(QChar c) {
  _fieldQuote = c;
  updateSpecialChars();
}

void CsvFormatter::setEscapeChar(QChar c) {
  _escapeChar = c;
  updateSpecialChars();
}

void CsvFormatter::updateSpecialChars() {
  _specialChars.clear();
  if (!_escapeChar.isNull())
    _specialChars.append(_escapeChar);
  if (!_fieldQuote.isNull())
    _specialChars.append(_fieldQuote);
  if (!_fieldSeparator.isNull())
    _specialChars.append(_fieldSeparator);
  _specialChars.append(_recordSeparator);
}

QString CsvFormatter::formatCell(QString data) const {
  data = StringUtils::elideMiddle(data, maxCellContentLength());
  QString s;
  if (!_fieldQuote.isNull())
    s.append(_fieldQuote);
  if (!_escapeChar.isNull()) {
    foreach (const QChar c, data) {
      if (_specialChars.contains(c))
        s.append(_escapeChar);
      s.append(c);
    }
  } else if (!_replacementChar.isNull()) {
    bool first = true;
    foreach (const QChar c, data) {
      if (_specialChars.contains(c)) {
        if (first) {
          s.append(_replacementChar);
          first = false;
        }
      } else {
        s.append(c);
        first = true;
      }
    }
  } else {
    foreach (const QChar c, data)
      if (!_specialChars.contains(c))
        s.append(c);
  }
  if (!_fieldQuote.isNull())
    s.append(_fieldQuote);
  return s;
}

QString CsvFormatter::formatTableHeader(
    const QStringList &columnHeaders) const {
  QString s;
  if (rowHeadersEnabled())
    s.append(formatCell(topLeftHeader())).append(_fieldSeparator);
  bool first = true;
  for (const QString &header : columnHeaders) {
    if (first)
      first = false;
    else
      s.append(_fieldSeparator);
    s.append(formatCell(header));
  }
  s.append(_recordSeparator);
  return s;
}

QString CsvFormatter::formatTableFooter(
    const QStringList &columnHeaders) const {
  Q_UNUSED(columnHeaders)
  return {};
}

QString CsvFormatter::formatRow(const QStringList &cells,
                                QString rowHeader) const {
  QString s;
  if (rowHeadersEnabled())
    s.append(formatCell(rowHeader)).append(_fieldSeparator);
  bool first = true;
  for (const QString &cell : cells) {
    if (first)
      first = false;
    else
      s.append(_fieldSeparator);
    s.append(formatCell(cell));
  }
  s.append(_recordSeparator);
  return s;
}
