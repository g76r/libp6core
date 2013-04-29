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

QString AsyncTextView::text(ParamsProvider *params, QString scope) const {
  Q_UNUSED(params)
  Q_UNUSED(scope)
  return _text;
}

void AsyncTextView::setModel(QAbstractItemModel *model) {
  //qDebug() << "AsyncTextView::setModel()" << model;
  TextView::setModel(model);
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
