/* Copyright 2014-2015 Hallowyn and others.
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
#ifndef CSVFILE_H
#define CSVFILE_H

#include "libqtssu_global.h"
#include <QStringList>
#include <QFile>
#include <QSaveFile>

// LATER implement auto-truncating / rows-count-caped mechanism
// LATER propose a non-all-in-memory mechanism
// LATER support for error() errorString() error reporting
// LATER implement quoting on write

/** Give read/write access to a CSV file content. */
class LIBQTSSUSHARED_EXPORT CsvFile : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(CsvFile)
  QString _filename;
  QIODevice::OpenMode _openMode;
  QList<QStringList> _rows;
  QStringList _headers;
  QChar _fieldSeparator, _escapeChar, _quoteChar;
  bool _areHeadersPresent;
  int _columnCount;

public:
  explicit CsvFile(QObject *parent = 0);
  CsvFile(QObject *parent, QString filename);
  explicit CsvFile(QString filename) : CsvFile(0, filename) { }
  CsvFile(QObject *parent, QIODevice *input);
  explicit CsvFile(QIODevice *input) : CsvFile(0, input) { }
  QStringList headers() const { return _headers; }
  QString header(int column) const { return headers().value(column); }
  QList<QStringList> rows() const { return _rows; }
  QStringList row(int row) const { return _rows.value(row); }
  QString cell(int row, int column) const {
    return _rows.value(row).value(column); }
  int columnCount() const { return _columnCount; }
  int rowCount() const { return _rows.size(); }
  bool open(QIODevice::OpenMode mode);
  bool open(QString filename, QIODevice::OpenMode mode);
  bool openReadonly(QIODevice *input);
  void close();
  QIODevice::OpenMode openMode() const { return _openMode; }
  bool isOpen() const { return _openMode != QIODevice::NotOpen; }
  bool isReadable() const { return _openMode & QIODevice::ReadOnly; }
  bool isWritable() const { return _openMode & QIODevice::WriteOnly; }
  QString filename() const { return _filename; }
  QChar fieldSeparator() const { return _fieldSeparator; }
  /** Default: , (comma) */
  CsvFile &setFieldSeparator(QChar fieldSeparator) {
    _fieldSeparator = fieldSeparator; return *this; }
  QChar escapeChar() const { return _escapeChar; }
  /** Default: \ (backslash) */
  CsvFile &setEscapeChar(QChar escapeChar) {
    _escapeChar = escapeChar; return *this; }
  QChar quoteChar() const { return _quoteChar; }
  /** Default: " (double quote) */
  CsvFile &setQuoteChar(QChar quoteChar) {
    _quoteChar = quoteChar; return *this; }
  bool areHeadersPresent() const { return _areHeadersPresent; }
  /** Default: true (first file line contains headers rather than data) */
  CsvFile &setAreHeadersPresent(bool areHeadersPresent = true) {
    _areHeadersPresent = areHeadersPresent; return *this; }
  bool setHeaders(QStringList data);

public slots:
  bool insertRow(int row, QStringList data);
  bool updateRow(int row, QStringList data);
  bool appendRow(QStringList data);
  bool removeRows(int first, int last);

private:
  bool readAll(QIODevice *input);
  inline bool readRow(QIODevice *input, QStringList *row, bool *atEnd);
  bool writeAll();
  inline bool writeRow(QSaveFile *file, QStringList row, QString specialChars);
};

#endif // CSVFILE_H
