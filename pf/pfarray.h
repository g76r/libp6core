/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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

#ifndef PFARRAY_H
#define PFARRAY_H

#include <QStringList>
#include "pfoptions.h"

class QIODevice;
class PfNode;

class LIBP6CORESHARED_EXPORT PfArrayData : public QSharedData {
  friend class PfArray;
  QStringList _headers;
  QList<QStringList> _rows;

public:
  PfArrayData() { }
};

class LIBP6CORESHARED_EXPORT PfArray {
  QSharedDataPointer<PfArrayData> d;

public:
  PfArray() {  }
  PfArray(const PfArray &other) : d(other.d) { }
  explicit PfArray(const QList<QStringList> &rows) { appendRows(rows); }
  explicit PfArray(const QStringList &headers,
                   const QList<QStringList> &rows) {
    appendHeaders(headers);
    appendRows(rows);
  }
  PfArray &operator=(const PfArray &other) { d = other.d; return *this; }
  /** @return true if null-size array (0 rows 0 columns 0 headers) */
  bool isNull() const { return !d; }
  /** @return true if no data (0 rows but maybe some headers defined) */
  bool isEmpty() const { return d ? d->_rows.isEmpty() : true; }
  int columnsCount() const { return d ? d->_headers.size() : 0; }
  /** do not include headers */
  int rowsCount() const { return d ? d->_rows.size() : 0; }
  QStringList headers() const { return d ? d->_headers : QStringList(); }
  /** @param column 0 for first column */
  QString header(int column) const {
    return d && column < d->_headers.size()
        ? d->_headers.at(column) : QString(); }
  /** do not include headers */
  QList<QStringList> rows() const {
    return d ? d->_rows : QList<QStringList>(); }
  /** @param row 0 for first row, not including headers */
  const QStringList row(int row) const {
    return d && row < d->_rows.size() ? d->_rows.at(row) : QStringList(); }
  /** @param row 0 for first row, not including headers
    * @param column 0 for first column
    * @return QString() if indexes are out of range, QString("") if empty */
  const QString cell(int row, int column) const {
    if (!d || row >= d->_rows.size())
      return QString();
    const QStringList &r(d->_rows.at(row));
    if (column >= r.size())
      return "";
    return r.at(column);
  }
  /** set value in a given cell, autoenlarging array if indexes are out of range
    * @param row 0 for first row, not including headers
    * @param column 0 for first column */
  void setCell(int row, int column, const QString &value) {
    if (!d)
      d = new PfArrayData();
    for (int i = d->_headers.size(); i < column; ++i)
      d->_headers.append(QString::number(i));
    for (int i = d->_rows.size(); i < row; ++i)
      d->_rows.append(QStringList());
    QStringList &r(d->_rows[row]);
    for (int i = r.size(); i < column; ++i)
      r.append(QString());
    r[column] = value;
  }
  void appendHeader(const QString &value) {
    if (!d)
      d = new PfArrayData();
    d->_headers.append(value);
  }
  void appendHeaders(const QStringList &headers) {
    if (!d)
      d = new PfArrayData();
    d->_headers.append(headers);
  }
  void appendRow(const QStringList &values = QStringList()) {
    if (!d)
      d = new PfArrayData();
    d->_rows.append(values);
    for (int i = d->_headers.size(); i < values.size(); ++i)
      d->_headers.append(QString::number(i));
  }
  void appendRows(const QList<QStringList> &rows) {
    for (auto row: rows)
      appendRow(row);
  }
  void appendCell(const QString &value) {
    if (!d)
      d = new PfArrayData();
    if (d->_rows.size() == 0)
      appendRow();
    for (int i = d->_headers.size(); i <= d->_rows.last().size(); ++i)
      d->_headers.append(QString::number(i));
    d->_rows.last().append(value);
  }
  /** convenience method for parser */
  void removeLastRowIfEmpty() {
    if (d && d->_rows.size() && d->_rows.last().isEmpty())
      d->_rows.removeLast();
  }
  void clear() {
    if (d) {
      d->_headers.clear();
      d->_rows.clear();
    }
  }
  /** Write array content in PF CSV-like format, escaping PF special characters.
    */
  qint64 writePf(QIODevice *target,
                 const PfOptions &options = PfOptions()) const;
  QString toPf(const PfOptions &options = PfOptions()) const;
  /** Write array content in HTML-like table-tr-th-td format.
    * @param withHeaders if true include a header line
    */
  qint64 writeTrTd(QIODevice *target, bool withHeaders = true,
                   const PfOptions &options = PfOptions()) const;
  /** Fill the target node (for instance the node containing this array) with
    * one child per row, named after the row number (begining with 0) and
    * itself having one child per cell, named after the column number and
    * containing the cell content.
    * By default (keepExistingChildren = false), target children having a number
    * name between 0 and rowsCount() are removed before being recreated.
    * Otherwise duplicates will be allowed. */
  void convertToChildrenTree(PfNode *target,
                             bool keepExistingChildren = false) const;
};

Q_DECLARE_TYPEINFO(PfArray, Q_MOVABLE_TYPE);

#endif // PFARRAY_H
