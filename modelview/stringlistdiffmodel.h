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
#ifndef STRINGLISTDIFFMODEL_H
#define STRINGLISTDIFFMODEL_H

#include "libp6core_global.h"
#include <QAbstractTableModel>
#include <QStringList>

// LATER make this model writeable, at less make it possible to the user to
// delete rows and so choose which diff lines he would like to apply

/** Model displaying two QString list side by side with diff-like decoration
 * (background colors).
 */
class LIBP6CORESHARED_EXPORT StringListDiffModel : public QAbstractTableModel {
  Q_OBJECT
  Q_DISABLE_COPY(StringListDiffModel)

public:
  enum Status { NoChange, Added, Removed, Modified };
  Q_ENUM(Status)
  class DiffLine {
    friend class StringListDiffModel;
    QString _before, _after;
    Status _status;

  public:
    DiffLine(const QString &before = { }, const QString &after = { },
             Status status = NoChange)
      : _before(before), _after(after), _status(status) { }
    QString before() const { return _before; }
    QString after() const { return _after; }
    Status status() const { return _status; }
  };

private:
  QVector<DiffLine> _lines;

public:
  StringListDiffModel(QObject *parent = 0);
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(
      const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  void setValues(const QList<QString> &beforeValues,
                 const QList<QString> &afterValues);
  void clear();
  QVector<DiffLine> lines() const { return _lines; }
  DiffLine line(int row) const { return _lines.value(row); }

protected:
  Status rowStatus(int row) const { return line(row).status(); }
};

// LATER not sure:
//Q_DECLARE_TYPEINFO(StringListDiffModel::DiffLine, Q_MOVABLE_TYPE);

#endif // STRINGLISTDIFFMODEL_H
