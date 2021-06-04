/* Copyright 2014-2021 Hallowyn, Gregoire Barbier and others.
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
#include "csvfile.h"
#include <QBuffer>

CsvFile::CsvFile(QObject *parent)
  : QObject(parent), _openMode(QIODevice::NotOpen),
    _fieldSeparator(','), _escapeChar('\\'), _quoteChar('"'),
    _headersEnabled(true), _columnCount(0) {
}

CsvFile::CsvFile(QObject *parent, QString filename)
  : CsvFile(parent) {
  _filename = filename;
}

CsvFile::CsvFile(QObject *parent, QIODevice *input)
  : CsvFile(parent) {
  openReadonly(input);
}

bool CsvFile::open(QIODevice::OpenMode mode) {
  close();
  QFile file(_filename);
  if (file.open(_openMode = mode)) {
    if (!(mode & QIODevice::ReadOnly))
      return true;
    if (readAll(&file))
      return true;
  }
  close();
  return false;
}

bool CsvFile::open(QString filename, QIODevice::OpenMode mode) {
  _filename = filename;
  return open(mode);
}

bool CsvFile::openReadonly(QIODevice *input) {
  close();
  _filename = QString();
  _openMode = QIODevice::ReadOnly;
  return readAll(input);
}

bool CsvFile::openReadonly(QByteArray input) {
  QBuffer buffer(&input);
  buffer.open(QIODevice::ReadOnly);
  return openReadonly(&buffer);
}

void CsvFile::close() {
  _rows.clear();
  _headers.clear();
  _columnCount = 0;
  _openMode = QIODevice::NotOpen;
}

bool CsvFile::setHeaders(QStringList data) {
  if (_openMode & QIODevice::WriteOnly) {
    _headers = data;
    _columnCount = qMax(_columnCount, data.size());
    return writeAll();
  }
  return false;
}

bool CsvFile::insertRow(int row, QStringList data) {
  if (row < 0 || row > _rows.size())
    return false;
  if (_openMode & QIODevice::WriteOnly) {
    _rows.insert(row, data);
    _columnCount = qMax(_columnCount, data.size());
    return writeAll();
  }
  return false;
}

bool CsvFile::updateRow(int row, QStringList data){
  if (row < 0 || row >= _rows.size())
    return false;
  if (_openMode & QIODevice::WriteOnly) {
    _rows[row] = data;
    _columnCount = qMax(_columnCount, data.size());
    return writeAll();
  }
  return false;
}

bool CsvFile::appendRow(QStringList data) {
  return insertRow(_rows.size(), data);
}

bool CsvFile::removeRows(int first, int last) {
  if (first > last || first < 0 || last >= _rows.size())
    return false;
  if (_openMode & QIODevice::WriteOnly) {
    int count = last - first + 1;
    while (count--)
      _rows.removeAt(first);
    return writeAll();
  }
  return false;
}

bool CsvFile::readAll(QIODevice *input) {
  bool atEnd = false;
  if (_headersEnabled) {
    if (!readRow(input, &_headers, &atEnd))
      return false;
    _columnCount = _headers.size();
  }
  while (!atEnd) {
    QStringList row;
    if (!readRow(input, &row, &atEnd))
      return false;
    if (!atEnd || !row.isEmpty())
      _rows.append(row);
    _columnCount = qMax(_columnCount, row.size());
  }
  return true;
}

bool CsvFile::readRow(QIODevice *input, QStringList *row, bool *atEnd) {
  row->clear();
  // LATER call waitForReadyRead() with a parametrized timeout (named pipes...)
  QByteArray data;
  bool quoting = false;
  forever {
    char c;
    switch (input->read(&c, 1)) {
    case 0: // end of file
      *atEnd = true;
      return true;
    case 1:
      if (c == _escapeChar) {
        switch (input->read(&c, 1)) {
        case 0:
          *atEnd = true;
          break; // ignore lone \ at end of file
        case 1:
          data.append(c);
          break;
        default: // error
          return false;
        }
      } else if (c == _quoteChar) {
        quoting = !quoting;
      } else if (!quoting && c == _fieldSeparator) {
        row->append(QString::fromUtf8(data));
        data.clear();
      } else if (c == '\r') {
        // silently ignore \r
      } else if (!quoting && c == '\n') {
        if (!data.isEmpty())
          row->append(QString::fromUtf8(data));
        return true;
      } else {
        data.append(c);
      }
      break;
    default: // error
      return false;
    }
  }
}

bool CsvFile::writeAll() {
  if (_openMode & QIODevice::WriteOnly) {
    QSaveFile file(_filename);
    if (file.open(QIODevice::WriteOnly)) {
      QString specialChars("\r\n");
      specialChars.append(_fieldSeparator).append(_escapeChar)
          .append(_quoteChar);
      if (_headersEnabled)
        if (!writeRow(&file, _headers, specialChars))
          return false;
      foreach (const QStringList &row, _rows)
        if (!writeRow(&file, row, specialChars))
          return false;
      return file.commit();
    }
  }
  return false;
}

bool CsvFile::writeRow(QSaveFile *file, QStringList row, QString specialChars) {
  QString s;
  bool firstColumn = true;
  foreach (const QString &cell, row) {
    if (firstColumn)
      firstColumn = false;
    else
      s.append(_fieldSeparator);
    for (int i = 0; i < cell.size(); ++i) {
      QChar c = cell.at(i);
      if (specialChars.contains(c))
        s.append(_escapeChar);
      s.append(c);
    }
  }
  s.append('\n');
  QByteArray bytes = s.toUtf8();
  return (file->write(bytes) == bytes.size());
}
