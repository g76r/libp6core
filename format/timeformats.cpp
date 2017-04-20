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
#include "timeformats.h"
#include <QHash>
#include <QString>
#include <QRegExp>
#include "log/log.h"
#include <QRegularExpression>
#include "util/characterseparatedexpression.h"

Q_GLOBAL_STATIC(TimeFormats, timeFormatsInstance)

class TimeFormatsPrivate {
public:
  QHash<QString,int> _fromDaysOfWeek3;
  QHash<int,QString> _toDaysOfWeek3;
  QHash<QString,int> _fromMonth3;
  QHash<int,QString> _toMonth3;
  // [english-day-of-week3,] day-of-month english-month-name3 year4 hour24:min:sec { {+|-}0000 | zone-name3 }
  // Wed   ,   1  Jan   2013   23:59:62+0400
  // Wed, 01 Jan 2013 23:59:62 GMT
  QRegExp _rfc2822DateTimeRE;
  QRegExp _minusExprRE, _minusTermRE;

  TimeFormatsPrivate()
    : _rfc2822DateTimeRE("(\\s*([a-zA-Z]{3})\\s*,)?" // day of week
                       "\\s*(\\d{1,2})\\s+([a-zA-Z]{3})\\s+(\\d{4})" // date
                       "\\s+(\\d{2}):(\\d{2}):(\\d{2})" // time
                       "\\s*(([+-]\\d{4})|([A-Z]{1,4}))" // timezone
                       "\\s*") {
    _fromDaysOfWeek3.insert("mon", 1);
    _fromDaysOfWeek3.insert("tue", 2);
    _fromDaysOfWeek3.insert("wed", 3);
    _fromDaysOfWeek3.insert("thu", 4);
    _fromDaysOfWeek3.insert("fri", 5);
    _fromDaysOfWeek3.insert("sat", 6);
    _fromDaysOfWeek3.insert("sun", 7);
    _toDaysOfWeek3.insert(1, "Mon");
    _toDaysOfWeek3.insert(2, "Tue");
    _toDaysOfWeek3.insert(3, "Wed");
    _toDaysOfWeek3.insert(4, "Thu");
    _toDaysOfWeek3.insert(5, "Fri");
    _toDaysOfWeek3.insert(6, "Sat");
    _toDaysOfWeek3.insert(7, "Sun");
    _toDaysOfWeek3.insert(0, "Sun");
    _fromMonth3.insert("jan", 1);
    _fromMonth3.insert("fev", 2);
    _fromMonth3.insert("mar", 3);
    _fromMonth3.insert("apr", 4);
    _fromMonth3.insert("may", 5);
    _fromMonth3.insert("jun", 6);
    _fromMonth3.insert("jul", 7);
    _fromMonth3.insert("aug", 8);
    _fromMonth3.insert("sep", 9);
    _fromMonth3.insert("oct", 10);
    _fromMonth3.insert("nov", 11);
    _fromMonth3.insert("dec", 12);
    _toMonth3.insert(1, "Jan");
    _toMonth3.insert(2, "Fev");
    _toMonth3.insert(3, "Mar");
    _toMonth3.insert(4, "Apr");
    _toMonth3.insert(5, "May");
    _toMonth3.insert(6, "Jun");
    _toMonth3.insert(7, "Jul");
    _toMonth3.insert(8, "Aug");
    _toMonth3.insert(9, "Sep");
    _toMonth3.insert(10, "Oct");
    _toMonth3.insert(11, "Nov");
    _toMonth3.insert(12, "Dec");
    _toMonth3.insert(0, "Dec");
  }
};

TimeFormats::TimeFormats() {
  d = new TimeFormatsPrivate;
}

TimeFormatsPrivate *TimeFormats::instance() {
  return timeFormatsInstance()->d;
}

// [english-day-of-week3,] day-of-month english-month-name3 year4 hour24:min:sec { {+|-}0000 | zone-name3 }
// Wed   ,   1  Jan   2013   23:59:62+0400
// Wed, 01 Jan 2013 23:59:62 GMT
QString TimeFormats::toRfc2822DateTime(QDateTime dt) {
  if (dt.isValid()) {
    if (dt.timeSpec() != Qt::UTC)
      dt = dt.toUTC();
    return QString("%1, %2 %3 %4 %5:%6:%7 GMT")
        .arg(instance()->_toDaysOfWeek3.value(dt.date().dayOfWeek()))
        .arg(dt.date().day())
        .arg(instance()->_toMonth3.value(dt.date().month()))
        .arg(dt.date().year(), 4, 10, QChar('0'))
        .arg(dt.time().hour(), 2, 10, QChar('0'))
        .arg(dt.time().minute(), 2, 10, QChar('0'))
        .arg(dt.time().second(), 2, 10, QChar('0'));
  } else
    return QString();
}

QDateTime TimeFormats::fromRfc2822DateTime(QString rfc2822DateTime,
                                           QString *errorString) {
  QRegExp re(instance()->_rfc2822DateTimeRE);
  if (re.exactMatch(rfc2822DateTime)) {
    QString s = re.cap(2);
    int day, month, year, hours, minutes, seconds, tz;
    bool ok;
    if (!s.isEmpty()
        && instance()->_fromDaysOfWeek3.value(s.toLower(), -1) == -1) {
      if (errorString)
        *errorString = "invalid rfc2822 day of week: '"+s+"'";
      //Log::debug() << errorString;
      return QDateTime();
    }
    day = re.cap(3).toInt(&ok);
    if (!ok) {
      if (errorString)
        *errorString = "invalid rfc2822 day of month: '"+re.cap(3)+"'";
      //Log::debug() << errorString;
      return QDateTime();
    }
    s = re.cap(4);
    if ((month = instance()->_fromMonth3.value(s.toLower(), -1)) == -1) {
      if (errorString)
        *errorString = "invalid rfc2822 month: '"+s+"'";
      //Log::debug() << errorString;
      return QDateTime();
    }
    year = re.cap(5).toInt(&ok);
    if (!ok) {
      if (errorString)
        *errorString = "invalid rfc2822 day of year: '"+re.cap(5)+"'";
      //Log::debug() << errorString;
      return QDateTime();
    }
    hours = re.cap(6).toInt(&ok);
    if (!ok || hours > 23 || hours < 0) {
      if (errorString)
        *errorString = "invalid rfc2822 hours: '"+re.cap(6)+"'";
      //Log::debug() << errorString;
      return QDateTime();
    }
    minutes = re.cap(7).toInt(&ok);
    if (!ok || minutes > 59 || minutes < 0) {
      if (errorString)
        *errorString = "invalid rfc2822 minutes: '"+re.cap(7)+"'";
      //Log::debug() << errorString;
      return QDateTime();
    }
    seconds = re.cap(8).toInt(&ok);
    if (!ok || seconds > 62 || seconds < 0) {
      if (errorString)
        *errorString = "invalid rfc2822 seconds: '"+re.cap(8)+"'";
      //Log::debug() << errorString;
      return QDateTime();
    }
    if (seconds > 59)
      seconds = 59; // because QDateTime/QTime doesn't support UTC leap seconds
    tz = INT_MAX;
    tz = re.cap(10).toInt(&ok);
    if (!ok && ((s = re.cap(11)) == "Z" || s == "GMT" || s == "UTC"))
      tz = 0;
    if (tz < -2400 || tz > 2400) {
      if (errorString)
        *errorString = "invalid rfc2822 timezone: '"+re.cap(9)+"'";
      //Log::debug() << errorString;
      return QDateTime();
    }
    //Log::fatal() << "timezone: " << tz << " with: " << re.cap(9) << "|"
    //             << re.cap(10) << "|" << re.cap(11) << "|" << ok;
    // MAYDO accept timestamp w/o timezone and assume GMT
    // MAYDO check consistency of day of week with other fields
    return QDateTime(QDate(year, month, day), QTime(hours, minutes, seconds),
                     Qt::UTC).addSecs(-60*(tz%100)-3600*(tz/100));
  }
  if (errorString)
    *errorString = "invalid rfc2822 timestamp: '"+rfc2822DateTime+"'";
  //Log::debug() << errorString;
  return QDateTime();
}

QString TimeFormats::toCoarseHumanReadableTimeInterval(
    qint64 msecs, bool absolute) {
  QString s(msecs < 0 && !absolute ? "-" : "");
  // LATER i18n
  // LATER hide second part of expression when %2 == 0
  msecs = qAbs(msecs);
  if (msecs <= 60*1000)
    s.append(QObject::tr("%1 seconds", "<= 1 min").arg(.001*msecs));
  else if (msecs <= 60*60*1000)
    s.append(QObject::tr("%1 minutes %2 seconds", "<= 1 h").arg(msecs/(60*1000))
             .arg((msecs/1000)%60));
  else if (msecs <= 24*60*60*1000)
    s.append(QObject::tr("%1 hours %2 minutes", "<= 1 day")
             .arg(msecs/(60*60*1000)).arg((msecs/(60*1000))%60));
  else if (msecs <= 365LL*24*60*60*1000)
    s.append(QObject::tr("%1 days %2 hours", "<= 1 year")
             .arg(msecs/(24*60*60*1000)).arg((msecs/(60*60*1000))%24));
  else
    s.append(QObject::tr("%1 years %2 days", "> 1 year")
             .arg(msecs/(365LL*24*60*60*1000)).arg((msecs/(24*60*60*1000))%365));
  return s;
}

QString TimeFormats::toCoarseHumanReadableRelativeDate(
    QDateTime dt, QDateTime reference) {
  // LATER i18n
  qint64 msecs = reference.msecsTo(dt);
  if (msecs == 0)
    return QObject::tr("now");
  QString s(msecs > 0 ? QObject::tr("in ", "relative date future prefix")
                      : QObject::tr("", "relative date past prefix"));
  s.append(toCoarseHumanReadableTimeInterval(msecs, true));
  s.append(msecs > 0 ? QObject::tr("", "relative date future suffix")
                     : QObject::tr(" ago", "relative date past suffix"));
  return s;
}

/*
#include "util/timeformats.h"
#include "log/log.h"
int main(int argc, char *argv[]) {
  QCoreApplication a(argc, argv);
  QThread::currentThread()->setObjectName("MainThread");
  Log::addConsoleLogger(Log::Debug);
  Log::fatal() << "result: " << TimeFormats::fromRfc2822DateTime(QString())
               << " " << TimeFormats::toRfc2822DateTime(TimeFormats::fromRfc2822DateTime(QString()));
  Log::fatal() << "result: " << TimeFormats::fromRfc2822DateTime("")
               << " " << TimeFormats::toRfc2822DateTime(TimeFormats::fromRfc2822DateTime(""));
  Log::fatal() << "result: " << TimeFormats::fromRfc2822DateTime("garbage")
               << " " << TimeFormats::toRfc2822DateTime(TimeFormats::fromRfc2822DateTime("garbage"));
  Log::fatal() << "result: " << TimeFormats::fromRfc2822DateTime(" Wed   ,   1  Jan   2013   23:40:62+0400")
               << " " << TimeFormats::toRfc2822DateTime(TimeFormats::fromRfc2822DateTime(" Wed   ,   1  Jan   2013   23:40:62+0400"));
  Log::fatal() << "result: " << TimeFormats::fromRfc2822DateTime("Wed, 01 Jan 2013 23:59:62 GMT")
               << " " << TimeFormats::toRfc2822DateTime(TimeFormats::fromRfc2822DateTime("Wed, 01 Jan 2013 23:59:62 GMT"));
  Log::fatal() << "result: " << TimeFormats::fromRfc2822DateTime("01 Jan 2013 23:59:62 GMT")
               << " " << TimeFormats::toRfc2822DateTime(TimeFormats::fromRfc2822DateTime("01 Jan 2013 23:59:62 GMT"));
  Log::fatal() << "result: " << TimeFormats::fromRfc2822DateTime("wEd, 01 JaN 2013 23:59:62+1200")
               << " " << TimeFormats::toRfc2822DateTime(TimeFormats::fromRfc2822DateTime("wEd, 01 JaN 2013 23:59:62+1200"));
  Log::fatal() << "result: " << TimeFormats::fromRfc2822DateTime("wEd, 01 JaN 2013 23:59:62+1201")
               << " " << TimeFormats::toRfc2822DateTime(TimeFormats::fromRfc2822DateTime("wEd, 01 JaN 2013 23:59:62+1201"));
  Log::fatal() << "result: " << TimeFormats::fromRfc2822DateTime("wEd, 01 JaN 2013 23:99:62 GMT")
               << " " << TimeFormats::toRfc2822DateTime(TimeFormats::fromRfc2822DateTime("wEd, 01 JaN 2013 23:99:62 GMT"));
  ::usleep(100000); // give a chance for last asynchronous log writing
  return 0;
}
*/
/*
2013-05-23T10:08:49,597 MainThread/0 : FATAL invalid rfc2822 timestamp: ''
2013-05-23T10:08:49,599 MainThread/0 : FATAL invalid rfc2822 timestamp: ''
2013-05-23T10:08:49,599 MainThread/0 : FATAL result:
2013-05-23T10:08:49,599 MainThread/0 : FATAL invalid rfc2822 timestamp: ''
2013-05-23T10:08:49,599 MainThread/0 : FATAL invalid rfc2822 timestamp: ''
2013-05-23T10:08:49,599 MainThread/0 : FATAL result:
2013-05-23T10:08:49,599 MainThread/0 : FATAL invalid rfc2822 timestamp: 'garbage'
2013-05-23T10:08:49,599 MainThread/0 : FATAL invalid rfc2822 timestamp: 'garbage'
2013-05-23T10:08:49,599 MainThread/0 : FATAL result:
2013-05-23T10:08:49,600 MainThread/0 : FATAL result: 2013-01-01T19:40:59Z Tue, 1 Jan 2013 19:40:59 GMT
2013-05-23T10:08:49,600 MainThread/0 : FATAL result: 2013-01-01T23:59:59Z Tue, 1 Jan 2013 23:59:59 GMT
2013-05-23T10:08:49,600 MainThread/0 : FATAL result: 2013-01-01T23:59:59Z Tue, 1 Jan 2013 23:59:59 GMT
2013-05-23T10:08:49,600 MainThread/0 : FATAL result: 2013-01-01T11:59:59Z Tue, 1 Jan 2013 11:59:59 GMT
2013-05-23T10:08:49,600 MainThread/0 : FATAL invalid rfc2822 timezone: '+1201'
2013-05-23T10:08:49,600 MainThread/0 : FATAL invalid rfc2822 timezone: '+1201'
2013-05-23T10:08:49,600 MainThread/0 : FATAL result:
2013-05-23T10:08:49,601 MainThread/0 : FATAL invalid rfc2822 minutes: '99'
2013-05-23T10:08:49,601 MainThread/0 : FATAL invalid rfc2822 minutes: '99'
2013-05-23T10:08:49,601 MainThread/0 : FATAL result:
*/

/*
#include "util/timeformats.h"
#include "log/log.h"
#include <QThread>
#include <QCoreApplication>
int main(int argc, char *argv[]) {
  QCoreApplication a(argc, argv);
  QThread::currentThread()->setObjectName("MainThread");
  Log::addConsoleLogger(Log::Debug);
  Log::fatal() << TimeFormats::toCoarseHumanReadableRelativeDate(QDateTime::currentDateTime());
  Log::fatal() << TimeFormats::toCoarseHumanReadableRelativeDate(QDateTime::currentDateTime().addMSecs(70));
  Log::fatal() << TimeFormats::toCoarseHumanReadableRelativeDate(QDateTime::currentDateTime().addMSecs(-3200));
  Log::fatal() << TimeFormats::toCoarseHumanReadableRelativeDate(QDateTime::currentDateTime().addMSecs(-322983));
  Log::fatal() << TimeFormats::toCoarseHumanReadableRelativeDate(QDateTime::currentDateTime().addMSecs(-3229839));
  Log::fatal() << TimeFormats::toCoarseHumanReadableRelativeDate(QDateTime::currentDateTime().addMSecs(-32298397));
  Log::fatal() << TimeFormats::toCoarseHumanReadableRelativeDate(QDateTime::currentDateTime().addMSecs(-322983972));
  Log::fatal() << TimeFormats::toCoarseHumanReadableRelativeDate(QDateTime::currentDateTime().addMSecs(-3229839728LL));
  Log::fatal() << TimeFormats::toCoarseHumanReadableRelativeDate(QDateTime::currentDateTime().addMSecs(-32298397284LL));
  Log::fatal() << TimeFormats::toCoarseHumanReadableRelativeDate(QDateTime::currentDateTime().addMSecs(-322983972842LL));
  Log::fatal() << TimeFormats::toCoarseHumanReadableRelativeDate(QDateTime::currentDateTime().addMSecs(-3229839728423LL));
  ::usleep(100000); // give a chance for last asynchronous log writing
  return 0;
}
*/
/*
2013-10-25T20:20:05,854 MainThread/0 : FATAL now
2013-10-25T20:20:05,855 MainThread/0 : FATAL in 0.07 seconds
2013-10-25T20:20:05,855 MainThread/0 : FATAL 3.2 seconds ago
2013-10-25T20:20:05,855 MainThread/0 : FATAL 5 minutes 22 seconds ago
2013-10-25T20:20:05,855 MainThread/0 : FATAL 53 minutes 49 seconds ago
2013-10-25T20:20:05,855 MainThread/0 : FATAL 8 hours 58 minutes ago
2013-10-25T20:20:05,858 MainThread/0 : FATAL 3 days 17 hours ago
2013-10-25T20:20:05,859 MainThread/0 : FATAL 37 days 9 hours ago
2013-10-25T20:20:05,859 MainThread/0 : FATAL 21 years 8 days ago
2013-10-25T20:20:05,859 MainThread/0 : FATAL 219 years 88 days ago
2013-10-25T20:20:05,859 MainThread/0 : FATAL 2195 years 152 days ago
*/

QString TimeFormats::toCustomTimestamp(
    QDateTime dt, QString format, RelativeDateTime relativeDateTime) {
  dt = relativeDateTime.apply(dt);
  if (format.isEmpty()) {
    return dt.toString("yyyy-MM-dd hh:mm:ss,zzz");
  } else {
    if (format == "ms1970") {
      return QString::number(dt.toMSecsSinceEpoch());
    } else if (format == "s1970") {
      return QString::number(dt.toMSecsSinceEpoch()/1000);
    } else {
      return dt.toString(format);
    }
  }
}

QString TimeFormats::toMultifieldSpecifiedCustomTimestamp(
    QDateTime dt, QString multifieldSpecifiedFormat) {
  CharacterSeparatedExpression params(multifieldSpecifiedFormat);
  //qDebug() << "TimeFormats::toMultifieldSpecifiedCustomTimestamp"
  //         << multifieldSpecifiedFormat << params.size() << params;
  QString format = params.value(0).trimmed();
  QString relativedatetime = params.value(1).trimmed().toLower();
  QTimeZone timezone(params.value(2).trimmed().toUtf8());
  if (timezone.isValid())
    dt = dt.toTimeZone(timezone);
  return toCustomTimestamp(dt, format, RelativeDateTime(relativedatetime));
}
