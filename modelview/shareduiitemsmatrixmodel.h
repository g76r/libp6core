/* Copyright 2015-2023 Hallowyn, Gregoire Barbier and others.
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
#ifndef SHAREDUIITEMSMATRIXMODEL_H
#define SHAREDUIITEMSMATRIXMODEL_H

#include "shareduiitemsmodel.h"

/** Model holding SharedUiItems, one or zero item per table cell and header,
 * with SharedUiItemParamsProvider-evaluated formulas (e.g. "%0 %1" to display
 * section 0, a space, and section 1).
 * @see SharedUiItemsModel
 * @see SharedUiItemParamsProvider
 */
class LIBP6CORESHARED_EXPORT SharedUiItemsMatrixModel
    : public SharedUiItemsModel {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemsMatrixModel)

protected:
  struct ItemBinding {
    SharedUiItem _item;
    QString _display, _tooltip;
    int _editableSection;
    ItemBinding(SharedUiItem item = {}, QString display = {},
                QString tooltip = {}, int editableSection = -1)
      : _item(item), _display(display),
        _tooltip(tooltip.isNull() ? display : tooltip),
        _editableSection(editableSection) { }
  };

private:
  QList<ItemBinding> _verticalHeaders, _horizontalHeaders;
  QList<QList<ItemBinding>> _cells;
  int _rowsCount = 0, _columnsCount = 0;
  bool _forceDisplayRoleWhenEvaluatingTooltips = true; // LATER provide setter
  static ItemBinding nullBinding;

public:
  explicit SharedUiItemsMatrixModel(QObject *parent = 0);
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &child) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  SharedUiItem itemAt(const QModelIndex &index) const override;
  using SharedUiItemsModel::indexOf;
  QModelIndex indexOf(const Utf8String &qualifiedId) const override;
  void insertItemAt(const SharedUiItem &newItem, int row,
                    const QModelIndex &parent = {}) override;
  void changeItem(
      const SharedUiItem &newItem, const SharedUiItem &oldItem,
                  const Utf8String &qualifier) override;
  QVariant data(const QModelIndex &index, int role) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QHash<int, QByteArray> roleNames() const override;
  virtual void bindHeader(int section, Qt::Orientation orientation,
                          SharedUiItem item,
                          QString display = u"%id"_s,
                          QString tooltip = {});
  virtual void bindCell(int row, int column, SharedUiItem item,
                        QString display = u"%id"_s,
                        QString tooltip = {},
                        int editableSection = -1);
  virtual void clearBindings();

signals:
  void headerBinded(int section, Qt::Orientation orientation,
                    SharedUiItem newItem, SharedUiItem oldItem,
                    QString newDisplay, QString newTooltip);
  void cellBinded(int row, int column, SharedUiItem newItem,
                  SharedUiItem oldItem, QString newDisplay, QString newTooltip,
                  int newEditableSection);

protected:
  const ItemBinding &cellBindingAt(const QModelIndex &index) const;

private:
  inline QVariant evaluate(
      SharedUiItemsMatrixModel::ItemBinding binding, int role) const;
  /** setHeaderDataFromTemplate() is non-sense for a matrix model. */
  void setHeaderDataFromTemplate(SharedUiItem templateItem, int role) override;
};

#endif // SHAREDUIITEMSMATRIXMODEL_H
