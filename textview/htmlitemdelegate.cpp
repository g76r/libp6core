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
#include "htmlitemdelegate.h"
#include "util/htmlutils.h"

int HtmlItemDelegate::_defaultMaxCellContentLength(200);

HtmlItemDelegate::HtmlItemDelegate(QObject *parent)
  : TextViewItemDelegate(parent),
    _maxCellContentLength(_defaultMaxCellContentLength) {
}

void HtmlItemDelegate::convertData(QString &data) const {
  if (_maxCellContentLength > 0 && data.size() > _maxCellContentLength) {
    data = data.left(_maxCellContentLength/2-1) + "..."
        + data.right(_maxCellContentLength/2-2);
  }
  switch (_conversion) {
  case HtmlEscaping:
    data = HtmlUtils::htmlEncode(data, false);
    break;
  case HtmlEscapingWithUrlAsLinks:
    data = HtmlUtils::htmlEncode(data, true);
    break;
  case AsIs:
    ;
  }
}

QString HtmlItemDelegate::affix(const AffixMapper &m,
                                const QModelIndex &index) const {
  QString affix = m._text;
  if (m._argIndex >= 0) {
    QString arg = index.model()->data(
          index.model()->index(index.row(), m._argIndex, index.parent()))
        .toString();
    affix = affix.arg(m._transcodeMap.isEmpty()
                      ? arg
                      : m._transcodeMap.value(arg));
  }
  return affix;
}

QString HtmlItemDelegate::affix(
    const AffixMapper &m, const QAbstractItemModel* model, int row) const {
  QString affix = m._text;
  if (m._argIndex >= 0) {
    QString arg = model->data(model->index(row, m._argIndex, QModelIndex()))
        .toString();
    affix = affix.arg(m._transcodeMap.isEmpty()
                      ? arg
                      : m._transcodeMap.value(arg));
  }
  return affix;
}

QString HtmlItemDelegate::text(const QModelIndex &index) const {
  if (!index.isValid())
    return QString();
  QString data = index.data().toString();
  convertData(data);
  if (_rowPrefixes.contains(All))
    data.prepend(affix(_rowPrefixes[All], index));
  if (_rowPrefixes.contains(index.row()))
    data.prepend(affix(_rowPrefixes[index.row()], index));
  if (_columnPrefixes.contains(All))
    data.prepend(affix(_columnPrefixes[All], index));
  if (_columnPrefixes.contains(index.column()))
    data.prepend(affix(_columnPrefixes[index.column()], index));
  if (_rowSuffixes.contains(All))
    data.append(affix(_rowSuffixes[All], index));
  if (_rowSuffixes.contains(index.row()))
    data.append(affix(_rowSuffixes[index.row()], index));
  if (_columnSuffixes.contains(All))
    data.append(affix(_columnSuffixes[All], index));
  if (_columnSuffixes.contains(index.column()))
    data.append(affix(_columnSuffixes[index.column()], index));
  return data;
}

QString HtmlItemDelegate::headerText(int section, Qt::Orientation orientation,
                                     const QAbstractItemModel* model) const {
  if (!model)
    return QString();
  QString data = model->headerData(section, orientation).toString();
  convertData(data);
  switch (orientation) {
  case Qt::Vertical:
    if (_rowPrefixes.contains(Header))
      data.prepend(affix(_rowPrefixes[Header], model, section));
    if (_rowSuffixes.contains(Header))
      data.append(affix(_rowSuffixes[Header], model, section));
    break;
  case Qt::Horizontal:
    if (_columnPrefixes.contains(Header))
      data.prepend(_columnPrefixes[Header]._text);
    if (_columnSuffixes.contains(Header))
      data.prepend(_columnSuffixes[Header]._text);
    ;
  }
  return data;
}
