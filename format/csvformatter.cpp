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
#include "csvformatter.h"

QChar CsvFormatter::_defaultFieldSeparator(',');
QString CsvFormatter::_defaultRecordSeparator("\n");
QChar CsvFormatter::_defaultFieldQuote;
QChar CsvFormatter::_defaultEscapeChar;
QChar CsvFormatter::_defaultReplacementChar;

CsvFormatter::CsvFormatter(
    QChar fieldSeparator, QString recordSeparator, QChar fieldQuote,
    QChar escapeChar, QChar replacementChar)
  : _recordSeparator(recordSeparator), _fieldSeparator(fieldSeparator),
    _fieldQuote(fieldQuote), _escapeChar(escapeChar),
    _replacementChar(replacementChar),
    _columnHeaders(true), _rowHeaders(false) {
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

QString CsvFormatter::formatField(QString rawData) const {
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

QString CsvFormatter::format(
    const SharedUiItem &item, int role, bool hideRowHeader) {
  QString s;
  int n = item.uiSectionCount();
  if (_rowHeaders && !hideRowHeader)
    s.append(formatField(role == SharedUiItem::HeaderDisplayRole
                         ? _topLeftHeader : item.id()))
        .append(_fieldSeparator);
  for (int i = 0; i < n; ++i) {
    if (i)
      s.append(_fieldSeparator);
    s.append(formatField(item.uiString(i, role)));
  }
  s.append(_recordSeparator);
  return s;
}

QString CsvFormatter::format(const SharedUiItem &item, int role) {
  return format(item, role, false);
}

QString CsvFormatter::format(const SharedUiItemList<> &list, int role) {
  QString s;
  if (_columnHeaders) {
    const SharedUiItem first = list.isEmpty() ? SharedUiItem() : list.first();
    s.append(format(first, SharedUiItem::HeaderDisplayRole));
  }
  if (_rowHeaders) {
    int row = 0;
    for (const SharedUiItem &item : list) {
      ++row;
      s.append(QString::number(row)).append(_fieldSeparator)
          .append(format(item, role, true));
    }
  } else {
    for (const SharedUiItem &item : list)
      s.append(format(item, role, true));
  }
  return s;
}

QString CsvFormatter::format(
    const QAbstractItemModel *model, int firstRow, int lastRow,
    const QModelIndex &parent, bool hideColumnsHeader) {
  QString s;
  if (!model)
    return s;
  if (_columnHeaders && !hideColumnsHeader)
    s.append(formatHeader(model, parent));
  int columns = model->columnCount(parent);
  if (firstRow < 0)
    firstRow = 0;
  if (lastRow == -1 || lastRow >= model->rowCount(parent))
    lastRow = model->rowCount(parent)-1;
  for (int row = firstRow; row <= lastRow; ++row) {
    if (_rowHeaders)
      s.append(formatField(model->headerData(row, Qt::Vertical).toString()))
          .append(_fieldSeparator);
    for (int column = 0; column < columns; ++column) {
      QModelIndex index = model->index(row, column, QModelIndex());
      s.append(formatField(model->data(index).toString()));
      if (column < columns-1)
        s.append(_fieldSeparator);
    }
    s.append(_recordSeparator);
  }
  return s;
}

QString CsvFormatter::formatHeader(
    const QAbstractItemModel *model, const QModelIndex &parent) {
  QString s;
  if (!model)
    return s;
  if (_rowHeaders)
    s.append(_topLeftHeader).append(_fieldSeparator);
  int columns = model->columnCount(parent);
  for (int i = 0; i < columns; ++i) {
    if (i)
      s.append(_fieldSeparator);
    s.append(formatField(model->headerData(i, Qt::Horizontal).toString()));
  }
  s.append(_recordSeparator);
  return s;
}
