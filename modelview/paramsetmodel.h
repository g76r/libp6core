/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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
#ifndef PARAMSETMODEL_H
#define PARAMSETMODEL_H

#include "util/paramset.h"
#include "modelview/shareduiitemdocumentmanager.h"
#include <QAbstractItemModel>

/** Model to display a ParamSet into a 4-columns (key, value, scope, overriden)
 * View.
 *
 * By default scope is empty when the value is set in the paramset and
 * "inherited" when set in a parent or ancestor paramset.
 * Overriden is true for parent or ancestor values that are overriden in one or
 * several descendant paramsets, for instance this is suitable to display them
 * struck through.
 *
 * Rows are sorted by inheritance depth then key.
 *
 * @see ParamSet
 * @see QAbstractItemModel
 */
class LIBP6CORESHARED_EXPORT ParamSetModel : public QAbstractListModel {
  Q_OBJECT
  Q_DISABLE_COPY(ParamSetModel)

public:
  /** Pointer to document manager's signal notifying a paramset has changed.
   * To be connected to changeParams() slot. */
  template <class T>
  using ChangedSignal = void (T::*)(const ParamSet &newParams,
  const ParamSet &oldParams, const Utf8String &paramsetId);
  /** Pointer to document manager's method to propagate a user interface
   * change occuring through the model.
   * To be connected to paramsChanged() signal. */
  template <class T>
  using ChangeSetter = void (T::*)(const ParamSet &newParams,
  const ParamSet &oldParams, const Utf8String &paramsetId);

private:
  struct ParamSetRow {
    QString _key, _value, _scope;
    bool _overriden, _inherited;
    ParamSetRow() : _overriden(false), _inherited(false) { }
    ParamSetRow(QString key, QString value, QString scope, bool overriden,
                bool inherited)
      : _key(key), _value(value), _scope(scope), _overriden(overriden),
        _inherited(inherited) { }
  };
  ParamSet _params;
  QByteArray _paramsetId, _qualifier;
  QList<ParamSetRow> _rows;
  QList<QString> _scopes;
  bool _inherit, _evaluate, _displayOverriden, _trimOnEdit;
  QByteArray _changeParamsIdFilter;
  QString _defaultScopeForInheritedParams = "inherited";
  QVariant _overridenDecoration, _localDecoration, _inheritedDecoration;

public:
  explicit ParamSetModel(
      QObject *parent = nullptr, bool inherit = false, bool evaluate = false,
      bool displayOverriden = false, bool trimOnEdit = true);
  // LATER make column names customizable (e.g. "Variable" instead of "Key")
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  bool removeRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  QString createNewParam();
  /** Return index of a param given its key.
   *
   * If allowInherited and the key is found several time, the last one, which is
   * the not overriden one, is preferred.
   */
  QModelIndex indexOf(QString key, bool allowInherited) const;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QList<QString> scopes() const { return _scopes; }
  void setScopes(const QList<QString> &scopes) { _scopes = scopes; }
  void setchangeParamsIdFilter(QString changeParamsIdFilter) {
    _changeParamsIdFilter = changeParamsIdFilter.toUtf8(); }
  void setchangeParamsIdFilter(const char *changeParamsIdFilter) {
    _changeParamsIdFilter = changeParamsIdFilter; }
  QVariant overridenDecoration() const { return _overridenDecoration; }
  /** Define column 0 (key) decoration for params w/ overriden value.
   * One should e.g. set an icon meaning 'hidden' or 'overriden' or 'deleted'.
   * If not defined (or null), inherited decoration will be used instead. */
  void setOverridenDecoration(QVariant decoration) {
    _overridenDecoration = decoration; }
  QVariant localDecoration() const { return _localDecoration; }
  /** Define column 0 (key) decoration for params neither inerhited nor
   * overriden.
   * One may e.g. set an icon meaning 'parameter' or 'key' or 'record', or
   * something representing the local scope, or nothing,
   * or QColor(Qt::transparent) as an invisible placeholder */
  void setLocalDecoration(QVariant decoration) {
    _localDecoration = decoration; }
  QVariant inheritedDecoration() const { return _inheritedDecoration; }
  // TODO rather support one decoration per scope
  /** Define column 0 (key) decoration for inherited params but if they are
   * overriden.
   * One may e.g. set an icon meaning 'parent' or 'legacy', or the same
   * decoration as the local one, or nothing, or QColor(Qt::transparent) */
  void setInheritedDecoration(QVariant decoration) {
    _inheritedDecoration = decoration; }
  /** All-in-one method to connect to a document manager class:
   * paramsChanged() signal, changeParams() slot, initial params, changeParams()
   * slot filter, and scopes. */
  template <class T>
  void connectToDocumentManager(
      T *documentManager, const ParamSet &initialParams,
      const Utf8String &changeParamsIdFilter, const Utf8String &qualifier,
      ParamSetModel::ChangedSignal<T> changedSignal,
      ParamSetModel::ChangeSetter<T> changeSetter,
      const QList<QString> &scopes = {}) {
    _scopes = scopes;
    _qualifier = qualifier;
    if (!changeParamsIdFilter.isEmpty())
      _changeParamsIdFilter = _paramsetId = changeParamsIdFilter;
    connect(documentManager, changedSignal,
            this, &ParamSetModel::changeParams);
    connect(this, &ParamSetModel::paramsChanged,
            documentManager, changeSetter);
    changeParams(initialParams, ParamSet(), changeParamsIdFilter);
  }
  /** Convenience method */
  template <class T>
  void connectToDocumentManager(
      T *documentManager, const ParamSet &initialParams,
      const Utf8String &changeParamsIdFilter, const Utf8String &qualifier,
      ParamSetModel::ChangedSignal<T> changedSignal,
      ParamSetModel::ChangeSetter<T> changeSetter, const QString &localScope) {
    QList<QString> scopes;
    scopes.append(localScope);
    connectToDocumentManager(
          documentManager, initialParams, changeParamsIdFilter, qualifier,
          changedSignal, changeSetter, scopes);
  }

public slots:
  /** Must be signaled each time the ParamSet data changes. */
  // TODO use ParamSet,ParamSet,QString signature and add filter, the SUIM way
  // then update DM signature and web console connections to use only one signal
  void changeParams(const ParamSet &newParams, const ParamSet &oldParams,
                    const Utf8String &paramsetId);

signals:
  /** Signal emited whenever user interface change occured, e.g. setData()
   * or removeRows() is called.
   * Not emited when changeParams() is called. */
  void paramsChanged(const ParamSet &newParams, const ParamSet &oldParams,
                     const Utf8String &paramsetId);

private:
  inline void fillRows(QList<ParamSetRow> *rows, const ParamSet &params,
                       int depth, QSet<QString> *allKeys);
  // hide functions that cannot work with SharedUiItem paradigm to avoid
  // misunderstanding
  using QAbstractItemModel::insertRows;
  using QAbstractItemModel::insertRow;
  using QAbstractItemModel::insertColumns;
  using QAbstractItemModel::insertColumn;
  using QAbstractItemModel::removeColumns;
  using QAbstractItemModel::removeColumn;
};

#endif // PARAMSETMODEL_H
