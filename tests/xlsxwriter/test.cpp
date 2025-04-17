/* Copyright 2023 Hallowyn, Gregoire Barbier and others.
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

#include <QtDebug>
#include "format/xlsxwriter.h"
#include <functional>
#include <string>
#include "log/log.h"
#include <unistd.h>
#include <QDate>

int main(int, char **) {
  p6::log::init();
  p6::log::addConsoleLogger(p6::log::Debug, true, stdout);
  p6::log::debug() << "test";
  qDebug() << "foo";
  QDate date(2023,1,1);
  QTime time(20,25,38);
  QDateTime dt(date,time);
  XlsxWriter sw("/tmp/xlsxwriter_test", false);
  sw.appendRow(QVariantList{"a","b","c",8,3.14,-3.141592168,QDate(2023,1,1)},
               "One");
  sw.appendRow(QVariantList{"A","B'D","<X>","1×1"}, "Two");
  sw.appendRow(QVariantList{"a é§ €≙☔ 𐐝𐓹🥨"}, "Two");
  sw.appendRow(QVariantList{date, time, dt}, "Two");
  sw.appendRow(QVariantList{QDateTime::fromMSecsSinceEpoch(1672527600'000)}, "Two");
  sw.appendRow(QVariantList{QDateTime::fromString("2023-01-01 20:25:38", Qt::ISODate)}, "Two");
  sw.appendRow(QVariantList{QDateTime::fromString("2023-08-01 20:25:38", Qt::ISODate)}, "Two");
  sw.appendRow(QVariantList{"a","b","z",QVariant(true)}, "One");
  sw.appendRow(QVariantList{"'a","   a",QVariant(),"2",2,2.0}, "One");
  sw.write("output.xlsx");
  XlsxWriter sw2("/tmp/xlsxwriter_test2", false);
  auto sheet_title = "SheetNameFar🥨ver31CharactersWhichIsTheMaximumAllowed";
  qDebug() << "rowcount:" << sw2.rowCount(sheet_title) << "== 0";
  sw2.appendRow({"foo", "bar"}, sheet_title);
  sw2.appendRow({"foo", "bar"}, sheet_title);
  sw2.appendRow({"foo", "bar"}, sheet_title);
  qDebug() << "rowcount:" << sw2.rowCount(sheet_title) << "== 3";
  sw2.write("output2.xlsx");
  ::usleep(1'000'000);
  return 0;
}
