/* Copyright 2012-2024 Hallowyn, Gregoire Barbier and others.
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
#include "log/log.h"
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

using EvalContext = PercentEvaluator::EvalContext;

static RadixTree<std::function<
QVariant(const Utf8String &key, const EvalContext &context, int ml)>>
_functions {
{ "'", [](const Utf8String &key, const EvalContext&, int) -> QVariant {
  return key.mid(1);
}, true},
{ "=date", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  return TimeFormats::toMultifieldSpecifiedCustomTimestamp(
        QDateTime::currentDateTime(), key.mid(ml), context);
}, true},
{ "=coarsetimeinterval", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto msecs = PercentEvaluator::eval_number<double>(
                 params.value(0), 0.0, context)*1000;
  return TimeFormats::toCoarseHumanReadableTimeInterval(msecs);
}, true},
{ "=eval", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto v = PercentEvaluator::eval_utf8(key.mid(ml+1), context);
  return PercentEvaluator::eval(v, context);
}, true},
{ "=default", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  for (int i = 0; i < params.size(); ++i) {
    auto v = PercentEvaluator::eval(params.value(i), context);
    if (!Utf8String(v).isEmpty())
      return v;
  }
  return {};
}, true},
{ "=rawvalue", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  if (params.size() < 1 || !context)
    return {};
  const ParamsProvider *pp = context;
  if (!pp)
    return {};
  auto v = pp->paramRawValue(params.value(0));
  auto flags = params.value(1);
  if (flags.contains('e')) // %-escape
    v = PercentEvaluator::escape(v);
  if (flags.contains('h')) // htmlencode
    v = StringUtils::htmlEncode(
          v.toString(), flags.contains('u'), // url as links
          flags.contains('n')); // newline as <br>
  return v;
}, true},
{ "=ifneq", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  // TODO remove =ifneq, only keep =switch
  if (params.size() < 3) [[unlikely]]
    return {};
  auto input = PercentEvaluator::eval(params.value(0), context);
  auto ref = PercentEvaluator::eval(params.value(1), context);
  if (MathUtils::compareQVariantAsNumberOrString(input, ref, true)
      != QPartialOrdering::Equivalent)
    return PercentEvaluator::eval(params.value(2), context);
  if (params.size() >= 4)
    return PercentEvaluator::eval(params.value(3), context);
  return input;
}, true},
{ "=switch", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  if (params.size() < 1) [[unlikely]]
    return {};
  auto input = PercentEvaluator::eval(params.value(0), context);
  // evaluating :case:value params, if any
  int n = (params.size() - 1) / 2;
  for (int i = 0; i < n; ++i) {
    auto ref = PercentEvaluator::eval(params.value(1+i*2), context);
    if (MathUtils::compareQVariantAsNumberOrString(input, ref, true)
        == QPartialOrdering::Equivalent)
      return PercentEvaluator::eval(params.value(1+i*2+1), context);
  }
  // evaluating :default param, if any
  if (params.size() % 2 == 0)
    return PercentEvaluator::eval(params.value(params.size()-1), context);
  // otherwise left input as is
  return input;
}, true},
{ "=match", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  if (params.size() < 1) [[unlikely]]
    return {};
  auto input = PercentEvaluator::eval(params.value(0), context);
  // evaluating :case:value params, if any
  int n = (params.size() - 1) / 2;
  for (int i = 0; i < n; ++i) {
    auto ref = PercentEvaluator::eval_utf16(params.value(1+i*2), {}, context);
    QRegularExpression re(
        ref, QRegularExpression::DotMatchesEverythingOption // can be canceled with (?-s)
        ); // LATER set up a regexp cache
    auto match = re.match(input.toString());
    if (match.hasMatch()) {
      auto rpp = RegexpParamsProvider(match);
      auto ppm = ParamsProviderMerger(&rpp)(context);
      EvalContext new_context = context;
      new_context.setParamsProvider(&ppm);
      return PercentEvaluator::eval(params.value(1+i*2+1), new_context);
    }
  }
  // evaluating :default param, if any
  if (params.size() % 2 == 0)
    return PercentEvaluator::eval(params.value(params.size()-1), context);
  // otherwise left input as is
  return input;
}, true},
{ "=sub", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  //qDebug() << "%=sub:" << key << params.size() << params;
  auto value = PercentEvaluator::eval_utf8(params.value(0), context);
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
      Log::warning() << "%=sub with invalid regular expression: "
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
        EvalContext new_context = context;
        new_context.setParamsProvider(&ppm);
        transformed += PercentEvaluator::eval_utf8(sFields.value(1),
                                                   new_context);
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
    if (optionsString.contains("↑"))
      transformed = transformed.toUpper();
    else if (optionsString.contains("↓"))
      transformed = transformed.toLower();
    value = transformed;
  }
  //qDebug() << "value:" << value;
  return value;
}, true},
{ "=uppercase", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context);
  return input.toUpper();
}, true},
{ "=lowercase", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context);
  return input.toLower();
}, true},
{ "=titlecase", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context);
  return input.toTitle();
}, true},
{ "=left", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context);
  bool ok;
  int i = params.value(1).toInt(&ok);
  if (ok) {
    auto flags = params.value(2);
    if (flags.contains('b'))
      input = input.left(i);
    else
      input = input.utf8left(i);
  }
  return input;
}, true},
{ "=right", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context);
  bool ok;
  int i = params.value(1).toInt(&ok);
  if (ok) {
    auto flags = params.value(2);
    if (flags.contains('b'))
      input = input.right(i);
    else
      input = input.utf8right(i);
  }
  return input;
}, true},
{ "=mid", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context);
  bool ok;
  int i = params.value(1).toInt(&ok);
  if (ok) {
    int j = params.value(2).toInt(&ok);
    auto flags = params.value(3);
    if (flags.contains('b'))
      input = input.mid(i, ok ? j : -1);
    else
      input = input.utf8mid(i, ok ? j : -1);
  }
  return input;
}, true},
{ "=trim", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto input = PercentEvaluator::eval_utf8(key.mid(ml+1), context);
  return input.trimmed();
}, true},
{ "=elideright", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context);
  bool ok;
  int i = params.value(1).toInt(&ok);
  auto placeHolder = params.value(2, "..."_u8);
  if (!ok || placeHolder.size() > i || input.size() <= i)
    return input;
  return StringUtils::elideRight(input.toUtf16(), i, placeHolder);
}, true},
{ "=elideleft", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context);
  bool ok;
  int i = params.value(1).toInt(&ok);
  auto placeHolder = params.value(2, "..."_u8);
  if (!ok || placeHolder.size() > i || input.size() <= i)
    return input;
  return StringUtils::elideLeft(input.toUtf16(), i, placeHolder);
}, true},
{ "=elidemiddle", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto input = PercentEvaluator::eval_utf8(params.value(0), context);
  bool ok;
  int i = params.value(1).toInt(&ok);
  auto placeHolder = params.value(2, "..."_u8);
  if (!ok || placeHolder.size() > i || input.size() <= i)
    return input;
  return StringUtils::elideMiddle(input.toUtf16(), i, placeHolder);
}, true},
{ "=htmlencode", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  if (params.size() < 1)
    return {};
  auto input = PercentEvaluator::eval_utf8(params.value(0), context);
  auto flags = params.value(1);
  return StringUtils::htmlEncode(
        input.toUtf16(), flags.contains('u'), // url as links
        flags.contains('n')); // newline as <br>
}, true},
{ "=random", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto modulo = ::llabs(PercentEvaluator::eval_number<qlonglong>(
                          params.value(0), 0, context));
  auto shift = PercentEvaluator::eval_number<qlonglong>(
                 params.value(1), 0, context);
  auto i = (qlonglong)QRandomGenerator::global()->generate64();
  if (modulo)
    i %= modulo;
  i += shift;
  return Utf8String::number(i);
}, true},
{ "=env", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto env = ParamsProvider::environment();
  int i = 0;
  auto ppm = ParamsProviderMerger(env)(context);
  EvalContext new_context = context;
  new_context.setParamsProvider(&ppm);
  do {
    auto name = PercentEvaluator::eval_utf8(params.value(i), context);
    auto v = env->paramValue(name, {}, new_context);
    if (v.isValid())
      return v;
    ++i;
  } while (i < params.size()-1); // first param and then all but last one
  // otherwise last one if there are at less 2, otherwise a non-existent one
  return PercentEvaluator::eval(params.value(i), context);
}, true},
{ "=ext", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto ext = ParamSet::externalParams(params.value(0));
  int i = 1;
  auto ppm = ParamsProviderMerger(ext)(context);
  //qDebug() << "***password =ext" << ext << params.size() << params.value(i)
  //         << ext.toString();
  EvalContext new_context = context;
  new_context.setParamsProvider(&ppm);
  do {
    auto name = PercentEvaluator::eval_utf8(params.value(i), new_context);
    auto v = ext.paramValue(name, {}, new_context);
    if (v.isValid())
      return v;
    ++i;
  } while (i < params.size()-1); // first param and then all but last one
  // otherwise last one if there are at less 2, otherwise a non-existent one
  return PercentEvaluator::eval(params.value(i), context);
}, true},
{ "=sha1", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto value = PercentEvaluator::eval_utf8(key.mid(ml+1), context);
  return Utf8String(QCryptographicHash::hash(
                      value, QCryptographicHash::Sha1).toHex());
}, true},
{ "=sha256", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto value = PercentEvaluator::eval_utf8(key.mid(ml+1), context);
  return Utf8String(QCryptographicHash::hash(
                      value, QCryptographicHash::Sha256).toHex());
}, true},
{ "=md5", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto value = PercentEvaluator::eval_utf8(key.mid(ml+1), context);
  return Utf8String(QCryptographicHash::hash(
                      value, QCryptographicHash::Md5).toHex());
}, true},
{ "=hex", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto value = PercentEvaluator::eval_utf8(params.value(0), context);
  auto separator = params.value(1);
  return value.toHex(separator.value(0));
}, true},
{ "=fromhex", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto value = PercentEvaluator::eval_utf8(params.value(0), context);
  return QByteArray::fromHex(value);
}, true},
{ "=base64", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto value = PercentEvaluator::eval_utf8(params.value(0), context);
  auto flags = params.value(1);
  QByteArray::Base64Options options =
      (flags.contains('u') ? QByteArray::Base64UrlEncoding
                           : QByteArray::Base64Encoding)
      |(flags.contains('a') ? QByteArray::AbortOnBase64DecodingErrors
                            : QByteArray::IgnoreBase64DecodingErrors);
  value = value.toBase64(options);
  return value;
}, true},
{ "=frombase64", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  auto value = PercentEvaluator::eval_utf8(params.value(0), context);
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
{ "=rpn", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  // TODO set up a MathExpr cache
   MathExpr expr(key.mid(ml), MathExpr::CharacterSeparatedRpn);
   return expr.eval(context, {});
}, true},
{ "=integer", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  for (auto param: params) {
    bool ok;
    auto i = PercentEvaluator::eval_number<qlonglong>(param, context, &ok);
    if (ok)
      return i;
  }
  return {};
}, true},
{ "=formatint64", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  bool ok;
  auto i = PercentEvaluator::eval_number<qint64>(params.value(0), context, &ok);
  if (!ok)
    return PercentEvaluator::eval_utf8(params.value(3), context);
  auto base = PercentEvaluator::eval_number<int>(params.value(1), 10, context);
  auto padding = PercentEvaluator::eval_utf8(params.value(2), context);
  auto s = Utf8String::number(i, base);
  return padding.utf8left(padding.utf8Size()-s.utf8Size())+s;
}, true},
{ "=formatuint64", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  bool ok;
  auto i = PercentEvaluator::eval_number<quint64>(params.value(0), context, &ok);
  if (!ok)
    return PercentEvaluator::eval_utf8(params.value(3), context);
  auto base = PercentEvaluator::eval_number<int>(params.value(1), 10, context);
  auto padding = PercentEvaluator::eval_utf8(params.value(2), context);
  auto s = Utf8String::number(i, base);
  return padding.utf8left(padding.utf8Size()-s.utf8Size())+s;
}, true},
{ "=formatdouble", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  bool ok;
  auto d = PercentEvaluator::eval_number<double>(params.value(0), context, &ok);
  char fmt = PercentEvaluator::eval_utf8(
                  params.value(1), "g"_u8, context).value(0);
  auto prec = PercentEvaluator::eval_number<int>(params.value(2), 6, context);
  auto def = PercentEvaluator::eval_utf8(params.value(3), context);
  return ok ? Utf8String::number(d, fmt, prec) : def;
}, true},
{ "=formatboolean", [](const Utf8String &key, const EvalContext &context, int ml) -> QVariant {
  auto params = key.splitByLeadingChar(ml);
  bool ok;
  auto b = PercentEvaluator::eval_number<bool>(params.value(0), context, &ok);
  // param 2 (format) is ignored
  auto def = PercentEvaluator::eval_utf8(params.value(2), context);
  return ok ? Utf8String::number(b) : def;
}, true},
};

QVariant PercentEvaluator::eval_function(
    const Utf8String &key, const EvalContext &context, bool *found) {
  int matchedLength;
  auto function = _functions.value(key, &matchedLength);
  if (found)
    *found = !!function;
  if (function)
    return function(key, context, matchedLength);
  return {};
}

/** key musn't have a scope filter specification e.g. "[bar]foo" */
static inline QVariant eval_key(
    const Utf8String &new_scope_filter, const Utf8String &key,
    const EvalContext &context) {
  //qDebug() << new_scope_filter << new_scope_filter.isNull() << "eval_key:" << key << context;
  if (key.isEmpty()) [[unlikely]]
    return {};
  if (context.containsVariable(key)) [[unlikely]] {
    Log::warning() << "unsupported variable substitution: loop detected with "
                      "variable \"" << key << "\"";
    return {};
  }
  EvalContext new_context = context;
  new_context.addVariable(key);
  if (!new_scope_filter.isNull()) { // "" but not null filter will reset to {}
    new_context.setScopeFilter(new_scope_filter);
    //qDebug() << "new scope filter:" << new_context;
  }
  int matchedLength;
  auto function = _functions.value(key, &matchedLength);
  if (function) {
    // LATER reset scope filter at % or only at %[] ? (currently: 2nd case)
//    if (new_scope_filter.isEmpty()) // don't let a function receive an
//      new_context.setScopeFilter({}); // implicit scope filter ?
    return function(key, new_context, matchedLength);
  }
  const ParamsProvider *pp = context;
  if (!pp)
    return {};
  auto v = pp->paramValue(key, {}, new_context);
  //qDebug() << "/eval_key by param" << v;
  if (v.isValid())
    return v;
  if (_variableNotFoundLoggingEnabled) [[unlikely]]
    Log::debug()
        << "Unsupported variable substitution: variable not found: "
           "%{" << key << "} with context: " << context;
  return {};
}

/** key can have a scope filter specification e.g. "[bar]foo" */
QVariant PercentEvaluator::eval_key(
    const Utf8String &key, const EvalContext &context) {
  if (key.value(0) != '[') // no scope filter at the begining of the key
    return ::eval_key({}, key, context);
  auto eos = key.indexOf(']');
  if (eos < 0) // no ] in key
    return {};
  return ::eval_key(key.mid(1, eos-1), key.mid(eos+1), context);
}

namespace {

enum State {
  Toplevel,
  NakedScope, // e.g. "%[bar]foo"
  CurlyKey, // e.g. "%{foo}" or "%{[bar]foo}"
  NakedKey, // e.g. "%foo"
};

} // unnamed namespace

QVariant PercentEvaluator::eval(
    const char *begin, const char *end, const EvalContext &context) {
  Q_ASSERT(begin);
  Q_ASSERT(end);
  //qDebug() << "eval:" << begin << end-begin << context;
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
              begin = s+1; // set begining of new top level
              break;
            case '{':
              scope.clear();
              state = CurlyKey;
              //curly_depth = 0;
              ++s; // ignore next {
              begin = s+1; // set begining of curly key
              break;
            case '[':
              state = NakedScope;
              ++s; // ignore next [
              begin = s+1; // set begining of scope
              break;
            default:
              scope.clear();
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
          scope = s == begin ? ""_u8 : Utf8String(begin, s-begin);
          ++s; // ignore ]
          if (s < end && *s == '{') {
            state = CurlyKey;
            //curly_depth = 0;
            ++s; // ignore {
          } else {
            state = NakedKey;
          }
          begin = s; // set begining of key
          break;
        }
        ++s; // include byte in scope
        break;
      case NakedKey:
        if (!::isalnum(*s) && *s != '_') {
          auto key = Utf8String(begin, s-begin); // not including special char
          auto value = ::eval_key(scope, key, context);
          if (s+1 == end && result.isNull())
            return value; // QVariant pass through
          else
            result += Utf8String(value); // otherwise: append value
          state = Toplevel;
          begin = s; // set begining of new toplevel
          break;
        }
        ++s; // include byte in key
        break;
      case CurlyKey:
        switch (*s) {
          case '}':
            //qDebug() << "eval}:" << curly_depth << begin << s;
            if (curly_depth) {
              --curly_depth;
              ++s; // include } in key
              break;
            }
            if (*begin == '[') { // there is a scope within curly braces
              auto eos = begin+1;
              for (; eos <= s && *eos != ']'; ++eos)
                ;
              scope = eos-begin == 1 ? ""_u8 : Utf8String(begin+1, eos-begin-1);
              begin = eos+1; // the key begins after ]
            } else {
              // keep naked scope before { if any
            }
            if (s-begin > 0) { // key is not empty
              auto key = Utf8String(begin, s-begin); // not including }
              auto value = ::eval_key(scope, key, context);
              if (s+1 == end && result.isNull())
                return value; // QVariant pass through
              else
                result += Utf8String(value); // otherwise: append value
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
  //qDebug() << "/eval:" << s << state << begin << scope << result;
  if (s > begin) {
    switch(state) {
      case Toplevel: {
        result.append(begin, s-begin); // not including *s == '\0'
        break;
      }
      case NakedKey: {
          auto key = Utf8String(begin, s-begin); // not including *s == '\0'
          auto value = ::eval_key(scope, key, context);
          if (result.isNull())
            return value; // QVariant pass through
          result += Utf8String(value);
        }
        break;
      case NakedScope:
      case CurlyKey:
        ;
    }
  }
stop:
  if (result.isNull())
    return {};
  return result;
}

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

const Utf8String PercentEvaluator::EvalContext::toUtf8() const {
  Utf8String s = "{ params: { "_u8;
  s += _params_provider ? _params_provider->paramKeys().join(", ") : "null"_u8;
  s += " } scopes: {" + Utf8String::number(_scope_filter.size()) + " "
       + _scope_filter.join(", ");
  s += " } role: " + Utf8String::number(_role) + " }";
  return s;
}

QDebug operator<<(QDebug dbg, const PercentEvaluator::EvalContext &c) {
  return dbg.space() << c.toUtf8();
}

LogHelper operator<<(LogHelper lh, const PercentEvaluator::EvalContext &c) {
  return lh << c.toUtf8();
}

PercentEvaluator::EvalContext &PercentEvaluator::EvalContext::setScopeFilter(
    const Utf8String &scope_expr) {
  if (scope_expr.isEmpty()) // empty string means no filter: {}
    _scope_filter = {};
  else // to have an empty filter { "" } you need to pass "," string
    _scope_filter = scope_expr.split(',', Qt::KeepEmptyParts).toSet();
  return *this;
}
