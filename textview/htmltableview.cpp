#include "htmltableview.h"
#include <QtDebug>

HtmlTableView::HtmlTableView(QObject *parent) : AsyncTextView(parent),
  _thClassRole(-1), _trClassRole(-1), _tdClassRole(-1), _linkRole(-1),
  _linkClassRole(-1), _htmlPrefixRole(-1) {
  //qDebug() << "HtmlTableView()" << parent;
}

void HtmlTableView::writeHtmlTableTree(QAbstractItemModel *m, QString &v,
                                       QModelIndex parent, int depth) {
  int rows = m->rowCount(parent);
  int columns = m->columnCount(parent);
  //qDebug() << "HtmlTableView::writeHtmlTableTree()" << depth << parent << rows
  //         << columns;
  for (int row = 0; row < rows; ++row) {
    QString trClass;
    if (_trClassRole >= 0)
      trClass = m->data(m->index(row, 0, parent), _trClassRole).toString();
    if (!trClass.isEmpty())
      v.append("<tr class=\"").append(trClass).append("\">");
    else
      v.append("<tr>");
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
    writeHtmlTableTree(m, v, m->index(row, 0, parent), depth+1);
  }
}

void HtmlTableView::updateText() {
  QAbstractItemModel *m = model();
  QString v;
  if (m) {
    if (_tableClass.isEmpty())
      v.append("<table>\n<tr>");
    else
      v.append(QString("<table class=\"%1\">\n<tr>").arg(_tableClass));
    int columns = m->columnCount(QModelIndex());
    for (int i = 0; i < columns; ++i)
      v.append("<th>").append(m->headerData(i, Qt::Horizontal).toString())
          .append("</th>");
    v.append("</tr>\n");
    writeHtmlTableTree(m, v, QModelIndex(), 0);
    v.append("</table>\n");
  }
  //qDebug() << "HtmlTableView::updateText()" << m << v << (m?m->rowCount():-1);
  _text = v; // this operation is atomic, therefore htmlPart is thread-safe
}
