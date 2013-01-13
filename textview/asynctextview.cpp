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
               this, SLOT(resetAll()));
    disconnect(m, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
               this, SLOT(resetAll()));
    disconnect(m, SIGNAL(modelReset()), this, SLOT(resetAll()));
    disconnect(m, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
               this, SLOT(rowsInserted(QModelIndex,int,int)));
    disconnect(m, SIGNAL(rowsRemoved(QModelIndex,int,int)),
               this, SLOT(rowsRemoved(QModelIndex,int,int)));
    disconnect(m, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
               this, SLOT(resetAll()));
    disconnect(m, SIGNAL(columnsInserted(const QModelIndex&,int,int)),
               this, SLOT(resetAll()));
    disconnect(m, SIGNAL(columnsRemoved(const QModelIndex&,int,int)),
               this, SLOT(resetAll()));
    disconnect(m, SIGNAL(columnsMoved(const QModelIndex&,int,int,const QModelIndex&,int)),
               this, SLOT(resetAll()));
  }
  if (model) {
    m = model;
    connect(m, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(dataChanged(QModelIndex,QModelIndex)));
    connect(m, SIGNAL(layoutChanged()),
            this, SLOT(resetAll()));
    connect(m, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
            this, SLOT(resetAll()));
    connect(m, SIGNAL(modelReset()), this, SLOT(resetAll()));
    connect(m, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
            this, SLOT(rowsInserted(QModelIndex,int,int)));
    connect(m, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(rowsRemoved(QModelIndex,int,int)));
    connect(m, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
            this, SLOT(resetAll()));
    connect(m, SIGNAL(columnsInserted(const QModelIndex&,int,int)),
            this, SLOT(resetAll()));
    connect(m, SIGNAL(columnsRemoved(const QModelIndex&,int,int)),
            this, SLOT(resetAll()));
    connect(m, SIGNAL(columnsMoved(const QModelIndex&,int,int,const QModelIndex&,int)),
            this, SLOT(resetAll()));
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

void AsyncTextView::rowsInserted (const QModelIndex &parent, int start,
                                  int end) {
  Q_UNUSED(parent)
  Q_UNUSED(start)
  Q_UNUSED(end)
  resetAll();
}
