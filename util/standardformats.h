/* Copyright 2013 Hallowyn and others.
 * This file is part of qron, see <http://qron.hallowyn.com/>.
 * Qron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Qron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with qron. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef STANDARDFORMATS_H
#define STANDARDFORMATS_H

#include "libqtssu_global.h"
#include <QString>
#include <QDateTime>

class StandardFormatsPrivate;
class StandardFormats;
#ifdef STANDARDFORMATS_CPP
static StandardFormats *standardFormatsInstance(void);
#endif

class LIBQTSSUSHARED_EXPORT StandardFormats {
  friend StandardFormats *standardFormatsInstance(void);
private:
  StandardFormatsPrivate *d;
  StandardFormats();
  static inline StandardFormatsPrivate *instance();

public:
  static QString toRfc2822DateTime(QDateTime dt);
  static QDateTime fromRfc2822DateTime(QString rfc2822DateTime,
                                       QString &errorString);
  static QDateTime fromRfc2822DateTime(QString rfc2822DateTime) {
    QString s;
    return fromRfc2822DateTime(rfc2822DateTime, s); }
};

#endif // STANDARDFORMATS_H
