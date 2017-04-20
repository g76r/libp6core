/* Copyright 2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef ABSTRACTTEXTFORMATTER_H
#define ABSTRACTTEXTFORMATTER_H

#include "libp6core_global.h"
#include <QAbstractItemModel>
#include <QModelIndex>
#include "modelview/shareduiitemlist.h"

/** Convenience shared feature for text formatters. */
class LIBPUMPKINSHARED_EXPORT AbstractTextFormatter {
  QString _topLeftHeader;
  int _maxCellContentLength;
  bool _rowHeadersEnabled, _columnHeadersEnabled;
  static int _defaultMaxCellContentLength;

public:
  explicit AbstractTextFormatter(
      int maxCellContentLength, bool rowHeadersEnabled = false,
      bool columnHeadersEnabled = true)
    : _maxCellContentLength(maxCellContentLength),
      _rowHeadersEnabled(rowHeadersEnabled),
      _columnHeadersEnabled(columnHeadersEnabled) { }
  AbstractTextFormatter()
    : AbstractTextFormatter(_defaultMaxCellContentLength) { }
  virtual ~AbstractTextFormatter();
  void setTopLeftHeader(QString rawText) { _topLeftHeader = rawText; }
  QString topLeftHeader() const { return _topLeftHeader; }
  void enableColumnHeaders(bool enabled = true) {
    _columnHeadersEnabled = enabled; }
  bool columnHeadersEnabled() const { return _columnHeadersEnabled; }
  void enableRowHeaders(bool enabled = true) {
    _rowHeadersEnabled = enabled; }
  bool rowHeadersEnabled() const { return _rowHeadersEnabled; }
  /** Maximum length of text inside a cell, measured before surface encoding if
   * any (e.g. before HTML entities escaping).
   * Use -1 to disable.
   * Default: 200. */
  void setMaxCellContentLength(int maxCellContentLength = 200) {
    _maxCellContentLength = maxCellContentLength; }
  int maxCellContentLength() const { return _maxCellContentLength; }
  /** Maximum length of text inside a cell, measured before surface encoding if
   * any (e.g. before HTML entities escaping).
   * Use -1 to disable.
   * Default: 200. */
  static void setDefaultMaxCellContentLength(int length) {
    _defaultMaxCellContentLength = length; }
  static int defaultMaxCellContentLength() {
    return _defaultMaxCellContentLength; }
  /** Format an item as a row.
   * If column headers are never written, being them enabled or not.
   * If row headers are enabled, item.id() is used as a row header but if role
   * is HeaderDisplayRole, in which case topLeftHeader is used insted. */
  QString formatRow(
      const SharedUiItem &item, int role = Qt::DisplayRole) const {
    return formatRowInternal(item, role, QString()); }
  /** Format items in a list as a table.
   * If column headers are enabled, a header row is added first (even if list
   * is empty), using item's section names.
   * If row headers are enabled, index in list is used (starting from 1). */
  QString formatTable(
      const SharedUiItemList<> &list, int role = Qt::DisplayRole) const;
  /** Format a row.
   * If column headers are never written, being them enabled or not.
   * If row headers are enabled, model's horizontal header is used. */
  QString formatRow(const QAbstractItemModel *model, int row,
                    const QModelIndex &parent = QModelIndex(),
                    int role = Qt::DisplayRole) const {
    return formatRowInternal(model, row, parent, role, QString()); }
  /** Format the row to which index belongs.
   * If column headers are never written, being them enabled or not.
   * If row headers are enabled, model's horizontal header is used. */
  QString formatRow(const QModelIndex &index,
                    int role = Qt::DisplayRole) const {
    return formatRow(index.model(), index.row(), index.parent(), role); }
  /** Format the rows under parent index as a table.
   * If lastRow is -1 or > to rowCount() every row is formatted until last one.
   * If column headers are enabled, a header row is added first (even if there
   * are no data rows), using model's horizontal headers.
   * If row headers are enabled, model's vertical header is used. */
  QString formatTable(
      const QAbstractItemModel *model, int firstRow = 0, int lastRow = -1,
      const QModelIndex &parent = QModelIndex(),
      int role = Qt::DisplayRole) const;
  /** Format the table header.
   * If column headers are enabled, the header may include a header row,
   * otherwise only outputs static part of the header (e.g. <table> for html).
   * If row headers are enabled, topLeftHeader is used.
   * When this method is called by other forms (specialized for
   * QAbstractItemModel* and the like) it can be called with an empty list of
   * header when column headers are disabled, for performance purposes (fetching
   * header names whereas they won't be formatted is not needed). */
  virtual QString formatTableHeader(const QStringList &columnHeaders) const = 0;
  /** Format the table footer.
   * If column headers are enabled, the footer may include header information,
   * otherwise only outputs static part of the footer (e.g. </table> for html).
   * If row headers are enabled, topLeftHeader is used.
   * When this method is called by other forms (specialized for
   * QAbstractItemModel* and the like) it can be called with an empty list of
   * header when column headers are disabled, for performance purposes (fetching
   * header names whereas they won't be formatted is not needed). */
  virtual QString formatTableFooter(const QStringList &columnHeaders) const = 0;
  /** Apply surface encoding to cell content (e.g. escape special characters,
   * add quotes when needed). */
  virtual QString formatCell(QString rawData) const = 0;
  /** Apply surface encoding to cell content (e.g. escape special characters,
   * add quotes when needed). */
  void formatCell(QString *data) const { *data = formatCell(*data); }
  /** Format the table header.
   * If row headers are enabled, topLeftHeader is used. */
  virtual QString formatRow(const QStringList &cells,
                            QString rowHeader = QString()) const = 0;

protected:
  void fetchHeaderList(QStringList *headers, const SharedUiItem &item) const;
  void fetchHeaderList(QStringList *headers, const QAbstractItemModel *model,
                       const QModelIndex &parent = QModelIndex(),
                       int role = Qt::DisplayRole) const ;

private:
  /** Format a row, using model's vertical header if rowHeader.isNull() */
  QString formatRowInternal(const QAbstractItemModel *model, int row,
                            const QModelIndex &parent, int role,
                            QString rowHeader) const {
    if (!model)
      return QString();
    QStringList cells;
    int columns = model->columnCount(parent);
    for (int column = 0; column < columns; ++column)
      cells.append(
            model->index(row, column, parent).data(role).toString());
    return formatRow(cells, rowHeader);
  }
  /** Format a row, using item's id if rowHeader.isNull() */
  QString formatRowInternal(const SharedUiItem &item, int role,
                            QString rowHeader) const {
    QStringList cells;
    int n = item.uiSectionCount();
    for (int i = 0; i < n; ++i)
      cells.append(item.uiString(i, role));
    return formatRow(cells, rowHeader);
  }
};

#endif // ABSTRACTTEXTFORMATTER_H
