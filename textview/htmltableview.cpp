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
#include "htmltableview.h"

HtmlTableView::HtmlTableView(QObject *parent) : TextTableView(parent),
  _thClassRole(-1), _trClassRole(-1), _tdClassRole(-1), _linkRole(-1),
  _linkClassRole(-1), _htmlPrefixRole(-1),
  _columnHeaders(true), _rowHeaders(false) {
  setEmptyPlaceholder("(empty)");
  setEllipsePlaceholder("...");
}

void HtmlTableView::setEmptyPlaceholder(const QString rawText) {
  QAbstractItemModel *m = model();
  int columns = m ? m->columnCount(QModelIndex()) : 1;
  if (rawText.isEmpty())
    TextTableView::setEmptyPlaceholder(QString());
  else
    TextTableView::setEmptyPlaceholder(
          "<tr><td colspan="+QString::number(columns)+">"+rawText
          +"</td></tr>\n");
}

void HtmlTableView::setEllipsePlaceholder(const QString rawText) {
  QAbstractItemModel *m = model();
  int columns = m ? m->columnCount(QModelIndex()) : 1;
  if (rawText.isEmpty())
    TextTableView::setEllipsePlaceholder(QString());
  else
    TextTableView::setEllipsePlaceholder(
          "<tr><td colspan="+QString::number(columns)+">"+rawText
          +"</td></tr>\n");
}

QString HtmlTableView::headerText() {
  QAbstractItemModel *m = model();
  if (m) {
    QString v;
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
    }
    return v;
  }
  return "<table>";
}

QString HtmlTableView::footerText() {
  return "</table>\n";
}

QString HtmlTableView::rowText(int row) {
  QAbstractItemModel *m = model();
  if (!m)
    return QString();
  int columns = m->columnCount();
  QString v, trClass;
  if (_trClassRole >= 0)
    trClass = m->data(m->index(row, 0, QModelIndex()), _trClassRole).toString();
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
    QModelIndex index = m->index(row, column, QModelIndex());
    if (_tdClassRole >= 0)
      v.append("<td class=\"")
          .append(m->data(index, _trClassRole).toString())
          .append("\">");
    else
      v.append("<td>");
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
  return v;
}
