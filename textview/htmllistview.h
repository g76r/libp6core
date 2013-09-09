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
#ifndef HTMLLISTVIEW_H
#define HTMLLISTVIEW_H

#include "asynctextview.h"

/** Display the model content as a HTML list, or list of lists to reflect the
 * tree of the model if any. */
// LATER add style options (html classes, ul or ol, icons, columns selection...)
class LIBQTSSUSHARED_EXPORT HtmlListView : public AsyncTextView {
  Q_OBJECT
public:
  explicit HtmlListView(QObject *parent = 0);

protected:
  void resetAll();

private:
  void writeHtmlListTree(QAbstractItemModel *m, QString &v,
                         QModelIndex parent, int depth);
};

#endif // HTMLLISTVIEW_H
