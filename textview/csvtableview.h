/* Copyright 2012-2013 Hallowyn and others.
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
#ifndef CSVTABLEVIEW_H
#define CSVTABLEVIEW_H

#include "texttableview.h"

/** Display the model content as a CSV table. Only rows of the root index
  * are displayed. */
// LATER add style options (separators, quotes, indentation string, columns selection, hide non-leaf rows...)
class LIBQTSSUSHARED_EXPORT CsvTableView : public TextTableView {
  Q_OBJECT
  QString _topLeftHeader, _recordSeparator, _specialChars;
  QChar _fieldSeparator, _fieldQuote, _escapeChar, _replacementChar;
  bool _columnHeaders, _rowHeaders;

public:
  explicit CsvTableView(QObject *parent = 0, int maxrows = 10000);
  void setTopLeftHeader(QString rawText) { _topLeftHeader = rawText; }
  /** Default: comma */
  void setFieldSeparator(QChar c = ',');
  /** Default: newline (a.k.a. Unix end of line) */
  void setRecordSeparator(QString string = "\n");
  /** Used to quote every field on both left and right sides.
   * Default: none
   * Example: double quote */
  void setFieldQuote(QChar c = QChar());
  /** Used to protect special character within field.
   * If field quote is set, only field quote and escape char are special chars,
   * otherwise field and record separators are also special chars.
   * If escape char is not found, special chars sequences are replaced with
   * replacement char, or removed if replacement char not found.
   * Default: none
   * Example: backslash */
  void setEscapeChar(QChar c = QChar());
  /** Used as a placeholder for special chars within field data.
   * If field quote is set, only field quote and escape char are special chars,
   * otherwise field and record separators are also special chars.
   * If escape char is not found, special chars sequences are replaced with
   * replacement char, or removed if replacement char not found.
   * Default: none
   * Examples: underscore, question mark */
  void setReplacementChar(QChar c = QChar()) { _replacementChar = c; }
  void setColumnHeaders(bool set = true) { _columnHeaders = set; }
  void setRowHeaders(bool set = true) { _rowHeaders = set; }

protected:
  QString headerText();
  QString footerText();
  QString rowText(int row);

private:
  inline void updateSpecialChars();
  inline QString formatField(QString rawData) const;
  Q_DISABLE_COPY(CsvTableView)
};

#endif // CSVTABLEVIEW_H
