/* Copyright 2012-2013 Hallowyn and others.
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
  QSharedDataPointer<ParamSetData> _parent;
  QHash<QString,QString> _params;
  ParamSetData() { }
  ParamSetData(const ParamSetData &other) : QSharedData(),
    _parent(other._parent), _params(other._params) { }
};

ParamSet::ParamSet() {
}

ParamSet::ParamSet(const ParamSet &other) : d(other.d) {
}

ParamSet::ParamSet(ParamSetData *data) : d(data) {
}

ParamSet::~ParamSet() {
}

ParamSet &ParamSet::operator =(const ParamSet &other) {
  if (this != &other)
    d = other.d;
  return *this;
}

const ParamSet ParamSet::parent() const {
  return d ? ParamSet(const_cast<QSharedDataPointer<ParamSetData>&>(
                        d.constData()->_parent))
           : ParamSet();
}

ParamSet ParamSet::parent() {
  return d ? ParamSet(d->_parent) : ParamSet();
}

void ParamSet::setParent(ParamSet parent) {
  if (!d)
    d = new ParamSetData();
  d->_parent = parent.d;
}

void ParamSet::setValue(const QString key, const QString value) {
  if (!d)
    d = new ParamSetData();
  d->_params.insert(key, value);
}

QString ParamSet::rawValue(const QString key, const QString defaultValue, bool inherit) const {
  QString value;
  if (d) {
    value = d->_params.value(key);
    if (value.isNull() && inherit)
      value = parent().rawValue(key);
  }
  return value.isNull() ? defaultValue : value;
}

QString ParamSet::evaluate(const QString rawValue, bool inherit,
                           const ParamsProvider *context) const {
  QStringList values = splitAndEvaluate(rawValue, QString(), inherit, context);
  return values.isEmpty() ? QString() : values.first();
}

void ParamSet::appendVariableValue(QString &value, QString &variable,
                                   bool inherit,
                                   const ParamsProvider *context) const {
  if (variable.isEmpty()) {
    Log::warning() << "unsupported variable substitution: empty variable name";
  } else if (variable.at(0) == '!') {
    // LATER %!Z %!taskid %!fqtn %!requestid %!taskgroupid %!retcode %!status %!eventid %!taskduration %!taskdelay
    if (variable == "!yyyy") {
      value.append(QDateTime::currentDateTime().toString("yyyy"));
    } else if (variable == "!mm") {
      value.append(QDateTime::currentDateTime().toString("MM"));
    } else if (variable == "!dd") {
      value.append(QDateTime::currentDateTime().toString("dd"));
    } else if (variable == "!HH") {
      value.append(QDateTime::currentDateTime().toString("hh"));
    } else if (variable == "!MM") {
      value.append(QDateTime::currentDateTime().toString("mm"));
    } else if (variable == "!SS") {
      value.append(QDateTime::currentDateTime().toString("ss"));
    } else if (variable == "!NNN") {
      value.append(QDateTime::currentDateTime().toString("zzz"));
    } else if (variable == "!8601") {
      value.append(QDateTime::currentDateTime()
                   .toString("yyyy-MM-ddThh:mm:ss,zzz"));
    } else if (variable == "!8601Z") {
      value.append(QDateTime::currentDateTimeUtc()
                   .toString("yyyy-MM-ddThh:mm:ss,zzzZ"));
    } else {
      QString s;
      if (context)
        s = context->paramValue(variable);
      if (s.isNull())
        Log::warning() << "unsupported variable substitution: %{" << variable
                       << "}";
      else
        value.append(s);
    }
  } else {
    const QString v = this->rawValue(variable, inherit);
    if (v.isNull()) {
      Log::debug() << "in paramset " << toString(false) << " " << d.constData()
                   << " parent " << parent().toString(false) << " "
                   << parent().d.constData();
      Log::debug() << "unsupported variable substitution: variable not found: "
                      "%{" << variable << "}";
    }
    else
      value.append(v);
  }
  variable.clear();
}


QStringList ParamSet::splitAndEvaluate(const QString rawValue,
                                       const QString separator,
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
        value.append('%').append(c);
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

const QSet<QString> ParamSet::keys(bool inherit) const {
  QSet<QString> set;
  if (d) {
    set = d->_params.keys().toSet();
    if (inherit)
      set += parent().keys();
  }
  return set;
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

QString ParamSet::paramValue(const QString key,
                             const QString defaultValue) const{
  return value(key, defaultValue, true);
}

QString ParamSet::toString(bool inherit) const {
  QString s;
  s.append("{ ");
  foreach(QString key, keys(inherit))
    s.append(key).append("=").append(value(key)).append(" ");
  return s.append("}");
}

QDebug operator<<(QDebug dbg, const ParamSet &params) {
  dbg.nospace() << "{";
  foreach(QString key, params.keys())
    dbg.space() << key << "=" << params.value(key) << ",";
  dbg.nospace() << "}";
  return dbg.space();
}

LogHelper operator <<(LogHelper lh, const ParamSet &params) {
  lh << "{ ";
  foreach(QString key, params.keys())
    lh << key << "=" << params.value(key) << " ";
  return lh << "}";
}
