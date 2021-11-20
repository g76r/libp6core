/* Copyright 2014-2021 Hallowyn, Gregoire Barbier and others.
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
#include "relativedatetime.h"
#include <QSharedData>
#include <QMutex>
#include <QHash>
#include <QRegularExpression>
#include <QtDebug>
#include <QTimeZone>
#include "format/timeformats.h"

#define TERM_RE "([+-][0-9]+)(ms|mil|s|min|h|d|w|mon|y)[a-z]*"
#define TZ_RE "(?:Z|(?:[+-][0-9]{2}:[0-9]{2}))"
static QRegularExpression _wholeDateRE {
  "\\A(?:([0-9][0-9 T:,.-]*[0-9](" TZ_RE ")?)|([a-zA-Z]+))?((?:" TERM_RE ")*)\\z" };
static QRegularExpression _termRE { TERM_RE };
static QRegularExpression _weekdayRE { "\\A(mon|tue|wed|thu|fri|sat|sun)[a-z]*\\z" };
static QRegularExpression _isoLikeDateRE {
  "\\A(?:(?:(?:([0-9]{4})-)?(?:([0-9]{2})-))?([0-9]{2}))?" };
static QRegularExpression _isoLikeTimeRE {
  "(?:\\A|[T ])([0-9]{2}):([0-9]{2})(?::([0-9]{2})(?:[,.]([0-9]{3}))?)?(" TZ_RE ")?\\z" };

static QMutex _cacheMutex;
static QHash<QString, RelativeDateTime> _cache;

class RelativeDateTimeData : public QSharedData {
  enum ReferenceMethod { Today = 0, DayOfWeek = 1, DayOfMonth, MonthAndDay,
                         ExactDate };
  qint64 _delta; // difference with reference, in milliseconds
  QDate _date; // reference date, interpreted depending on method
  QTime _time; // reference time, interpreted depending on method
  QTimeZone _tz; // reference timezone, interpreted depending on method
  ReferenceMethod _method; // method used to interpret reference date and time
public:
  RelativeDateTimeData() : _delta(0), _method(Today) { }

  RelativeDateTimeData(QString pattern) : _delta(0), _method(Today) {
    if (!pattern.isEmpty()) {
      //qDebug() << "RelativeDateTimeData" << pattern;
      auto m = _wholeDateRE.match(pattern);
      if (m.hasMatch()) {
        QString timestamp = m.captured(1), tz = m.captured(2),
            weekday = m.captured(3).toLower(), terms = m.captured(4);
        m = _weekdayRE.match(weekday);
        if (m.hasMatch()) {
          QString day = m.captured(1);
          _method = DayOfWeek;
          if (day == "thu") {
            _date = QDate(1970, 1, 1); // it was a thursday, trust me
          } else if (day == "fri") {
            _date = QDate(1970, 1, 2);
          } else if (day == "sat") {
            _date = QDate(1970, 1, 3);
          } else if (day == "sun") {
            _date = QDate(1970, 1, 4);
          } else if (day == "mon") {
            _date = QDate(1970, 1, 5);
          } else if (day == "tue") {
            _date = QDate(1970, 1, 6);
          } else if (day == "wed") {
            _date = QDate(1970, 1, 7);
          }
          //qDebug() << "found day of week" << _date;
        } else if (!timestamp.isEmpty()) {
          m = _isoLikeTimeRE.match(timestamp);
          if (m.hasMatch()) {
            int hour = m.captured(1).toInt();
            int minute = m.captured(2).toInt();
            int second = m.captured(3).toInt();
            int ms = m.captured(4).toInt();
            _time = QTime(hour, minute, second, ms);
            //qDebug() << "found time" << _time;
          }
          if (m.capturedStart() != 0) { // either no time of day or time of day not at begining
            m = _isoLikeDateRE.match(timestamp.left(m.capturedStart()-1)); // left(<0) == whole string
            if (m.hasMatch()) {
              int year = m.captured(1).toInt();
              int month = m.captured(2).toInt();
              int day = m.captured(3).toInt();
              _date = QDate(year ? year : 1970, month ? month : 1,
                            day ? day : 1);
              if (year)
                _method = ExactDate;
              else if (month)
                _method = MonthAndDay;
              else if (day)
                _method = DayOfMonth;
              //qDebug() << "found date" << year << month << day << _date << _method;
            } else {
              qDebug() << "RelativeDateTime : invalid timestamp :"
                       << pattern << "date does not match" << timestamp;
              return;
            }
          }
          if (!tz.isEmpty()) {
            _tz = TimeFormats::tzFromIso8601(tz);
            //qDebug() << "RelativeDateTime with timezone" << pattern << tz << _tz;
          }
          /*if (_method != Today) {
          // in case method is not Today, it is more efficient to convert time
          // of day into delta (as if "12:03" was "+12h+3min")
          _delta += _time.msecsSinceStartOfDay();
          _time = QTime();
        }*/
        }
        //qDebug() << "terms" << terms;
        auto it = _termRE.globalMatch(terms);
        while (it.hasNext()) {
          m = it.next();
          qint64 ms = m.captured(1).toInt();
          QString unit = m.captured(2);
          if (unit == "s") {
            ms *= 1000;
          } else if (unit == "min") {
            ms *= 1000*60;
          } else if (unit == "h") {
            ms *= 1000*60*60;
          } else if (unit == "d") {
            ms *= 1000*60*60*24;
          } else if (unit == "w") {
            ms *= 1000*60*60*24*7;
          } else if (unit == "mon") {
            ms *= 1000LL*60*60*24*31;
          } else if (unit == "y") {
            ms *= 1000LL*60*60*24*366;
          }
          // else unit is "ms" or "mil.*"
          _delta += ms;
          //qDebug() << "found term" << ms << unit << _delta;
        }
      } else {
        qDebug() << "RelativeDateTime : invalid pattern :" << pattern
                 << "whole expression does not match";
        return;
      }
    }
  }

  QDateTime apply(QDateTime reference) {
    //qDebug() << "applying" << _method << _date << _time << _delta << "to" << origin;
    QDate date = reference.date();
    switch (_method) {
    case Today:
      // nothing to do
      break;
    case DayOfWeek:
      while (date.dayOfWeek() != _date.dayOfWeek())
        date = date.addDays(-1);
      break;
    case DayOfMonth:
      while (date.day() != _date.day())
        date = date.addDays(-1);
      break;
    case MonthAndDay:
      // LATER optimize (not so easy if day > 28)
      while (date.month() != _date.month() || date.day() != _date.day())
        date = date.addDays(-1);
      break;
    case ExactDate:
      date = _date;
      break;
    }
    if (!_time.isNull() || _method != Today)
      reference.setTime(_time);
    reference.setDate(date); // also sets time to midnight is time is null
    if (_tz.isValid()) {
      //qDebug() << "RelativeDateTime::apply is converting timezones"
      //         << reference << _tz << reference.toTimeZone(_tz);
      //reference = reference.toTimeZone(_tz);
      reference.setTimeZone(_tz);
    }
    reference = reference.addMSecs(_delta);
    //qDebug() << "applied:" << origin;
    return reference;
  }
};

RelativeDateTime::RelativeDateTime() {
}

RelativeDateTime::RelativeDateTime(QString pattern) {
  if (!pattern.isEmpty()) {
    pattern = pattern.trimmed();
    QMutexLocker locker(&_cacheMutex);
    RelativeDateTime cached = _cache.value(pattern);
    locker.unlock();
    if (!cached.isNull())
      d = cached.d;
    else {
      d = new RelativeDateTimeData(pattern);
      locker.relock();
      _cache.insert(pattern, *this);
    }
  }
}

RelativeDateTime::RelativeDateTime(const RelativeDateTime &rhs)
  : d(rhs.d) {
}

RelativeDateTime &RelativeDateTime::operator=(const RelativeDateTime &rhs) {
  if (this != &rhs)
    d.operator=(rhs.d);
  return *this;
}

RelativeDateTime::~RelativeDateTime() {
}

bool RelativeDateTime::isNull() {
  return !d;
}

QDateTime RelativeDateTime::apply(QDateTime reference) {
  return d ? d->apply(reference) : reference;
}
