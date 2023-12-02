/* Copyright 2017-2023 Hallowyn, Gregoire Barbier and others.
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
#include "util/utf8string.h"
#include <QRegularExpression>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonArray>

static QRegularExpression _linebreaksRE { "\\s*[\\n\\r]+\\s*" };

namespace {

template<class T>
QString dict2string(const T &dict) {
  QJsonObject json;
  for (auto [k,v] : dict.asKeyValueRange())
    json.insert(k, v);
  QJsonDocument doc;
  doc.setObject(json);
  return QString::fromUtf8(doc.toJson()).replace(_linebreaksRE, u" "_s);
}

template<class T>
T string2dict(const QString &string) {
  T dict;
  QJsonParseError error;
  QJsonDocument doc = QJsonDocument::fromJson(string.toUtf8(), &error);
  QJsonObject json = doc.object();
  for (const QString &key: json.keys()) {
    QJsonValue v = json.value(key);
    switch (v.type()) {
    case QJsonValue::String:
      dict.insert(key, v.toString());
      break;
    case QJsonValue::Bool:
      dict.insert(key, v.toBool() ? u"true"_s : u"false"_s);
      break;
    case QJsonValue::Double:
      dict.insert(key, QString::number(v.toDouble()));
      break;
    case QJsonValue::Null:
    case QJsonValue::Array:
    case QJsonValue::Object:
    case QJsonValue::Undefined:
      // ignore
      break;
    }
  }
  return dict;
}

} // unnamed namespace

QString JsonFormats::hash2string(const QHash<QString,QString> &hash) {
  return dict2string(hash);
}

QHash<QString,QString> JsonFormats::string2hash(const QString &string) {
  return string2dict<QHash<QString,QString>>(string);
}

QString JsonFormats::map2string(const QMap<QString,QString> &map) {
  return dict2string(map);
}

QMap<QString,QString> JsonFormats::string2map(const QString &string) {
  return string2dict<QMap<QString,QString>>(string);
}

QString JsonFormats::list2string(const QList<QString> &list) {
  QJsonArray json;
  for (const QString &string : list)
    json.append(string);
  QJsonDocument doc;
  doc.setArray(json);
  return QString::fromUtf8(doc.toJson()).replace(_linebreaksRE, u" "_s);
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
      list.append(v.toBool() ? u"true"_s : u"false"_s);
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

void JsonFormats::recursive_insert(
    QJsonObject &target, QStringList path, const QJsonValue &value) {
  if (path.isEmpty())
    return;
  auto name = path.takeFirst();
  if (path.isEmpty()) {
    target[name] = value;
    return;
  }
  QJsonObject newChild;
  QJsonValue oldValue = target.value(name);
  if (oldValue.isObject())
    newChild = oldValue.toObject();
  recursive_insert(newChild, path, value);
  target.insert(name, newChild);
}
