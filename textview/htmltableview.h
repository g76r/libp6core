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
#ifndef HTMLTABLEVIEW_H
#define HTMLTABLEVIEW_H

#include "texttableview.h"

/** Display the model content as a HTML table. Only rows of the root index
  * are displayed. */
// LATER implement thClassRole and tdClassRole for real
class LIBQTSSUSHARED_EXPORT HtmlTableView : public TextTableView {
  Q_OBJECT
  QString _tableClass, _topLeftHeader;
  int _thClassRole, _trClassRole, _tdClassRole, _linkRole, _linkClassRole;
  int _htmlPrefixRole, _htmlSuffixRole;
  bool _columnHeaders, _rowHeaders;

public:
  explicit HtmlTableView(QObject *parent = 0);
  void setTableClass(const QString tableClass) { _tableClass = tableClass; }
  void setTopLeftHeader(const QString rawHtml) { _topLeftHeader = rawHtml; }
  void setThClassRole(int role) { _thClassRole = role; }
  void setTrClassRole(int role) { _trClassRole = role; }
  void setTdClassRole(int role) { _tdClassRole = role; }
  /** Surround Qt::DisplayRole text with <a href="${linkRole}"> and </a>. */
  void setLinkRole(int role) { _linkRole = role; }
  void setLinkClassRole(int role) { _linkClassRole = role; }
  /** Prefix with unescaped HTML text, e.g. "<img src='icon/foo.png'/>". */
  void setHtmlPrefixRole(int role) { _htmlPrefixRole = role; }
  /** Suffix with unescaped HTML text, e.g. "<a href='help.html'>help</a>". */
  void setHtmlSuffixRole(int role) { _htmlSuffixRole = role; }
  void setColumnHeaders(bool set = true) { _columnHeaders = set; }
  void setRowHeaders(bool set = true) { _rowHeaders = set; }
  void setEmptyPlaceholder(const QString rawText);
  void setEllipsePlaceholder(const QString rawText);

protected:
  QString headerText();
  QString footerText();
  QString rowText(int row);
  Q_DISABLE_COPY(HtmlTableView)
};

#endif // HTMLTABLEVIEW_H
