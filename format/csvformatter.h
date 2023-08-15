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
#ifndef CSVFORMATTER_H
#define CSVFORMATTER_H

#include "abstracttextformatter.h"

/** Formats various data types to CSV row, table or header. */
class LIBP6CORESHARED_EXPORT CsvFormatter : public AbstractTextFormatter {
  QString _recordSeparator, _specialChars;
  QChar _fieldSeparator, _fieldQuote, _escapeChar, _replacementChar;
  static QString _defaultRecordSeparator;
  static QChar _defaultFieldSeparator, _defaultFieldQuote, _defaultEscapeChar,
  _defaultReplacementChar;

public:
  CsvFormatter(QChar fieldSeparator, QString recordSeparator, QChar fieldQuote,
               QChar escapeChar, QChar replacementChar,
               int maxCellContentLength);
  CsvFormatter(QChar fieldSeparator, QString recordSeparator, QChar fieldQuote,
               QChar escapeChar, QChar replacementChar)
    : CsvFormatter(fieldSeparator, recordSeparator, fieldQuote, escapeChar,
                   replacementChar,
                   AbstractTextFormatter::defaultMaxCellContentLength()) { }
  CsvFormatter(QChar fieldSeparator, QString recordSeparator, QChar fieldQuote,
               QChar escapeChar)
    : CsvFormatter(fieldSeparator, recordSeparator, fieldQuote, escapeChar,
                   _defaultReplacementChar) { }
  CsvFormatter(QChar fieldSeparator, QString recordSeparator, QChar fieldQuote)
    : CsvFormatter(fieldSeparator, recordSeparator, fieldQuote,
                   _defaultEscapeChar) { }
  CsvFormatter(QChar fieldSeparator, QString recordSeparator)
    : CsvFormatter(fieldSeparator, recordSeparator, _defaultFieldQuote) { }
  explicit CsvFormatter(QChar fieldSeparator)
    : CsvFormatter(fieldSeparator, _defaultRecordSeparator) { }
  CsvFormatter() : CsvFormatter(_defaultFieldSeparator) { }
  /** Default: comma */
  void setFieldSeparator(QChar c = ',');
  /** Default: comma */
  static void setDefaultFieldSeparator(QChar c = ',') {
    _defaultFieldSeparator = c; }
  /** Default: newline (a.k.a. Unix end of line) */
  void setRecordSeparator(QString string = "\n");
  /** Default: newline (a.k.a. Unix end of line) */
  static void setDefaultRecordSeparator(QString s = "\n") {
    _defaultRecordSeparator = s; }
  /** Used to quote every field on both left and right sides.
   * Default: none
   * Example: double quote */
  void setFieldQuote(QChar c = QChar());
  /** @see setFieldQuote() */
  static void setDefaultFieldQuote(QChar c = QChar()) {
    _defaultFieldQuote = c; }
  /** Used to protect special character within field.
   * If field quote is set, only field quote and escape char are special chars,
   * otherwise field and record separators are also special chars.
   * If escape char is not found, special chars sequences are replaced with
   * replacement char, or removed if replacement char not found.
   * Default: none
   * Example: backslash */
  void setEscapeChar(QChar c = QChar());
  /** @see setEscapeChar() */
  static void setDefaultEscapeChar(QChar c = QChar()) {
    _defaultEscapeChar = c; }
  /** Used as a placeholder for special chars within field data.
   * If field quote is set, only field quote and escape char are special chars,
   * otherwise field and record separators are also special chars.
   * If escape char is not found, special chars sequences are replaced with
   * replacement char, or removed if replacement char not found.
   * Default: none
   * Examples: underscore, question mark */
  void setReplacementChar(QChar c = QChar()) { _replacementChar = c; }
  /** @see setReplacementChar() */
  static void setDefaultReplacementChar(QChar c = QChar()) {
    _defaultReplacementChar = c; }
  using AbstractTextFormatter::formatCell;
  QString formatCell(QString data) const override;
  using AbstractTextFormatter::formatTableHeader;
  QString formatTableHeader(const QStringList &columnHeaders) const override;
  using AbstractTextFormatter::formatTableFooter;
  QString formatTableFooter(const QStringList &columnHeaders) const override;
  using AbstractTextFormatter::formatRow;
  QString formatRow(const QStringList &cells,
                    QString rowHeader = QString()) const override;

private:
  inline void updateSpecialChars();
};

#endif // CSVFORMATTER_H
