/* Copyright 2013 Hallowyn and others.
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
#ifndef HTMLITEMDELEGATE_H
#define HTMLITEMDELEGATE_H

#include "textviewitemdelegate.h"

class LIBQTSSUSHARED_EXPORT HtmlItemDelegate : public TextViewItemDelegate {
  Q_OBJECT
  Q_DISABLE_COPY(HtmlItemDelegate)
public:
  enum TextConversion { AsIs, HtmlEscaping, HtmlEscapingWithUrlAsLinks };
  enum SpecialSections { All = -1, Header = -2 };
  enum SpecialArgIndexes { None = -1 };

private:
  class AffixMapper {
  public:
    QString _text;
    int _argIndex;
    QHash<QString,QString> _transcodeMap;
    AffixMapper(QString text, int argIndex, QHash<QString,QString> transcodeMap)
      : _text(text), _argIndex(argIndex), _transcodeMap(transcodeMap) { }
    AffixMapper() : _argIndex(None) { }
  };

  TextConversion _conversion;
  QHash<int,AffixMapper> _columnPrefixes;
  QHash<int,AffixMapper> _columnSuffixes;
  QHash<int,AffixMapper> _rowPrefixes;
  QHash<int,AffixMapper> _rowSuffixes;
  int _maxCellContentLength;
  static int _defaultMaxCellContentLength;

public:
  explicit HtmlItemDelegate(QObject *parent = 0);
  QString text(const QModelIndex &index) const;
  QString headerText(int section, Qt::Orientation orientation,
                     const QAbstractItemModel* model) const;
  /** Set the way text data in the model is converted.
   * <ul>
   * <li> AsIs: no conversion at all, even leave HTML special chars as is
   * <li> HtmlEscaping: transform HTML special chars into HTML entities
   * <li> HtmlEscapingWithUrlAsLinks: URLs are surrounded with a href tags
   * </ul>
   * default: HtmlEscapingWithUrlAsLinks */
  HtmlItemDelegate *setTextConversion(
      TextConversion conversion = HtmlEscapingWithUrlAsLinks) {
    _conversion = conversion; return this; }
  TextConversion textConversion() const { return _conversion; }
  /** All data in column column will be prefixed with raw (= copied as is,
   * without text conversion) html pattern that can optionnaly contain a
   * variable part that is defined by a given model column for the same row
   * and parent. The model column can be one that is not displayed in the view.
   * The data can optionnaly be transcoded through a constant map.
   * Placeholders and transcoding is not supported for column headers (but it
   * is for row headers, even though it only make sense for table views).
   * @param column column on which to apply prefix or All or Header
   * @param html prefix template, can contain %1 placeholder to be replaced
   * @param argIndex index within the model of column containing data to replace
   *   %1 with
   * @param transcodeMap if found in the map, argIndex data found in the model
   *   is replaced by matching value before %1 replacements
   * @return this
   */
  HtmlItemDelegate *setPrefixForColumn(
      int column, QString html, int argIndex = None,
      QHash<QString,QString> transcodeMap = QHash<QString,QString>()) {
    _columnPrefixes.insert(column, AffixMapper(html, argIndex, transcodeMap));
    return this; }
  /** @see setPrefixForColumn() */
  HtmlItemDelegate *setSuffixForColumn(
      int column, QString html, int argIndex = None,
      QHash<QString,QString> transcodeMap = QHash<QString,QString>()){
    _columnSuffixes.insert(column, AffixMapper(html, argIndex, transcodeMap));
    return this; }
  /** @see setPrefixForColumn() */
  HtmlItemDelegate *setPrefixForRow(
      int row, QString html, int argIndex = None,
      QHash<QString,QString> transcodeMap = QHash<QString,QString>()){
    _rowPrefixes.insert(row, AffixMapper(html, argIndex, transcodeMap));
    return this; }
  /** @see setPrefixForColumn() */
  HtmlItemDelegate *setSuffixForRow(
      int row, QString html, int argIndex = None,
      QHash<QString,QString> transcodeMap = QHash<QString,QString>()){
    _rowSuffixes.insert(row, AffixMapper(html, argIndex, transcodeMap));
    return this; }
  /** Clear any previous suffix or prefix definition. */
  HtmlItemDelegate *clearAffixes() {
    _columnPrefixes.clear();
    _columnSuffixes.clear();
    _rowPrefixes.clear();
    _rowSuffixes.clear();
    return this; }
  /** Maximum length of text inside a cell, measured before HTML encoding if
   * any. Default: 200. */
  void setMaxCellContentLength(int maxCellContentLength = 200) {
    _maxCellContentLength = maxCellContentLength; }
  /** Maximum length of text inside a cell, measured before HTML encoding if
   * any. Default: 200. */
  static void setDefaultMaxCellContentLength(int length) {
    _defaultMaxCellContentLength = length; }

private:
  inline void convertData(QString &data) const;
  inline QString affix(const AffixMapper &m, const QModelIndex &index) const;
  inline QString affix(const AffixMapper &m, const QAbstractItemModel* model,
                       int row) const;
};

#endif // HTMLITEMDELEGATE_H
