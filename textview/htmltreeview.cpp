/* Copyright 2012-2013 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
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
#include "htmltreeview.h"
#include <QtDebug>

HtmlTreeView::HtmlTreeView(QObject *parent) : AsyncTextView(parent),
  _emptyPlaceholder("(empty)"), _ellipsePlaceholder("..."),
  _thClassRole(-1), _trClassRole(-1), _tdClassRole(-1), _linkRole(-1),
  _linkClassRole(-1), _htmlPrefixRole(-1),
  _columnHeaders(true), _rowHeaders(false),
  _maxrows(100) {
}


void HtmlTreeView::writeHtmlTableTree(QAbstractItemModel *m, QString &v,
                                       QModelIndex parent, int depth,
                                       int &totalRaws) {
  int rows = m->rowCount(parent);
  int columns = m->columnCount(parent);
  //qDebug() << "HtmlTableView::writeHtmlTableTree()" << depth << parent << rows
  //         << columns;
  for (int row = 0; row < rows && totalRaws < _maxrows; ++row, ++totalRaws) {
    QString trClass;
    if (_trClassRole >= 0)
      trClass = m->data(m->index(row, 0, parent), _trClassRole).toString();
    if (!trClass.isEmpty())
      v.append("<tr class=\"").append(trClass).append("\">");
    else
      v.append("<tr>");
    if (_rowHeaders) {
      v.append("<th>");
      if (_htmlPrefixRole >= 0)
        v.append(m->headerData(row, Qt::Vertical, _htmlPrefixRole).toString());
      v.append(m->headerData(row, Qt::Vertical).toString()).append("</th>");
    }
    for (int column = 0; column < columns; ++column) {
      QModelIndex index = m->index(row, column, parent);
      if (_tdClassRole >= 0)
        v.append("<td class=\"")
            .append(m->data(index, _trClassRole).toString())
            .append("\">");
      else
        v.append("<td>");
      if (!column) {
        for (int i = 0; i < depth; ++i)
          v.append("&nbsp;&nbsp;");
      }
      if (_htmlPrefixRole >= 0)
        v.append(m->data(index, _htmlPrefixRole).toString());
      QString link;
      if (_linkRole >= 0)
        link = m->data(index, _linkRole).toString();
      if (!link.isEmpty()) {
        v.append("<a href=\"").append(link);
        if (_linkClassRole >= 0)
          v.append("\" class=\"")
              .append(m->data(index, _linkClassRole).toString())
              .append("\">");
        else
          v.append("\">");
      }
      v.append(m->data(index).toString());
      if (!link.isEmpty())
        v.append("</a>");
      v.append("</td>");
    }
    v.append("</tr>\n");
    QModelIndex index = m->index(row, 0, parent);
    writeHtmlTableTree(m, v, index, depth+1, totalRaws);
  }
}

void HtmlTreeView::resetAll() {
  QAbstractItemModel *m = model();
  QString v;
  int totalRaws = 0;
  if (m) {
    int columns = m->columnCount(QModelIndex());
    if (_tableClass.isEmpty())
      v.append("<table>\n");
    else
      v.append(QString("<table class=\"%1\">\n").arg(_tableClass));
    if (_columnHeaders) {
      v.append("<tr>");
      if (_rowHeaders)
        v.append("<th>").append(_topLeftHeader).append("</th>");
      for (int i = 0; i < columns; ++i) {
        v.append("<th>");
        if (_htmlPrefixRole >= 0)
          v.append(m->headerData(i, Qt::Horizontal, _htmlPrefixRole)
                   .toString());
        v.append(m->headerData(i, Qt::Horizontal).toString()).append("</th>");
      }
      v.append("</tr>\n");
      //qDebug() << "headers written:" << v;
    }
    if (!_emptyPlaceholder.isEmpty() && m->rowCount(QModelIndex()) == 0)
      v.append("<tr><td colspan=").append(QString::number(columns)).append(">")
          .append(_emptyPlaceholder).append("</td>");
    else
      writeHtmlTableTree(m, v, QModelIndex(), 0, totalRaws);
    // LATER don't print ellipse place holder if _maxraws is *exactly* reached
    if (totalRaws == _maxrows) {
      v.append("<tr><td colspan=").append(QString::number(columns)).append(">")
          .append(_ellipsePlaceholder).append("</td>");
      //for (int i = 0; i < columns; ++i)
      //  v.append("<td></td>");
      v.append("</tr>\n");
    }
    v.append("</table>\n");
  }
  //qDebug() << "HtmlTableView::updateText()" << m << v << (m?m->rowCount():-1);
  _text = v; // this operation is atomic, therefore htmlPart is thread-safe
}
