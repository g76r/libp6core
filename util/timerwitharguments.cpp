/* Copyright 2012-2013 Hallowyn and others.
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
#include "timerwitharguments.h"
#include <QMetaObject>
#include <QtDebug>
#include <QAbstractEventDispatcher>

TimerWithArguments::TimerWithArguments(QObject *parent) : QTimer(parent) {
  connect(this, SIGNAL(timeout()), this, SLOT(forwardTimeout()));
}

void TimerWithArguments::connectWithArgs(
    QObject *object, const char *member, QVariant arg0, QVariant arg1,
    QVariant arg2, QVariant arg3, QVariant arg4, QVariant arg5, QVariant arg6,
    QVariant arg7, QVariant arg8, QVariant arg9) {
  if (object && member) {
    _object = object;
    const char *parenthesis;
    if (*member >= '0' && *member <= '9' && (parenthesis = strchr(member, '(')))
      _member = QString::fromUtf8(member+1, parenthesis-member-1);
    else
      _member = member;
    //qDebug() << "    member" << _member << member;
    _arg[0] = arg0;
    _arg[1] = arg1;
    _arg[2] = arg2;
    _arg[3] = arg3;
    _arg[4] = arg4;
    _arg[5] = arg5;
    _arg[6] = arg6;
    _arg[7] = arg7;
    _arg[8] = arg8;
    _arg[9] = arg9;
  }
}

void TimerWithArguments::forwardTimeout() {
  //qDebug() << "TimerWithArguments::forwardTimeout" << _member
  //         << _arg[0].toString();
  if (_object && !_member.isEmpty()) {
    if (!QMetaObject::invokeMethod(
          _object.data(), _member.toUtf8().constData(), Qt::QueuedConnection,
          _arg[0].isNull() ? QGenericArgument() : Q_ARG(QVariant, _arg[0]),
          _arg[1].isNull() ? QGenericArgument() : Q_ARG(QVariant, _arg[1]),
          _arg[2].isNull() ? QGenericArgument() : Q_ARG(QVariant, _arg[2]),
          _arg[3].isNull() ? QGenericArgument() : Q_ARG(QVariant, _arg[3]),
          _arg[4].isNull() ? QGenericArgument() : Q_ARG(QVariant, _arg[4]),
          _arg[5].isNull() ? QGenericArgument() : Q_ARG(QVariant, _arg[5]),
          _arg[6].isNull() ? QGenericArgument() : Q_ARG(QVariant, _arg[6]),
          _arg[7].isNull() ? QGenericArgument() : Q_ARG(QVariant, _arg[7]),
          _arg[8].isNull() ? QGenericArgument() : Q_ARG(QVariant, _arg[8]),
          _arg[9].isNull() ? QGenericArgument() : Q_ARG(QVariant, _arg[9]))) {
      qWarning() << "cannot signal timer timeout to" << _object.data()
                 << _member << _arg[0] << _arg[1];
    }
  } else {
    qWarning() << "timer timeout occur before target is configured";
  }
}

void TimerWithArguments::singleShot(
    int msec, QObject *receiver, const char *member, QVariant arg0,
    QVariant arg1, QVariant arg2, QVariant arg3, QVariant arg4, QVariant arg5,
    QVariant arg6, QVariant arg7, QVariant arg8, QVariant arg9) {
  if (receiver && member) {
    TimerWithArguments *t =
        new TimerWithArguments(QAbstractEventDispatcher::instance());
    t->setSingleShot(true);
    // this is less optimized than the internal mechanism of QSingleShotTimer
    connect(t, SIGNAL(timeout()), t, SLOT(deleteLater()));
    t->connectWithArgs(receiver, member, arg0, arg1, arg2, arg3, arg4, arg5,
                       arg6, arg7, arg8, arg9);
    if (msec < 0) {
      qDebug() << "TimerWithArguments::singleShot abormal ms" << msec
               << receiver << receiver->metaObject()->className() << member;
    }
    t->setTimerType(Qt::PreciseTimer); // FIXME parametrize
    t->start(msec);
    //qDebug() << "TimerWithArguments::singleShot" << msec << member
    //         << arg0.toString();
  }
}
