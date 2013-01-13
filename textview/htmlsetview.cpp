/* Copyright 2012 Hallowyn and others.
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
#include "htmlsetview.h"

HtmlSetView::HtmlSetView(QObject *parent) : AsyncTextView(parent),
  _separator(" "), _emptyPlaceholder("(empty"),
  _displayedColumn(0), _linkRole(-1), _linkClassRole(-1), _htmlPrefixRole(-1) {
}

void HtmlSetView::resetAll() {
  QAbstractItemModel *m = model();
  QString v;
  //v.append("{ ");
  if (m) {
    int rows = m->rowCount();
    if (rows)
      for (int row = 0; row < rows; ++row) {
        if (row > 0)
          v.append(_separator);
        v.append(_constantPrefix);
        if (_htmlPrefixRole >= 0)
          v.append(m->data(m->index(row, _displayedColumn), _htmlPrefixRole)
                   .toString());
        // LATER escape html special chars
        v.append(m->data(m->index(row, _displayedColumn)).toString());
      }
    else
      v.append(_emptyPlaceholder);
  }
  //v.append(" }");
  _text = v; // this operation is atomic, therefore htmlPart is thread-safe
}
