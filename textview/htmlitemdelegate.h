/* Copyright 2013-2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef HTMLITEMDELEGATE_H
#define HTMLITEMDELEGATE_H

#include "textviewitemdelegate.h"
#include "format/htmltableformatter.h"

// LATER try to factorize code with TemplatingHttpHandler
class LIBPUMPKINSHARED_EXPORT HtmlItemDelegate : public TextViewItemDelegate,
    public HtmlTableFormatter {
  Q_OBJECT
  Q_DISABLE_COPY(HtmlItemDelegate)
public:
  enum SpecialSections { AllSections = -1 };
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

  QHash<int,TextMapper> _columnPrefixes;
  QHash<int,TextMapper> _columnSuffixes;
  QHash<int,TextMapper> _rowPrefixes;
  QHash<int,TextMapper> _rowSuffixes;
  QHash<int,QString> _columnHeaderPrefixes;
  QHash<int,QString> _columnHeaderSuffixes;
  QHash<int,TextMapper> _rowHeaderPrefixes;
  QHash<int,TextMapper> _rowHeaderSuffixes;

public:
  explicit HtmlItemDelegate(QObject *parent = 0);
  QString text(const QModelIndex &index) const;
  QString headerText(int section, Qt::Orientation orientation,
                     const QAbstractItemModel* model) const;
  /** All data in column column will be prefixed with raw (= copied as is,
   * without text conversion) html pattern that can optionnaly contain a
   * variable part that is defined by a given model column for the same row
   * and parent. The model column can be one that is not displayed in the view.
   *
   * The data can optionnaly be transcoded through a constant map.
   * Placeholders and transcoding is not supported for column headers (but it
   * is for row headers, even though it only make sense for table views).
   *
   * Different affixes definition are overided according to the following
   * rule of precedence: exact row > exact column > all rows > all columns.
   * For instance if two prefixes are defined, one for column #3 and another
   * one for row #2, the first one will be applied on every cell of column #3
   * except the one on row #2 where the second prefix will be applied.
   *
   * @param column column on which to apply prefix or All or Header
   * @param pattern prefix template, can contain %1 placeholder to be replaced
   * @param argIndex index within the model of column containing data to replace
   *   %1 with
   * @param transcodeMap if found in the map, argIndex data found in the model
   *   is replaced by matching value before %1 replacements
   * @return this
   */
  HtmlItemDelegate *setPrefixForColumn(
      int column, QString pattern, int argIndex,
      QHash<QString,QString> transcodeMap);
  /** syntaxic sugar */
  HtmlItemDelegate *setPrefixForColumn(
      int column, QString pattern, int argIndex = None) {
    return setPrefixForColumn(
          column, pattern, argIndex, QHash<QString,QString>()); }
  /** @see setPrefixForColumn() */
  HtmlItemDelegate *setSuffixForColumn(
      int column, QString pattern, int argIndex,
      QHash<QString,QString> transcodeMap);
  /** syntaxic sugar */
  HtmlItemDelegate *setSuffixForColumn(
      int column, QString pattern, int argIndex = None) {
    return setSuffixForColumn(
          column, pattern, argIndex, QHash<QString,QString>()); }
  /** @see setPrefixForColumn() */
  HtmlItemDelegate *setPrefixForRow(
      int row, QString pattern, int argIndex,
      QHash<QString,QString> transcodeMap);
  /** syntaxic sugar */
  HtmlItemDelegate *setPrefixForRow(
      int row, QString pattern, int argIndex = None) {
    return setPrefixForRow(
          row, pattern, argIndex, QHash<QString,QString>()); }
  /** @see setPrefixForColumn() */
  HtmlItemDelegate *setSuffixForRow(
      int row, QString pattern, int argIndex,
      QHash<QString,QString> transcodeMap);
  /** syntaxic sugar */
  HtmlItemDelegate *setSuffixForRow(
      int row, QString pattern, int argIndex = None) {
    return setSuffixForRow(
          row, pattern, argIndex, QHash<QString,QString>()); }
  /** @see setPrefixForColumn() */
  HtmlItemDelegate *setPrefixForColumnHeader(int column, QString text);
  /** @see setPrefixForColumn() */
  HtmlItemDelegate *setSuffixForColumnHeader(int column, QString text);
  /** @see setPrefixForColumn() */
  HtmlItemDelegate *setPrefixForRowHeader(
      int row, QString pattern, int argIndex,
      QHash<QString,QString> transcodeMap);
  /** syntaxic sugar */
  HtmlItemDelegate *setPrefixForRowHeader(
      int row, QString pattern, int argIndex = None){
    return setPrefixForRowHeader(
          row, pattern, argIndex, QHash<QString,QString>()); }
  /** @see setPrefixForColumn() */
  HtmlItemDelegate *setSuffixForRowHeader(
      int row, QString pattern, int argIndex,
      QHash<QString,QString> transcodeMap);
  /** syntaxic sugar */
  HtmlItemDelegate *setSuffixForRowHeader(
      int row, QString pattern, int argIndex = None){
    return setSuffixForRowHeader(
          row, pattern, argIndex, QHash<QString,QString>()); }
  /** Clear any previous suffix or prefix definition. */
  HtmlItemDelegate *clearAffixes();
  /** Overriden to emit textChanged() signal otherwise same as
   * HtmlTableFormater's implementation. */
  void setTextConversion(
      TextConversion conversion = HtmlEscapingWithUrlAsLinks);
  /** Overriden to emit textChanged() signal otherwise same as
   * HtmlTableFormater's implementation. */
  void setMaxCellContentLength(int maxCellContentLength = 200);

private:
  inline QString dataAffix(const TextMapper &m, const QModelIndex &index) const;
  inline QString rowHeaderAffix(
      const TextMapper &m, const QAbstractItemModel* model, int row) const;
};

#endif // HTMLITEMDELEGATE_H
