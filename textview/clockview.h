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
#ifndef CLOCKVIEW_H
#define CLOCKVIEW_H

#include "textview.h"

/** Whereas TextView is intended to be used with Qt's Model/View framework,
 * this view is a (trivial) example of a view fully independent of any model.
 */
class LIBP6CORESHARED_EXPORT ClockView: public TextView {
  Q_OBJECT
  Q_DISABLE_COPY(ClockView)

  QString _textFormat;
  Qt::DateFormat _dateFormat;

public:
  ClockView(QObject *parent, QString format);
  explicit ClockView(QObject *parent = 0, Qt::DateFormat format = Qt::ISODate);
  QString text(ParamsProvider *params, QString scope) const override;
  void setFormat(QString format) { _textFormat = format; }
  void setFormat(Qt::DateFormat format) {
    _dateFormat = format; _textFormat = QString(); }
  void resetAll() override;
};

#endif // CLOCKVIEW_H
