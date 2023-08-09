/* Copyright 2022-2023 Gregoire Barbier and others.
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
#include "mathexpr.h"
#include <functional>
#include "util/radixtree.h"
#include "util/characterseparatedexpression.h"
#include <QRegularExpression>
#include "mathutils.h"
#include <QtDebug>

namespace {

static const auto _defaultReOpts =
  QRegularExpression::DotMatchesEverythingOption // can be canceled with (?-s)
  | QRegularExpression::DontCaptureOption // can be canceled with (?-n)
  ;

class Operand;

using OperandEvaluator = std::function<QVariant(
  const ParamsProvider *context,
  Utf8StringSet *alreadyEvaluated)>;

using OperatorEvaluator = std::function<QVariant(
  QList<Operand> args,
  const ParamsProvider *context,
  Utf8StringSet *alreadyEvaluated)>;

class Operand {
  OperandEvaluator _evaluator;

  // generic case
  Operand(OperandEvaluator evaluator) : _evaluator(evaluator) { }

public:
  // constant (incl. default)
  explicit Operand(QVariant value = QVariant())
    : Operand([value](const ParamsProvider *, Utf8StringSet *) {
    //qDebug() << "Operand constant " << value;
    return value;
  }) { }
  // variable
  Operand(const Utf8String &key, const QVariant &defaultValue)
    : Operand([key, defaultValue](
              const ParamsProvider *context,
              Utf8StringSet *alreadyEvaluated) {
    //qDebug() << "Operand variable " << key << " " << !!context;
    if (context)
      return context->paramValue(key, 0, defaultValue, alreadyEvaluated);
    return defaultValue;
  }) { }
  // operator
  Operand(QList<Operand> args, OperatorEvaluator evaluator)
    : Operand([args, evaluator](const ParamsProvider *context,
              Utf8StringSet *alreadyEvaluated) {
    //qDebug() << "Operand evaluator " << !!context << args.size();
    return evaluator(args, context, alreadyEvaluated);
  }) { }
  QVariant operator()(const ParamsProvider *context,
                      Utf8StringSet *alreadyEvaluated) const {
    return _evaluator(context, alreadyEvaluated);
  }
};

struct OperatorDef {
  int _arity = -1;
  int _priority = -1;
  bool _rightToLeft = false;
  // https://en.wikipedia.org/wiki/Operators_in_C_and_C%2B%2B#Operator_precedence
  // https://en.wikipedia.org/wiki/Order_of_operations#Programming_languages
  // at some extends: https://www.lua.org/manual/5.4/manual.html#3.4.8
  OperatorEvaluator _evaluator;
};

static inline QPartialOrdering compareTwoOperands(
  QList<Operand> args, const ParamsProvider *context,
  Utf8StringSet *alreadyEvaluated, bool anyStringRepresentation) {
  auto x = args.value(0)(context, alreadyEvaluated);
  auto y = args.value(1)(context, alreadyEvaluated);
  auto po = MathUtils::compareQVariantAsNumber(x, y, anyStringRepresentation);
  return po;
}

RadixTree<OperatorDef> operatordefs {
  { "??*", { 2, 2, true, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated);
        auto y = args.value(1)(context, alreadyEvaluated);
        // null or invalid coalescence
        return !x.isValid() || x.isNull() ? y : x;
      } }, true },
  { "??", { 2, 2, true, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated);
        auto y = args.value(1)(context, alreadyEvaluated);
        // null, invalid or empty coalescence
        return !x.isValid() || x.isNull()
            || x.toString().isEmpty() ? y : x;
      } }, true },
  { "!", { 1, 3, true, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated).toBool();
        return QVariant(!x);
      } }, true },
  { "!!", { 1, 3, true, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated).toBool();
        return QVariant(x);
      } }, true },
  { "!*", { 1, 3, true, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated);
        // either invalid or null
        return !x.isValid() || x.isNull();
      } }, true },
  { "?*", { 1, 3, true, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated);
        // neither invalid nor null
        return x.isValid() && !x.isNull();
      } }, true },
  { "!-", { 1, 3, true, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated);
        // either invalid, null or empty
        return !x.isValid() || x.isNull() || x.toString().isEmpty();
      } }, true },
  { "?-", { 1, 3, true, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated);
        // neither invalid, null nor empty
        return x.isValid() && !x.isNull() && !x.toString().isEmpty();
      } }, true },
  { "~", { 1, 3, true, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        bool ok;
        auto x = args.value(0)(context, alreadyEvaluated).toLongLong(&ok);
        return ok ? QVariant(~x) : QVariant();
      } }, true },
  { "~~", { 1, 3, true, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        bool ok;
        auto x = args.value(0)(context, alreadyEvaluated).toLongLong(&ok);
        return ok ? QVariant(x) : QVariant();
      } }, true },
  { "*", { 2, 5, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        return MathUtils::mulQVariantAsNumber(
              args.value(0)(context, alreadyEvaluated),
              args.value(1)(context, alreadyEvaluated));
      } }, true },
  { "/", { 2, 5, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        return MathUtils::divQVariantAsNumber(
              args.value(0)(context, alreadyEvaluated),
              args.value(1)(context, alreadyEvaluated));
      } }, true },
  { { "%", "*/*"},
    { 2, 5, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        return MathUtils::modQVariantAsNumber(
              args.value(0)(context, alreadyEvaluated),
              args.value(1)(context, alreadyEvaluated));
      } }, true },
  { "+", { 2, 6, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        return MathUtils::addQVariantAsNumber(
              args.value(0)(context, alreadyEvaluated),
              args.value(1)(context, alreadyEvaluated));
      } }, true },
  { "-", { 2, 6, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        return MathUtils::subQVariantAsNumber(
              args.value(0)(context, alreadyEvaluated),
              args.value(1)(context, alreadyEvaluated));
      } }, true },
  { "..", { 2, 6, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated).toString();
        auto y = args.value(1)(context, alreadyEvaluated).toString();
        return x+y;
      } }, true },
  { "<?", { 2, 7, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated);
        auto y = args.value(1)(context, alreadyEvaluated);
        auto po = compareTwoOperands(args, context, alreadyEvaluated, true);
        if (po == QPartialOrdering::Less)
          return x;
        if (po == QPartialOrdering::Equivalent)
          return x;
        if (po == QPartialOrdering::Greater)
          return y;
        return QVariant{}; // this should never happen
      } }, true },
  { ">?", { 2, 7, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated);
        auto y = args.value(1)(context, alreadyEvaluated);
        auto po = compareTwoOperands(args, context, alreadyEvaluated, true);
        if (po == QPartialOrdering::Less)
          return y;
        if (po == QPartialOrdering::Equivalent)
          return x;
        if (po == QPartialOrdering::Greater)
          return x;
        return QVariant{}; // this should never happen
      } }, true },
  { "<?*", { 2, 7, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated);
        auto y = args.value(1)(context, alreadyEvaluated);
        auto po = compareTwoOperands(args, context, alreadyEvaluated, false);
        if (po == QPartialOrdering::Less)
          return x;
        if (po == QPartialOrdering::Equivalent)
          return x;
        if (po == QPartialOrdering::Greater)
          return y;
        return QVariant{};
      } }, true },
  { ">?*", { 2, 7, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated);
        auto y = args.value(1)(context, alreadyEvaluated);
        auto po = compareTwoOperands(args, context, alreadyEvaluated, false);
        if (po == QPartialOrdering::Less)
          return y;
        if (po == QPartialOrdering::Equivalent)
          return x;
        if (po == QPartialOrdering::Greater)
          return x;
        return QVariant{};
      } }, true },
  { "<=>", { 2, 8, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto po = compareTwoOperands(args, context, alreadyEvaluated, false);
        if (po == QPartialOrdering::Less)
          return QVariant(-1);
        if (po == QPartialOrdering::Equivalent)
          return QVariant(0);
        if (po == QPartialOrdering::Greater)
          return QVariant(1);
        return QVariant{};
      } }, true },
  { "<=", { 2, 9, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto po = compareTwoOperands(args, context, alreadyEvaluated, false);
        if (po == QPartialOrdering::Less)
          return QVariant(true);
        if (po == QPartialOrdering::Equivalent)
          return QVariant(true);
        if (po == QPartialOrdering::Greater)
          return QVariant(false);
        return QVariant{};
      } }, true },
  { "<", { 2, 9, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto po = compareTwoOperands(args, context, alreadyEvaluated, false);
        if (po == QPartialOrdering::Less)
          return QVariant(true);
        if (po == QPartialOrdering::Equivalent)
          return QVariant(false);
        if (po == QPartialOrdering::Greater)
          return QVariant(false);
        return QVariant{};
      } }, true },
  { ">=", { 2, 9, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto po = compareTwoOperands(args, context, alreadyEvaluated, false);
        if (po == QPartialOrdering::Less)
          return QVariant(false);
        if (po == QPartialOrdering::Equivalent)
          return QVariant(true);
        if (po == QPartialOrdering::Greater)
          return QVariant(true);
        return QVariant{};
      } }, true },
  { ">", { 2, 9, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto po = compareTwoOperands(args, context, alreadyEvaluated, false);
        if (po == QPartialOrdering::Less)
          return QVariant(false);
        if (po == QPartialOrdering::Equivalent)
          return QVariant(false);
        if (po == QPartialOrdering::Greater)
          return QVariant(true);
        return QVariant{};
      } }, true },
  { "==*", { 2, 10, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto po = compareTwoOperands(args, context, alreadyEvaluated, false);
        if (po == QPartialOrdering::Less)
          return QVariant(false);
        if (po == QPartialOrdering::Equivalent)
          return QVariant(true);
        if (po == QPartialOrdering::Greater)
          return QVariant(false);
        return QVariant{};
      } }, true },
  { "!=*", { 2, 10, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto po = compareTwoOperands(args, context, alreadyEvaluated, false);
        if (po == QPartialOrdering::Less)
          return QVariant(true);
        if (po == QPartialOrdering::Equivalent)
          return QVariant(false);
        if (po == QPartialOrdering::Greater)
          return QVariant(true);
        return QVariant{};
      } }, true },
  { "==", { 2, 10, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto po = compareTwoOperands(args, context, alreadyEvaluated, true);
        if (po == QPartialOrdering::Less)
          return QVariant(false);
        if (po == QPartialOrdering::Equivalent)
          return QVariant(true);
        if (po == QPartialOrdering::Greater)
          return QVariant(false);
        return QVariant{}; // this should never happen
      } }, true },
  { "!=", { 2, 10, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto po = compareTwoOperands(args, context, alreadyEvaluated, true);
        if (po == QPartialOrdering::Less)
          return QVariant(true);
        if (po == QPartialOrdering::Equivalent)
          return QVariant(false);
        if (po == QPartialOrdering::Greater)
          return QVariant(true);
        return QVariant{}; // this should never happen
      } }, true },
  { "=~", { 2, 10, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated).toString();
        auto y = args.value(1)(context, alreadyEvaluated);
        QRegularExpression re =
            (y.metaType().id() == QMetaType::QRegularExpression)
            ? y.toRegularExpression()
            : QRegularExpression(y.toString(), _defaultReOpts);
        return QVariant(re.match(x).hasMatch());
      } }, true },
  { "!=~", { 2, 10, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated).toString();
        auto y = args.value(1)(context, alreadyEvaluated);
        QRegularExpression re =
            (y.metaType().id() == QMetaType::QRegularExpression)
            ? y.toRegularExpression()
            : QRegularExpression(y.toString(), _defaultReOpts);
        return QVariant(re.isValid() && !re.match(x).hasMatch());
      } }, true },
  { "&&", { 2, 14, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        return MathUtils::andQVariantAsNumber(
              args.value(0)(context, alreadyEvaluated),
              args.value(1)(context, alreadyEvaluated));
      } }, true },
  { "^^", { 2, 15, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        return MathUtils::xorQVariantAsNumber(
              args.value(0)(context, alreadyEvaluated),
              args.value(1)(context, alreadyEvaluated));
      } }, true },
  { "||", { 2, 16, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        return MathUtils::orQVariantAsNumber(
              args.value(0)(context, alreadyEvaluated),
              args.value(1)(context, alreadyEvaluated));
      } }, true },
  { "?:", { 3, 17, false, [](QList<Operand> args, const ParamsProvider *context,
          Utf8StringSet *alreadyEvaluated) {
        auto x = args.value(0)(context, alreadyEvaluated).toBool();
        return x ? args.value(1)(context, alreadyEvaluated)
                 : args.value(2)(context, alreadyEvaluated);
      } }, true },
};

QMap<Utf8String, OperatorDef> operatordefsMap { operatordefs.toUtf8Map() };

} // namespace

// terms (rpn) : x '1' ==
// stack: == '1' x
// tree building: push(var x) push(const 1) pop(2);push(operator ==(x,'1'))

static Operand compileRpn(Utf8StringList terms, bool *ok) {
  QList<Operand> stack;
  bool altok;
  if (!ok)
    ok = &altok;
  *ok = true;
  //qDebug() << "compileRpn" << terms;
  for (auto t: terms) {
    const auto term = t.trimmed();
    OperatorDef od = operatordefsMap.value(term);
    //qDebug() << "term " << t << " " << od._arity << " " << stack.size();
    if (od._arity > -1) {
      if (stack.size() >= od._arity) {
        auto args = stack.sliced(0, od._arity);
        std::reverse(args.begin(), args.end());
        stack.remove(0, od._arity);
        stack.prepend(Operand(args, od._evaluator));
        //qDebug() << "prepended operator " << args.size() << " " << stack.size();
      } else { // not enough operands for this operator
        terms.clear();
        stack.clear();
        *ok = false;
        //qDebug() << "not enough operands in stack";
        return Operand();
      }
      continue;
    }
    if (term.size() && term.at(0) == '\'') {
      qsizetype n = term.size()-1;
      if (term.at(n) == '\'')
        --n;
      stack.prepend(Operand(term.mid(1, n)));
      //qDebug() << "prepended constant " << term.mid(1, n);
      continue;
    }
    stack.prepend(Operand(term, QVariant()));
    //qDebug() << "prepended variable " << term << " " << stack.size();
  }
  //qDebug() << "at end: " << stack.size();
  if (stack.size() != 1)
    *ok = false;
  return stack.value(0);
}

class MathExprData : public QSharedData {
  Operand _root;
  Utf8String _expr;
  MathExprData(Operand root, Utf8String expr) : _root(root), _expr(expr) {
  }
public:
  MathExprData() { };
  static MathExprData *fromExpr(
    const Utf8String expr, MathExpr::MathDialect dialect) {
    bool ok = false;
    MathExprData *d = 0;
    if (dialect == MathExpr::CharacterSeparatedRpn) {
      auto terms = expr.splitByLeadingChar();
      auto root = compileRpn(terms, &ok);
      if (ok)
        d = new MathExprData(root, expr);
    } else {
      qWarning() << "Cannot create MathExprData with unsupported dialect type "
                 << dialect;
    }
    return d;
  }
  inline QVariant evaluate(const ParamsProvider *context,
                           Utf8StringSet *alreadyEvaluated) const {
    return _root(context, alreadyEvaluated);
  }
  inline Utf8String expr() const { return _expr; }
};

MathExpr::MathExpr(const Utf8String expr, MathDialect dialect)
  : d(MathExprData::fromExpr(expr, dialect)) {
}

MathExpr::MathExpr(const MathExpr &other) : d{other.d} {
}

MathExpr &MathExpr::operator=(const MathExpr &other) {
  if (this != &other)
    d.operator=(other.d);
  return *this;
}

MathExpr::~MathExpr() {
}

const QVariant MathExpr::evaluate(
    const ParamsProvider *context, const QVariant &defaultValue,
    Utf8StringSet *alreadyEvaluated) const {
  return d ? d->evaluate(context, alreadyEvaluated) : defaultValue;
}

const Utf8String MathExpr::expr() const {
  return d ? d->expr() : Utf8String();
}

QDebug operator<<(QDebug dbg, const MathExpr &expr) {
  dbg.nospace() << "{";
  dbg.space() << expr.expr() << "}";
  return dbg.space();
}

LogHelper operator<<(LogHelper lh, const MathExpr &expr) {
  lh << "{ ";
  return lh << expr.expr() << " }";
}
