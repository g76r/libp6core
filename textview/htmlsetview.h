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
#ifndef HTMLSETVIEW_H
#define HTMLSETVIEW_H

#include "asynctextview.h"

/** Display the model content as a HTML inline list. */
// LATER provide an alternative formating with <ul> or <ol>
// LATER implement linkRole and linkClassRole for real
// LATER implement maximum size + ellipsePlaceholder
class LIBQTSSUSHARED_EXPORT HtmlSetView : public AsyncTextView {
  Q_OBJECT
  QString _separator, _constantPrefix, _emptyPlaceholder;
  int _displayedColumn, _linkRole, _linkClassRole, _htmlPrefixRole;

public:
  explicit HtmlSetView(QObject *parent = 0);
  /** Separator between values, default is a single space. */
  void setSeparator(QString rawHtml) { _separator = rawHtml; }
  /** Prefix before a given value. Printed before HtmlPrefixRole prefix if any.
   * Default is empty.
   */
  void setConstantPrefix(QString rawHtml) { _constantPrefix = rawHtml; }
  /** Text printed if the set is empty. Default is "(empty)". */
  void setEmptyPlaceholder(QString rawHtml) { _emptyPlaceholder = rawHtml; }
  void setDisplayedColumn(int column) { _displayedColumn = column; }
  /** Surround Qt::DisplayRole text with <a href="${linkRole}"> and </a>. */
  void setLinkRole(int role) { _linkRole = role; }
  void setLinkClassRole(int role) { _linkClassRole = role; }
  /** Prefix with unescaped HTML text, e.g. "<img src='icon/foo.png'/>". */
  void setHtmlPrefixRole(int role) { _htmlPrefixRole = role; }

protected:
  void resetAll();
};

#endif // HTMLSETVIEW_H
