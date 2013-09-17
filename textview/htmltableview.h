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
  Q_DISABLE_COPY(HtmlTableView)

  QString _tableClass, _topLeftHeader, _rowAnchorPrefix, _tableHeader,
  _pageUrlPrefix;
  int _thClassRole, _trClassRole, _tdClassRole, _linkRole, _linkClassRole;
  int _htmlPrefixRole, _htmlSuffixRole, _rowAnchorColumn, _maxCellContentLength;
  bool _columnHeaders, _rowHeaders, _rowHeaderHtmlEncode;
  QSet<int> _dataHtmlDisableEncode, _columnHeaderHtmlDisableEncode;

public:
  /** Implicitely set empty placeholder to "(empty)", ellipse placeholder
   * to "...", columns headers to true and row headers to false. */
  explicit HtmlTableView(QObject *parent = 0,
                         int cachedRows = defaultCachedRows,
                         int rowsPerPage = defaultRowsPerPage);
  void setTableClass(QString tableClass) {
    _tableClass = tableClass; updateHeaderAndFooterCache(); }
  void setTopLeftHeader(QString rawHtml) {
    _topLeftHeader = rawHtml; updateHeaderAndFooterCache(); }
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
  void setColumnHeaders(bool set = true) {
    _columnHeaders = set; updateHeaderAndFooterCache(); }
  void setRowHeaders(bool set = true) {
    _rowHeaders = set; updateHeaderAndFooterCache(); }
  void setEmptyPlaceholder(QString rawText);
  void setEllipsePlaceholder(QString rawText);
  void setRowAnchor(QString prefix = "", int column = 0) {
    _rowAnchorPrefix = prefix;
    _rowAnchorColumn = column;
  }
  /** Prefix of set page url.
   * Will be suffixed with e.g. "page=42" "myscope.page=1&anchor=pagebar.foo"
   * Default: "?" Example: "../setpage?" */
  void setPageUrlPrefix(QString urlPrefix) { _pageUrlPrefix = urlPrefix; }
  void setDataHtmlEncode(int column, bool encode = true) {
    if (encode)
      _dataHtmlDisableEncode.remove(column);
    else
      _dataHtmlDisableEncode.insert(column); }
  void setColumnHeaderHtmlEncode(int column, bool encode = true) {
    if (encode)
      _columnHeaderHtmlDisableEncode.remove(column);
    else
      _columnHeaderHtmlDisableEncode.insert(column); }
  void setRowHeaderHtmlEncode(bool encode = true) {
    _rowHeaderHtmlEncode = encode; }
  /** Maximum length of text inside a cell, measured before HTML encoding if
   * any. Default: 200. */
  void setMaxCellContentLength(int maxCellContentLength = 200) {
    _maxCellContentLength = maxCellContentLength; }

protected:
  void updateHeaderAndFooterCache();
  QString rowText(int row);
  QString header(int currentPage, int lastPage, QString pageVariableName) const;
  QString footer(int currentPage, int lastPage, QString pageVariableName) const;

private:
  inline QString pageLink(int page, QString pageVariableName,
                          QString pagebarAnchor) const;
  QString pagebar(int currentPage, int lastPage,
                  QString pageVariableName, bool defineAnchor) const;
};

#endif // HTMLTABLEVIEW_H
