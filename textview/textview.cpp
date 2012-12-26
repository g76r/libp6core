#include "textview.h"

TextView::TextView(QObject *parent)
  : QObject(parent), _model(0) {
}

void TextView::setModel(QAbstractItemModel *model) {
  _model = model;
  emit modelChanged();
}
