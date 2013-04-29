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
#include "csvtableview.h"

CsvTableView::CsvTableView(QObject *parent, int maxrows)
  : TextTableView(parent, maxrows), _recordSeparator("\n"),
    _fieldSeparator(','), _columnHeaders(true), _rowHeaders(false) {
  updateSpecialChars();
}

void CsvTableView::updateHeaderAndFooterText() {
  QAbstractItemModel *m = model();
  QString v;
  if (m && _columnHeaders) {
    if (_rowHeaders)
      v.append(_topLeftHeader).append(_fieldSeparator);
    int columns = m->columnCount(QModelIndex());
    for (int i = 0; i < columns; ++i) {
      v.append(formatField(m->headerData(i, Qt::Horizontal).toString()));
      if (i < columns-1)
        v.append(_fieldSeparator);
    }
    v.append(_recordSeparator);
  }
  _header = v;
  _footer = QString();
}

QString CsvTableView::rowText(int row) {
  QAbstractItemModel *m = model();
  QString v;
  if (!m)
    return QString();
  int columns = m->columnCount();
  if (_rowHeaders)
    v.append(formatField(m->headerData(row, Qt::Vertical).toString()))
        .append(_fieldSeparator);
  for (int column = 0; column < columns; ++column) {
    QModelIndex index = m->index(row, column, QModelIndex());
    v.append(formatField(m->data(index).toString()));
    if (column < columns-1)
      v.append(_fieldSeparator);
  }
  v.append(_recordSeparator);
  return v;
}

void CsvTableView::setFieldSeparator(QChar c) {
  _fieldSeparator = c;
  updateSpecialChars();
}

void CsvTableView::setRecordSeparator(QString string) {
  _recordSeparator = string;
  updateSpecialChars();
}

void CsvTableView::setFieldQuote(QChar c) {
  _fieldQuote = c;
  updateSpecialChars();
}

void CsvTableView::setEscapeChar(QChar c) {
  _escapeChar = c;
  updateSpecialChars();
}

void CsvTableView::updateSpecialChars() {
  _specialChars.clear();
  if (!_escapeChar.isNull())
    _specialChars.append(_escapeChar);
  if (!_fieldQuote.isNull()) {
    _specialChars.append(_fieldQuote);
  } else {
    if (!_fieldSeparator.isNull())
      _specialChars.append(_fieldSeparator);
    _specialChars.append(_recordSeparator);
  }
}

QString CsvTableView::formatField(QString rawData) const {
  QString s;
  if (!_fieldQuote.isNull())
    s.append(_fieldQuote);
  if (!_escapeChar.isNull()) {
    foreach (const QChar c, rawData) {
      if (_specialChars.contains(c))
        s.append(_escapeChar);
      s.append(c);
    }
  } else if (!_replacementChar.isNull()) {
    bool first = true;
    foreach (const QChar c, rawData) {
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
    foreach (const QChar c, rawData)
      if (!_specialChars.contains(c))
        s.append(c);
  }
  if (!_fieldQuote.isNull())
    s.append(_fieldQuote);
  return s;
}
