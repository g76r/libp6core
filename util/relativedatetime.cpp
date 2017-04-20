/* Copyright 2014-2017 Hallowyn, Gregoire Barbier and others.
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
#include <QRegExp>
#include <QtDebug>

#define TERM_RE "([+-][0-9]+)(ms|mil|s|min|h|d|w|mon|y)[a-z]*"
static QRegExp wholeDateRE("(?:([0-9][0-9 t:,-]*[0-9])|([a-z]+))?((?:" TERM_RE ")*)");
static QRegExp termRE(TERM_RE);
static QRegExp weekdayRE("(mon|tue|wed|thu|fri|sat|sun)[a-z]*");
static QRegExp isoLikeDateRE("(?:(?:(?:([0-9]{4})-)?(?:([0-9]{2})-))?([0-9]{2}))?");
static QRegExp isoLikeTimeRE("([0-9]{2}):([0-9]{2})(?::([0-9]{2})(?:,([0-9]{3}))?)?");

class RelativeDateTimeData : public QSharedData {
  enum ReferenceMethod { Today = 0, DayOfWeek = 1, DayOfMonth, MonthAndDay,
                         ExactDate };
  qint64 _delta; // difference with reference, in milliseconds
  QDate _date; // reference date, interpreted depending on method
  QTime _time; // reference time, interpreted depending on method
  ReferenceMethod _method; // method used to interpret reference date and time
public:
  RelativeDateTimeData() : _delta(0), _method(Today) { }

  RelativeDateTimeData(QString pattern) : _delta(0), _method(Today) {
    QRegExp re = wholeDateRE;
    if (!pattern.isEmpty()) {
      //qDebug() << "RelativeDateTimeData" << pattern;
      if (re.exactMatch(pattern)) {
        QString timestamp = re.cap(1), weekday = re.cap(2), terms = re.cap(3);
        re = weekdayRE;
        if (!weekday.isEmpty() && re.exactMatch(weekday)) {
          QString day = re.cap(1);
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
          re = isoLikeTimeRE;
          int pos;
          if ((pos = re.indexIn(timestamp)) >= 0) {
            int hour = re.cap(1).toInt();
            int minute = re.cap(2).toInt();
            int second = re.cap(3).toInt();
            int ms = re.cap(4).toInt();
            _time = QTime(hour, minute, second, ms);
            //qDebug() << "found time" << _time;
          }
          if (pos != 0) { // either no time of day or time of day not at begining
            re = isoLikeDateRE;
            if (re.exactMatch(timestamp.left(pos-1))) { // left(<0) == whole string
              int year = re.cap(1).toInt();
              int month = re.cap(2).toInt();
              int day = re.cap(3).toInt();
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
              qDebug() << "RelativeDateTimeData : invalid timestamp :"
                       << pattern;
              return;
            }
          }
          /*if (_method != Today) {
          // in case method is not Today, it is more efficient to convert time
          // of day into delta (as if "12:03" was "+12h+3min")
          _delta += _time.msecsSinceStartOfDay();
          _time = QTime();
        }*/
        }
        //qDebug() << "terms" << terms;
        re = termRE;
        for (int pos = -1; (pos = re.indexIn(terms, pos+1)) >= 0; ) {
          qint64 ms = re.cap(1).toInt();
          QString unit = re.cap(2);
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
        qDebug() << "RelativeDateTimeData : invalid pattern :" << pattern;
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
    reference.setDate(date);
    if (!_time.isNull())
      reference.setTime(_time);
    reference = reference.addMSecs(_delta);
    //qDebug() << "applied:" << origin;
    return reference;
  }
};

RelativeDateTime::RelativeDateTime() {
}

static QMutex mutex;
static QHash<QString, RelativeDateTime> cache;

RelativeDateTime::RelativeDateTime(QString pattern) {
  if (!pattern.isEmpty()) {
    pattern = pattern.trimmed().toLower();
    QMutexLocker locker(&mutex);
    RelativeDateTime cached = cache.value(pattern);
    locker.unlock();
    if (!cached.isNull())
      d = cached.d;
    else {
      d = new RelativeDateTimeData(pattern);
      locker.relock();
      cache.insert(pattern, *this);
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
