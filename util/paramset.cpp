/* Copyright 2012-2015 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
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
#include "paramset.h"
#include <QSharedData>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QtDebug>
#include "relativedatetime.h"

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

QString ParamSet::evaluate(
    QString rawValue, bool inherit, const ParamsProvider *context,
    QSet<QString> alreadyEvaluated) const {
  //Log::debug() << "evaluate " << rawValue << " " << QString::number((qint64)context, 16);
  QStringList values = splitAndEvaluate(rawValue, QString(), inherit, context,
                                        alreadyEvaluated);
  if (values.isEmpty())
    return rawValue.isNull() ? QString() : "";
  return values.first();
}

QString ParamSet::evaluateImplicitVariable(
    QString key, bool inherit, const ParamsProvider *context,
    QSet<QString> alreadyEvaluated) const {
  static QRegExp dateFunctionRE("!date(?:!([^!]+)(?:!([^!]+)(?:!([^!]+))?)?)?");
  static QRegExp defaultFunctionRE("!default!([^!]+)(?:!(.*))?");
  if (key.at(0) == '!') {
    if (key.startsWith("!date")) {
      /* %!date function: %!date!format!relativedatetime!timezone
       * format defaults to pseudo-iso-8601 "yyyy-MM-dd hh:mm:ss,zzz"
       * relativedatetime defaults to current date time
       * timezone defaults to local time
       *
       * examples:
       * %!date
       * %{!date!yyyy-MM-dd}
       * %{!date!!-2days}
       * %{!date!!!UTC}
       * %{!date!hh:mm:ss,zzz!01-01T20:02-2w+1d!GMT}
       */
      QRegExp re = dateFunctionRE;
      if (re.exactMatch(key)) {
        QString format = re.cap(1).trimmed();
        QString date = re.cap(2).trimmed().toLower();
        QString timezone = re.cap(3).trimmed().toUpper();
        // LATER handle more timezones, such as GMT+05:30
        QDateTime dt = (timezone == "Z" || timezone == "GMT"
                        || timezone == "UTC")
            ? QDateTime::currentDateTimeUtc()
            : QDateTime::currentDateTime();
        RelativeDateTime rdt(date);
        dt = rdt.apply(dt);
        if (format.isEmpty()) {
          return dt.toString("yyyy-MM-dd hh:mm:ss,zzz");
        } else {
          if (format == "ms1970") {
            return QString::number(dt.toMSecsSinceEpoch());
          } else if (format == "s1970") {
            return QString::number(dt.toMSecsSinceEpoch()/1000);
          } else {
            return dt.toString(format);
          }
        }
      } else {
        //qDebug() << "%!date function invalid syntax:" << key;
      }
    } else if (key.startsWith("!default")) {
      /* %!default function: %!default!variable!value_if_not_set
       * value_if_not_set defaults to an empty string (the whole expression
       * being equivalent to %variable apart of the absence of warning due to
       * undefined variable evaluation)
       * it works like %{variable:-value_if_not_set} in shell scripts and almost
       * like nvl/ifnull functions in sql
       *
       * examples:
       * %{!default!foo!null}
       * %{!default!foo!foo not set}
       * %{!default!foo!foo not set!!!}
       * %{!default!foo!%bar}
       * %{!default!foo}
       */
      QRegExp re = defaultFunctionRE;
      if (re.exactMatch(key)) {
        QString variable = re.cap(1);
        QString valueIfNotSet = re.cap(2);
        QString value;
        if (!appendVariableValue(&value, variable, inherit, context,
                                alreadyEvaluated, false)) {
          value = evaluate(valueIfNotSet, inherit, context, alreadyEvaluated);
          //qDebug() << "!default:" << key << valueIfNotSet << value;
        }
        return value;
      } else {
        //qDebug() << "%!default function invalid syntax:" << key;
      }
    }
  }
  return QString();
}

bool ParamSet::appendVariableValue(
    QString *value, QString variable, bool inherit,
    const ParamsProvider *context, QSet<QString> alreadyEvaluated,
    bool logIfVariableNotFound) const {
  if (variable.isEmpty()) {
    Log::warning() << "unsupported variable substitution: empty variable name";
    return false;
  } else if (alreadyEvaluated.contains(variable)) {
    Log::warning() << "unsupported variable substitution: loop detected with "
                      "variable \"" << variable << "\"";
    return false;
  }
  QString s = evaluateImplicitVariable(
        variable, inherit, context, alreadyEvaluated);
  if (!s.isNull()) {
    value->append(s);
    return true;
  }
  if (context) {
    s = context->paramValue(variable, QVariant(), alreadyEvaluated).toString();
    if (!s.isNull()) {
      value->append(s);
      return true;
    }
  }
  s = this->rawValue(variable, inherit);
  if (!s.isNull()) {
    value->append(s);
    return true;
  }
  if (logIfVariableNotFound) {
    Log::debug()
        << "unsupported variable substitution: variable not found: "
           "%{" << variable << "} in paramset " << toString(false)
        << " " << d.constData() << " parent "
        << parent().toString(false);
  }
  return false;
}

QStringList ParamSet::splitAndEvaluate(
    QString rawValue, QString separator, bool inherit,
    const ParamsProvider *context, QSet<QString> alreadyEvaluated) const {
  QStringList values;
  QString value, variable;
  int i = 0;
  while (i < rawValue.size()) {
    QChar c = rawValue.at(i++);
    if (c == '%') {
      QSet<QString> nowEvaluated = alreadyEvaluated;
      c = rawValue.at(i++);
      if (c == '{') {
        while (i < rawValue.size()) {
          c = rawValue.at(i++);
          if (c == '}')
            break;
          else
            variable.append(c);
        }
        appendVariableValue(&value, variable, inherit, context,
                            alreadyEvaluated, true);
        nowEvaluated.insert(variable);
        value = evaluate(value, inherit, context, nowEvaluated);
        variable.clear();
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
        appendVariableValue(&value, variable, inherit, context,
                            alreadyEvaluated, true);
        nowEvaluated.insert(variable);
        value = evaluate(value, inherit, context, nowEvaluated);
        variable.clear();
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

QVariant ParamSet::paramValue(
    QString key, QVariant defaultValue, QSet<QString> alreadyEvaluated) const {
  return evaluate(rawValue(key, defaultValue.toString(), true),
                  true, 0, alreadyEvaluated);
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
