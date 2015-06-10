/* Copyright 2012-2015 Hallowyn and others.
See the NOTICE file distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file to you under
the Apache License, Version 2.0 (the "License"); you may not use this
file except in compliance with the License. You may obtain a copy of the
License at http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations
under the License.
*/

#ifndef PFARRAY_H
#define PFARRAY_H

#include "libqtpf_global.h"
#include <QStringList>
#include <QList>
#include <QSharedData>
#include "pfoptions.h"

class QIODevice;
class PfNode;

class LIBQTPFSHARED_EXPORT PfArrayData : public QSharedData {
  friend class PfArray;
private:
  QStringList _headers;
  QList<QStringList> _rows;

public:
  inline PfArrayData() { }
};

class LIBQTPFSHARED_EXPORT PfArray {
private:
  QSharedDataPointer<PfArrayData> d;

public:
  inline PfArray() {  }
  inline PfArray(const PfArray &other) : d(other.d) { }
  PfArray &operator=(const PfArray &other) { d = other.d; return *this; }
  /** @return true if null-size array (0 rows 0 columns 0 headers) */
  inline bool isNull() const { return !d; }
  /** @return true if no data (0 rows but maybe some headers defined) */
  inline bool isEmpty() const { return d ? d->_rows.isEmpty() : true; }
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
  void setCell(int row, int column, QString value) {
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
  inline void appendHeader(QString value) {
    if (!d)
      d = new PfArrayData();
    d->_headers.append(value);
  }
  inline void appendRow(QStringList values = QStringList()) {
    if (!d)
      d = new PfArrayData();
    d->_rows.append(values);
    for (int i = d->_headers.size(); i < values.size(); ++i)
      d->_headers.append(QString::number(i));
  }
  inline void appendCell(QString value) {
    if (!d)
      d = new PfArrayData();
    if (d->_rows.size() == 0)
      appendRow();
    for (int i = d->_headers.size(); i <= d->_rows.last().size(); ++i)
      d->_headers.append(QString::number(i));
    d->_rows.last().append(value);
  }
  /** convenience method for parser */
  inline void removeLastRowIfEmpty() {
    if (d && d->_rows.size() && d->_rows.last().isEmpty())
      d->_rows.removeLast();
  }
  inline void clear() {
    if (d) {
      d->_headers.clear();
      d->_rows.clear();
    }
  }
  /** Write array content in PF CSV-like format, escaping PF special characters.
    */
  qint64 writePf(QIODevice *target,
                 PfOptions options = PfOptions()) const;
  QString toPf(PfOptions options = PfOptions()) const;
  /** Write array content in HTML-like table-tr-th-td format.
    * @param withHeaders if true include a header line
    */
  qint64 writeTrTd(QIODevice *target, bool withHeaders = true,
                   PfOptions options = PfOptions()) const;
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

#endif // PFARRAY_H
