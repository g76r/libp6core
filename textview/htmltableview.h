/* Copyright 2012-2021 Hallowyn, Gregoire Barbier and others.
 * This file is part of libpumpkin, see <http://libpumpkin.g76r.eu/>.
 * Libpumpkin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libpumpkin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libpumpkin.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef HTMLTABLEVIEW_H
#define HTMLTABLEVIEW_H

#include "texttableview.h"

/** Display the model content as a HTML table. Only rows of the root index
  * are displayed. */
class LIBP6CORESHARED_EXPORT HtmlTableView : public TextTableView {
  Q_OBJECT
  Q_DISABLE_COPY(HtmlTableView)
public:
  enum SpecialArgIndexes { None = -1 };

private:
  class TextMapper {
  public:
    QString _text;
    int _argIndex;
    QHash<QString,QString> _transcodeMap;
    TextMapper(QString text, int argIndex, QHash<QString,QString> transcodeMap)
      : _text(text), _argIndex(argIndex), _transcodeMap(transcodeMap) { }
    TextMapper() : _argIndex(None) { }
  };

  QString _tableClass, _topLeftHeader, _rowAnchorPrefix, _tableHeader,
  _pageUrlPrefix;
  TextMapper _trClassMapper;
  int _rowAnchorColumn;
  bool _columnHeadersEnabled, _rowHeadersEnabled;
  static QString _defaultTableClass;

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
  void enableColumnHeaders(bool set = true) {
    _columnHeadersEnabled = set; updateHeaderAndFooterCache(); }
  void enableRowHeaders(bool set = true) {
    _rowHeadersEnabled = set; updateHeaderAndFooterCache(); }
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
  /** Each row tr element will have the given class, regarding the pattern that
   * can optionnaly contain a
   * variable part that is defined by a given model column for the same row
   * and parent. The model column can be one that is not displayed in the view.
   * The data can optionnaly be transcoded through a constant map.
   * @param pattern class template, can contain %1 placeholder to be replaced
   * @param argIndex index within the model of column containing data to replace
   *   %1 with
   * @param transcodeMap if found in the map, argIndex data found in the model
   *   is replaced by matching value before %1 replacements
   * @return this
   */
  HtmlTableView *setTrClass(QString pattern, int argIndex,
      QHash<QString,QString> transcodeMap) {
    _trClassMapper = TextMapper(pattern, argIndex, transcodeMap);
    return this; }
  /** syntaxic sugar */
  HtmlTableView *setTrClass(QString pattern, int argIndex = None) {
    return setTrClass(pattern, argIndex, QHash<QString,QString>());}
  HtmlTableView *clearTrClass() {
    _trClassMapper = TextMapper();
    return this; }


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
