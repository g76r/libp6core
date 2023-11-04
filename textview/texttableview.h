/* Copyright 2013-2023 Hallowyn, Gregoire Barbier and others.
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
#ifndef TEXTTABLEVIEW_H
#define TEXTTABLEVIEW_H

#include "textview.h"
#include "thread/atomicvalue.h"

/** Base class for text table views.
 * @see HtmlTableView
 * @see CsvTableView */
class LIBP6CORESHARED_EXPORT TextTableView : public TextView {
  Q_OBJECT
  Q_DISABLE_COPY(TextTableView)

  int _cachedRows, _rowsPerPage;
  QList<int> _columnIndexes, _effectiveColumnIndexes;
  AtomicValue<QStringList> _rows;
  QString _emptyPlaceholder, _ellipsePlaceholder;

public:
  const static int defaultCachedRows = 100, defaultRowsPerPage = 25;

protected:
  explicit TextTableView(QObject *parent = 0, QString objectName = QString(),
                         int cachedRows = defaultCachedRows,
                         int rowsPerPage = defaultRowsPerPage);

public:
  /** Provide the HTML table view of the model, including headers and footers.
   * If params is set, param named scope+".page" (or "page" if scope is not set)
   * is expected to contain either "disabled" or the current page number, e.g.
   * "42", any unexpected value (e.g. "0", "-37", "foo" or a number > to last
   * page) will be processed as if it were "1".
   * If params is not set, page param is assumed to be "disabled".
   */
  QString text(ParamsProvider *params = 0,
               QString scope = QString()) const override;
  inline QString text(ParamSet params, QString scope = QString()) {
    return text(&params, scope); }
  /** Max number of rows to display. Default is 100. Use -1 if you want
    * no limit. */
  void setCachedRows(int cachedRows) { _cachedRows = cachedRows; }
  /** @see setCachedRows() */
  int cachedRows() const { return _cachedRows; }
  /** Max number of rows to display on one page. Default is 25.
   * Use -1 to disable. */
  void setRowsPerPage(int rowsPerPage) {
    _rowsPerPage = rowsPerPage; updateHeaderAndFooterCache(); }
  /** @see setRowsPerPage() */
  int rowsPerPage() const { return _rowsPerPage; }
  /** Set model columns to be displayed.
   * e.g. view->setColumnIndexes({0,3,4});
   * Default: all columns */
  void setColumnIndexes(QList<int> columnIndexes = QList<int>()) {
    _columnIndexes = columnIndexes; _effectiveColumnIndexes = columnIndexes;
    updateHeaderAndFooterCache(); }
  /** Text printed if the table is empty.
   * Default: no placeholder */
  virtual void setEmptyPlaceholder(QString rawText = QString());
  /** Text printed if the table is truncated.
   * Default: "..." */
  virtual void setEllipsePlaceholder(QString rawText = "...");
  void setModel(QAbstractItemModel *model) override;
  /** Calls dataChanged() with whole table range. */
  void invalidateCache() override;

protected:
  QList<int> effectiveColumnIndexes() const { return _effectiveColumnIndexes; }
  void updateText();
  void resetAll() override;
  void layoutChanged() override;
  void dataChanged(
      const QModelIndex &topLeft, const QModelIndex &bottomRight) override;
  void rowsRemoved(const QModelIndex &parent, int start, int end) override;
  void rowsInserted (const QModelIndex &parent, int start, int end) override;
  /** Update cachable (i.e. not page-related) header and footer data */
  virtual void updateHeaderAndFooterCache() = 0;
  virtual QString rowText(int row) = 0;
  /** Table header, including optionnal page navigation header.
   * This method implementation must be thread-safe.
   * Default: QString() */
  virtual QString header(int currentPage, int lastPage,
                         QString pageVariableName) const;
  /** Table footer, including optionnal page navigation footer.
   * This method implementation must be thread-safe.
   * Default: QString() */
  virtual QString footer(int currentPage, int lastPage,
                         QString pageVariableName) const;
private:
  void doRowsInserted(
      AtomicValue<QStringList>::LockedData &rows,
      const QModelIndex &parent, int start, int end);
};

#endif // TEXTTABLEVIEW_H
