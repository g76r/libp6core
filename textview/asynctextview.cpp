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
  QAbstractItemModel *prev = this->model();
  if (prev) {
    disconnect(prev, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
               this, SLOT(update()));
    disconnect(prev, SIGNAL(layoutChanged()),
               this, SLOT(update()));
    disconnect(prev, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
               this, SLOT(update()));
    disconnect(prev, SIGNAL(modelReset()), this, SLOT(update()));
    disconnect(prev, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
               this, SLOT(update()));
    disconnect(prev, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
               this, SLOT(update()));
    disconnect(prev, SIGNAL(columnsInserted(const QModelIndex&,int,int)),
               this, SLOT(update()));
    disconnect(prev, SIGNAL(columnsRemoved(const QModelIndex&,int,int)),
               this, SLOT(update()));
    disconnect(prev, SIGNAL(rowsMoved(const QModelIndex&,int,int,const QModelIndex&,int)),
               this, SLOT(update()));
    disconnect(prev, SIGNAL(columnsMoved(const QModelIndex&,int,int,const QModelIndex&,int)),
               this, SLOT(update()));
  }
  if (model) {
    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(update()));
    connect(model, SIGNAL(layoutChanged()),
            this, SLOT(update()));
    connect(model, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
            this, SLOT(update()));
    connect(model, SIGNAL(modelReset()), this, SLOT(update()));
    connect(model, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
            this, SLOT(update()));
    connect(model, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
            this, SLOT(update()));
    connect(model, SIGNAL(columnsInserted(const QModelIndex&,int,int)),
            this, SLOT(update()));
    connect(model, SIGNAL(columnsRemoved(const QModelIndex&,int,int)),
            this, SLOT(update()));
    connect(model, SIGNAL(rowsMoved(const QModelIndex&,int,int,const QModelIndex&,int)),
            this, SLOT(update()));
    connect(model, SIGNAL(columnsMoved(const QModelIndex&,int,int,const QModelIndex&,int)),
            this, SLOT(update()));
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
