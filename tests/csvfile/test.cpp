/* Copyright 2016 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
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

#include "csv/csvfile.h"
#include <QThread>
#include <QString>
#include <QtDebug>
#include <QCoreApplication>
#include <QDateTime>
#include <QTimer>

int main(int, char **) {
  CsvFile f;
  f.open("./file1.csv", QIODevice::ReadOnly);
  qDebug() << f.columnCount() << f.rowCount() << f.cell(0, 0) << f.cell(0, 1) << f.cell(1, 0) << f.cell(1,1) << f.cell(3,0) << f.cell(3,1) << f.cell(4,0);
}
