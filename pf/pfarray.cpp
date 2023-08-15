/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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

#include "pfarray.h"
#include "pfinternals_p.h"
#include "pfnode.h"
#include <QBuffer>

qint64 PfArray::writePf(QIODevice *target, const PfOptions &options) const {
  Q_UNUSED(options)
  qint64 total = 0, r;
  QString line;
  bool first = true;
  if (isNull())
    return 0;
  foreach (const QString header, d->_headers) {
    if (first)
      first = false;
    else
      line.append(";");
    line.append(PfUtils::escape(header, options));
  }
  line.append("\n");
  if ((r = target->write(line.toUtf8())) < 0)
    return -1;
  total += r;
  foreach (const QStringList row, d->_rows) {
    line.clear();
    first = true;
    foreach (const QString cell, row) {
      if (first)
        first = false;
      else
        line.append(";");
      line.append(PfUtils::escape(cell, options));
    }
    line.append("\n");
    if ((r = target->write(line.toUtf8())) < 0)
      return -1;
    total += r;
  }
  return total;
}

qint64 PfArray::writeTrTd(QIODevice *target, bool withHeaders,
                          const PfOptions &options) const {
  Q_UNUSED(options)
  qint64 total = 0, r;
  if (isNull())
    return 0;
  QString line("<table>\n");
  if (withHeaders) {
    line.append("<tr>");
    foreach (const QString header, d->_headers)
      line.append("<th>").append(pftoxmltext(header)).append("</th>");
    line.append("</tr>\n");
    if ((r = target->write(line.toUtf8())) < 0)
      return -1;
    total += r;
  }
  foreach (const QStringList row, d->_rows) {
    line = "<tr>";
    foreach (const QString cell, row)
      line.append("<td>").append(pftoxmltext(cell)).append("</td>");
    line.append("</tr>\n");
    if ((r = target->write(line.toUtf8())) < 0)
      return -1;
    total += r;
  }
  if ((r = target->write("</table>\n")) < 0)
    return -1;
  return total+r;
}

QString PfArray::toPf(const PfOptions &options) const {
  QBuffer buf;
  buf.open(QIODevice::WriteOnly);
  if (writePf(&buf, options) >= 0)
    return QString::fromUtf8(buf.data().constData()); // LATER optimize
  return QString(); // LATER log error?
}

void PfArray::convertToChildrenTree(PfNode *target,
                                    bool keepExistingChildren) const {
  if (!target) {
    qWarning() << "PfArray::convertToChildrenTree(null)";
    return;
  }
  if (!keepExistingChildren && !target->isLeaf()) {
    // LATER optimize
    for (int r = 0; r < rowsCount(); ++r)
      target->removeChildrenByName(QString::number(r));
  }
  int r = 0;
  foreach (const QStringList row, rows()) {
    PfNode n(QString::number(r++));
    int c = 0;
    foreach (const QString cell, row) {
      n.appendChild(PfNode(header(c++), cell));
    }
    target->appendChild(n);
  }
}
