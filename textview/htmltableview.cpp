#include "htmltableview.h"
#include <QtDebug>

HtmlTableView::HtmlTableView(QObject *parent) : AsyncTextView(parent) {
  //qDebug() << "HtmlTableView()" << parent;
}

/*inline void writeHtmlUlTree(QAbstractItemModel *m, QString &v,
                            QModelIndex parent, int depth) {
  v.append("<ul>");
  int rows = m->rowCount(parent);
  int columns = m->columnCount(parent);
  qDebug() << "TreeHtmlView::writeHtmlTree()" << depth << parent << rows
           << columns;
  for (int row = 0; row < rows; ++row) {
    v.append("<li>");
    for (int column = 0; column < columns; ++column) {
      QModelIndex index = m->index(row, column, parent);
      v.append(m->data(index).toString()).append(" ");
    }
    writeHtmlUlTree(m, v, m->index(row, 0, parent), depth+1);
  }
  v.append("</ul>");
}*/

void HtmlTableView::writeHtmlTableTree(QAbstractItemModel *m, QString &v,
                                       QModelIndex parent, int depth) {
  int rows = m->rowCount(parent);
  int columns = m->columnCount(parent);
  //qDebug() << "HtmlTableView::writeHtmlTableTree()" << depth << parent << rows
  //         << columns;
  for (int row = 0; row < rows; ++row) {
    v.append("<tr>");
    for (int column = 0; column < columns; ++column) {
      v.append("<td>");
      if (!column) {
        for (int i = 0; i < depth; ++i)
          v.append("&nbsp;&nbsp;");
      }
      QModelIndex index = m->index(row, column, parent);
      v.append(m->data(index).toString());
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
    v.append("<table>\n<tr>");
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
