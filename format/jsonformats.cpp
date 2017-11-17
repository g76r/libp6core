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
#include "jsonformats.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>

static QRegularExpression _linebreaksRE { "\\s*[\\n\\r]+\\s*" };

QString JsonFormats::hash2string(const QHash<QString,QString> &hash) {
  QJsonObject json;
  for (const QString &key : hash.keys())
    json.insert(key, hash.value(key));
  QJsonDocument doc;
  doc.setObject(json);
  return QString::fromUtf8(doc.toJson())
      .replace(_linebreaksRE, QStringLiteral(" "));
}

QHash<QString,QString> JsonFormats::string2hash(const QString &string) {
  QHash<QString,QString> hash;
  QJsonParseError error;
  QJsonDocument doc = QJsonDocument::fromJson(string.toUtf8(), &error);
  QJsonObject json = doc.object();
  for (const QString &key: json.keys()) {
    QJsonValue v = json.value(key);
    switch (v.type()) {
    case QJsonValue::String:
      hash.insert(key, v.toString());
      break;
    case QJsonValue::Bool:
      hash.insert(key, v.toBool() ? QStringLiteral("true")
                                  : QStringLiteral("false"));
      break;
    case QJsonValue::Double:
      hash.insert(key, QString::number(v.toDouble()));
      break;
    case QJsonValue::Null:
    case QJsonValue::Array:
    case QJsonValue::Object:
    case QJsonValue::Undefined:
      // ignore
      break;
    }
  }
  return hash;
}

QString JsonFormats::list2string(const QList<QString> &list) {
  QJsonArray json;
  for (const QString &string : list)
    json.append(string);
  QJsonDocument doc;
  doc.setArray(json);
  return QString::fromUtf8(doc.toJson())
      .replace(_linebreaksRE, QStringLiteral(" "));
}

QStringList JsonFormats::string2list(const QString &string) {
  QStringList list;
  QJsonParseError error;
  QJsonDocument doc = QJsonDocument::fromJson(string.toUtf8(), &error);
  QJsonArray json = doc.array();
  for (const QJsonValue &v : json) {
    switch (v.type()) {
    case QJsonValue::String:
      list.append(v.toString());
      break;
    case QJsonValue::Bool:
      list.append(v.toBool() ? QStringLiteral("true")
                             : QStringLiteral("false"));
      break;
    case QJsonValue::Double:
      list.append(QString::number(v.toDouble()));
      break;
    case QJsonValue::Null:
    case QJsonValue::Array:
    case QJsonValue::Object:
    case QJsonValue::Undefined:
      // ignore
      break;
    }
  }
  return list;
}
