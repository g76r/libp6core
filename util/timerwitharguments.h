/* Copyright 2012-2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef TIMERWITHARGUMENTS_H
#define TIMERWITHARGUMENTS_H

#include "libp6core_global.h"
#include <QTimer>
#include <QVariant>
#include <QPointer>

/** TimerWithArguments replaces QTimer when the called method needs to
 * receive arguments when the timer times out.
 *
 * Example:
 * TimerWithArguments::singleShot(200, this, "myMethod", QString("foo"));
 *
 * Rationale:
 * This class is slightly less optimized than QTimer is, because QTimer
 * relies on QObject low level mechanisms whereas TimerWithArguments uses
 * regular QMetaObject (signals/slots and invokeMethod) mechanisms. In the
 * other hand, using QTimer needs the receiver class to hold the context
 * by itself since QTimer does not transmit it. */
class LIBP6CORESHARED_EXPORT TimerWithArguments : public QTimer {
  Q_OBJECT
  Q_DISABLE_COPY(TimerWithArguments)

  QPointer<QObject> _object;
  QString _member;
  QVariant _arg[10];

public:
  explicit TimerWithArguments(QObject *parent = 0);
  /** @param member is either the raw method name (e.g. "foo") either a SLOT
    * macro (e.g. SLOT(foo(QVariant,QVariant))) */
  void connectWithArgs(QObject *object, const char *member,
                       QVariant arg0 = QVariant(), QVariant arg1 = QVariant(),
                       QVariant arg2 = QVariant(), QVariant arg3 = QVariant(),
                       QVariant arg4 = QVariant(), QVariant arg5 = QVariant(),
                       QVariant arg6 = QVariant(), QVariant arg7 = QVariant(),
                       QVariant arg8 = QVariant(), QVariant arg9 = QVariant());
  static void singleShot(int msec, QObject *receiver, const char *member,
                         QVariant arg0 = QVariant(), QVariant arg1 = QVariant(),
                         QVariant arg2 = QVariant(), QVariant arg3 = QVariant(),
                         QVariant arg4 = QVariant(), QVariant arg5 = QVariant(),
                         QVariant arg6 = QVariant(), QVariant arg7 = QVariant(),
                         QVariant arg8 = QVariant(),
                         QVariant arg9 = QVariant());
  // LATER
  //Qt::TimerType	timerType() const;
  //void	setTimerType(Qt::TimerType atype);

private slots:
  void forwardTimeout();
};

#endif // TIMERWITHARGUMENTS_H
