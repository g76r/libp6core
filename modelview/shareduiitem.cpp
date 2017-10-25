/* Copyright 2014-2017 Hallowyn, Gregoire Barbier and others.
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
#include "shareduiitem.h"
#include <QtDebug>
#include <QRegularExpression>

static int staticInit() {
  qMetaTypeId<SharedUiItem>();
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

SharedUiItemData::~SharedUiItemData() {
}

QVariant SharedUiItemData::uiData(int section, int role) const {
  Q_UNUSED(section)
  Q_UNUSED(role)
  return QVariant();
}

bool SharedUiItemData::setUiData(
    int section, const QVariant &value, QString *errorString,
    SharedUiItemDocumentTransaction *transaction, int role) {
  Q_UNUSED(role)
  Q_UNUSED(value)
  Q_UNUSED(transaction)
  Q_ASSERT(transaction != 0);
  Q_ASSERT(errorString != 0);
  *errorString = "Field \""+uiHeaderData(section, Qt::DisplayRole).toString()
      +"\" is not ui-editable for item of type "+idQualifier()+"";
  return false;
}

Qt::ItemFlags SharedUiItemData::uiFlags(int section) const {
  Q_UNUSED(section)
  return Qt::ItemIsEnabled;
}

QVariant SharedUiItemData::uiHeaderData(int section, int role) const {
  Q_UNUSED(section)
  Q_UNUSED(role)
  return QVariant();
}

QString SharedUiItemData::id() const {
  return uiData(0, Qt::DisplayRole).toString();
}

int SharedUiItemData::uiSectionCount() const {
  return 0;
}

static QRegularExpression nonAlphanum("[^a-z0-9]+");

QString SharedUiItemData::uiSectionName(int section) const {
  return uiHeaderData(section, Qt::DisplayRole).toString().trimmed()
      .toLower().replace(nonAlphanum, QStringLiteral("_"));
}

QDebug operator<<(QDebug dbg, const SharedUiItem &i) {
  dbg.nospace() << i.qualifiedId();
  return dbg.space();
}

QVariant SharedUiItemParamsProvider::paramValue(
    QString key, QVariant defaultValue, QSet<QString> alreadyEvaluated) const {
  Q_UNUSED(alreadyEvaluated)
  bool ok;
  int section = key.toInt(&ok);
  if (!ok) {
    if (key == "id")
      return _item.id();
    if (key == "idQualifier")
      return _item.idQualifier();
    if (key == "qualifiedId")
      return _item.qualifiedId();
    section = _item.uiSectionByName(key);
    ok = section >= 0;
  }
  if (ok) {
    QVariant value = _item.uiData(section, _role);
    if (value.isValid())
      return value;
  }
  return defaultValue;
}

bool SharedUiItemData::operator<(const SharedUiItemData &other) const {
  return idQualifier() < other.idQualifier() || id() < other.id();
}
