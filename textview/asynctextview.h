/* Copyright 2012 Hallowyn and others.
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
#ifndef ASYNCTEXTVIEW_H
#define ASYNCTEXTVIEW_H

#include "textview.h"
#include <QModelIndex>

/** Asynchronously updating and caching base class for text views.
 * Cache last rendered text string and calls updateText() on model updates.
 * Calls updateText() only once per event loop iteration regardless the number
 * of times update() was called, like QWidget::update() does.
 * @see QAbstractItemView
 */
// LATER provide several update strategies (on change, on request, delayed change)
class LIBQTSSUSHARED_EXPORT AsyncTextView : public TextView {
  Q_OBJECT
protected:
  QString _text;

public:
  explicit AsyncTextView(QObject *parent = 0);
  QString text() const;
  /** Set which model the view will display.
   */
  void setModel(QAbstractItemModel *model);

public slots:
  /** Force update now.
   * This method can be called several times in the same event loop iteration
   * without forcing several updates, like QWidget::update() does.
   * It is automatically connected to model signals when setModel() is called,
   * therefore there should be no need to call update() explicitly. */
  void update();

protected slots:
  virtual void resetAll() = 0;
  /** Default: call resetAll(). */
  virtual void dataChanged(const QModelIndex &topLeft,
                            const QModelIndex &bottomRight);
  /** Default: call resetAll(). */
  virtual void rowsRemoved(const QModelIndex &parent, int start, int end);
  /** Default: call resetAll(). */
  virtual void rowsInserted (const QModelIndex &parent, int start, int end);

protected:
  void customEvent(QEvent *event);
};

#endif // ASYNCTEXTVIEW_H
