#include "pfarray.h"
#include "pfinternals.h"
#include <QIODevice>
#include <QBuffer>
#include "pfnode.h"
#include <QtDebug>

qint64 PfArray::writePf(QIODevice *target, const PfOptions options) const {
  Q_UNUSED(options)
  qint64 total = 0, r;
  QString line;
  bool first = true;
  foreach (const QString header, d->_headers) {
    if (first)
      first = false;
    else
      line.append(";");
    line.append(pfescape(header));
  }
  line.append("\n");
  if ((r = target->write(line.toUtf8())) < 0)
    return -1;
  total += r;
  foreach (const QList<QString> row, d->_rows) {
    line.clear();
    first = true;
    foreach (const QString cell, row) {
      if (first)
        first = false;
      else
        line.append(";");
      line.append(pfescape(cell));
    }
    line.append("\n");
    if ((r = target->write(line.toUtf8())) < 0)
      return -1;
    total += r;
  }
  return total;
}

qint64 PfArray::writeTrTd(QIODevice *target, bool withHeaders,
                          const PfOptions options) const {
  Q_UNUSED(options)
  qint64 total = 0, r;
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
  foreach (const QList<QString> row, d->_rows) {
    line = "<tr>";
    foreach (const QString cell, row)
      line.append("<td>").append(pftoxmltext(cell)).append("</td>");
    line.append("</tr>\n");
    if ((r = target->write(line.toUtf8())) < 0)
      return -1;
    total += r;
  }
  if ((r = target->write("</table>\n") < 0))
    return -1;
  return total+r;
}

QString PfArray::toPf(const PfOptions options) const {
  QBuffer buf;
  buf.open(QIODevice::WriteOnly);
  if (writePf(&buf, options) >= 0)
    return QString::fromUtf8(buf.data().constData()); // LATER optimize
  else
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
  foreach (const QList<QString> row, rows()) {
    PfNode n(QString::number(r++));
    int c = 0;
    foreach (const QString cell, row) {
      n.appendChild(PfNode(header(c++), cell));
    }
    target->appendChild(n);
  }
}
