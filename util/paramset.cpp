/* Copyright 2012-2014 Hallowyn and others.
 * This file is part of qron, see <http://qron.hallowyn.com/>.
 * Qron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Qron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with qron. If not, see <http://www.gnu.org/licenses/>.
 */
#include "paramset.h"
#include <QSharedData>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QtDebug>

class ParamSetData : public QSharedData {
public:
  ParamSet _parent;
  QHash<QString,QString> _params;
  ParamSetData() { }
  ParamSetData(QHash<QString,QString> params) : _params(params) { }
};

ParamSet::ParamSet() {
}

ParamSet::ParamSet(const ParamSet &other) : d(other.d) {
}

ParamSet::ParamSet(QHash<QString,QString> params)
  : d(new ParamSetData(params)) {
}

ParamSet::ParamSet(QMap<QString,QString> params) {
  foreach(QString key, params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::~ParamSet() {
}

ParamSet &ParamSet::operator=(const ParamSet &other) {
  if (this != &other)
    d = other.d;
  return *this;
}

ParamSet ParamSet::parent() const {
  return d ? d->_parent : ParamSet();
}

void ParamSet::setParent(ParamSet parent) {
  if (!d)
    d = new ParamSetData();
  if (d.constData() != parent.d.constData())
    d->_parent = parent;
}

void ParamSet::setValue(QString key, QString value) {
  if (!d)
    d = new ParamSetData();
  d->_params.insert(key, value);
}

void ParamSet::removeValue(QString key) {
  if (d)
    d->_params.remove(key);
}

void ParamSet::clear() {
  d = new ParamSetData();
}

QString ParamSet::rawValue(QString key, QString defaultValue,
                           bool inherit) const {
  QString value;
  if (d) {
    value = d->_params.value(key);
    if (value.isNull() && inherit)
      value = parent().rawValue(key);
  }
  return value.isNull() ? defaultValue : value;
}

QString ParamSet::evaluate(QString rawValue, bool inherit,
                           const ParamsProvider *context) const {
  //Log::debug() << "evaluate " << rawValue << " " << QString::number((qint64)context, 16);
  QStringList values = splitAndEvaluate(rawValue, QString(), inherit, context);
  if (values.isEmpty())
    return rawValue.isNull() ? QString() : "";
  return values.first();
}

static inline QString automaticValue(QString key) {
  if (key.at(0) == '!') {
    if (key == "!yyyy") {
      return QDateTime::currentDateTime().toString("yyyy");
    } else if (key == "!mm") {
      return QDateTime::currentDateTime().toString("MM");
    } else if (key == "!dd") {
      return QDateTime::currentDateTime().toString("dd");
    } else if (key == "!HH") {
      return QDateTime::currentDateTime().toString("hh");
    } else if (key == "!MM") {
      return QDateTime::currentDateTime().toString("mm");
    } else if (key == "!SS") {
      return QDateTime::currentDateTime().toString("ss");
    } else if (key == "!NNN") {
      return QDateTime::currentDateTime().toString("zzz");
    } else if (key == "!8601") {
      return QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss,zzz");
    } else if (key == "!8601Z") {
      return QDateTime::currentDateTimeUtc()
          .toString("yyyy-MM-ddThh:mm:ss,zzzZ");
    } else if (key == "!s1970") {
      return QString::number(
            QDateTime::currentDateTime().toMSecsSinceEpoch()/1000);
    } else if (key == "!ms1970") {
      return QString::number(
            QDateTime::currentDateTime().toMSecsSinceEpoch());
    } else if (key == "!s1970Z") {
      return QString::number(
            QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()/1000);
    } else if (key == "!ms1970Z") {
      return QString::number(
            QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    }
  }
  return QString();
}

void ParamSet::appendVariableValue(QString &value, QString &variable,
                                   bool inherit,
                                   const ParamsProvider *context) const {
  if (!variable.isEmpty()) {
    QString s = automaticValue(variable);
    if (!s.isNull())
      value.append(s);
    else {
      if (context)
        s = context->paramValue(variable).toString();
      if (!s.isNull())
        value.append(s);
      else {
        s = this->rawValue(variable, inherit);
        if (!s.isNull())
          value.append(s);
        else {
          //Log::warning() << "unsupported variable substitution: %{" << variable
          //               << "} " << QString::number((qint64)context, 16);
          Log::debug()
              << "unsupported variable substitution: variable not found: "
                 "%{" << variable << "} in paramset " << toString(false)
              << " " << d.constData() << " parent "
              << parent().toString(false);
        }
      }
    }
  } else {
    Log::warning() << "unsupported variable substitution: empty variable name";
  }
  variable.clear();
}

QStringList ParamSet::splitAndEvaluate(QString rawValue,
                                       QString separator,
                                       bool inherit,
                                       const ParamsProvider *context) const {
  QStringList values;
  QString value, variable;
  int i = 0;
  while (i < rawValue.size()) {
    QChar c = rawValue.at(i++);
    if (c == '%') {
      c = rawValue.at(i++);
      if (c == '{') {
        // LATER support imbrication in parameters substitution
        // LATER support shell-like ${:-} and so on substitution
        while (i < rawValue.size()) {
          c = rawValue.at(i++);
          if (c == '}')
            break;
          else
            variable.append(c);
        }
        appendVariableValue(value, variable, inherit, context);
      } else if (c == '!' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                 || c == '_') {
        variable.append(c);
        while (i < rawValue.size()) {
          c = rawValue.at(i++);
          if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
              || (c >= '0' && c <= '9') || c == '_') {
            variable.append(c);
          } else {
            --i;
            break;
          }
        }
        appendVariableValue(value, variable, inherit, context);
      } else {
        // % is used as an escape character, for '%', a separator or anything
        value.append(c);
      }
    } else if (separator.contains(c)) {
      if (!value.isEmpty())
        values.append(value);
      value.clear();
    } else {
      value.append(c);
    }
  }
  if (!value.isEmpty())
    values.append(value);
  return values;
}

QPair<QString,QString> ParamSet::valueAsStringsPair(
    QString key, bool inherit, const ParamsProvider *context) const {
  static QRegExp whitespace("\\s");
  QString v = rawValue(key, inherit).trimmed();
  int i = v.indexOf(whitespace);
  if (i == -1)
    return QPair<QString,QString>(v,QString());
  return QPair<QString,QString>(v.left(i),
                                evaluate(v.mid(i+1).trimmed(), context));
}

QString ParamSet::matchingPattern(QString rawValue) {
  int i = 0;
  QString value, variable;
  QString specialChars("*?[]\\");
  while (i < rawValue.size()) {
    QChar c = rawValue.at(i++);
    if (c == '%') {
      variable.clear();
      c = rawValue.at(i++);
      if (c == '{') {
        while (i < rawValue.size()) {
          c = rawValue.at(i++);
          if (c == '}')
            break;
          else
            variable.append(c);
        }
        // LATER handle ! variables in a specific manner, e.g. !yyyy -> ????
        value.append("*");
      } else if (c == '!' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                 || c == '_') {
        variable.append(c);
        while (i < rawValue.size()) {
          c = rawValue.at(i++);
          if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
              || (c >= '0' && c <= '9') || c == '_') {
            variable.append(c);
          } else {
            --i;
            break;
          }
        }
        // LATER handle ! variables in a specific manner, e.g. !yyyy -> ????
        value.append("*");
      } else {
        value.append(specialChars.contains(c) ? '?' : c);
      }
    } else {
      value.append(specialChars.contains(c) ? '?' : c);
    }
  }
  return value;
}

QSet<QString> ParamSet::keys(bool inherit) const {
  QSet<QString> set;
  if (d) {
    set = d->_params.keys().toSet();
    if (inherit)
      set += parent().keys();
  }
  return set;
}

bool ParamSet::contains(QString key, bool inherit) const {
  return d && (d->_params.contains(key) || (inherit && parent().contains(key)));
}

bool ParamSet::isNull() const {
  return !d;
}

int ParamSet::size() const {
  return d ? d->_params.size() : 0;
}

bool ParamSet::isEmpty() const {
  return d ? d->_params.isEmpty() : true;
}

QVariant ParamSet::paramValue(QString key,
                              QVariant defaultValue) const{
  return value(key, defaultValue.toString(), true);
}

QString ParamSet::toString(bool inherit, bool decorate) const {
  QString s;
  if (decorate)
    s.append("{ ");
  bool first = true;
  foreach(QString key, keys(inherit)) {
    if (first)
      first = false;
    else
      s.append(' ');
    s.append(key).append('=').append(rawValue(key));
  }
  if (decorate)
    s.append('}');
  return s;
}

QDebug operator<<(QDebug dbg, const ParamSet &params) {
  dbg.nospace() << "{";
  foreach(QString key, params.keys())
    dbg.space() << key << "=" << params.rawValue(key) << ",";
  dbg.nospace() << "}";
  return dbg.space();
}

LogHelper operator<<(LogHelper lh, const ParamSet &params) {
  lh << "{ ";
  foreach(QString key, params.keys())
    lh << key << "=" << params.rawValue(key) << " ";
  return lh << "}";
}

ParamSet ParamSet::createChild() const {
  ParamSet params;
  params.setParent(*this);
  return params;
}
