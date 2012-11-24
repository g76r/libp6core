#ifndef PFARRAY_H
#define PFARRAY_H

#include <QString>
#include <QList>
#include <QSharedData>
#include "pfoptions.h"

class QIODevice;
class PfNode;

class PfArrayData : public QSharedData {
  friend class PfArray;
private:
  QList<QString> _headers;
  QList<QList<QString> > _rows;

public:
  inline PfArrayData() { }
  inline PfArrayData(const PfArrayData &other) : QSharedData(),
    _headers(other._headers), _rows(other._rows) { }
};

class PfArray {
private:
  QSharedDataPointer<PfArrayData> d;

public:
  inline PfArray() : d(new PfArrayData()) {  }
  inline PfArray(const PfArray &other) : d(other.d) { }
  /** @return true if null-size array (0 rows 0 columns 0 headers)
    */
  inline bool isNull() const { return d->_headers.isEmpty(); }
  /** @return true if no data (0 rows but maybe some headers defined)
    */
  inline bool isEmpty() const { return d->_rows.isEmpty(); }
  int columnsCount() const { return d->_headers.size(); }
  /** do not include headers
    */
  int rowsCount() const { return d->_rows.size(); }
  const QList<QString> headers() const { return d->_headers; }
  /** @param column 0 for first column
    */
  QString header(int column) const {
    return column < d->_headers.size() ? d->_headers.at(column) : QString(); }
  /** do not include headers
    */
  const QList<QList<QString> > rows() const { return d->_rows; }
  /** @param row 0 for first row, not including headers
    */
  const QList<QString> row(int row) const {
    return row < d->_rows.size() ? d->_rows.at(row) : QList<QString>(); }
  /** @param row 0 for first row, not including headers
    * @param column 0 for first column
    * @return QString() if indexes are out of range, QString("") if empty
    */
  const QString cell(int row, int column) const {
    if (row >= d->_rows.size())
      return QString();
    const QList<QString> &r(d->_rows.at(row));
    if (column >= r.size())
      return "";
    return r.at(column);
  }
  /** set value in a given cell, autoenlarging array if indexes are out of range
    * @param row 0 for first row, not including headers
    * @param column 0 for first column
    */
  void setCell(int row, int column, QString value) {
    for (int i = d->_headers.size(); i < column; ++i)
      d->_headers.append(QString::number(i));
    for (int i = d->_rows.size(); i < row; ++i)
      d->_rows.append(QList<QString>());
    QList<QString> &r(d->_rows[row]);
    for (int i = r.size(); i < column; ++i)
      r.append(QString());
    r[column] = value;
  }
  inline void appendHeader(QString value) {
    d->_headers.append(value);
  }
  inline void appendRow(QList<QString> values = QList<QString>()) {
    d->_rows.append(values);
    for (int i = d->_headers.size(); i < values.size(); ++i)
      d->_headers.append(QString::number(i));
  }
  inline void appendCell(QString value) {
    if (d->_rows.size() == 0)
      appendRow();
    for (int i = d->_headers.size(); i <= d->_rows.last().size(); ++i)
      d->_headers.append(QString::number(i));
    d->_rows.last().append(value);
  }
  /** convenience method for parser
    */
  inline void removeLastRowIfEmpty() {
    if (d->_rows.size() && d->_rows.last().isEmpty())
      d->_rows.removeLast();
  }

  inline void clear() {
    d->_headers.clear();
    d->_rows.clear();
  }
  /** Write array content in PF CSV-like format, escaping PF special characters.
    */
  qint64 writePf(QIODevice *target,
                 const PfOptions options = PfOptions()) const;
  QString toPf(const PfOptions options = PfOptions()) const;
  /** Write array content in HTML-like table-tr-th-td format.
    * @param withHeaders if true include a header line
    */
  qint64 writeTrTd(QIODevice *target, bool withHeaders = true,
                   const PfOptions options = PfOptions()) const;
  /** Fill the target node (for instance the node containing this array) with
    * one child per row, named after the row number (begining with 0) and
    * itself having one child per cell, named after the column number and
    * containing the cell content.
    * By default (keepExistingChildren = false), target children having a number
    * name between 0 and rowsCount() are removed before being recreated.
    * Otherwise duplicates will be allowed.
    */
  void convertToChildrenTree(PfNode *target,
                             bool keepExistingChildren = false) const;
};

#endif // PFARRAY_H
