/* Copyright 2012-2015 Hallowyn and others.
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
#include "htmltableview.h"
#include "util/htmlutils.h"
#include "htmlitemdelegate.h"
#include <QRegularExpression>

QString HtmlTableView::_defaultTableClass;

HtmlTableView::HtmlTableView(QObject *parent, QString objectName,
                             int cachedRows, int rowsPerPage)
  : TextTableView(parent, objectName, cachedRows, rowsPerPage),
  _tableClass(_defaultTableClass), _pageUrlPrefix("?"),
  _rowAnchorColumn(-1), _columnHeaders(true), _rowHeaders(false) {
  setEmptyPlaceholder("(empty)");
  setEllipsePlaceholder("...");
  setItemDelegate(new HtmlItemDelegate(this));
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
    v = QString("<table class=\"%1\" id=\"%2\">\n").arg(_tableClass)
        .arg(objectName());
  QAbstractItemModel *m = model();
  if (m) {
    if (_columnHeaders) {
      v.append("<thead><tr>");
      if (_rowHeaders)
        v.append("<th>").append(_topLeftHeader).append("</th>");
      int displayedColumn = 0;
      foreach (int column, effectiveColumnIndexes()) {
        v.append("<th>");
        TextViewItemDelegate *d =
            itemDelegateForColumnOrDefault(displayedColumn);
        v.append(d ? d->headerText(column, Qt::Horizontal, m)
                   : HtmlUtils::htmlEncode(m->headerData(column, Qt::Horizontal)
                                           .toString(), true));
        v.append("</th>");
        ++displayedColumn;
      }
      v.append("</tr></thead>\n");
    }
  }
  v.append("<tbody>");
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
    QString pagebarAnchor("pagebar-"+pageVariableName);
    currentPage = qMax(currentPage, 1);
    v.append("<div class=\"pagination-frame\">");
    if (defineAnchor)
      v.append("<a name=\""+pagebarAnchor+"\"></a>");
    v.append("<ul class=\"pagination\">");
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
  return "</tbody></table>\n"+pagebar(currentPage, lastPage, pageVariableName, false);
}

static QRegularExpression notNameRE{"[^a-zA-Z0-9\\_]+"};

QString HtmlTableView::rowText(int row) {
  QAbstractItemModel *m = model();
  if (!m)
    return QString();
  QString v, id;
  if (!_rowAnchorPrefix.isNull())
    id = _rowAnchorPrefix
        + m->data(m->index(row, _rowAnchorColumn, QModelIndex()))
        .toString().replace(notNameRE, QStringLiteral("_"));
  v.append("<tr");
  // TODO trClassMapper should go to HtmlItemDelegate (but not TextItemDelegate)
  // the HtmlTableView::setTrClass should stay has wrapper to item delegate
  // we even should add HtmlTableView::setPrefix() and so on as syntactic sugar
  if (!_trClassMapper._text.isEmpty()) {
    QString trClass = _trClassMapper._text;
    if (_trClassMapper._argIndex >= 0) {
      QString arg = m->data(m->index(row, _trClassMapper._argIndex))
          .toString();
      trClass = trClass.arg(_trClassMapper._transcodeMap.isEmpty()
                            ? arg : _trClassMapper._transcodeMap.value(arg));
    }
    v.append(" class=\"").append(trClass).append('"');
  }
  if (!id.isNull())
    v.append(" id=\"").append(id).append('"');
  v.append(">");
  if (_rowHeaders) {
    v.append("<th>");
    TextViewItemDelegate *d = itemDelegateForRowOrDefault(row);
    v.append(d ? d->headerText(row, Qt::Vertical, m)
               : HtmlUtils::htmlEncode(
                   m->headerData(row, Qt::Vertical).toString(), true));
    v.append("</th>");
  }
  bool first = true;
  foreach (int column, effectiveColumnIndexes()) {
    QModelIndex index = m->index(row, column, QModelIndex());
    v.append("<td>");
    if (first) {
      first = false;
      if (!id.isNull())
          v.append("<a name=\"").append(id).append("\"></a>");
    }
    TextViewItemDelegate *d = itemDelegateForCellOrDefault(row, column);
    v.append(d ? d->text(index)
               : HtmlUtils::htmlEncode(index.data().toString(), true));
    v.append("</td>");
  }
  v.append("</tr>\n");
  return v;
}
