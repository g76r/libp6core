/* Copyright 2012-2014 Hallowyn and others.
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
#ifndef TEXTVIEW_H
#define TEXTVIEW_H

#include <QObject>
#include <QAbstractItemModel>
#include "libqtssu_global.h"
#include "util/paramset.h"
#include "textviewitemdelegate.h"

/** Class to use Qt's Model-View UI framework for text-oriented display, such
 * as web applications, REST APIs and command line interfaces.
 * Implementing TextView consist of either:
 * <ul><li>just implementing text() method, with neither cache nor any other
 * complex mechanism, which is the easiest way for very simple views;
 * <li>inherit from partial implementations such as AsyncTextView or
 * TextTableView, which already provides some more efficient or more high level
 * mechanisms (caching, pages...);
 * <li>cache content in members data updated by protected slots (such as
 * columnsInserted()) and implement text() to build actual full display data
 * using cached content. */
class LIBQTSSUSHARED_EXPORT TextView : public QObject {
  Q_OBJECT
  QAbstractItemModel *_model;
  TextViewItemDelegate *_defaultDelegate;
  QHash<int,TextViewItemDelegate*> _columnDelegates, _rowDelegates;

public:
  explicit TextView(QObject *parent = 0);
  explicit TextView(QObject *parent, QString objectName);

signals:
  /** Emited when model is changed, e.g. when setModel() is called. */
  void modelChanged();

public slots:
  /** Connect to a given model */
  virtual void setModel(QAbstractItemModel *model);
  QAbstractItemModel *model() const { return _model; }
  /** Set global (not row or column-related) item delegate.
   * Any existing global delegate will be removed,
   * but not deleted. Does not take ownership of delegate.
   * @see QAbstractItemView::setItemDelegate() */
  void setItemDelegate(TextViewItemDelegate *delegate);
  /** @return delegate global delegate, or 0. */
  TextViewItemDelegate *itemDelegate() const;
  /** Set item delegate for a given column.
   * Any existing delegate for this column will be removed,
   * but not deleted. Does not take ownership of delegate.
   * @see QAbstractItemView::setItemDelegateForColumn() */
  void setItemDelegateForColumn(int column, TextViewItemDelegate *delegate);
  /** @return delegate for this column, or 0. */
  TextViewItemDelegate *itemDelegateForColumn(int column) const;
  /** Set item delegate for a given row.
   * Any existing delegate for this row will be removed,
   * but not deleted. Does not take ownership of delegate.
   * @see QAbstractItemView::setItemDelegateForRow() */
  void setItemDelegateForRow(int row, TextViewItemDelegate *delegate);
  /** @return delegate for this row, or 0. */
  TextViewItemDelegate *itemDelegateForRow(int row) const;
  /** Provide the text view of the model, e.g. a HTML string that can be pasted
   * within a HTML page body, a JSON document or an ASCII art string for a
   * text interface.
   * This method must be thread-safe, since it may be called by any thread,
   * e.g. a HTTP server thread.
   * @param params optionnal display params, e.g. request or session params
   * @param scope optionnal scope name, e.g. instance name or URI
   */
  virtual QString text(ParamsProvider *params = 0,
                       QString scope = QString()) const = 0;
  /** Syntaxic sugar. */
  inline QString text(ParamSet params, QString scope = QString()) {
    return text(&params, scope); }
  /** Invalidate all cached data.
   * Default: call resetAll() */
  virtual void invalidateCache();

protected slots:
  /** Recompute the whole view: headers, data, layout... */
  virtual void resetAll() = 0;
  /** Recompute the view part impacted by a layout change.
   * Default: call resetAll(). */
  virtual void layoutChanged();
  /** Recompute the view part impacted by a header data change.
   * Default: call resetAll(). */
  virtual void headerDataChanged(Qt::Orientation orientation, int first,
                                 int last);
  /** Recompute the view part impacted by a data change.
   * Default: call resetAll(). */
  virtual void dataChanged(const QModelIndex &topLeft,
                           const QModelIndex &bottomRight);
  /** Recompute the view part impacted by removing rows.
   * Default: call resetAll(). */
  virtual void rowsRemoved(const QModelIndex &parent, int start, int end);
  /** Recompute the view part impacted by inserting rows.
   * Default: call resetAll(). */
  virtual void rowsInserted (const QModelIndex &parent, int start, int end);
  /** Recompute the view part impacted my moving rows.
   * Default: call resetAll(). */
  virtual void rowsMoved(const QModelIndex &sourceParent, int sourceStart,
                         int sourceEnd, const QModelIndex &destinationParent,
                         int destinationRow);
  /** Recompute the view part impacted my inserting columns.
   * Default: call resetAll(). */
  virtual void columnsInserted(const QModelIndex &parent, int start, int end);
  /** Recompute the view part impacted my removing columns.
   * Default: call resetAll(). */
  virtual void columnsRemoved(const QModelIndex &parent, int start, int end);
  /** Recompute the view part impacted my moving columns.
   * Default: call resetAll(). */
  virtual void columnsMoved(const QModelIndex &sourceParent, int sourceStart,
                            int sourceEnd, const QModelIndex &destinationParent,
                            int destinationColumn);

protected:
  /** @return delegate for this row, or this column, or global delegate, or 0 */
  TextViewItemDelegate *itemDelegateForCellOrDefault(
      int row, int column) const {
    TextViewItemDelegate *d = _rowDelegates.value(row);
    return d ? d : _columnDelegates.value(column, _defaultDelegate); }
  /** @return delegate for this row, or global delegate, or 0 */
  TextViewItemDelegate *itemDelegateForRowOrDefault(int row) const {
    return _rowDelegates.value(row, _defaultDelegate); }
  /** @return delegate for this column, or global delegate, or 0 */
  TextViewItemDelegate *itemDelegateForColumnOrDefault(int column) const {
    return _columnDelegates.value(column, _defaultDelegate); }
};

#endif // TEXTVIEW_H
