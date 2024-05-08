/* Copyright 2024 Gregoire Barbier and others.
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
#include "paramsformula.h"
#include "util/percentevaluator.h"
#include "util/radixtree.h"
#include "util/mathutils.h"
#include <functional>
#include <QStack>

using EvalContext = ParamsFormula::EvalContext;
using FormulaDialect = ParamsFormula::FormulaDialect;

namespace {

static const auto _defaultReOpts =
  QRegularExpression::DotMatchesEverythingOption // can be canceled with (?-s)
  | QRegularExpression::DontCaptureOption // can be canceled with (?-n)
  ;

class StackItem;

class Stack {
  QStack<StackItem> _items;

public:
  Stack() {}
  inline void push(StackItem &&item) { _items.push(item); }
  /** safe popeval and eval
   *  @return def if stack is empty
   */
  inline QVariant popeval(
      Stack *stack, const EvalContext &context, const QVariant &def);
  inline Utf8String popeval_utf8(
      Stack *stack, const EvalContext &context, const QVariant &def) {
    return Utf8String{popeval(stack, context, def)};
  }
  inline bool popeval_bool(
      Stack *stack, const EvalContext &context, bool def) {
    auto x = popeval(stack, context, {});
    return x.isValid() ? x.toBool() : def;
  }
  inline void detach() {
    _items.detach();
  }
  inline bool is_empty() const { return _items.isEmpty(); }
};

using StackItemOperator
= std::function<QVariant(Stack *stack, const EvalContext &context,
const QVariant &def)>;

class StackItem final {
  StackItemOperator _op;

public:
  StackItem(const StackItemOperator &op) : _op(op) { }
  StackItem(const QVariant &constant_value = {})
    : _op([constant_value](Stack *, const EvalContext &, const QVariant &) {
    return constant_value;
  }) { }
  StackItem(const Utf8String &constant_value)
    : _op([constant_value](Stack *, const EvalContext &, const QVariant &) -> QVariant {
    return constant_value;
  }) { }
  inline QVariant operator()(Stack *stack, const EvalContext &context,
                             const QVariant &def) {
    return _op(stack, context, def);
  }
};

QVariant Stack::popeval(Stack *stack, const EvalContext &context,
                        const QVariant &def) {
  if (_items.isEmpty())
    return def;
  return _items.pop()(stack, context, def);
}

struct OperatorDefinition {
  int _arity = -1;
  int _priority = -1;
  bool _rightToLeft = false;
  // https://en.wikipedia.org/wiki/Operators_in_C_and_C%2B%2B#Operator_precedence
  // https://en.wikipedia.org/wiki/Order_of_operations#Programming_languages
  // at some extends: https://www.lua.org/manual/5.4/manual.html#3.4.8
  bool _lastArgIsRegexp = false;
  StackItemOperator _op;
  bool operator!() const { return _arity == -1; }
};

static inline QPartialOrdering compareTwoOperands(
  Stack *stack, const EvalContext &context, bool pretends_invalid_is_empty) {
  auto y = stack->popeval(stack, context, {});
  auto x = stack->popeval(stack, context, {});
  if ((!x.isValid() || !y.isValid()) && !pretends_invalid_is_empty)
    return QPartialOrdering::Unordered;
  auto po = MathUtils::compareQVariantAsNumberOrString(x, y, pretends_invalid_is_empty);
  return po;
}

static inline QPartialOrdering compareTwoOperands(
  const QVariant &x, const QVariant &y, bool pretends_invalid_is_empty) {
  if ((!x.isValid() || !y.isValid()) && !pretends_invalid_is_empty)
    return QPartialOrdering::Unordered;
  auto po = MathUtils::compareQVariantAsNumberOrString(x, y, pretends_invalid_is_empty);
  return po;
}

const static StackItemOperator _percentOperator = [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant {
  return stack->popeval_utf8(stack, context, def) % context;
};

const RadixTree<OperatorDefinition> _operatorDefinitions {
  { "<%>", { 1, 1, false, false, _percentOperator }, true },
  { "??*", { 2, 2, true, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant {
        auto y = stack->popeval(stack, context, def);
        auto x = stack->popeval(stack, context, {});
        // null or invalid coalescence
        return !x.isValid() || x.isNull() ? y : x;
      } }, true },
  { "??", { 2, 2, true, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant {
        auto y = stack->popeval(stack, context, def);
        auto x = stack->popeval(stack, context, {});
        // null, invalid or empty coalescence
        return !x.isValid() || x.isNull() || Utf8String{x}.isEmpty() ? y : x;
      } }, true },
  { "!", { 1, 3, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant {
        auto x = stack->popeval_bool(stack, context, false);
        return !x;
      } }, true },
  { "!!", { 1, 3, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant {
        auto x = stack->popeval_bool(stack, context, false);
        return x;
      } }, true },
  { "!*", { 1, 3, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant {
        auto x = stack->popeval(stack, context, {});
        // either invalid or null
        return !x.isValid() || x.isNull();
      } }, true },
  { "?*", { 1, 3, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant {
        auto x = stack->popeval(stack, context, {});
        // neither invalid nor null
        return x.isValid() && !x.isNull();
      } }, true },
  { "!-", { 1, 3, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant {
        auto x = stack->popeval(stack, context, {});
        // either invalid, null or empty
        return !x.isValid() || x.isNull() || Utf8String{x}.isEmpty();
      } }, true },
  { "?-", { 1, 3, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant {
        auto x = stack->popeval(stack, context, {});
        // neither invalid, null nor empty
        return x.isValid() && !x.isNull() && !Utf8String{x}.isEmpty();
      } }, true },
  { "~", { 1, 3, false, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant {
        bool ok;
        auto x = stack->popeval(stack, context, {}).toLongLong(&ok);
        return ok ? ~x : def;
      } }, true },
  { "~~", { 1, 3, false, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant {
        bool ok;
        auto x = stack->popeval(stack, context, {}).toLongLong(&ok);
        return ok ? x : def;
      } }, true },
  { "#", { 1, 3, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant {
        // LATER support lists
        return stack->popeval_utf8(stack, context, {}).utf8Size();
      } }, true },
  { "##", { 1, 3, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant {
        return stack->popeval_utf8(stack, context, {}).size();
      } }, true },
  { "*", { 2, 5, false, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        auto x = stack->popeval(stack, context, {});
        auto r = MathUtils::mulQVariantAsNumber(x, y);
        return r.isValid() ? r : def;
      } }, true },
  { "/", { 2, 5, false, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        auto x = stack->popeval(stack, context, {});
        auto r = MathUtils::divQVariantAsNumber(x, y);
        return r.isValid() ? r : def;
      } }, true },
  { "%", { 2, 5, false, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        auto x = stack->popeval(stack, context, {});
        auto r = MathUtils::modQVariantAsNumber(x, y);
        return r.isValid() ? r : def;
      } }, true },
  { "+", { 2, 6, false, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        auto x = stack->popeval(stack, context, {});
        auto r = MathUtils::addQVariantAsNumber(x, y);
        return r.isValid() ? r : def;
      } }, true },
  { "-", { 2, 6, false, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        auto x = stack->popeval(stack, context, {});
        auto r = MathUtils::subQVariantAsNumber(x, y);
        return r.isValid() ? r : def;
      } }, true },
  { "@", { 2, 6, false, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant {
        auto y = stack->popeval(stack, context, {});
        auto x = stack->popeval(stack, context, {});
        return x.isValid() || y.isValid() ? Utf8String{x}+Utf8String{y} : def;
      } }, true },
  { "<?", { 2, 7, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        auto x = stack->popeval(stack, context, {});
        auto po = compareTwoOperands(x, y, true);
        if (po == QPartialOrdering::Less)
          return x;
        if (po == QPartialOrdering::Equivalent)
          return x;
        if (po == QPartialOrdering::Greater)
          return y;
        return {};
      } }, true },
  { ">?", { 2, 7, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        auto x = stack->popeval(stack, context, {});
        auto po = compareTwoOperands(x, y, true);
        if (po == QPartialOrdering::Less)
          return y;
        if (po == QPartialOrdering::Equivalent)
          return x;
        if (po == QPartialOrdering::Greater)
          return x;
        return {};
      } }, true },
  { "<?*", { 2, 7, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        auto x = stack->popeval(stack, context, {});
        auto po = compareTwoOperands(x, y, false);
        if (po == QPartialOrdering::Less)
          return x;
        if (po == QPartialOrdering::Equivalent)
          return x;
        if (po == QPartialOrdering::Greater)
          return y;
        return {};
      } }, true },
  { ">?*", { 2, 7, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        auto x = stack->popeval(stack, context, {});
        auto po = compareTwoOperands(x, y, false);
        if (po == QPartialOrdering::Less)
          return y;
        if (po == QPartialOrdering::Equivalent)
          return x;
        if (po == QPartialOrdering::Greater)
          return x;
        return {};
      } }, true },
  { "<=>", { 2, 8, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant  {
        auto po = compareTwoOperands(stack, context, false);
        if (po == QPartialOrdering::Less)
          return -1;
        if (po == QPartialOrdering::Equivalent)
          return 0;
        if (po == QPartialOrdering::Greater)
          return 1;
        return {};
      } }, true },
  { "<=", { 2, 9, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant  {
        auto po = compareTwoOperands(stack, context, false);
        if (po == QPartialOrdering::Less)
          return true;
        if (po == QPartialOrdering::Equivalent)
          return true;
        if (po == QPartialOrdering::Greater)
          return false;
        return {};
      } }, true },
  { "<", { 2, 9, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant  {
        auto po = compareTwoOperands(stack, context, false);
        if (po == QPartialOrdering::Less)
          return true;
        if (po == QPartialOrdering::Equivalent)
          return false;
        if (po == QPartialOrdering::Greater)
          return false;
        return {};
      } }, true },
  { ">=", { 2, 9, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant  {
        auto po = compareTwoOperands(stack, context, false);
        if (po == QPartialOrdering::Less)
          return true;
        if (po == QPartialOrdering::Equivalent)
          return true;
        if (po == QPartialOrdering::Greater)
          return true;
        return {};
      } }, true },
  { ">", { 2, 9, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant  {
        auto po = compareTwoOperands(stack, context, false);
        if (po == QPartialOrdering::Less)
          return false;
        if (po == QPartialOrdering::Equivalent)
          return false;
        if (po == QPartialOrdering::Greater)
          return true;
        return {};
      } }, true },
  { "==*", { 2, 10, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant  {
        auto po = compareTwoOperands(stack, context, false);
        if (po == QPartialOrdering::Less)
          return false;
        if (po == QPartialOrdering::Equivalent)
          return true;
        if (po == QPartialOrdering::Greater)
          return false;
        return {};
      } }, true },
  { "!=*", { 2, 10, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant  {
        auto po = compareTwoOperands(stack, context, false);
        if (po == QPartialOrdering::Less)
          return true;
        if (po == QPartialOrdering::Equivalent)
          return false;
        if (po == QPartialOrdering::Greater)
          return true;
        return {};
      } }, true },
  { "==", { 2, 10, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant  {
        auto po = compareTwoOperands(stack, context, true);
        if (po == QPartialOrdering::Less)
          return false;
        if (po == QPartialOrdering::Equivalent)
          return true;
        if (po == QPartialOrdering::Greater)
          return false;
        return {};
      } }, true },
  { "!=", { 2, 10, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant  {
        auto po = compareTwoOperands(stack, context, true);
        if (po == QPartialOrdering::Less)
          return true;
        if (po == QPartialOrdering::Equivalent)
          return false;
        if (po == QPartialOrdering::Greater)
          return true;
        return {};
      } }, true },
  { "=~", { 2, 10, false, true, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        QRegularExpression re;
        if (y.metaType().id() == QMetaType::QRegularExpression)//qMetaTypeId<QRegularExpression>())
          re = y.toRegularExpression();
        else
          re = QRegularExpression(y.toString(), _defaultReOpts);
        if (!re.isValid())
          return def;
        auto x = stack->popeval_utf8(stack, context, {});
        return re.match(x).hasMatch();
      } }, true },
  { "!=~", { 2, 10, false, true, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        QRegularExpression re;
        if (y.metaType().id() == QMetaType::QRegularExpression)//qMetaTypeId<QRegularExpression>())
          re = y.toRegularExpression();
        else
          re = QRegularExpression(y.toString(), _defaultReOpts);
        if (!re.isValid())
          return def;
        auto x = stack->popeval_utf8(stack, context, {});
        return !re.match(x).hasMatch();
      } }, true },
  { "&&", { 2, 14, false, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        // LATER get rid of andQVariantAsNumber and do lazy evaluation here if y is false
        auto x = stack->popeval(stack, context, {});
        auto r = MathUtils::andQVariantAsNumber(x, y);
        return r.isValid() ? r : def;
      } }, true },
  { "^^", { 2, 15, false, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        auto x = stack->popeval(stack, context, {});
        auto r = MathUtils::xorQVariantAsNumber(x, y);
        return r.isValid() ? r : def;
      } }, true },
  { "||", { 2, 16, false, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        auto x = stack->popeval(stack, context, {});
        auto r = MathUtils::orQVariantAsNumber(x, y);
        return r.isValid() ? r : def;
      } }, true },
  { "?:", { 3, 17, false, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant  {
        auto z = stack->popeval(stack, context, def);
        auto y = stack->popeval(stack, context, def);
        auto x = stack->popeval(stack, context, {});
        if (!x.isValid())
          return def;
        return x.toBool() ? y : z;
      } }, true },
  { { "<null>", "<nil>" }, { 0, 0, false, false, [](Stack *, const EvalContext &, const QVariant &) -> QVariant {
        return {};
      } }, true },
  { "<pi>", { 0, 0, false, false, [](Stack *, const EvalContext &, const QVariant &) -> QVariant {
        return 3.141592653589793238462643383279502884;
      } }, true },
  { { ":=:", "<swap>" }, { 2, -1, true, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant {
        auto y = stack->popeval(stack, context, def);
        auto x = stack->popeval(stack, context, def);
        stack->push(y); // swapping x and y
        return x;
      } }, true },
  { "<dup>", { 1, -1, true, false, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant {
        auto x = stack->popeval(stack, context, def);
        stack->push(x); // duplicating x
        return x;
      } }, true },
};

QMap<Utf8String, OperatorDefinition> _operatorDefinitionsMap { _operatorDefinitions.toUtf8Map() };

} // unnamed namespace

struct ParamsFormulaData : public QSharedData {
  Utf8String _expr;
  FormulaDialect _dialect;
  Stack _stack = {};
  explicit ParamsFormulaData(
      Utf8String expr = {}, FormulaDialect dialect = ParamsFormula::InvalidFormula)
    : _expr(expr), _dialect(dialect) {}
  QVariant eval(const EvalContext &context, const QVariant &def) const;
};

ParamsFormula::ParamsFormula(const Utf8String expr, FormulaDialect dialect) {
  // LATER implement a formula cache (expr,dialect)->ParamsFormulaData
  auto data = new ParamsFormulaData{expr, dialect};
  switch(dialect) {
    case PercentExpression:
      data->_stack.push(expr);
      // LATER better guess if %-eval is needed or not
      if (expr.contains('%'))
        data->_stack.push(_percentOperator);
      break;
    case RpnWithPercents: {
      auto list = expr.split_headed_list();
      for (auto i : list) {
        auto operator_definition = _operatorDefinitionsMap.value(i);
        if (!!operator_definition) {
          data->_stack.push(operator_definition._op);
          continue;
        }
        // LATER support ::int[eger] ::double ::bool[ean] etc. suffixes or prefixes list:: ::
        data->_stack.push(i);
        // LATER better guess if %-eval is needed or not
        if (i.contains('%'))
          data->_stack.push(_percentOperator);
      }
      break;
    }
    case InvalidFormula:
      data->_expr = {};
      ;
  }
  d = data;
}

ParamsFormula::ParamsFormula(
    const Utf8StringList list, FormulaDialect dialect) {
  auto data = new ParamsFormulaData{"FIXME", dialect};
  switch(dialect) {
    case PercentExpression:
      data->_expr = list.value(0);
      data->_stack.push(data->_expr);
      // LATER better guess if %-eval is needed or not
      if (data->_expr.contains('%'))
        data->_stack.push(_percentOperator);
      break;
    case RpnWithPercents: {
      for (auto i : list) {
        auto operator_definition = _operatorDefinitionsMap.value(i);
        if (!!operator_definition) {
          data->_stack.push(operator_definition._op);
          continue;
        }
        // LATER support ::int[eger] ::double ::bool[ean] etc. suffixes or prefixes list:: ::
        data->_stack.push(i);
        // LATER better guess if %-eval is needed or not
        if (i.contains('%'))
          data->_stack.push(_percentOperator);
      }
      break;
    }
    case InvalidFormula:
      data->_expr = {};
      ;
  }
  d = data;
}


ParamsFormula::ParamsFormula(const ParamsFormula &other) : d{other.d} {
}

ParamsFormula &ParamsFormula::operator=(const ParamsFormula &other) {
  if (this != &other)
    d.operator=(other.d);
  return *this;
}

ParamsFormula::~ParamsFormula() {
}

Utf8String ParamsFormula::expr() const {
  return d->_expr;
}

FormulaDialect ParamsFormula::dialect() const {
  return d->_dialect;
}

QVariant ParamsFormula::eval(
    const EvalContext &context, const QVariant &def) const {
  return d->eval(context, def);
}

QVariant ParamsFormulaData::eval(
    const EvalContext &context, const QVariant &def) const {
  auto stack = _stack;
  stack.detach();
  auto x = stack.popeval(&stack, context, def);
  //if (!stack.is_empty()) // FIXME probably remove
  //  qDebug() << "FormulaData::eval with non empty stack at evaluation end";
  return x;
}

QDebug operator<<(QDebug dbg, const ParamsFormula &formula) {
  dbg.nospace() << "{";
  dbg.space() << formula.expr() << "}";
  return dbg.space();
}

LogHelper operator<<(LogHelper lh, const ParamsFormula &formula) {
  lh << "{ ";
  return lh << formula.expr() << " }";
}
