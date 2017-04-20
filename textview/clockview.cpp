/* Copyright 2013-2017 Hallowyn, Gregoire Barbier and others.
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
#include "clockview.h"

ClockView::ClockView(QObject *parent, QString format)
  : TextView(parent), _textFormat(format), _dateFormat(Qt::ISODate) {
}

ClockView::ClockView(QObject *parent, Qt::DateFormat format)
  : TextView(parent), _dateFormat(format) {

}

QString ClockView::text(ParamsProvider *params, QString scope) const {
  Q_UNUSED(params)
  Q_UNUSED(scope)
  return _textFormat.isNull()
      ? QDateTime::currentDateTime().toString(_dateFormat)
      : QDateTime::currentDateTime().toString(_textFormat);
}

void ClockView::resetAll() {
}
