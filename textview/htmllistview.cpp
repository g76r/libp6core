#include "htmllistview.h"
#include <QtDebug>

HtmlListView::HtmlListView(QObject *parent) : AsyncTextView(parent) {
}

void HtmlListView::writeHtmlListTree(QAbstractItemModel *m, QString &v,
                            QModelIndex parent, int depth) {
  v.append("<ul>");
  int rows = m->rowCount(parent);
  int columns = m->columnCount(parent);
  qDebug() << "HtmlListView::writeHtmlTree()" << depth << parent << rows
           << columns;
  for (int row = 0; row < rows; ++row) {
    v.append("<li>");
    for (int column = 0; column < columns; ++column) {
      QModelIndex index = m->index(row, column, parent);
      v.append(m->data(index).toString()).append(" ");
    }
    writeHtmlListTree(m, v, m->index(row, 0, parent), depth+1);
  }
  v.append("</ul>");
}

void HtmlListView::updateText() {
  QAbstractItemModel *m = model();
  QString v;
  if (m) {
    writeHtmlListTree(m, v, QModelIndex(), 0);
    v.append("\n");
  }
  //qDebug() << "HtmlListView::updateText()" << m << v << (m?m->rowCount():-1);
  _text = v; // this operation is atomic, therefore htmlPart is thread-safe
}
