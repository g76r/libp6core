/* Copyright 2014 Hallowyn and others.
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
#ifndef SHAREDUIITEMTREEITEM_H
#define SHAREDUIITEMTREEITEM_H

#include "shareduiitem.h"
#include <QList>

class SharedUiItemTreeItem {
  SharedUiItem _item;
  int _row;
  SharedUiItemTreeItem *_parent;
  QList<SharedUiItemTreeItem*> _children;

public:
  /** Constructor for root items. */
  SharedUiItemTreeItem(SharedUiItem item, int row)
    : _item(item), _row(row), _parent(0) { }
  /** Constructor for non-root items. */
  SharedUiItemTreeItem(SharedUiItem item, SharedUiItemTreeItem *parent)
    : _item(item), _row(parent->childrenCount()), _parent(parent) {
    parent->_children.append(this);
  }
  ~SharedUiItemTreeItem() {
    qDeleteAll(_children);
  }
  SharedUiItem &item() { return _item; }
  int row() const { return _row; }
  SharedUiItemTreeItem *parent() const { return _parent; }
  int childrenCount() const { return _children.size(); }
  SharedUiItemTreeItem *child(int row) const {
    return row >= 0 && row < _children.size() ? _children[row] : 0;
  }
};

#endif // SHAREDUIITEMTREEITEM_H
