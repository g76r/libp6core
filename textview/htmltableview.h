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
  int _thClassRole, _trClassRole, _tdClassRole, _rowAnchorColumn;
  bool _columnHeaders, _rowHeaders;
  static QString _defaultTableClass;
  static int _defaultThClassRole, _defaultTrClassRole, _defaultTdClassRole;

public:
  /** Implicitely set empty placeholder to "(empty)", ellipse placeholder
   * to "...", columns headers to true and row headers to false. */
  explicit HtmlTableView(QObject *parent = 0, QString objectName = QString(),
                         int cachedRows = defaultCachedRows,
                         int rowsPerPage = defaultRowsPerPage);
  void setTableClass(QString tableClass) {
    _tableClass = tableClass; updateHeaderAndFooterCache(); }
  void setTopLeftHeader(QString rawHtml) {
    _topLeftHeader = rawHtml; updateHeaderAndFooterCache(); }
  void setThClassRole(int role) { _thClassRole = role; }
  void setTrClassRole(int role) { _trClassRole = role; }
  void setTdClassRole(int role) { _tdClassRole = role; }
  void setColumnHeaders(bool set = true) {
    _columnHeaders = set; updateHeaderAndFooterCache(); }
  void setRowHeaders(bool set = true) {
    _rowHeaders = set; updateHeaderAndFooterCache(); }
  void setEmptyPlaceholder(QString rawText);
  void setEllipsePlaceholder(QString rawText);
  /** Add an "<a name=" anchor to every row, the anchor is prefix + content of
   *  column column. Also add id= to <tr> tag. */
  void enableRowAnchor(QString prefix, int column = 0) {
    _rowAnchorPrefix = prefix;
    _rowAnchorColumn = column;
  }
  /** Add an "<a name=" anchor to every row, using objectName() + "." as
   *  prefix. Also add id= to <tr> tag. */
  void enableRowAnchor(int column = 0) {
    _rowAnchorPrefix = objectName()+"-";
    _rowAnchorColumn = column;
  }
  /** Do not add "<a name=" anchor to every row. */
  void disableRowAnchor() {
    _rowAnchorPrefix = QString();
  }
  /** Prefix of set page url.
   * Will be suffixed with e.g. "page=42" "myscope.page=1&anchor=pagebar.foo"
   * Default: "?" Example: "../setpage?" */
  void setPageUrlPrefix(QString urlPrefix) { _pageUrlPrefix = urlPrefix; }
  static void setDefaultTableClass(QString tableClass) {
    _defaultTableClass = tableClass; }
  static void setDefaultThClassRole(int role) {
    _defaultThClassRole = role; }
  static void setDefaultTrClassRole(int role) {
    _defaultTrClassRole = role; }
  static void setDefaultTdClassRole(int role) {
    _defaultTdClassRole = role; }

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
