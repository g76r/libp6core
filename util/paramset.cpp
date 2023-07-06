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
#include "paramset.h"
#include <QSharedData>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QtDebug>
#include "format/timeformats.h"
#include "characterseparatedexpression.h"
#include "regexpparamsprovider.h"
#include "paramsprovidermerger.h"
#include "format/stringutils.h"
#include <stdlib.h>
#include "radixtree.h"
#include <functional>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include "pf/pfnode.h"
#include "util/mathexpr.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QSqlError>
#include "pf/pfutils.h"
#include "csv/csvfile.h"
#include <QMap>
#include <QMutex>
#include <QProcess>
#include <QTimer>
#include "util/utf8string.h"

bool ParamSet::_variableNotFoundLoggingEnabled { false };

static QMap<QByteArray,ParamSet> _externals;

QMutex _externals_mutex;

static int staticInit() {
  if (qgetenv("ENABLE_PARAMSET_VARIABLE_NOT_FOUND_LOGGING") == "true")
    ParamSet::enableVariableNotFoundLogging();
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

/** Tests if a character is allowed in % expression variable identifiers as
 * a second character and beyond (first character can be anything apart of
 * syntaxicaly meaningful characters such as % and {, including e.g. !).
 */
static inline bool isPercentVariableIdentifierSecondCharacter(char c) {
  return (c >= '0' && c <= '9')
      || (c >= 'A' && c <= 'Z')
      || c == '_'
      || (c >= 'a' && c <= 'z');
}

static inline bool shouldBeEscapedWithinRegexp(char c) {
  switch (c) {
  case '$':
  case '(':
  case ')':
  case '*':
  case '+':
  case '.':
  case '?':
  case '[':
  case '\\':
  case ']':
  case '^':
  case '{':
  case '|':
  case '}':
    return true;
  default:
    return false;
  }
}

/** Read rawValue begining at index percentPosition, which should be set just
 * on a % character, and return the length of the whole % expression, including
 * the leading % and nested % expressions if any.
 * Returns 1 if % is the last character, 2 if % is followed by another % and 0
 * if percentPosition is beyond rawValue end.
 * E.g. ("foobar%{xy%{z%1}}baz", 6) -> 11
 */
static inline int nestedPercentVariableLength(
    const QString &rawValue, int percentPosition) {
  int i = percentPosition+1, len = rawValue.size();
  if (i > len)
    return 0;
  if (i >= len)
    return 1;
  QChar c = rawValue[i];
  if (c == '%')
    return 2;
  if (c == '{') {
    while (i < len) {
      c = rawValue[++i];
      if (c == '}')
        break;
      if (c == '%')
        i += nestedPercentVariableLength(rawValue, i)-1;
    }
    return i-percentPosition+1;
  }
  // any other character, e.g. 'a' or '=', is interpreted as the first
  // character of a variable name that will continue with letters
  // digits and underscores
  // e.g. "abc",  "23", "=date", "!foobar"
  ++i;
  while (i < len) {
    c = rawValue[++i];
    if (!isPercentVariableIdentifierSecondCharacter(c.toLatin1()))
      break;
  }
  return i-percentPosition;
}

class ParamSetData : public QSharedData {
public:
  ParamSet _parent;
  QHash<QString,QString> _params;
  ParamSetData() { }
  ParamSetData(QHash<QString,QString> params) : _params(params) { }
  ParamSetData(ParamSet parent) : _parent(parent) { }
};

ParamSet::ParamSet() {
}

ParamSet::ParamSet(ParamSetData *data) : d(data){
}

ParamSet::ParamSet(std::initializer_list<QString> list) {
  for (auto it = std::begin(list); it != std::end(list); ++it) {
    auto key = *it;
    ++it;
    if (it != std::end(list)) {
      setValue(key, *it);
    } else {
      setValue(key, QStringLiteral(""));
      break;
    }
  }
}

ParamSet::ParamSet(std::initializer_list<std::pair<QString,QVariant>> list) {
  for (const std::pair<QString,QVariant> &p : list)
    setValue(p.first, p.second.toString());
}

ParamSet::ParamSet(const ParamSet &other) : d(other.d) {
}

ParamSet::ParamSet(const QHash<QString, QString> &params)
  : d(new ParamSetData(params)) {
}

ParamSet::ParamSet(const QHash<QByteArray,QByteArray> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(const QMap<QString, QString> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(const QMap<QByteArray,QByteArray> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(const QMultiMap<QString, QString> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(const QMultiHash<QString, QString> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(
  const PfNode &parentnode, const QString &attrname,
  const QString &constattrname, const ParamSet &parent)
  : d(new ParamSetData(parent)) {
  if (!attrname.isEmpty()) {
    for (auto p: parentnode.stringsPairChildrenByName(attrname)) {
      if (p.first.isEmpty())
        continue;
      auto value = p.second;
      d->_params.insert(p.first, value.isNull() ? QStringLiteral("") : value);
    }
  }
  if (!constattrname.isEmpty()) {
    ParamSet constparams(parentnode, constattrname, QString(), ParamSet());
    for (auto k: constparams.keys()) {
      auto value = escape(constparams.value(k, this));
      d->_params.insert(k, value.isNull() ? QStringLiteral("") : value);
    }
  }
  if (d->_params.isEmpty() && parent.isNull())
    d.reset();
}

ParamSet::ParamSet(
  const PfNode &parentnode, const QString &attrname, const ParamSet &parent)
  : ParamSet(parentnode, attrname, QString(), parent) {
}

ParamSet::ParamSet(
  const PfNode &parentnode, const QSet<QString> &attrnames,
  const ParamSet &parent) : d(new ParamSetData(parent)) {
  for (const PfNode &child : parentnode.children())
    if (attrnames.contains(child.name()))
      d->_params.insert(child.name(), child.contentAsString());
  if (d->_params.isEmpty() && parent.isNull())
    d.reset();
}

ParamSet::ParamSet(
  const QSqlDatabase &db, const QString &sql, const QMap<int,QString> &bindings,
  const ParamSet &parent) : d(new ParamSetData(parent)) {
  QSqlQuery query(db);
  query.prepare(parent.evaluate(sql));
  if (!query.exec()) {
    QSqlError error = query.lastError();
    Log::warning() << "failure trying to load params from SQL query: "
                   << " error: " << error.nativeErrorCode() << " "
                   << error.driverText() << " " << error.databaseText()
                   << " " << sql;
    return;
  }
  QMap<int,QStringList> values;
  while (query.next()) {
    auto r = query.record();
    for (int i = 0; i < r.count(); ++i) {
      if (!bindings.contains(i))
        continue;
      auto s = r.field(i).value().toString();
      if (s.isEmpty()) // ignoring both nulls and empty strings
        continue;
      values[i].append(escape(s));
    }
  }
  for (auto i: bindings.keys()) {
    setValue(bindings.value(i), values[i].join(" "));
  }
}

ParamSet::ParamSet(
    QIODevice *input, const QByteArray &format,
    const QMap<QByteArray,QByteArray> options, const bool escape_percent,
    const ParamSet &parent)
  : d(fromQIODevice(input, format, options, escape_percent, parent)) {
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

void ParamSet::setValues(ParamSet params, bool inherit) {
  if (!d)
    d = new ParamSetData();
  for (auto k: params.keys(inherit))
    d->_params.insert(k, params.rawValue(k));
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
  if (d) [[likely]] {
    value = d->_params.value(key);
    if (value.isNull() && inherit)
      value = parent().rawValue(key);
  }
  return value.isNull() ? defaultValue : value;
}

QString ParamSet::evaluate(
  QString rawValue, bool inherit, const ParamsProvider *context,
    QSet<QString> *alreadyEvaluated) const {
  //Log::debug() << "evaluate " << rawValue << " " << QString::number((qint64)context, 16);
  QStringList values = splitAndEvaluate(rawValue, QString(), inherit, context,
                                        alreadyEvaluated);
  if (values.isEmpty())
    return rawValue.isNull() ? QString{} : u""_s;
  return values.first();
}

static RadixTree<std::function<
QString(const ParamSet &params, const QString &key, bool inherit,
const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
        int matchedLength)>>
_functions {
{ "'", [](const ParamSet &, const QString &key, bool, const ParamsProvider *,
          QSet<QString> *, int) {
  return key.mid(1);
 }, true},
{ "=date", [](const ParamSet &paramset, const QString &key, bool inherit,
     const ParamsProvider *context, QSet<QString> *alreayEvaluated,
     int matchedLength) {
  return TimeFormats::toMultifieldSpecifiedCustomTimestamp(
        QDateTime::currentDateTime(), key.mid(matchedLength), paramset,
        inherit, context, alreayEvaluated);
}, true},
{ "=coarsetimeinterval", [](const ParamSet &paramset, const QString &key,
     bool inherit, const ParamsProvider *context,
     QSet<QString> *alreadyEvaluated, int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  qint64 msecs = (qint64)(
        paramset.evaluate(params.value(0), inherit, context, alreadyEvaluated)
        .toDouble()*1000);
  return TimeFormats::toCoarseHumanReadableTimeInterval(msecs);
}, true},
{ "=eval", [](const ParamSet &paramset, const QString &key, bool inherit,
     const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
     int matchedLength) {
  QString value = paramset.evaluate(
        key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return paramset.evaluate(value, inherit, context, alreadyEvaluated);
}, true},
{ "=escape", [](const ParamSet &paramset, const QString &key, bool inherit,
     const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
     int matchedLength) {
  QString value = paramset.evaluate(
        key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return ParamSet::escape(value);
}, true},
{ "=default", [](const ParamSet &paramset, const QString &key, bool inherit,
              const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  QString value;
  for (int i = 0; i < params.size(); ++i) {
    value = paramset.evaluate(params.value(i), inherit, context,
                              alreadyEvaluated);
    if (!value.isEmpty())
      break;
  }
  return value;
}, true},
{ "=rawvalue", [](const ParamSet &paramset, const QString &key, bool,
              const ParamsProvider *, QSet<QString> *, int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  if (params.size() < 1)
    return QString();
  QString value = paramset.rawValue(params.value(0));
  if (params.size() >= 2) {
    QString flags = params.value(1);
    if (flags.contains('e')) { // %-escape
      value = ParamSet::escape(value);
    }
    if (flags.contains('h')) { // htmlencode
      value = StringUtils::htmlEncode(
            value, flags.contains('u'), // url as links
            flags.contains('n')); // newline as <br>
    }
  }
  return value;
}, true},
{ "=ifneq", [](const ParamSet &paramset, const QString &key, bool inherit,
              const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  if (params.size() >= 3) [[likely]] {
    QString input = paramset.evaluate(params.value(0), inherit, context,
                                      alreadyEvaluated);
    QString ref = paramset.evaluate(params.value(1), inherit, context,
                                    alreadyEvaluated);
    if (input != ref)
      return paramset.evaluate(params.value(2), inherit, context,
                               alreadyEvaluated);
    return params.size() >= 4
        ? paramset.evaluate(params.value(3), inherit, context,
                            alreadyEvaluated)
        : input;
  }
  //qDebug() << "%=ifneq function invalid syntax:" << key;
  return QString();
}, true},
{ "=switch", [](const ParamSet &paramset, const QString &key, bool inherit,
              const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  if (params.size() >= 1) [[likely]] {
    QString input = paramset.evaluate(params.value(0), inherit, context,
                                      alreadyEvaluated);
    // evaluating :case:value params, if any
    int n = (params.size() - 1) / 2;
    for (int i = 0; i < n; ++i) {
      QString ref = paramset.evaluate(params.value(1+i*2), inherit, context,
                                      alreadyEvaluated);
      if (input == ref)
        return paramset.evaluate(params.value(1+i*2+1), inherit, context,
                                 alreadyEvaluated);
    }
    // evaluating :default param, if any
    if (params.size() % 2 == 0) {
      return paramset.evaluate(params.value(params.size()-1), inherit, context,
                               alreadyEvaluated);
    }
    // otherwise left input as is
    return input;
  }
  return QString();
}, true},
{ "=match", [](const ParamSet &paramset, const QString &key, bool inherit,
              const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  if (params.size() >= 1) [[likely]] {
    QString input = paramset.evaluate(params.value(0), inherit, context,
                                      alreadyEvaluated);
    // evaluating :case:value params, if any
    int n = (params.size() - 1) / 2;
    for (int i = 0; i < n; ++i) {
      QString ref = paramset.evaluate(params.value(1+i*2), inherit, context,
                                      alreadyEvaluated);
      QRegularExpression re(ref, QRegularExpression::DotMatchesEverythingOption // can be canceled with (?-s)
                            );
      if (re.match(input).hasMatch())
        return paramset.evaluate(params.value(1+i*2+1), inherit, context,
                                 alreadyEvaluated);
    }
    // evaluating :default param, if any
    if (params.size() % 2 == 0) {
      return paramset.evaluate(params.value(params.size()-1), inherit, context,
                               alreadyEvaluated);
    }
    // otherwise left input as is
    return input;
  }
  return QString();
}, true},
{ "=sub", [](const ParamSet &paramset, const QString &key, bool inherit,
              const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  //qDebug() << "%=sub:" << key << params.size() << params;
  QString value = paramset.evaluate(params.value(0), inherit, context,
                                    alreadyEvaluated);
  for (int i = 1; i < params.size(); ++i) {
    CharacterSeparatedExpression sFields(params[i]);
    //qDebug() << "pattern" << i << params[i] << sFields.size() << sFields;
    QString optionsString = sFields.value(2);
    QRegularExpression::PatternOptions patternOptions
        = QRegularExpression::DotMatchesEverythingOption // can be canceled with (?-s)
      ;
    if (optionsString.contains('i'))
      patternOptions |= QRegularExpression::CaseInsensitiveOption;
    // LATER add support for other available options: s,x...
    // LATER add a regexp cache because same =sub is likely to be evaluated several times
    // options must be part of the cache key
    // not sure if QRegularExpression::optimize() should be called
    QRegularExpression re(sFields.value(0), patternOptions);
    if (!re.isValid()) [[unlikely]] {
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
        transformed += paramset.evaluate(sFields.value(1), inherit, &reContext,
                                         alreadyEvaluated);
        // skip current match for next iteration
        offset = match.capturedEnd();
      } else {
        // stop matching
        repeat = false;
      }
      if (!repeat) {
        //qDebug() << "no more match:" << value.mid(offset);
        // append text between previous match and end of value
        transformed += value.mid(offset);
      }
      //qDebug() << "transformed:" << transformed;
    } while(repeat);
    value = transformed;
  }
  //qDebug() << "value:" << value;
  return value;
}, true},
{ "=left", [](const ParamSet &paramset, const QString &key, bool inherit,
              const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  QString input = paramset.evaluate(params.value(0), inherit, context,
                                    alreadyEvaluated);
  bool ok;
  int i = params.value(1).toInt(&ok);
  return ok ? input.left(i) : input;
}, true},
{ "=right", [](const ParamSet &paramset, const QString &key, bool inherit,
              const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  QString input = paramset.evaluate(params.value(0), inherit, context,
                                    alreadyEvaluated);
  bool ok;
  int i = params.value(1).toInt(&ok);
  return ok ? input.right(i) : input;
}, true},
{ "=mid", [](const ParamSet &paramset, const QString &key, bool inherit,
              const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  QString input = paramset.evaluate(params.value(0), inherit, context,
                                    alreadyEvaluated);
  bool ok;
  int i = params.value(1).toInt(&ok);
  if (ok) {
    int j = params.value(2).toInt(&ok);
    return input.mid(i, ok ? j : -1);
  }
  return input;
}, true},
{ "=trim", [](const ParamSet &paramset, const QString &key, bool inherit,
             const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
             int matchedLength) {
   auto input = paramset.evaluate(key.mid(matchedLength), inherit, context,
                                  alreadyEvaluated);
   return input.trimmed();
}, true},
{ "=elideright", [](const ParamSet &paramset, const QString &key, bool inherit,
              const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  QString input = paramset.evaluate(params.value(0), inherit, context,
                                    alreadyEvaluated);
  bool ok;
  int i = params.value(1).toInt(&ok);
  QString placeHolder = params.value(2, QStringLiteral("..."));
  if (!ok || placeHolder.size() > i || input.size() <= i)
    return input;
  return StringUtils::elideRight(input, i, placeHolder);
}, true},
{ "=elideleft", [](const ParamSet &paramset, const QString &key, bool inherit,
              const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  QString input = paramset.evaluate(params.value(0), inherit, context,
                                    alreadyEvaluated);
  bool ok;
  int i = params.value(1).toInt(&ok);
  QString placeHolder = params.value(2, QStringLiteral("..."));
  if (!ok || placeHolder.size() > i || input.size() <= i)
    return input;
  return StringUtils::elideLeft(input, i, placeHolder);
}, true},
{ "=elidemiddle", [](const ParamSet &paramset, const QString &key, bool inherit,
              const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  QString input = paramset.evaluate(params.value(0), inherit, context,
                                    alreadyEvaluated);
  bool ok;
  int i = params.value(1).toInt(&ok);
  QString placeHolder = params.value(2, QStringLiteral("..."));
  if (!ok || placeHolder.size() > i || input.size() <= i)
    return input;
  return StringUtils::elideMiddle(input, i, placeHolder);
}, true},
{ "=htmlencode", [](const ParamSet &paramset, const QString &key, bool inherit,
              const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  if (params.size() < 1)
    return QString();
  QString input = paramset.evaluate(params.value(0),
                                    inherit, context, alreadyEvaluated);
  if (params.size() >= 2) {
    QString flags = params.value(1);
    return StringUtils::htmlEncode(
          input, flags.contains('u'), // url as links
          flags.contains('n')); // newline as <br>
  }
  return StringUtils::htmlEncode(input, false, false);
}, true},
{ "=random", [](const ParamSet &, const QString &key, bool,
              const ParamsProvider *, QSet<QString> *, int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  // TODO evaluate modulo and shift
  int modulo = abs(params.value(0).toInt());
  int shift = params.value(1).toInt();
  quint32 i = QRandomGenerator::global()->generate();
  if (modulo)
    i %= modulo;
  i += shift;
  return QString::number(i);
}, true},
{ "=env", [](const ParamSet &paramset, const QString &key, bool inherit,
              const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  auto env = ParamsProvider::environment();
  int i = 0;
  auto ppm = ParamsProviderMerger(paramset)(context);
  do {
    QString name = paramset.evaluate(params.value(i), inherit, context,
                                     alreadyEvaluated);
    auto v = env->paramValue(name, &ppm, QVariant(), alreadyEvaluated);
    if (v.isValid())
      return v.toString();
    ++i;
  } while (i < params.size()-1); // first param and then all but last one
  // otherwise last one if there are at less 2, otherwise a non-existent one
  return paramset.evaluate(params.value(i), inherit, context, alreadyEvaluated);
}, true},
{ "=ext", [](const ParamSet &paramset, const QString &key, bool inherit,
              const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  auto ext = ParamSet::externalParams(params.value(0).toUtf8());
  int i = 1;
  auto ppm = ParamsProviderMerger(paramset, inherit)(context);
  //qDebug() << "***password =ext" << ext << params.size() << params.value(i)
  //         << ext.toString();
  do {
    QString name = ppm.evaluate(params.value(i), alreadyEvaluated);
    auto v = ext.paramValue(name, &ppm, {}, alreadyEvaluated);
    if (v.isValid())
      return v.toString();
    ++i;
  } while (i < params.size()-1); // first param and then all but last one
  // otherwise last one if there are at less 2, otherwise a non-existent one
  return ppm.evaluate(params.value(i), alreadyEvaluated);
}, true},
{ "=sha1", [](const ParamSet &paramset, const QString &key, bool inherit,
     const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
     int matchedLength) {
  QString value = paramset.evaluate(
        key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return QCryptographicHash::hash(
        value.toUtf8(), QCryptographicHash::Sha1).toHex();
}, true},
{ "=sha256", [](const ParamSet &paramset, const QString &key, bool inherit,
     const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
     int matchedLength) {
  QString value = paramset.evaluate(
        key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return QCryptographicHash::hash(
        value.toUtf8(), QCryptographicHash::Sha256).toHex();
}, true},
{ "=md5", [](const ParamSet &paramset, const QString &key, bool inherit,
     const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
     int matchedLength) {
  QString value = paramset.evaluate(
        key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return QCryptographicHash::hash(
        value.toUtf8(), QCryptographicHash::Md5).toHex();
}, true},
{ "=hex", [](const ParamSet &paramset, const QString &key, bool inherit,
     const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
     int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  QString value = paramset.evaluate(
        params.value(0), inherit, context, alreadyEvaluated);
  QString separator = params.value(1);
  QString flags = params.value(2);
  QByteArray data = flags.contains('b') ? value.toLatin1() : value.toUtf8();
  if (separator.isEmpty())
    data = data.toHex();
  else
    data = data.toHex(separator.at(0).toLatin1());
  return QString::fromLatin1(data);
}, true},
{ "=fromhex", [](const ParamSet &paramset, const QString &key, bool inherit,
     const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
     int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  QString value = paramset.evaluate(
        params.value(0), inherit, context, alreadyEvaluated);
  QString flags = params.value(1);
  QByteArray data = QByteArray::fromHex(value.toLatin1());
  if (flags.contains('b'))
    return QString::fromLatin1(data);
  return QString::fromUtf8(data);
}, true},
{ "=base64", [](const ParamSet &paramset, const QString &key, bool inherit,
     const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
     int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  QString value = paramset.evaluate(
        params.value(0), inherit, context, alreadyEvaluated);
  QString flags = params.value(1);
  QByteArray data = flags.contains('b') ? value.toLatin1() : value.toUtf8();
  data = data.toBase64(
        (flags.contains('u') ? QByteArray::Base64UrlEncoding
                             : QByteArray::Base64Encoding)
        |(flags.contains('t') ? QByteArray::OmitTrailingEquals
                              : QByteArray::KeepTrailingEquals)
        );
  return data;
}, true},
{ "=frombase64", [](const ParamSet &paramset, const QString &key, bool inherit,
     const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
     int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  QString value = paramset.evaluate(
        params.value(0), inherit, context, alreadyEvaluated);
  QString flags = params.value(1);
  QByteArray data = QByteArray::fromBase64(
        value.toLatin1(),
        (flags.contains('u') ? QByteArray::Base64UrlEncoding
                             : QByteArray::Base64Encoding)
        |(flags.contains('a') ? QByteArray::AbortOnBase64DecodingErrors
                              : QByteArray::IgnoreBase64DecodingErrors)
        );
  if (flags.contains('b'))
    return QString::fromLatin1(data);
  return QString::fromUtf8(data);
}, true},
{ "=rpn", [](const ParamSet &paramset, const QString &key, bool,
      const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
      int matchedLength) {
   MathExpr expr(key.mid(matchedLength), MathExpr::CharacterSeparatedRpn);
   auto ppm = ParamsProviderMerger(paramset)(context);
   return expr.evaluate(&ppm, QString(), alreadyEvaluated).toString();
}, true},
};

QString ParamSet::evaluateFunction(
  const ParamSet &paramset, const QString &key, bool inherit,
  const ParamsProvider *context, QSet<QString> *alreayEvaluated, bool *found) {
  int matchedLength;
  auto implicitVariable = _functions.value(key, &matchedLength);
  if (implicitVariable) {
    auto s = implicitVariable(paramset, key, inherit, context,
                              alreayEvaluated, matchedLength);
    if (found)
      *found = true;
    return s;
  }
  if (found)
    *found = false;
  return QString();
}

bool ParamSet::appendVariableValue(
  QString *value, const QString &variable, bool inherit,
  const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
  bool logIfVariableNotFound) const {
  if (variable.isEmpty()) [[unlikely]] {
    Log::warning() << "unsupported variable substitution: empty variable name";
    return false;
  }
  if (!alreadyEvaluated) [[unlikely]] {
    Log::warning() << "unsupported variable substitution: loop detection is "
                      "broken";
    return false;
  }
  if (alreadyEvaluated->contains(variable)) [[unlikely]] {
    Log::warning() << "unsupported variable substitution: loop detected with "
                      "variable \"" << variable << "\"";
    return false;
  }
  QSet<QString> newAlreadyEvaluated = *alreadyEvaluated;
  newAlreadyEvaluated.insert(variable);
  bool functionFound = false;
  auto s = evaluateFunction(*this, variable, inherit, context,
                            &newAlreadyEvaluated, &functionFound);
  if (functionFound) {
    value->append(s);
    return true;
  }
  if (context) {
    s = context->paramValue(variable, QVariant(), &newAlreadyEvaluated).toString();
    if (!s.isNull()) {
      value->append(s);
      return true;
    }
  }
  s = this->value(variable, inherit, context, &newAlreadyEvaluated);
  if (!s.isNull()) {
    value->append(s);
    return true;
  }
  if (_variableNotFoundLoggingEnabled && logIfVariableNotFound) [[unlikely]] {
    Log::debug()
        << "unsupported variable substitution: variable not found: "
           "%{" << variable << "} in paramset " << toString(false)
        << " " << d.constData() << " parent "
        << parent().toString(false);
  }
  return false;
}

QStringList ParamSet::splitAndEvaluate(
  QString rawValue, QString separators, bool inherit,
    const ParamsProvider *context, QSet<QString> *alreadyEvaluated) const {
  QStringList values;
  QString value, variable;
  int i = 0;
  while (i < rawValue.size()) {
    QChar c = rawValue.at(i++);
    if (c == '%') {
      if (i < rawValue.size()) // otherwise keep % and process as %%
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
        variable.clear();
      } else if (c == '%') {
        // %% is used as an escape sequence for %
        value.append(c);
      } else {
        // any other character, e.g. 'a' or '=', is interpreted as the first
        // character of a variable name that will continue with letters
        // digits and underscores
        // e.g. "abc",  "23", "=date", "!foobar"
        variable.append(c);
        while (i < rawValue.size()) {
          c = rawValue.at(i++);
          if (isPercentVariableIdentifierSecondCharacter(c.toLatin1())) {
            variable.append(c);
          } else {
            --i;
            break;
          }
        }
        appendVariableValue(&value, variable, inherit, context,
                            alreadyEvaluated, true);
        variable.clear();
      }
    } else if (!separators.isEmpty() && c =='\\') {
      if (i < rawValue.size()) // otherwise process as double backslash
        c = rawValue.at(i++);
      value.append(c);
    } else if (separators.contains(c)) {
      if (!value.isEmpty())
        values.append(value);
      value.clear();
    } else [[likely]] {
      value.append(c);
    }
  }
  if (!value.isEmpty())
    values.append(value);
  return values;
}

static QRegularExpression _whitespace("\\s");

const QPair<QString, QString> ParamSet::valueAsStringsPair(
  QString key, bool inherit, const ParamsProvider *context) const {
  QString v = rawValue(key, inherit).trimmed();
  int i = v.indexOf(_whitespace);
  if (i == -1)
    return { v, {} };
  return { v.left(i), evaluate(v.mid(i+1).trimmed(), context) };
}

const QString ParamSet::matchingRegexp(QString rawValue) {
  int i = 0, len = rawValue.size();
  QString value;
  while (i < len) {
    QChar c = rawValue[i];
    if (c == '%') {
      if (i+1 < len && rawValue[i+1] == '%') {
        value.append('%');
        i += 2; // skip next % too
      } else {
        i += nestedPercentVariableLength(rawValue, i);
        value.append(".*");
      }
    } else {
      if (shouldBeEscapedWithinRegexp(c.toLatin1()))
        value.append('\\');
      value.append(c);
      ++i;
    }
  }
  return value;
}

const QSet<QString> ParamSet::keys(bool inherit) const {
  QSet<QString> set;
  if (d) [[likely]] {
    auto keys = d->_params.keys();
#if QT_VERSION >= 0x050f00
    set = { keys.begin(), keys.end() };
#else
    set = keys.toSet();
#endif
    if (inherit)
      set += parent().keys();
  }
  return set;
}

const QSet<QString> ParamSet::keys() const {
  return keys(true);
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

const QVariant ParamSet::paramValue(
  const QString &key, const ParamsProvider *context,
  const QVariant &defaultValue, QSet<QString> *alreadyEvaluated) const {
  QString v = evaluate(rawValue(key, defaultValue.toString(), true),
                       true, context, alreadyEvaluated);
  return v.isNull() ? QVariant{} : v;
}

const QString ParamSet::toString(bool inherit, bool decorate) const {
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

const QHash<QString, QString> ParamSet::toHash(bool inherit) const {
  QHash<QString,QString> hash;
  for (auto key: keys(inherit))
    hash.insert(key, rawValue(key));
  return hash;
}

const QMap<QString,QString> ParamSet::toMap(bool inherit) const {
  QMap<QString,QString> map;
  for (auto key: keys(inherit))
    map.insert(key, rawValue(key));
  return map;
}

QDebug operator<<(QDebug dbg, const ParamSet &params) {
  dbg.nospace() << "{";
  auto keys = params.keys().values();
  std::sort(keys.begin(), keys.end());
  for (auto key: keys)
    dbg.space() << key << "=" << params.rawValue(key) << ",";
  dbg.nospace() << "}";
  return dbg.space();
}

LogHelper operator<<(LogHelper lh, const ParamSet &params) {
  lh << "{ ";
  auto keys = params.keys().values();
  std::sort(keys.begin(), keys.end());
  for (auto key: keys)
    lh << key << "=" << params.rawValue(key) << " ";
  return lh << "}";
}

void ParamSet::detach() {
  d.detach();
}

void ParamSet::setValuesFromSqlDb(
  QSqlDatabase db, QString sql, QMap<int,QString> bindings) {
  ParamSet params(db, sql, bindings, *this);
  this->setValues(params, false);
}

void ParamSet::setValuesFromSqlDb(
  QString dbname, QString sql, QMap<int,QString> bindings) {
  setValuesFromSqlDb(QSqlDatabase::database(dbname), sql, bindings);
}

void ParamSet::setValuesFromSqlDb(
  QString dbname, QString sql, QStringList bindings) {
  setValuesFromSqlDb(QSqlDatabase::database(dbname), sql, bindings);
}

void ParamSet::setValuesFromSqlDb(
  QSqlDatabase db, QString sql, QStringList bindings) {
  QMap<int,QString> map;
  int i = 0;
  for (auto key: bindings)
    map.insert(i++, key);
  setValuesFromSqlDb(db, sql, map);
}

qlonglong ParamSet::valueAsLong(
  QString key, qlonglong defaultValue, bool inherit,
  const ParamsProvider *context) const {
  auto s = evaluate(rawValue(key, QString(), inherit), inherit, context);
  return PfUtils::stringAsLongLong(s, defaultValue);
}

int ParamSet::valueAsInt(
  QString key, int defaultValue, bool inherit,
  const ParamsProvider *context) const {
  auto s = evaluate(rawValue(key, QString(), inherit), inherit, context);
  return PfUtils::stringAsInt(s, defaultValue);
}

double ParamSet::valueAsDouble(
  QString key, double defaultValue, bool inherit,
  const ParamsProvider *context) const {
  auto s = evaluate(rawValue(key, QString(), inherit), inherit, context);
  return PfUtils::stringAsDouble(s, defaultValue);
}

bool ParamSet::valueAsBool(
  QString key, bool defaultValue, bool inherit,
  const ParamsProvider *context) const {
  QString v = evaluate(rawValue(key, QString(), inherit), inherit, context);
  return PfUtils::stringAsBool(v, defaultValue);
}

ParamSetData *ParamSet::fromQIODevice(
    QIODevice *input, const QByteArray &format,
    const QMap<QByteArray,QByteArray> options,
    const bool escape_percent, const ParamSet &parent) {
  auto d = new ParamSetData(parent);
  if (!input || format != "csv")
    return d;
  if (!input->isOpen()) {
    if (!input->open(QIODevice::ReadOnly)) {
      QFile *file = qobject_cast<QFile*>(input);
      Log::error() << "cannot open file to read parameters: "
                   << (file ? file->fileName() : input->metaObject()->className())
                   << input->errorString();
      return d;
    }
  }
  auto separator = Utf8String(options.value("separator"_ba)).value(0,',');
  auto quote = Utf8String(options.value("quote"_ba)).value(0,'"');
  auto escape = Utf8String(options.value("escape"_ba)).value(0,'\\');
  CsvFile csvfile;
  csvfile.enableHeaders(false);
  csvfile.setFieldSeparator(separator);
  csvfile.setQuoteChar(quote);
  csvfile.setEscapeChar(escape);
  csvfile.openReadonly(input);
  auto rows = csvfile.rows();
  //qDebug() << "***password from csv" << rows << separator << quote << escape;
  for (auto row: rows) {
    auto key = row.value(0);
    auto value = row.value(1);
    if (key.isEmpty())
      continue;
    if (escape_percent)
      d->_params.insert(key, ParamSet::escape(value));
    else
      d->_params.insert(key, value);
  }
  return d;
}

ParamSet ParamSet::fromFile(
    const QByteArray &file_name, const QByteArray &format,
    const QMap<QByteArray,QByteArray> options,
    const bool escape_percent, const ParamSet &parent) {
  //qDebug() << "***password fromFile" << file_name;
  QFile file(file_name);
  return ParamSet(fromQIODevice(&file, format, options, escape_percent, parent));
}

ParamSet ParamSet::fromCommandOutput(
    const QStringList &cmdline, const QByteArray &format,
    const QMap<QByteArray,QByteArray> options,
    const bool escape_percent, const ParamSet &parent){
  //qDebug() << "***password fromCommandOutput" << cmdline;
  ParamSet params(parent);
  if (cmdline.size() < 1) {
    Log::error() << "cannot start external params command with empty cmdline";
    return params;
  }
  auto program = cmdline.value(0);
  auto args = cmdline.sliced(1);
  auto process = new QProcess;
  QObject::connect(process, &QProcess::finished,
                   [cmdline](int exitCode, QProcess::ExitStatus exitStatus) {
    bool success = (exitStatus == QProcess::NormalExit
                    && exitCode == 0);
    if (success)
      return;
    Log::error() << "cannot execute external params command " << cmdline
                 << ": process failed with exit code " << exitCode;
  });
  process->setStandardErrorFile(QProcess::nullDevice());
  QTimer::singleShot(10'000, process, &QProcess::terminate);
  process->start(program, args);
  if (!process->waitForStarted(10'000)) {
    Log::error() << "cannot start external params command " << cmdline;
    process->deleteLater();
    return params;
  }
  if (!process->waitForFinished(10'000)) {
    Log::error() << "cannot wait for external params command finishing "
                 << cmdline;
    process->deleteLater();
    return params;
  }
  auto output = process->readAllStandardOutput();
  QBuffer buffer(&output);
  params = fromQIODevice(&buffer, format, options, escape_percent, parent);
  process->deleteLater();
  return params;
}

ParamSet ParamSet::externalParams(QByteArray set_name) {
  QMutexLocker ml(&_externals_mutex);
  return _externals.value(set_name);
}

void ParamSet::registerExternalParams(
    const QByteArray &set_name, ParamSet params) {
  QMutexLocker ml(&_externals_mutex);
  //qDebug() << "***password registerExternalParams" << set_name << params.toString();
  _externals.insert(set_name, params);
}

void ParamSet::clearExternalParams() {
  QMutexLocker ml(&_externals_mutex);
  _externals.clear();
}

QByteArrayList ParamSet::externalParamsNames() {
  QMutexLocker ml(&_externals_mutex);
  auto list = _externals.keys();
  list.detach();
  return list;
}
