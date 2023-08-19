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
#include "percentevaluator.h"
#include "paramsprovider.h"
#include "paramset.h"
#include "util/utf8stringset.h"
#include "util/radixtree.h"
#include "util/mathutils.h"
#include "util/mathexpr.h"
#include "util/regexpparamsprovider.h"
#include "util/paramsprovidermerger.h"
#include "format/timeformats.h"
#include "format/stringutils.h"
#include <functional>
#include <stdlib.h>
#include <QCryptographicHash>

static bool _variableNotFoundLoggingEnabled = false;

static int staticInit() {
  if (qgetenv("ENABLE_PERCENT_VARIABLE_NOT_FOUND_LOGGING") == "true")
    _variableNotFoundLoggingEnabled = true;
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

static RadixTree<std::function<
QVariant(const Utf8String &scope, const Utf8String &key,
         const ParamsProvider *context, Utf8StringSet *ae, int ml)>>
_functions {
{ "'", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *, Utf8StringSet *, int) -> QVariant {
  return key.mid(1);
}, true},
{ "=date", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  return TimeFormats::toMultifieldSpecifiedCustomTimestamp(
        QDateTime::currentDateTime(), key.mid(ml), context, ae);
}, true},
{ "=coarsetimeinterval", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto msecs = PercentEvaluator::eval_number<double>(
                   params.value(0), context, ae)*1000;
  return TimeFormats::toCoarseHumanReadableTimeInterval(msecs);
}, true},
{ "=eval", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto v = PercentEvaluator::eval_utf8(key.mid(ml+1), context, ae);
  return PercentEvaluator::eval(v, context, ae);
}, true},
{ "=escape", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto v = PercentEvaluator::eval_utf8(key.mid(ml+1), context, ae);
  return PercentEvaluator::escape(v);
}, true},
{ "=default", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  for (int i = 0; i < params.size(); ++i) {
    auto v = PercentEvaluator::eval(params.value(i), context, ae);
    if (v.isValid())
      return v;
  }
  return {};
}, true},
{ "=rawvalue", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  if (params.size() < 1)
    return {};
  auto v = PercentEvaluator::eval(params.value(0), context, ae).value;
  if (params.size() >= 2) {
    auto flags = params.value(1);
    if (flags.contains('e')) // %-escape
      v = PercentEvaluator::escape(v);
    if (flags.contains('h')) // htmlencode
      v = StringUtils::htmlEncode(
            v.toString(), flags.contains('u'), // url as links
            flags.contains('n')); // newline as <br>
  }
  return v;
}, true},
{ "=ifneq", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  // TODO remove =ifneq, only keep =switch
  if (params.size() < 3) [[unlikely]]
    return {};
  auto input = PercentEvaluator::eval(params.value(0), context, ae);
  auto ref = PercentEvaluator::eval(params.value(1), context, ae);
  if (QVariant::compare(input, ref) != QPartialOrdering::Equivalent)
    return PercentEvaluator::eval(params.value(2), context, ae);
  if (params.size() >= 4)
    return PercentEvaluator::eval(params.value(3), context, ae);
  return input;
}, true},
{ "=switch", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  if (params.size() < 1) [[unlikely]]
    return {};
  auto input = PercentEvaluator::eval(params.value(0), context, ae);
  // evaluating :case:value params, if any
  int n = (params.size() - 1) / 2;
  for (int i = 0; i < n; ++i) {
    auto ref = PercentEvaluator::eval(params.value(1+i*2), context, ae);
    if (QVariant::compare(input, ref) == QPartialOrdering::Equivalent)
      return PercentEvaluator::eval(params.value(1+i*2+1), context, ae);
  }
  // evaluating :default param, if any
  if (params.size() % 2 == 0)
    return PercentEvaluator::eval(params.value(params.size()-1), context, ae);
  // otherwise left input as is
  return input;
}, true},
{ "=match", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  if (params.size() < 1) [[unlikely]]
    return {};
  auto input = PercentEvaluator::eval(params.value(0), context, ae).value;
  // evaluating :case:value params, if any
  int n = (params.size() - 1) / 2;
  for (int i = 0; i < n; ++i) {
    auto ref = PercentEvaluator::eval_string(params.value(1+i*2), context, ae);
    QRegularExpression re(
        ref, QRegularExpression::DotMatchesEverythingOption // can be canceled with (?-s)
        ); // LATER set up a regexp cache
    auto match = re.match(input.toString());
    if (match.hasMatch()) {
      auto rpp = RegexpParamsProvider(match);
      auto ppm = ParamsProviderMerger(&rpp)(context);
      return PercentEvaluator::eval(params.value(1+i*2+1), &ppm, ae);
    }
  }
  // evaluating :default param, if any
  if (params.size() % 2 == 0)
    return PercentEvaluator::eval(params.value(params.size()-1), context, ae);
  // otherwise left input as is
  return input;
}, true},
{ "=sub", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  //qDebug() << "%=sub:" << key << params.size() << params;
  auto value = PercentEvaluator::eval_utf8(params.value(0), context, ae);
  for (int i = 1; i < params.size(); ++i) {
    auto sFields = params[i].splitByLeadingChar();
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
        auto rpp = RegexpParamsProvider(match);
        auto ppm = ParamsProviderMerger(&rpp)(context);
        transformed += PercentEvaluator::eval_utf8(sFields.value(1), &ppm, ae);
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
{ "=left", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context, ae);
  bool ok;
  int i = params.value(1).toInt(&ok);
  if (ok) {
    auto flags = params.value(2);
    if (flags.contains('b'))
      return input.left(i);
    else
      return input.utf8Left(i);
  }
  return input;
}, true},
{ "=right", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context, ae);
  bool ok;
  int i = params.value(1).toInt(&ok);
  if (ok) {
    auto flags = params.value(2);
    if (flags.contains('b'))
      return input.right(i);
    else
      return input.utf8Right(i);
  }
  return input;
}, true},
{ "=mid", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context, ae);
  bool ok;
  int i = params.value(1).toInt(&ok);
  if (ok) {
    int j = params.value(2).toInt(&ok);
    auto flags = params.value(3);
    if (flags.contains('b'))
      return input.mid(i, ok ? j : -1);
    else
      return input.utf8Mid(i, ok ? j : -1);
  }
  return input;
}, true},
{ "=trim", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto input = PercentEvaluator::eval_utf8(key.mid(ml+1), context, ae);
  return input.trimmed();
}, true},
{ "=elideright", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context, ae);
  bool ok;
  int i = params.value(1).toInt(&ok);
  auto placeHolder = params.value(2, "..."_u8);
  if (!ok || placeHolder.size() > i || input.size() <= i)
    return input;
  return StringUtils::elideRight(input.toString(), i, placeHolder);
}, true},
{ "=elideleft", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context, ae);
  bool ok;
  int i = params.value(1).toInt(&ok);
  auto placeHolder = params.value(2, "..."_u8);
  if (!ok || placeHolder.size() > i || input.size() <= i)
    return input;
  return StringUtils::elideLeft(input.toString(), i, placeHolder);
}, true},
{ "=elidemiddle", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context, ae);
  bool ok;
  int i = params.value(1).toInt(&ok);
  auto placeHolder = params.value(2, "..."_u8);
  if (!ok || placeHolder.size() > i || input.size() <= i)
    return input;
  return StringUtils::elideMiddle(input.toString(), i, placeHolder);
}, true},
{ "=htmlencode", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  if (params.size() < 1)
    return Utf8String();
  auto input = PercentEvaluator::eval_utf8(params.value(0), context, ae);
  auto flags = params.value(1);
  return StringUtils::htmlEncode(
        input.toString(), flags.contains('u'), // url as links
        flags.contains('n')); // newline as <br>
}, true},
{ "=random", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto modulo = ::llabs(PercentEvaluator::eval_number<qlonglong>(
                          params.value(0), context, ae));
  auto shift = PercentEvaluator::eval_number<qlonglong>(
                 params.value(1), context, ae);
  auto i = (qlonglong)QRandomGenerator::global()->generate64();
  if (modulo)
    i %= modulo;
  i += shift;
  return Utf8String::number(i);
}, true},
{ "=env", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto env = ParamsProvider::environment();
  int i = 0;
  auto ppm = ParamsProviderMerger(env)(context);
  do {
    auto name = PercentEvaluator::eval_utf8(params.value(i), context, ae);
    auto v = env->paramValue(name, {}, &ppm, ae);
    if (v.isValid())
      return v;
    ++i;
  } while (i < params.size()-1); // first param and then all but last one
  // otherwise last one if there are at less 2, otherwise a non-existent one
  return PercentEvaluator::eval(params.value(i), context, ae);
}, true},
{ "=ext", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto ext = ParamSet::externalParams(params.value(0));
  int i = 1;
  auto ppm = ParamsProviderMerger(ext)(context);
  //qDebug() << "***password =ext" << ext << params.size() << params.value(i)
  //         << ext.toString();
  do {
    Utf8StringSet name_ae = *ae;
    auto name = PercentEvaluator::eval_utf8(params.value(i), &ppm, &name_ae);
    auto v = ext.paramValue(name, {}, &ppm, ae);
    if (v.isValid())
      return v;
    ++i;
  } while (i < params.size()-1); // first param and then all but last one
  // otherwise last one if there are at less 2, otherwise a non-existent one
  return PercentEvaluator::eval(params.value(i), context, ae);
}, true},
{ "=sha1", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto value = PercentEvaluator::eval_utf8(key.mid(ml+1), context, ae);
  return QCryptographicHash::hash(value, QCryptographicHash::Sha1).toHex();
}, true},
{ "=sha256", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto value = PercentEvaluator::eval_utf8(key.mid(ml+1), context, ae);
  return QCryptographicHash::hash(value, QCryptographicHash::Sha256).toHex();
}, true},
{ "=md5", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto value = PercentEvaluator::eval_utf8(key.mid(ml+1), context, ae);
  return QCryptographicHash::hash(value, QCryptographicHash::Md5).toHex();
}, true},
{ "=hex", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto value = PercentEvaluator::eval_utf8(params.value(0), context, ae);
  auto separator = params.value(1);
  return value.toHex(separator.value(0));
}, true},
{ "=fromhex", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto value = PercentEvaluator::eval_utf8(params.value(0), context, ae);
  return QByteArray::fromHex(value);
}, true},
{ "=base64", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto value = PercentEvaluator::eval_utf8(params.value(0), context, ae);
  auto flags = params.value(1);
  QByteArray::Base64Options options =
      (flags.contains('u') ? QByteArray::Base64UrlEncoding
                           : QByteArray::Base64Encoding)
      |(flags.contains('a') ? QByteArray::AbortOnBase64DecodingErrors
                            : QByteArray::IgnoreBase64DecodingErrors);
  value = value.toBase64(options);
  return value;
}, true},
{ "=frombase64", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto value = PercentEvaluator::eval_utf8(params.value(0), context, ae);
  auto flags = params.value(1);
  QByteArray::Base64Options options =
      QByteArray::IgnoreBase64DecodingErrors
      |(flags.contains('u') ? QByteArray::Base64UrlEncoding
                            : QByteArray::Base64Encoding)
      |(flags.contains('a') ? QByteArray::AbortOnBase64DecodingErrors
                            : QByteArray::IgnoreBase64DecodingErrors);
  value = QByteArray::fromBase64(value, options);
  return value;
}, true},
{ "=rpn", [](const Utf8String &, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae, int ml) -> QVariant {
  // TODO set up a MathExpr cache
   MathExpr expr(key.mid(ml), MathExpr::CharacterSeparatedRpn);
   return expr.evaluate(context, {}, ae);
}, true},
};

/** @param key must not begin with a scope specifier */
static inline PercentEvaluator::ScopedValue eval_key(
    const Utf8String &scope, const Utf8String &key,
    const ParamsProvider *context, Utf8StringSet *ae) {
  if (key.isEmpty()) [[unlikely]] {
    Log::warning() << "unsupported variable substitution: empty variable name";
    return {};
  }
  if (!ae) [[unlikely]] {
    Log::warning() << "unsupported variable substitution: loop detection is "
                      "broken";
    return {};
  }
  if (ae->contains(key)) [[unlikely]] {
    Log::warning() << "unsupported variable substitution: loop detected with "
                      "variable \"" << key << "\"";
    return {};
  }
  Utf8StringSet new_ae = *ae;
  new_ae.insert(key);
  int matchedLength;
  auto function = _functions.value(key, &matchedLength);
  if (function)
    return { {}, function(scope, key, context, &new_ae, matchedLength) };
  if (!context)
    return {};
  // FIXME paramRawValue must have a scope filter param
  auto v = context->paramValue(key, {}, context, &new_ae);
  if (v.isValid())
    return v;
  if (_variableNotFoundLoggingEnabled) [[unlikely]]
    Log::debug()
        << "Unsupported variable substitution: variable not found: "
           "%{" << key << "} with context: " << context << " scope: "
        << scope;
  return {};
}

const PercentEvaluator::ScopedValue PercentEvaluator::eval_key(
    const Utf8String &key, const ParamsProvider *context,
    Utf8StringSet *already_evaluated) {
  if (key.value(0) == '[') { // key begins with a scope specifier
    auto eos = key.indexOf(']');
    return ::eval_key(key.mid(1, eos-1), key.mid(eos+1), context,
                      already_evaluated);
  }
  return ::eval_key({}, key, context, already_evaluated);
}

namespace {

enum State {
  Toplevel,
  NakedScope, // e.g. "%[bar]foo"
  CurlyKey, // e.g. "%{foo}" or "%{[bar]foo}"
  NakedKey, // e.g. "%foo"
};

} // unnamed namespace

const PercentEvaluator::ScopedValue PercentEvaluator::eval(
    const char *begin, const char *end, const ParamsProvider *context,
    Utf8StringSet *alreadyEvaluated) {
  Q_ASSERT(begin);
  Q_ASSERT(end);
  auto s = begin;
  State state = Toplevel;
  Utf8String result, scope;
  int curly_depth = 0;
  while (s < end && *s) {
    switch(state) {
      case Toplevel:
        if (*s == '%') {
          if (s > begin)
            result.append(begin, s-begin); // take bytes not including %
          if (s+1 == end)
            goto stop; // just ignore trailing % if there is nothing left
          switch(s[1]) {
            case '%': // %% is an escape sequence for %
              result.append('%'); // take escaped %
              ++s; // ignore next %
              break;
            case '{':
              state = CurlyKey;
              curly_depth = 0;
              ++s; // ignore next {
              begin = s+1; // set begining of curly key
              break;
            case '[':
              state = NakedScope;
              ++s; // ignore next [
              begin = s+1; // set begining of scope
              break;
            default:
              state = NakedKey;
              ++s; // won't eval next char, it's part of the key
              begin = s; // set begining of naked key
              break;
          }
        }
        ++s;
        break;
      case NakedScope:
        if (*s == ']') {
          scope = Utf8String(begin, s-begin);
          ++s; // ignore ]
          state = NakedKey;
          begin = s; // set begining of naked key
          break;
        }
        ++s; // include byte in scope
        break;
      case NakedKey:
        if (!::isalnum(*s) && *s != '_') {
          auto key = Utf8String(begin, s-begin+1); // including current byte
          auto value = ::eval_key(scope, key, context, alreadyEvaluated);
          if (s+1 == end && result.isEmpty())
            return value; // end is reached and result empty: return QVariant
          else
            result.append(Utf8String(value)); // otherwise: append value
          state = Toplevel;
          begin = s; // set begining of new toplevel
          break;
        }
        ++s; // include byte in key
        break;
      case CurlyKey:
        switch (*s) {
          case '}':
            --curly_depth;
            if (curly_depth) {
              ++s; // include } in key
              break;
            }
            if (*begin == '[') { // there is a scope within curly braces
              auto eos = begin;
              for (; eos <= s && *eos != ']'; ++s)
                ;
              scope = Utf8String(begin+1, eos-begin-1);
              begin = eos+1;
            } else {
              scope.clear();
            }
            if (s-begin > 0) { // key is not empty
              auto key = Utf8String(begin, s-begin); // not including }
              auto value = ::eval_key(scope, key, context, alreadyEvaluated);
              if (s+1 == end && result.isEmpty())
                return value; // end is reached and result empty: return QVariant
              else
                result.append(Utf8String(value)); // otherwise: append value
            }
            state = Toplevel;
            ++s; // ignore }
            begin = s; // set begining of new toplevel
            break;
          case '{':
            ++curly_depth;
            [[fallthrough]];
          default:
            ++s; // include byte in key
        }
        break;
    }
  }
stop:
  if (s > begin && state == Toplevel)
    result.append(begin, s-begin+1); // take remaining bytes
  if (result.isNull())
    return {};
  return { {}, result }; // LATER join scopes when mixed, rather than null ?
}

const PercentEvaluator::ScopedValue PercentEvaluator::eval(
    const Utf8String &expr, const ParamsProvider *context) {
  Utf8StringSet ae;
  return eval(expr, context, &ae);
}

#if 0
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
#endif

const QString PercentEvaluator::matching_regexp(const Utf8String &expr) {
  QString pattern;
  QRegularExpression re;
  auto begin = expr.constData(), s = begin;
  auto end = begin + expr.size();
  qsizetype i;
  while (s < end) {
    switch (*s) {
      case '%':
        if (s-begin >= 1)
          pattern += QRegularExpression::escape(Utf8String(begin, s-begin));
        if (s+1 == end)
          return pattern;
        ++s; // skip %
        switch (*s) {
          case '{':
            // skip until last } to avoid dealing with nesting, could be smarter
            i = Utf8String(s, end-s).lastIndexOf('}');
            if (i < 0)
              return pattern;
            s = s+i+1;
            break;
          case '[':
            // could be smarter, again
            i = Utf8String(s, end-s).lastIndexOf(']');
            if (i < 0)
              return pattern;
            s = s+i+1;
            if (*s == '{') {
              // could be smarter, again
              i = Utf8String(s, end-s).lastIndexOf('}');
              if (i < 0)
                return pattern;
              s = s+i+1;
            } else {
              ++s; // skil on byte, can be special char
              // skip until next special char
              while (s < end && (::isalnum(*s) || *s == '_'))
                ++s;
            }
            break;
          default:
            ++s; // skil on byte, can be special char
            // skip until next special char
            while (s < end && (::isalnum(*s) || *s == '_'))
              ++s;
            break;
        }
        pattern += u".*"_s;
        begin = s;
        break;
      default:
        ++s; // include byte in pattern
    }
  }
  if (begin < s)
    pattern += QRegularExpression::escape(Utf8String(begin, s-begin));
  return pattern;
}

void PercentEvaluator::enable_variable_not_found_logging(bool enabled) {
  _variableNotFoundLoggingEnabled = enabled;
}

