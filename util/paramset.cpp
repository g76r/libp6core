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

static QMap<Utf8String,ParamSet> _externals;

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
    const Utf8String &rawValue, int percentPosition) {
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
  QMap<Utf8String,Utf8String> _params;
  ParamSetData() { }
  ParamSetData(QMap<Utf8String,Utf8String> params) : _params(params) { }
  ParamSetData(ParamSet parent) : _parent(parent) { }
};

ParamSet::ParamSet() {
}

ParamSet::ParamSet(ParamSetData *data) : d(data){
}

ParamSet::ParamSet(std::initializer_list<Utf8String> list) {
  for (auto it = std::begin(list); it != std::end(list); ++it) {
    auto key = *it;
    ++it;
    if (it != std::end(list)) {
      setValue(key, *it);
    } else {
      setValue(key, ""_ba);
      break;
    }
  }
}

ParamSet::ParamSet(
    std::initializer_list<std::pair<Utf8String, QVariant> > list) {
  for (const std::pair<Utf8String,QVariant> &p : list)
    setValue(p.first, Utf8String(p.second));
}

ParamSet::ParamSet(const ParamSet &other) : d(other.d) {
}

ParamSet::ParamSet(const QHash<QString, QString> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(const QHash<Utf8String,Utf8String> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(const QMap<QString, QString> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(const QMap<Utf8String,Utf8String> &params)
  : d(new ParamSetData(params)) {
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

ParamSet::ParamSet(const QMultiMap<Utf8String, Utf8String> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(const QMultiHash<Utf8String, Utf8String> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(
  const PfNode &parentnode, const Utf8String &attrname,
  const Utf8String &constattrname, const ParamSet &parent)
  : d(new ParamSetData(parent)) {
  if (!attrname.isEmpty()) {
    for (auto p: parentnode.stringsPairChildrenByName(attrname)) {
      if (p.first.isEmpty())
        continue;
      Utf8String value = p.second;
      d->_params.insert(p.first, value.isNull() ? ""_u8 : value);
    }
  }
  if (!constattrname.isEmpty()) {
    ParamSet constparams(parentnode, constattrname, Utf8String(), ParamSet());
    for (auto k: constparams.keys()) {
      auto value = escape(constparams.value(k, this));
      d->_params.insert(k, value.isNull() ? ""_u8 : value);
    }
  }
  if (d->_params.isEmpty() && parent.isNull())
    d.reset();
}

ParamSet::ParamSet(const PfNode &parentnode, const Utf8String &attrname,
                   const ParamSet &parent)
  : ParamSet(parentnode, attrname, Utf8String(), parent) {
}

ParamSet::ParamSet(const PfNode &parentnode, const Utf8StringSet &attrnames,
  const ParamSet &parent) : d(new ParamSetData(parent)) {
  for (const PfNode &child : parentnode.children())
    if (attrnames.contains(child.name()))
      d->_params.insert(child.name(), child.contentAsString());
  if (d->_params.isEmpty() && parent.isNull())
    d.reset();
}

ParamSet::ParamSet(
    const QSqlDatabase &db, const Utf8String &sql,
    const QMap<int,Utf8String> &bindings, const ParamSet &parent)
  : d(new ParamSetData(parent)) {
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
  QMap<int,Utf8StringList> values;
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
    QIODevice *input, const Utf8String &format,
    const QMap<Utf8String,Utf8String> options, const bool escape_percent,
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

void ParamSet::setValue(Utf8String key, Utf8String value) {
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

void ParamSet::removeValue(Utf8String key) {
  if (d)
    d->_params.remove(key);
}

void ParamSet::clear() {
  d = new ParamSetData();
}

Utf8String ParamSet::rawValue(
    Utf8String key, Utf8String defaultValue, bool inherit) const {
  Utf8String value;
  if (d) [[likely]] {
    value = d->_params.value(key);
    if (value.isNull() && inherit)
      value = parent().rawValue(key);
  }
  return value.isNull() ? defaultValue : value;
}

Utf8String ParamSet::evaluate(
    Utf8String rawValue, bool inherit, const ParamsProvider *context,
    Utf8StringSet *alreadyEvaluated) const {
  //Log::debug() << "evaluate " << rawValue << " " << QString::number((qint64)context, 16);
  auto values = splitAndEvaluate(rawValue, Utf8String(), inherit, context,
                                 alreadyEvaluated);
  if (values.isEmpty())
    return rawValue.isNull() ? Utf8String{} : ""_u8;
  return values.first();
}

static RadixTree<std::function<
Utf8String(const ParamSet &params, const Utf8String &key, bool inherit,
const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
        int matchedLength)>>
_functions {
{ "'", [](const ParamSet &, const Utf8String &key, bool, const ParamsProvider *,
          Utf8StringSet *, int) {
  return key.mid(1);
 }, true},
{ "=date", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
     const ParamsProvider *context, Utf8StringSet *alreayEvaluated,
     int matchedLength) {
  return TimeFormats::toMultifieldSpecifiedCustomTimestamp(
        QDateTime::currentDateTime(), key.mid(matchedLength), paramset,
        inherit, context, alreayEvaluated);
}, true},
{ "=coarsetimeinterval", [](const ParamSet &paramset, const Utf8String &key,
     bool inherit, const ParamsProvider *context,
     Utf8StringSet *alreadyEvaluated, int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  qint64 msecs = (qint64)(
        paramset.evaluate(params.value(0), inherit, context, alreadyEvaluated)
        .toDouble()*1000);
  return TimeFormats::toCoarseHumanReadableTimeInterval(msecs);
}, true},
{ "=eval", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
     const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
     int matchedLength) {
  auto value = paramset.evaluate(
        key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return paramset.evaluate(value, inherit, context, alreadyEvaluated);
}, true},
{ "=escape", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
     const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
     int matchedLength) {
  auto value = paramset.evaluate(
        key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return ParamSet::escape(value);
}, true},
{ "=default", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
              const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  Utf8String value;
  for (int i = 0; i < params.size(); ++i) {
    value = paramset.evaluate(params.value(i), inherit, context,
                              alreadyEvaluated);
    if (!value.isEmpty())
      break;
  }
  return value;
}, true},
{ "=rawvalue", [](const ParamSet &paramset, const Utf8String &key, bool,
              const ParamsProvider *, Utf8StringSet *, int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  if (params.size() < 1)
    return Utf8String{};
  auto value = paramset.rawValue(params.value(0));
  if (params.size() >= 2) {
    auto flags = params.value(1);
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
{ "=ifneq", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
              const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  if (params.size() >= 3) [[likely]] {
    auto input = paramset.evaluate(params.value(0), inherit, context,
                                      alreadyEvaluated);
    auto ref = paramset.evaluate(params.value(1), inherit, context,
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
  return Utf8String{};
}, true},
{ "=switch", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
              const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  if (params.size() >= 1) [[likely]] {
    auto input = paramset.evaluate(params.value(0), inherit, context,
                                   alreadyEvaluated);
    // evaluating :case:value params, if any
    int n = (params.size() - 1) / 2;
    for (int i = 0; i < n; ++i) {
      auto ref = paramset.evaluate(params.value(1+i*2), inherit, context,
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
  return Utf8String();
}, true},
{ "=match", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
              const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  if (params.size() >= 1) [[likely]] {
    auto input = paramset.evaluate(params.value(0), inherit, context,
                                   alreadyEvaluated);
    // evaluating :case:value params, if any
    int n = (params.size() - 1) / 2;
    for (int i = 0; i < n; ++i) {
      auto ref = paramset.evaluate(params.value(1+i*2), inherit, context,
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
  return Utf8String();
}, true},
{ "=sub", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
              const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  //qDebug() << "%=sub:" << key << params.size() << params;
  auto value = paramset.evaluate(params.value(0), inherit, context,
                                 alreadyEvaluated);
  for (int i = 1; i < params.size(); ++i) {
    CharacterSeparatedExpression sFields(params[i]);
    //qDebug() << "pattern" << i << params[i] << sFields.size() << sFields;
    auto optionsString = sFields.value(2);
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
    Utf8String transformed;
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
{ "=left", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
              const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  auto input = paramset.evaluate(params.value(0), inherit, context,
                                 alreadyEvaluated);
  bool ok;
  int i = params.value(1).toInt(&ok);
  return ok ? input.left(i) : input;
}, true},
{ "=right", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
              const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  auto input = paramset.evaluate(params.value(0), inherit, context,
                                 alreadyEvaluated);
  bool ok;
  int i = params.value(1).toInt(&ok);
  return ok ? input.right(i) : input;
}, true},
{ "=mid", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
              const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
              int matchedLength) -> Utf8String {
  CharacterSeparatedExpression params(key, matchedLength);
  auto input = paramset.evaluate(params.value(0), inherit, context,
                                 alreadyEvaluated);
  bool ok;
  int i = params.value(1).toInt(&ok);
  if (ok) {
    int j = params.value(2).toInt(&ok);
    return input.mid(i, ok ? j : -1);
  }
  return input;
}, true},
{ "=trim", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
             const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
             int matchedLength) {
   auto input = paramset.evaluate(key.mid(matchedLength), inherit, context,
                                  alreadyEvaluated);
   return input.trimmed();
}, true},
{ "=elideright", [](
              const ParamSet &paramset, const Utf8String &key, bool inherit,
              const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
              int matchedLength) -> Utf8String {
  CharacterSeparatedExpression params(key, matchedLength);
  auto input = paramset.evaluate(params.value(0), inherit, context,
                                 alreadyEvaluated);
  bool ok;
  int i = params.value(1).toInt(&ok);
  auto placeHolder = params.value(2, "..."_u8);
  if (!ok || placeHolder.size() > i || input.size() <= i)
    return input;
  return StringUtils::elideRight(input, i, placeHolder);
}, true},
{ "=elideleft", [](
              const ParamSet &paramset, const Utf8String &key, bool inherit,
              const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
              int matchedLength) -> Utf8String {
  CharacterSeparatedExpression params(key, matchedLength);
  auto input = paramset.evaluate(params.value(0), inherit, context,
                                 alreadyEvaluated);
  bool ok;
  int i = params.value(1).toInt(&ok);
  auto placeHolder = params.value(2, "..."_u8);
  if (!ok || placeHolder.size() > i || input.size() <= i)
    return input;
  return StringUtils::elideLeft(input, i, placeHolder);
}, true},
{ "=elidemiddle", [](
              const ParamSet &paramset, const Utf8String &key, bool inherit,
              const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
              int matchedLength) -> Utf8String {
  CharacterSeparatedExpression params(key, matchedLength);
  auto input = paramset.evaluate(params.value(0), inherit, context,
                                 alreadyEvaluated);
  bool ok;
  int i = params.value(1).toInt(&ok);
  auto placeHolder = params.value(2, "..."_u8);
  if (!ok || placeHolder.size() > i || input.size() <= i)
    return input;
  return StringUtils::elideMiddle(input, i, placeHolder);
}, true},
{ "=htmlencode", [](
              const ParamSet &paramset, const Utf8String &key, bool inherit,
              const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
              int matchedLength) -> Utf8String {
  CharacterSeparatedExpression params(key, matchedLength);
  if (params.size() < 1)
    return Utf8String();
  auto input = paramset.evaluate(params.value(0),
                                 inherit, context, alreadyEvaluated);
  if (params.size() >= 2) {
    auto flags = params.value(1);
    return StringUtils::htmlEncode(
          input, flags.contains('u'), // url as links
          flags.contains('n')); // newline as <br>
  }
  return StringUtils::htmlEncode(input, false, false);
}, true},
{ "=random", [](const ParamSet &, const Utf8String &key, bool,
              const ParamsProvider *, Utf8StringSet *, int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  // TODO evaluate modulo and shift
  int modulo = abs(params.value(0).toInt());
  int shift = params.value(1).toInt();
  quint32 i = QRandomGenerator::global()->generate();
  if (modulo)
    i %= modulo;
  i += shift;
  return Utf8String::number(i);
}, true},
{ "=env", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
              const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
              int matchedLength) -> Utf8String {
  CharacterSeparatedExpression params(key, matchedLength);
  auto env = ParamsProvider::environment();
  int i = 0;
  auto ppm = ParamsProviderMerger(paramset)(context);
  do {
    auto name = paramset.evaluate(params.value(i), inherit, context,
                                  alreadyEvaluated);
    auto v = env->paramValue(name, &ppm, QVariant(), alreadyEvaluated);
    if (v.isValid())
      return v.toString();
    ++i;
  } while (i < params.size()-1); // first param and then all but last one
  // otherwise last one if there are at less 2, otherwise a non-existent one
  return paramset.evaluate(params.value(i), inherit, context, alreadyEvaluated);
}, true},
{ "=ext", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
              const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
              int matchedLength) -> Utf8String {
  CharacterSeparatedExpression params(key, matchedLength);
  auto ext = ParamSet::externalParams(params.value(0).toUtf8());
  int i = 1;
  auto ppm = ParamsProviderMerger(paramset, inherit)(context);
  //qDebug() << "***password =ext" << ext << params.size() << params.value(i)
  //         << ext.toString();
  do {
    auto name = ppm.evaluate(params.value(i), alreadyEvaluated);
    auto v = ext.paramValue(name, &ppm, {}, alreadyEvaluated);
    if (v.isValid())
      return v.toString();
    ++i;
  } while (i < params.size()-1); // first param and then all but last one
  // otherwise last one if there are at less 2, otherwise a non-existent one
  return ppm.evaluate(params.value(i), alreadyEvaluated);
}, true},
{ "=sha1", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
     const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
     int matchedLength) {
  auto value = paramset.evaluate(
        key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return QCryptographicHash::hash(
        value, QCryptographicHash::Sha1).toHex();
}, true},
{ "=sha256", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
     const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
     int matchedLength) {
  auto value = paramset.evaluate(
                 key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return QCryptographicHash::hash(
        value, QCryptographicHash::Sha256).toHex();
}, true},
{ "=md5", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
     const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
     int matchedLength) {
  auto value = paramset.evaluate(
        key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return QCryptographicHash::hash(
        value, QCryptographicHash::Md5).toHex();
}, true},
{ "=hex", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
     const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
     int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  auto value = paramset.evaluate(
        params.value(0), inherit, context, alreadyEvaluated);
  auto separator = params.value(1);
  auto flags = params.value(2);
  //QByteArray data = flags.contains('b') ? value.toLatin1() : value.toUtf8();
  QByteArray data = value; // FIXME what about b flag ?
  if (separator.isEmpty())
    data = data.toHex();
  else
    data = data.toHex(separator.at(0).toLatin1());
  return Utf8String(data);
}, true},
{ "=fromhex", [](const ParamSet &paramset, const QString &key, bool inherit,
     const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
     int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  auto value = paramset.evaluate(
        params.value(0), inherit, context, alreadyEvaluated);
  auto flags = params.value(1);
  QByteArray data = QByteArray::fromHex(value);
  if (flags.contains('b')) // FIXME b flag no longer relevant ?
    return Utf8String(data); // FIXME QByteArray instead ?
  return Utf8String(data);
}, true},
{ "=base64", [](const ParamSet &paramset, const Utf8String &key, bool inherit,
     const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
     int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  auto value = paramset.evaluate(
        params.value(0), inherit, context, alreadyEvaluated);
  auto flags = params.value(1);
  //QByteArray data = flags.contains('b') ? value.toLatin1() : value.toUtf8();
  QByteArray data = value; // FIXME what about b flag ?
  data = data.toBase64(
        (flags.contains('u') ? QByteArray::Base64UrlEncoding
                             : QByteArray::Base64Encoding)
        |(flags.contains('t') ? QByteArray::OmitTrailingEquals
                              : QByteArray::KeepTrailingEquals)
        );
  return data;
}, true},
{ "=frombase64", [](
     const ParamSet &paramset, const Utf8String &key, bool inherit,
     const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
     int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  auto value = paramset.evaluate(
        params.value(0), inherit, context, alreadyEvaluated);
  auto flags = params.value(1);
  QByteArray data = QByteArray::fromBase64(
        value,
        (flags.contains('u') ? QByteArray::Base64UrlEncoding
                             : QByteArray::Base64Encoding)
        |(flags.contains('a') ? QByteArray::AbortOnBase64DecodingErrors
                              : QByteArray::IgnoreBase64DecodingErrors)
        );
  if (flags.contains('b')) // FIXME b flag no longer relevant ?
    return Utf8String(data); // FIXME QByteArray instead ?
  return Utf8String(data);
}, true},
{ "=rpn", [](const ParamSet &paramset, const Utf8String &key, bool,
      const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
      int matchedLength) {
   MathExpr expr(key.mid(matchedLength), MathExpr::CharacterSeparatedRpn);
   auto ppm = ParamsProviderMerger(paramset)(context);
   return expr.evaluate(&ppm, Utf8String(), alreadyEvaluated).toString();
}, true},
};

Utf8String ParamSet::evaluateFunction(
    const ParamSet &paramset, const Utf8String &key, bool inherit,
    const ParamsProvider *context, Utf8StringSet *alreayEvaluated,
    bool *found) {
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
  return Utf8String();
}

bool ParamSet::appendVariableValue(
    Utf8String *value, const Utf8String &variable, bool inherit,
  const ParamsProvider *context, Utf8StringSet *alreadyEvaluated,
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
  Utf8StringSet newAlreadyEvaluated = *alreadyEvaluated;
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

Utf8StringList ParamSet::splitAndEvaluate(
    Utf8String rawValue, Utf8String separators, bool inherit,
    const ParamsProvider *context, Utf8StringSet *alreadyEvaluated) const {
  Utf8StringList values;
  Utf8String value, variable;
  int i = 0;
  while (i < rawValue.size()) {
    char c = rawValue.at(i++);
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
          if (isPercentVariableIdentifierSecondCharacter(c)) {
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

const QPair<Utf8String, Utf8String> ParamSet::valueAsStringsPair(
    Utf8String key, bool inherit, const ParamsProvider *context) const {
  auto v = rawValue(key, inherit).trimmed();
  qsizetype n = v.size();
  for (qsizetype i = 0; i < n; ++i)
    if (::isspace((v[i])))
      return { v.left(i), evaluate(v.mid(i+1).trimmed(), context) };
  return { v, {} };
}

const Utf8String ParamSet::matchingRegexp(Utf8String rawValue) {
  int i = 0, len = rawValue.size();
  Utf8String value;
  while (i < len) {
    char c = rawValue[i];
    if (c == '%') {
      if (i+1 < len && rawValue[i+1] == '%') {
        value.append('%');
        i += 2; // skip next % too
      } else {
        i += nestedPercentVariableLength(rawValue, i);
        value.append(".*");
      }
    } else {
      if (shouldBeEscapedWithinRegexp(c))
        value.append('\\');
      value.append(c);
      ++i;
    }
  }
  return value;
}

const Utf8StringSet ParamSet::keys(bool inherit) const {
  if (!d) [[unlikely]]
      return {};
  Utf8StringSet set(d->_params.keys());
  if (inherit)
    set += parent().keys();
  return set;
}

const Utf8StringSet ParamSet::keys() const {
  return keys(true);
}

bool ParamSet::contains(Utf8String key, bool inherit) const {
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
    const Utf8String &key, const ParamsProvider *context,
    const QVariant &defaultValue, Utf8StringSet *alreadyEvaluated) const {
  auto v = evaluate(rawValue(key, defaultValue.toString(), true),
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

const QHash<Utf8String, Utf8String> ParamSet::toHash(bool inherit) const {
  QHash<Utf8String,Utf8String> hash;
  for (auto key: keys(inherit))
    hash.insert(key, rawValue(key));
  return hash;
}

const QMap<Utf8String, Utf8String> ParamSet::toMap(bool inherit) const {
  QMap<Utf8String,Utf8String> map;
  for (auto key: keys(inherit))
    map.insert(key, rawValue(key));
  return map;
}

const QHash<QString, QString> ParamSet::toStringHash(bool inherit) const {
  QHash<QString,QString> hash;
  for (auto key: keys(inherit))
    hash.insert(key, rawValue(key));
  return hash;
}

const QMap<QString,QString> ParamSet::toStringMap(bool inherit) const {
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
  QSqlDatabase db, Utf8String sql, QMap<int,Utf8String> bindings) {
  ParamSet params(db, sql, bindings, *this);
  this->setValues(params, false);
}

void ParamSet::setValuesFromSqlDb(
    Utf8String dbname, Utf8String sql, QMap<int, Utf8String> bindings) {
  setValuesFromSqlDb(QSqlDatabase::database(dbname), sql, bindings);
}

void ParamSet::setValuesFromSqlDb(
  Utf8String dbname, Utf8String sql, Utf8StringList bindings) {
  setValuesFromSqlDb(QSqlDatabase::database(dbname), sql, bindings);
}

void ParamSet::setValuesFromSqlDb(
  QSqlDatabase db, Utf8String sql, Utf8StringList bindings) {
  QMap<int,Utf8String> map;
  int i = 0;
  for (auto key: bindings)
    map.insert(i++, key);
  setValuesFromSqlDb(db, sql, map);
}

qlonglong ParamSet::valueAsLong(
    Utf8String key, qlonglong defaultValue, bool inherit,
  const ParamsProvider *context) const {
  auto s = evaluate(rawValue(key, {}, inherit), inherit, context);
  return s.toLongLong(nullptr, defaultValue);
}

int ParamSet::valueAsInt(
  Utf8String key, int defaultValue, bool inherit,
  const ParamsProvider *context) const {
  auto s = evaluate(rawValue(key, {}, inherit), inherit, context);
  return s.toInt(nullptr, defaultValue);
}

double ParamSet::valueAsDouble(
  Utf8String key, double defaultValue, bool inherit,
  const ParamsProvider *context) const {
  auto s = evaluate(rawValue(key, {}, inherit), inherit, context);
  return s.toDouble(nullptr, defaultValue);
}

bool ParamSet::valueAsBool(Utf8String key, bool defaultValue, bool inherit,
  const ParamsProvider *context) const {
  auto s = evaluate(rawValue(key, {}, inherit), inherit, context);
  return s.toBool(nullptr, defaultValue);
}

ParamSetData *ParamSet::fromQIODevice(
    QIODevice *input, const Utf8String &format,
    const QMap<Utf8String,Utf8String> options,
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
    const QByteArray &file_name, const Utf8String &format,
    const QMap<Utf8String,Utf8String> options,
    const bool escape_percent, const ParamSet &parent) {
  //qDebug() << "***password fromFile" << file_name;
  QFile file(file_name);
  return ParamSet(fromQIODevice(&file, format, options, escape_percent, parent));
}

ParamSet ParamSet::fromCommandOutput(
    const QStringList &cmdline, const Utf8String &format,
    const QMap<Utf8String, Utf8String> options,
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

ParamSet ParamSet::externalParams(Utf8String set_name) {
  QMutexLocker ml(&_externals_mutex);
  return _externals.value(set_name);
}

void ParamSet::registerExternalParams(
    const Utf8String &set_name, ParamSet params) {
  QMutexLocker ml(&_externals_mutex);
  //qDebug() << "***password registerExternalParams" << set_name << params.toString();
  _externals.insert(set_name, params);
}

void ParamSet::clearExternalParams() {
  QMutexLocker ml(&_externals_mutex);
  _externals.clear();
}

Utf8StringList ParamSet::externalParamsNames() {
  QMutexLocker ml(&_externals_mutex);
  auto list = _externals.keys();
  list.detach();
  return list;
}
