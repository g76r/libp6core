/* Copyright 2012-2013 Hallowyn and others.
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
#include "asynctextview.h"
#include <QCoreApplication>
#include <QtDebug>
#include <QAbstractItemModel>

#define UPDATE_EVENT (QEvent::Type(QEvent::User+1))

AsyncTextView::AsyncTextView(QObject *parent) : TextView(parent) {
}

QString AsyncTextView::text() const {
  return _text;
}

void AsyncTextView::setModel(QAbstractItemModel *model) {
  QAbstractItemModel *m = this->model();
  if (m) {
    disconnect(m, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
               this, SLOT(dataChanged(QModelIndex,QModelIndex)));
    disconnect(m, SIGNAL(layoutChanged()),
               this, SLOT(layoutChanged()));
    disconnect(m, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
               this, SLOT(headerDataChanged(Qt::Orientation,int,int)));
    disconnect(m, SIGNAL(modelReset()), this, SLOT(resetAll()));
    disconnect(m, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
               this, SLOT(rowsInserted(QModelIndex,int,int)));
    disconnect(m, SIGNAL(rowsRemoved(QModelIndex,int,int)),
               this, SLOT(rowsRemoved(QModelIndex,int,int)));
    disconnect(m, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
               this, SLOT(rowsMoved(QModelIndex,int,int,QModelIndex,int)));
    disconnect(m, SIGNAL(columnsInserted(const QModelIndex&,int,int)),
               this, SLOT(columnsInserted(QModelIndex,int,int)));
    disconnect(m, SIGNAL(columnsRemoved(const QModelIndex&,int,int)),
               this, SLOT(columnsRemoved(QModelIndex,int,int)));
    disconnect(m, SIGNAL(columnsMoved(const QModelIndex&,int,int,const QModelIndex&,int)),
               this, SLOT(columnsMoved(QModelIndex,int,int,QModelIndex,int)));
  }
  if (model) {
    m = model;
    connect(m, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(dataChanged(QModelIndex,QModelIndex)));
    connect(m, SIGNAL(layoutChanged()),
            this, SLOT(layoutChanged()));
    connect(m, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
            this, SLOT(headerDataChanged(Qt::Orientation,int,int)));
    connect(m, SIGNAL(modelReset()), this, SLOT(resetAll()));
    connect(m, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
            this, SLOT(rowsInserted(QModelIndex,int,int)));
    connect(m, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(rowsRemoved(QModelIndex,int,int)));
    connect(m, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
            this, SLOT(rowsMoved(QModelIndex,int,int,QModelIndex,int)));
    connect(m, SIGNAL(columnsInserted(const QModelIndex&,int,int)),
            this, SLOT(columnsInserted(QModelIndex,int,int)));
    connect(m, SIGNAL(columnsRemoved(const QModelIndex&,int,int)),
            this, SLOT(columnsRemoved(QModelIndex,int,int)));
    connect(m, SIGNAL(columnsMoved(const QModelIndex&,int,int,const QModelIndex&,int)),
            this, SLOT(columnsMoved(QModelIndex,int,int,QModelIndex,int)));
  }
  TextView::setModel(model);
  //qDebug() << "AsyncTextView::setModel()" << model;
  update();
}

void AsyncTextView::update() {
  //qDebug() << "AsyncTextView::update()";
  QCoreApplication::postEvent(this, new QEvent(UPDATE_EVENT));
}

void AsyncTextView::customEvent(QEvent *event) {
  if (event->type() == UPDATE_EVENT) {
    QCoreApplication::removePostedEvents(this, UPDATE_EVENT);
    updateText();
  } else {
    TextView::customEvent(event);
  }
}

void AsyncTextView::layoutChanged() {
  resetAll();
}

void AsyncTextView::headerDataChanged(Qt::Orientation orientation, int first,
                                      int last) {
  Q_UNUSED(orientation)
  Q_UNUSED(first)
  Q_UNUSED(last)
  resetAll();
}

void AsyncTextView::dataChanged(const QModelIndex &topLeft,
                                const QModelIndex &bottomRight) {
  Q_UNUSED(topLeft)
  Q_UNUSED(bottomRight)
  resetAll();
}

void AsyncTextView::rowsRemoved(const QModelIndex &parent, int start,
                                int end) {
  Q_UNUSED(parent)
  Q_UNUSED(start)
  Q_UNUSED(end)
  resetAll();
}

void AsyncTextView::rowsInserted(const QModelIndex &parent, int start,
                                 int end) {
  Q_UNUSED(parent)
  Q_UNUSED(start)
  Q_UNUSED(end)
  resetAll();
}

void AsyncTextView::rowsMoved(const QModelIndex &sourceParent, int sourceStart,
                              int sourceEnd,
                              const QModelIndex &destinationParent,
                              int destinationRow) {
  Q_UNUSED(sourceParent)
  Q_UNUSED(sourceStart)
  Q_UNUSED(sourceEnd)
  Q_UNUSED(destinationParent)
  Q_UNUSED(destinationRow)
  resetAll();
}

void AsyncTextView::columnsInserted(const QModelIndex &parent, int start,
                                    int end) {
  Q_UNUSED(parent)
  Q_UNUSED(start)
  Q_UNUSED(end)
  resetAll();
}

void AsyncTextView::columnsRemoved(const QModelIndex &parent, int start,
                                   int end) {
  Q_UNUSED(parent)
  Q_UNUSED(start)
  Q_UNUSED(end)
  resetAll();
}

void AsyncTextView::columnsMoved(const QModelIndex &sourceParent,
                                 int sourceStart, int sourceEnd,
                                 const QModelIndex &destinationParent,
                                 int destinationColumn) {
  Q_UNUSED(sourceParent)
  Q_UNUSED(sourceStart)
  Q_UNUSED(sourceEnd)
  Q_UNUSED(destinationParent)
  Q_UNUSED(destinationColumn)
  resetAll();
}
