#include "csvview.h"
#include <QtDebug>

CsvView::CsvView(QObject *parent) : AsyncTextView(parent) {
}

void CsvView::writeCsvTree(QAbstractItemModel *m, QString &v,
                                       QModelIndex parent, int depth) {
  int rows = m->rowCount(parent);
  int columns = m->columnCount(parent);
  //qDebug() << "CsvView::writeCsvTree()" << depth << parent << rows << columns;
  for (int row = 0; row < rows; ++row) {
    for (int column = 0; column < columns; ++column) {
      if (!column) {
        for (int i = 0; i < depth; ++i)
          v.append(" ");
      }
      QModelIndex index = m->index(row, column, parent);
      v.append(m->data(index).toString());
      if (column < columns-1)
        v.append(";");
    }
    v.append("\n");
    writeCsvTree(m, v, m->index(row, 0, parent), depth+1);
  }
}

void CsvView::updateText() {
  QAbstractItemModel *m = model();
  QString v;
  if (m) {
    int columns = m->columnCount(QModelIndex());
    for (int i = 0; i < columns; ++i) {
      v.append(m->headerData(i, Qt::Horizontal).toString());
      if (i < columns-1)
        v.append(";");
    }
    v.append("\n");
    writeCsvTree(m, v, QModelIndex(), 0);
  }
  //qDebug() << "CsvView::updateText()" << m << v << (m?m->rowCount():-1);
  _text = v; // this operation is atomic, therefore htmlPart is thread-safe
}
