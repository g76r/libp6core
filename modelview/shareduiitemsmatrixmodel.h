/* Copyright 2015 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
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
#ifndef SHAREDUIITEMSMATRIXMODEL_H
#define SHAREDUIITEMSMATRIXMODEL_H

#include "shareduiitemsmodel.h"

/** Model holding SharedUiItems, one or zero item per table cell and header,
 * with SharedUiItemParamsProvider-evaluated formulas (e.g. "%0 %1" to display
 * section 0, a space, and section 1).
 * @see SharedUiItemsModel
 * @see SharedUiItemParamsProvider
 */
class LIBQTSSUSHARED_EXPORT SharedUiItemsMatrixModel
    : public SharedUiItemsModel {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemsMatrixModel)
  struct ItemBinding {
    SharedUiItem _item;
    QString _display, _tooltip;
    int _editableSection;
    ItemBinding(SharedUiItem item = SharedUiItem(),
         QString display = QStringLiteral("%id"), QString tooltip = QString(),
                int editableSection = -1)
      : _item(item), _display(display),
        _tooltip(tooltip.isNull() ? display : tooltip),
        _editableSection(editableSection) { }
  };
  QVector<ItemBinding> _verticalHeaders, _horizontalHeaders;
  QVector<QVector<ItemBinding>> _cells;
  int _rowsCount = 0, _columnsCount = 0;
  bool _forceDisplayRoleWhenEvaluatingTooltips = true; // LATER provide setter

public:
  explicit SharedUiItemsMatrixModel(QObject *parent = 0);
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &child) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  SharedUiItem itemAt(const QModelIndex &index) const;
  using SharedUiItemsModel::indexOf;
  QModelIndex indexOf(QString qualifiedId) const;
  void insertItemAt(SharedUiItem newItem, int row,
                    QModelIndex parent = QModelIndex());
  void changeItem(SharedUiItem newItem, SharedUiItem oldItem,
                  QString idQualifier);
  QVariant data(const QModelIndex &index, int role) const;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole);
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  QHash<int, QByteArray> roleNames() const;
  virtual void bindHeader(int section, Qt::Orientation orientation,
                          SharedUiItem item,
                          QString display = QStringLiteral("%id"),
                          QString tooltip = QString());
  virtual void bindCell(int row, int column, SharedUiItem item,
                        QString display = QStringLiteral("%id"),
                        QString tooltip = QString(),
                        int editableSection = -1);
  virtual void clearBindings();

signals:
  void headerBinded(int section, Qt::Orientation orientation,
                    SharedUiItem newItem, SharedUiItem oldItem,
                    QString newDisplay, QString newTooltip);
  void cellBinded(int row, int column, SharedUiItem newItem,
                  SharedUiItem oldItem, QString newDisplay, QString newTooltip,
                  int newEditableSection);

private:
  inline QVariant evaluate(
      SharedUiItemsMatrixModel::ItemBinding binding, int role) const;
  /** setHeaderDataFromTemplate() is non-sense for a matrix model. */
  void setHeaderDataFromTemplate(SharedUiItem templateItem, int role);
};

#endif // SHAREDUIITEMSMATRIXMODEL_H
