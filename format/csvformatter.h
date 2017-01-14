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
#ifndef CSVFORMATTER_H
#define CSVFORMATTER_H

#include "modelview/shareduiitemlist.h"
#include <QAbstractItemModel>

/** Formats various data types to CSV row, table or header. */
class LIBQTSSUSHARED_EXPORT CsvFormatter {
  QString _topLeftHeader, _recordSeparator, _specialChars;
  QChar _fieldSeparator, _fieldQuote, _escapeChar, _replacementChar;
  bool _columnHeaders, _rowHeaders;
  static QString _defaultRecordSeparator;
  static QChar _defaultFieldSeparator, _defaultFieldQuote, _defaultEscapeChar,
  _defaultReplacementChar;

public:
  CsvFormatter(QChar fieldSeparator, QString recordSeparator, QChar fieldQuote,
               QChar escapeChar, QChar replacementChar);
  CsvFormatter(QChar fieldSeparator, QString recordSeparator, QChar fieldQuote,
               QChar escapeChar)
    : CsvFormatter(fieldSeparator, recordSeparator, fieldQuote, escapeChar,
                   _defaultReplacementChar) { }
  CsvFormatter(QChar fieldSeparator, QString recordSeparator, QChar fieldQuote)
    : CsvFormatter(fieldSeparator, recordSeparator, fieldQuote,
                   _defaultEscapeChar) { }
  CsvFormatter(QChar fieldSeparator, QString recordSeparator)
    : CsvFormatter(fieldSeparator, recordSeparator, _defaultFieldQuote) { }
  CsvFormatter(QChar fieldSeparator)
    : CsvFormatter(fieldSeparator, _defaultRecordSeparator) { }
  CsvFormatter() : CsvFormatter(_defaultFieldSeparator) { }
  void setTopLeftHeader(QString rawText) { _topLeftHeader = rawText; }
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
  void setColumnHeaders(bool set = true) { _columnHeaders = set; }
  void setRowHeaders(bool set = true) { _rowHeaders = set; }
  /** Format an item as a CSV row.
   * To format a CSV header row, use SharedUiItem::HeaderDisplayRole as role.
   * If row headers are enabled, item.id() is used as a row header but if role
   * is HeaderDisplayRole, in which case topLeftHeader is used insted. */
  QString format(const SharedUiItem &item, int role = Qt::DisplayRole);
  /** Format items in a list as CSV rows.
   * If column headers are enabled, a header row is added before rows (even if
   * list is empty).
   * If row headers are enabled, row number is used (starting from 1). */
  QString format(const SharedUiItemList<> &list, int role = Qt::DisplayRole);
  /** Format the rows under parent index as CSV rows.
   * If lastRow is -1 or > to rowCount() every row is formatted until last one.
   * If column headers are enabled, model's horizontal header are used.
   * If row headers are enabled, model's vertical header is used. */
  QString format(const QAbstractItemModel *model, int firstRow = 0,
                 int lastRow = -1, const QModelIndex &parent = QModelIndex()) {
    return format(model, firstRow, lastRow, parent, false); }
  /** Format a row as CSV.
   * If column headers are never written, being them enabled or not.
   * If row headers are enabled, model's horizontal header is used. */
  QString formatRow(const QAbstractItemModel *model, int row,
                    const QModelIndex &parent = QModelIndex()) {
    return format(model, row, row, parent, true); }
  /** Format the row to which index belongs as CSV.
   * If column headers are never written, being them enabled or not.
   * If row headers are enabled, model's horizontal header is used. */
  QString formatRow(const QModelIndex &index) {
    return formatRow(index.model(), index.row(), index.parent()); }
  /** Format model's horizontal headers under parent index as a CSV row.
   * If row headers are enabled, topLeftHeader is used. */
  QString formatHeader(const QAbstractItemModel *model,
                       const QModelIndex &parent = QModelIndex());

private:
  QString format(const QAbstractItemModel *model, int firstRow, int lastRow,
                 const QModelIndex &parent, bool hideColumnsHeader);
  inline QString format(const SharedUiItem &item, int role, bool hideRowHeader);
  inline void updateSpecialChars();
  inline QString formatField(QString rawData) const;
};

#endif // CSVFORMATTER_H
