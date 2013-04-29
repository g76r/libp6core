/* Copyright 2013 Hallowyn and others.
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
#ifndef TEXTTABLEVIEW_H
#define TEXTTABLEVIEW_H

#include "asynctextview.h"

class LIBQTSSUSHARED_EXPORT TextTableView : public TextView {
  Q_OBJECT
  bool _headersAndFootersAlreadyRead;
  int _maxrows;
  QList<int> _columnIndexes, _effectiveColumnIndexes;
  QList<QString> _rows;
  QString _emptyPlaceholder, _ellipsePlaceholder;

protected:
  QString _header, _footer;

public:
  explicit TextTableView(QObject *parent = 0, int maxrows = 100);
  QString text(ParamsProvider *params = 0,
               QString scope = QString()) const;
  inline QString text(ParamSet params, QString scope = QString()) {
    return text(&params, scope); }
  /** Max number of rows to display. Default is 100. Use INT_MAX if you want
    * no limit. */
  void setMaxrows(int maxrows) { _maxrows = maxrows; }
  /** Set model columns to be displayed.
   * Default: all columns */
  void setColumnIndexes(QList<int> columnIndexes = QList<int>()) {
    _columnIndexes = columnIndexes; _effectiveColumnIndexes = columnIndexes;
    updateHeaderAndFooterText(); }
  /** Text printed if the table is empty.
   * Default: "(empty)" */
  virtual void setEmptyPlaceholder(const QString rawText = "(empty)");
  /** Text printed if the table is truncated to maxrows.
   * Default: "..." */
  virtual void setEllipsePlaceholder(const QString rawText = "...");
  void setModel(QAbstractItemModel *model);

protected:
  const QList<int> effectiveColumnIndexes() const {
    return _effectiveColumnIndexes; }
  void updateText();
  void resetAll();
  void layoutChanged();
  void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
  void rowsRemoved(const QModelIndex &parent, int start, int end);
  void rowsInserted (const QModelIndex &parent, int start, int end);
  virtual void updateHeaderAndFooterText() = 0;
  virtual QString rowText(int row) = 0;

private:
  Q_DISABLE_COPY(TextTableView)
};

#endif // TEXTTABLEVIEW_H
