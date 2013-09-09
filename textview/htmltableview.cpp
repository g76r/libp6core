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
#include "util/htmlutils.h"

HtmlTableView::HtmlTableView(QObject *parent, int cachedRows, int rowsPerPage)
  : TextTableView(parent, cachedRows, rowsPerPage),
  _pageUrlPrefix("?"),
  _thClassRole(-1), _trClassRole(-1), _tdClassRole(-1), _linkRole(-1),
  _linkClassRole(-1), _htmlPrefixRole(-1), _htmlSuffixRole(-1),
  _rowAnchorColumn(-1),
  _columnHeaders(true), _rowHeaders(false) {
  setEmptyPlaceholder("(empty)");
  setEllipsePlaceholder("...");
}

void HtmlTableView::setEmptyPlaceholder(QString rawText) {
  int columns = effectiveColumnIndexes().size();
  if (rawText.isEmpty())
    TextTableView::setEmptyPlaceholder(QString());
  else
    TextTableView::setEmptyPlaceholder(
          "<tr><td colspan="+QString::number(columns)+">"+rawText
          +"</td></tr>\n");
}

void HtmlTableView::setEllipsePlaceholder(QString rawText) {
  int columns = effectiveColumnIndexes().size();
  if (rawText.isEmpty())
    TextTableView::setEllipsePlaceholder(QString());
  else
    TextTableView::setEllipsePlaceholder(
          "<tr><td colspan="+QString::number(columns)+">"+rawText
          +"</td></tr>\n");
}

void HtmlTableView::updateHeaderAndFooterCache() {
  QString v;
  if (_tableClass.isEmpty())
    v = "<table>\n";
  else
    v = QString("<table class=\"%1\">\n").arg(_tableClass);
  QAbstractItemModel *m = model();
  if (m) {
    if (_columnHeaders) {
      v.append("<tr>");
      if (_rowHeaders)
        v.append("<th>").append(_topLeftHeader).append("</th>");
      foreach (int i, effectiveColumnIndexes()) {
        v.append("<th>");
        if (_htmlPrefixRole >= 0)
          v.append(m->headerData(i, Qt::Horizontal, _htmlPrefixRole)
                   .toString());
        if (_columnHeaderHtmlDisableEncode.contains(i))
          v.append(m->headerData(i, Qt::Horizontal).toString());
        else
          v.append(HtmlUtils::htmlEncode(
                     m->headerData(i, Qt::Horizontal).toString(), true));
        v.append("</th>");
        if (_htmlSuffixRole >= 0)
          v.append(m->headerData(i, Qt::Horizontal, _htmlSuffixRole)
                   .toString());
      }
      v.append("</tr>\n");
    }
  }
  _tableHeader = v;
}

QString HtmlTableView::pageLink(int page, QString pageVariableName,
                                QString pagebarAnchor) const {
  QString s("<li><a href=\""), n(QString::number(page));
  s.append(_pageUrlPrefix).append(pageVariableName).append("=").append(n);
  if (!pagebarAnchor.isEmpty())
    s.append("&anchor=").append(pagebarAnchor);
  s.append("\">").append(n).append("</a></li>");
  return s;
}

QString HtmlTableView::pagebar(
    int currentPage, int lastPage, QString pageVariableName,
    bool defineAnchor) const {
  QString v;
  if (currentPage > 1 || currentPage < lastPage) {
    QString pagebarAnchor("pagebar."+pageVariableName);
    currentPage = qMax(currentPage, 1);
    v.append("<div class=\"pagination pagination-centered\">");
    if (defineAnchor)
      v.append("<a name=\""+pagebarAnchor+"\"></a>");
    v.append("<ul>");
    if (currentPage > 1) {
      v.append(pageLink(1, pageVariableName, pagebarAnchor));
      if (currentPage > 2) {
        if (currentPage > 3)
          v.append("<li class=\"disabled\"><a href=\"#\">...</a></li>");
        v.append(pageLink(currentPage-1, pageVariableName, pagebarAnchor));
      }
    }
    v.append("<li class=\"active\"><a href=\"#\">"+QString::number(currentPage)
             +"</a></li>");
    if (currentPage < lastPage) {
      v.append(pageLink(currentPage+1, pageVariableName, pagebarAnchor));
      if (currentPage < lastPage-1) {
        if (currentPage < lastPage-2)
          v.append("<li class=\"disabled\"><a href=\"#\">...</a></li>");
        v.append(pageLink(lastPage, pageVariableName, pagebarAnchor));
      }
    }
    // LATER if (lastPage > 5) add a form to go to any arbitrary page
    v.append("</ul></div>\n");
  }
  return v;
}

// LATER change the HtmlTableView API to avoid computing pagebar twice per page display

QString HtmlTableView::header(int currentPage, int lastPage,
                              QString pageVariableName) const {
  return pagebar(currentPage, lastPage, pageVariableName, true)+_tableHeader;
}

QString HtmlTableView::footer(int currentPage, int lastPage,
                              QString pageVariableName) const {
  return "</table>\n"+pagebar(currentPage, lastPage, pageVariableName, false);
}
QString HtmlTableView::rowText(int row) {
  static QRegExp notName("[^a-zA-Z0-9\\_]+");
  QAbstractItemModel *m = model();
  if (!m)
    return QString();
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
    if (_rowHeaderHtmlEncode)
      v.append(HtmlUtils::htmlEncode(
                 m->headerData(row, Qt::Vertical).toString(), true));
    else
      v.append(m->headerData(row, Qt::Vertical).toString());
    v.append("</th>");
    if (_htmlSuffixRole >= 0)
      v.append(m->headerData(row, Qt::Vertical, _htmlSuffixRole).toString());
  }
  bool first = true;
  foreach (int column, effectiveColumnIndexes()) {
    QModelIndex index = m->index(row, column, QModelIndex());
    if (_tdClassRole >= 0)
      v.append("<td class=\"")
          .append(m->data(index, _trClassRole).toString())
          .append("\">");
    else
      v.append("<td>");
    if (first) {
      first = false;
      if (!_rowAnchorPrefix.isNull())
          v.append("<a name=\"").append(_rowAnchorPrefix).append(
                m->data(m->index(row, _rowAnchorColumn, QModelIndex()))
                .toString().replace(QRegExp(notName), "_"))
              .append("\"></a>");
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
      v.append(HtmlUtils::htmlEncode(m->data(index).toString(), false));
      v.append("</a>");
    } else {
      if (_dataHtmlDisableEncode.contains(index.column()))
        v.append(m->data(index).toString());
      else
        v.append(HtmlUtils::htmlEncode(m->data(index).toString(), true));
    }
    if (_htmlSuffixRole >= 0)
      v.append(m->data(index, _htmlSuffixRole).toString());
    v.append("</td>");
  }
  v.append("</tr>\n");
  return v;
}
