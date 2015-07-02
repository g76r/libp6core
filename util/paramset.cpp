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
#include "timeformats.h"
#include "characterseparatedexpression.h"
#include "regexpparamsprovider.h"
#include "paramsprovidermerger.h"
#include "htmlutils.h"

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

// LATER add functions: %=ifgt %=ifgte %=iflt %=iflte
QString ParamSet::evaluateImplicitVariable(
    QString key, bool inherit, const ParamsProvider *context,
    QSet<QString> alreadyEvaluated) const {
  if (key.at(0) == '=') {
    if (key.startsWith("=date")) {
      return TimeFormats::toMultifieldSpecifiedCustomTimestamp(
            QDateTime::currentDateTime(), key.mid(5));
    } else if (key.startsWith("=default")) {
      CharacterSeparatedExpression params(key, 8);
      QString value;
      if (params.size() >= 1) {
        for (int i = 0; i < params.size()-1; ++i) {
          if (appendVariableValue(&value, params.value(i), inherit, context,
                                  alreadyEvaluated, false))
            return value;
        }
        if (params.size() >= 2)
          value = evaluate(params.value(params.size()-1), inherit, context,
                           alreadyEvaluated);
        return value;
      }
      return value;
    } else if (key.startsWith("=ifneq")) {
      CharacterSeparatedExpression params(key, 6);
      if (params.size() >= 3) {
        QString input = evaluate(params.value(0), inherit, context,
                                 alreadyEvaluated);
        QString ref = evaluate(params.value(1), inherit, context,
                               alreadyEvaluated);
        if (input != ref) {
          return evaluate(params.value(2), inherit, context, alreadyEvaluated);
        } else {
          return params.size() >= 4
              ? evaluate(params.value(3), inherit, context, alreadyEvaluated)
              : input;
        }
      } else {
        //qDebug() << "%=ifneq function invalid syntax:" << key;
      }
    } else if (key.startsWith("=switch")) {
      CharacterSeparatedExpression params(key, 7);
      if (params.size() >= 1) {
        QString input = evaluate(params.value(0), inherit, context,
                                 alreadyEvaluated);
        // evaluating :case:value params, if any
        int n = (params.size() - 1) / 2;
        for (int i = 0; i < n; ++i) {
          QString ref = evaluate(params.value(1+i*2), inherit, context,
                                 alreadyEvaluated);
          if (input == ref)
            return evaluate(params.value(1+i*2+1), inherit, context,
                            alreadyEvaluated);
        }
        // evaluating :default param, if any
        if (params.size() % 2 == 0) {
          return evaluate(params.value(params.size()-1), inherit, context,
                          alreadyEvaluated);
        }
        // otherwise left input as is
        return input;
      } else {
        //qDebug() << "%=switch function invalid syntax:" << key;
      }
    } else if (key.startsWith("=sub")) {
      CharacterSeparatedExpression params(key, 4);
      //qDebug() << "%=sub:" << key << params.size() << params;
      QString value = evaluate(params.value(0), inherit, context,
                               alreadyEvaluated);
      for (int i = 1; i < params.size(); ++i) {
        CharacterSeparatedExpression sFields(params[i]);
        //qDebug() << "pattern" << i << params[i] << sFields.size() << sFields;
        QString optionsString = sFields.value(2);
        QRegularExpression::PatternOption patternOptions
            = QRegularExpression::NoPatternOption;
        if (optionsString.contains('i'))
          patternOptions = QRegularExpression::CaseInsensitiveOption;
        QRegularExpression re(sFields.value(0), patternOptions);
        if (!re.isValid()) {
          qDebug() << "%=sub with invalid regular expression: "
                   << sFields.value(0);
          continue;
        }
        bool repeat = optionsString.contains('g');
        int offset = 0;
        QString transformed;
        do {
          QRegularExpressionMatch match = re.match(value, offset);
          if (match.hasMatch()) {
            //qDebug() << "match:" << match.captured()
            //         << value.mid(offset, match.capturedStart()-offset);
            // append text between previous match and start of this match
            transformed += value.mid(offset, match.capturedStart()-offset);
            // replace current match with (evaluated) replacement string
            RegexpParamsProvider repp(match);
            ParamsProviderMerger reContext =
                ParamsProviderMerger(&repp)(context);
            transformed += evaluate(sFields.value(1), inherit, &reContext,
                                    alreadyEvaluated);
            // skip current match for next iteration
            offset = match.capturedEnd();
          } else {
            //qDebug() << "no more match:" << value.mid(offset);
            // append text between previous match and end of value
            transformed += value.mid(offset);
            // stop matching
            repeat = false;
          }
          //qDebug() << "transformed:" << transformed;
        } while(repeat);
        value = transformed;
      }
      //qDebug() << "value:" << value;
      return value;
    } else if (key.startsWith("=left")) {
      CharacterSeparatedExpression params(key, 5);
      QString input = evaluate(params.value(0), inherit, context,
                               alreadyEvaluated);
      bool ok;
      int i = params.value(1).toInt(&ok);
      return ok ? input.left(i) : QString();
    } else if (key.startsWith("=right")) {
      CharacterSeparatedExpression params(key, 6);
      QString input = evaluate(params.value(0), inherit, context,
                               alreadyEvaluated);
      bool ok;
      int i = params.value(1).toInt(&ok);
      return ok ? input.right(i) : QString();
    } else if (key.startsWith("=mid")) {
      CharacterSeparatedExpression params(key, 4);
      QString input = evaluate(params.value(0), inherit, context,
                               alreadyEvaluated);
      bool ok;
      int i = params.value(1).toInt(&ok);
      if (ok) {
        int j = params.value(2).toInt(&ok);
        return input.mid(i, ok ? j : -1);
      }
      return QString();
    } else if (key.startsWith("=htmlencode")) {
      // LATER provide more options such as encoding <br> or links, through e.g. and =htmlencodeext function
      QString input = evaluate(key.mid(12), inherit, context,
                               alreadyEvaluated);
      return HtmlUtils::htmlEncode(input, false, false);
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
  bool isToplevelEvaluation = false;
  if (!alreadyEvaluated.contains(QStringLiteral("%"))) {
    alreadyEvaluated.insert(QStringLiteral("%"));
    isToplevelEvaluation = true;
  }
  while (i < rawValue.size()) {
    QChar c = rawValue.at(i++);
    if (c == '%') {
      QSet<QString> nowEvaluated = alreadyEvaluated;
      c = rawValue.at(i++);
      if (c == '{') {
        // '{' and '}' are used as variable name delimiters, the way a Unix
        // shell use them along with $
        // e.g. "%{name.with#strange|characters}"
        int depth = 1;
        while (i < rawValue.size()) {
          c = rawValue.at(i++);
          // LATER provide a way to escape { and }
          if (c == '{')
            ++depth;
          else if (c == '}')
            --depth;
          if (depth == 0)
            break;
          variable.append(c);
        }
        appendVariableValue(&value, variable, inherit, context,
                            alreadyEvaluated, true);
        nowEvaluated.insert(variable);
        value = evaluate(value, inherit, context, nowEvaluated);
        variable.clear();
      } else if (c == '%') {
        // %% is used as an escape sequence for %
        // but must not be replaced with % during recursive evaluation
        if (isToplevelEvaluation)
          value.append(c);
        else
          value.append(QStringLiteral("%%"));
      } else {
        // any other character, e.g. '=', is interpreted as the first
        // character of a variable name that will continue with letters
        // digits and underscores
        // e.g. "=date" in "=date-foo"
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
  static QRegularExpression whitespace("\\s");
  QString v = rawValue(key, inherit).trimmed();
  int i = v.indexOf(whitespace);
  if (i == -1)
    return QPair<QString,QString>(v,QString());
  return QPair<QString,QString>(v.left(i),
                                evaluate(v.mid(i+1).trimmed(), context));
}

static const char matchingPatternSpecialChars[] = "*?[]\\";
#define CONTAINS(array, item) (::memchr((array), (item), sizeof(array)/sizeof(*(array))))

QString ParamSet::matchingPattern(QString rawValue) {
  int i = 0;
  QString value, variable;
  while (i < rawValue.size()) {
    QChar c = rawValue.at(i++);
    if (c == '%') {
      variable.clear();
      c = rawValue.at(i++);
      // FIXME support for { imbrication
      if (c == '{') {
        while (i < rawValue.size()) {
          c = rawValue.at(i++);
          if (c == '}')
            break;
          else
            variable.append(c);
        }
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
        value.append("*");
      } else {
        value.append(CONTAINS(matchingPatternSpecialChars, c.toLatin1())
                     ? '?' : c);
      }
    } else {
      value.append(CONTAINS(matchingPatternSpecialChars, c.toLatin1())
                   ? '?' : c);
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
