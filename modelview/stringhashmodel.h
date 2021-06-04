/* Copyright 2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef STRINGHASHMODEL_H
#define STRINGHASHMODEL_H

#include "libp6core_global.h"
#include <QAbstractTableModel>
#include <QHash>
#include <QMap>

/** Model displaying a QHash<QString,QString> as a QAbstractTableModel.
 * One key-value pair per row, key on column 0, value on column 1.
 */
class LIBP6CORESHARED_EXPORT StringHashModel : public QAbstractTableModel {
  Q_OBJECT
  Q_DISABLE_COPY(StringHashModel)

public:
  static const QString _keysMimeType, _valuesMimeType;

private:
  QHash<QString,QString> _values;
  QStringList _rowNames;

public:
  StringHashModel(QObject *parent = 0);
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  bool removeRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  QStringList mimeTypes() const override;
  QMimeData *mimeData(const QModelIndexList &indexes) const override;
  Qt::DropActions supportedDragActions() const override;
  void clear();
  void setValues(const QHash<QString,QString> &values);
  void setValues(const QMap<QString,QString> &values);
  QHash<QString,QString> values() const { return _values; }
  QMap<QString,QString> valuesAsMap() const;
  void setValue(const QString &key, const QString &value);
  void removeValue(const QString &key);
  /** @return new key */
  QString addNewKey();
  /** @return -1 if not found */
  int rowOf(const QString &key);

signals:
  void valuesChanged(const QHash<QString,QString> &values);
};

#endif // STRINGHASHMODEL_H
