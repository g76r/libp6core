/* Copyright 2012-2021 Hallowyn, Gregoire Barbier and others.
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

bool ParamSet::_variableNotFoundLoggingEnabled { false };
const QString ParamSet::_true { "true" };
const QString ParamSet::_false { "false" };

static int staticInit() {
  qMetaTypeId<ParamSet>();
  char *value = getenv("ENABLE_PARAMSET_VARIABLE_NOT_FOUND_LOGGING");
  if (value && strcmp(value, "true") == 0)
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
};

ParamSet::ParamSet() {
}

ParamSet::ParamSet(std::initializer_list<QString> list) {
  for (auto it = std::begin(list); it != std::end(list); ++it) {
    auto key = *it;
    ++it;
    if (it != std::end(list)) {
      setValue(key, *it);
    } else {
      setValue(key, QString(""));
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

ParamSet::ParamSet(QHash<QString,QString> params)
  : d(new ParamSetData(params)) {
}

ParamSet::ParamSet(QMap<QString,QString> params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(QMultiMap<QString,QString> params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(QMultiHash<QString,QString> params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
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
    return rawValue.isNull() ? QString() : QStringLiteral("");
  return values.first();
}

static RadixTree<std::function<
QString(ParamSet params, QString key, bool inherit,
const ParamsProvider *context, QSet<QString> alreadyEvaluated,
        int matchedLength)>>
implicitVariables {
{ "=date", [](ParamSet paramset, QString key, bool inherit,
     const ParamsProvider *context, QSet<QString> alreayEvaluated,
     int matchedLength) {
  return TimeFormats::toMultifieldSpecifiedCustomTimestamp(
        QDateTime::currentDateTime(), key.mid(matchedLength), paramset,
        inherit, context, alreayEvaluated);
}, true},
{ "=coarsetimeinterval", [](ParamSet paramset, QString key, bool inherit,
     const ParamsProvider *context, QSet<QString> alreadyEvaluated,
     int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  qint64 msecs = (qint64)(
        paramset.evaluate(params.value(0), inherit, context, alreadyEvaluated)
        .toDouble()*1000);
  return TimeFormats::toCoarseHumanReadableTimeInterval(msecs);
}, true},
{ "=eval", [](ParamSet paramset, QString key, bool inherit,
     const ParamsProvider *context, QSet<QString> alreadyEvaluated,
     int matchedLength) {
  QString value = paramset.evaluate(
        key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return paramset.evaluate(value, inherit, context, alreadyEvaluated);
}, true},
{ "=escape", [](ParamSet paramset, QString key, bool inherit,
     const ParamsProvider *context, QSet<QString> alreadyEvaluated,
     int matchedLength) {
  QString value = paramset.evaluate(
        key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return ParamSet::escape(value);
}, true},
{ "=default", [](ParamSet paramset, QString key, bool inherit,
              const ParamsProvider *context, QSet<QString> alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  QString value;
  for (int i = 0; i < params.size()-1; ++i) {
    value = paramset.evaluate(params.value(i), inherit, context,
                              alreadyEvaluated);
    if (!value.isEmpty())
      return value;
  }
  if (params.size() >= 2)
    value = paramset.evaluate(params.value(params.size()-1), inherit, context,
                              alreadyEvaluated);
  return value;
}, true},
{ "=rawvalue", [](ParamSet paramset, QString key, bool,
              const ParamsProvider *, QSet<QString>, int matchedLength) {
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
{ "=ifneq", [](ParamSet paramset, QString key, bool inherit,
              const ParamsProvider *context, QSet<QString> alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  if (params.size() >= 3) {
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
{ "=switch", [](ParamSet paramset, QString key, bool inherit,
              const ParamsProvider *context, QSet<QString> alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  if (params.size() >= 1) {
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
  //qDebug() << "%=switch function invalid syntax:" << key;
  return QString();
}, true},
{ "=sub", [](ParamSet paramset, QString key, bool inherit,
              const ParamsProvider *context, QSet<QString> alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  //qDebug() << "%=sub:" << key << params.size() << params;
  QString value = paramset.evaluate(params.value(0), inherit, context,
                                    alreadyEvaluated);
  for (int i = 1; i < params.size(); ++i) {
    CharacterSeparatedExpression sFields(params[i]);
    //qDebug() << "pattern" << i << params[i] << sFields.size() << sFields;
    QString optionsString = sFields.value(2);
    QRegularExpression::PatternOption patternOptions
        = QRegularExpression::NoPatternOption;
    if (optionsString.contains('i'))
      patternOptions = QRegularExpression::CaseInsensitiveOption;
    // LATER add support for other available options: s,x...
    // LATER add a regexp cache because same =sub is likely to be evaluated several times
    // options must be part of the cache key
    // not sure if QRegularExpression::optimize() should be called
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
{ "=left", [](ParamSet paramset, QString key, bool inherit,
              const ParamsProvider *context, QSet<QString> alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  QString input = paramset.evaluate(params.value(0), inherit, context,
                                    alreadyEvaluated);
  bool ok;
  int i = params.value(1).toInt(&ok);
  return ok ? input.left(i) : input;
}, true},
{ "=right", [](ParamSet paramset, QString key, bool inherit,
              const ParamsProvider *context, QSet<QString> alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  QString input = paramset.evaluate(params.value(0), inherit, context,
                                    alreadyEvaluated);
  bool ok;
  int i = params.value(1).toInt(&ok);
  return ok ? input.right(i) : input;
}, true},
{ "=mid", [](ParamSet paramset, QString key, bool inherit,
              const ParamsProvider *context, QSet<QString> alreadyEvaluated,
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
{ "=elideright", [](ParamSet paramset, QString key, bool inherit,
              const ParamsProvider *context, QSet<QString> alreadyEvaluated,
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
{ "=elideleft", [](ParamSet paramset, QString key, bool inherit,
              const ParamsProvider *context, QSet<QString> alreadyEvaluated,
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
{ "=elidemiddle", [](ParamSet paramset, QString key, bool inherit,
              const ParamsProvider *context, QSet<QString> alreadyEvaluated,
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
{ "=htmlencode", [](ParamSet paramset, QString key, bool inherit,
              const ParamsProvider *context, QSet<QString> alreadyEvaluated,
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
{ "=random", [](ParamSet, QString key, bool,
              const ParamsProvider *, QSet<QString>, int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  // TODO evaluate modulo and shift
  // TODO replace rand with QRandomGenerator
  int modulo = abs(params.value(0).toInt());
  int shift = params.value(1).toInt();
  int i = ::rand();
  if (modulo)
    i %= modulo;
  i += shift;
  return QString::number(i);
}, true},
{ "=env", [](ParamSet paramset, QString key, bool inherit,
              const ParamsProvider *context, QSet<QString> alreadyEvaluated,
              int matchedLength) {
  CharacterSeparatedExpression params(key, matchedLength);
  int i = 0;
  do {
    QString name = paramset.evaluate(
          params.value(i), inherit, context, alreadyEvaluated);
    const char *value = ::getenv(name.toUtf8());
    if (value && *value)
      return QString::fromUtf8(value);
    ++i;
  } while (i < params.size()-1); // first param and then all but last one
  // otherwise last one if there are at less 2, otherwise a non-existent one
  return paramset.evaluate(params.value(i), inherit, context, alreadyEvaluated);
}, true},
{ "=sha1", [](ParamSet paramset, QString key, bool inherit,
     const ParamsProvider *context, QSet<QString> alreadyEvaluated,
     int matchedLength) {
  QString value = paramset.evaluate(
        key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return QCryptographicHash::hash(
        value.toUtf8(), QCryptographicHash::Sha1).toHex();
}, true},
{ "=sha256", [](ParamSet paramset, QString key, bool inherit,
     const ParamsProvider *context, QSet<QString> alreadyEvaluated,
     int matchedLength) {
  QString value = paramset.evaluate(
        key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return QCryptographicHash::hash(
        value.toUtf8(), QCryptographicHash::Sha256).toHex();
}, true},
{ "=md5", [](ParamSet paramset, QString key, bool inherit,
     const ParamsProvider *context, QSet<QString> alreadyEvaluated,
     int matchedLength) {
  QString value = paramset.evaluate(
        key.mid(matchedLength+1), inherit, context, alreadyEvaluated);
  return QCryptographicHash::hash(
        value.toUtf8(), QCryptographicHash::Md5).toHex();
}, true},
{ "=hex", [](ParamSet paramset, QString key, bool inherit,
     const ParamsProvider *context, QSet<QString> alreadyEvaluated,
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
{ "=fromhex", [](ParamSet paramset, QString key, bool inherit,
     const ParamsProvider *context, QSet<QString> alreadyEvaluated,
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
{ "=base64", [](ParamSet paramset, QString key, bool inherit,
     const ParamsProvider *context, QSet<QString> alreadyEvaluated,
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
{ "=frombase64", [](ParamSet paramset, QString key, bool inherit,
     const ParamsProvider *context, QSet<QString> alreadyEvaluated,
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
};

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
  alreadyEvaluated.insert(variable);
  QString s;
  int matchedLength;
  auto implicitVariable = implicitVariables.value(variable, &matchedLength);
  //qDebug() << "implicitVariable" << variable << !!implicitVariable;
  if (implicitVariable) {
    s = implicitVariable(*this, variable, inherit, context,
                         alreadyEvaluated, matchedLength);
    //qDebug() << "" << s;
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
  s = this->value(variable, inherit, context, alreadyEvaluated);
  if (!s.isNull()) {
    value->append(s);
    return true;
  }
  if (_variableNotFoundLoggingEnabled && logIfVariableNotFound) {
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
    const ParamsProvider *context, QSet<QString> alreadyEvaluated) const {
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
    } else {
      value.append(c);
    }
  }
  if (!value.isEmpty())
    values.append(value);
  return values;
}

static QRegularExpression whitespace("\\s");

QPair<QString,QString> ParamSet::valueAsStringsPair(
    QString key, bool inherit, const ParamsProvider *context) const {
  QString v = rawValue(key, inherit).trimmed();
  int i = v.indexOf(whitespace);
  if (i == -1)
    return QPair<QString,QString>(v,QString());
  return QPair<QString,QString>(v.left(i),
                                evaluate(v.mid(i+1).trimmed(), context));
}

QString ParamSet::matchingRegexp(QString rawValue) {
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

QSet<QString> ParamSet::keys(bool inherit) const {
  QSet<QString> set;
  if (d) {
    auto keys = d->_params.keys();
#if QT_VERSION >= 0x050f00
    set = QSet<QString>(keys.begin(), keys.end());
#else
    set = keys.toSet();
#endif
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
        QString key, const ParamsProvider *context, QVariant defaultValue,
        QSet<QString> alreadyEvaluated) const {
  QString v = evaluate(rawValue(key, defaultValue.toString(), true),
                       true, context, alreadyEvaluated);
  return v.isNull() ? QVariant() : v;
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

QString ParamSet::escape(QString string) {
  return string.replace(QStringLiteral("%"), QStringLiteral("%%"));
}
