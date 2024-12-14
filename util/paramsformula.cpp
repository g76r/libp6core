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
#include "util/datacache.h"
#include <functional>
#include <QStack>
#include <QRegularExpression>
#if __cpp_lib_math_constants >= 201907L
#include <numbers>
#endif

using EvalContext = ParamsFormula::EvalContext;
using FormulaDialect = ParamsFormula::FormulaDialect;

static int staticInit() {
  qRegisterMetaType<ParamsFormula>();
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

namespace {

static thread_local DataCache<QString,QRegularExpression> _regexp_cache{ 4096 };
static const auto _re_match_opts =
  QRegularExpression::DotMatchesEverythingOption // can be canceled with (?-s)
  | QRegularExpression::DontCaptureOption // can be canceled with (?-n)
  ;

class StackItem;

class Stack {
  QStack<StackItem> _items;

public:
  Stack() {}
  inline void push(StackItem &&item) { _items.push(item); }
  /** Safe popeval and eval.
   *  @return def if stack is empty
   */
  inline QVariant popeval(
      Stack *stack, const EvalContext &context, const QVariant &def);
  /** Safe popeval and eval as text.
   *  @return def if stack is empty
   */
  inline Utf8String popeval_utf8(
      Stack *stack, const EvalContext &context, const QVariant &def) {
    return Utf8String{popeval(stack, context, def)};
  }
  /** Safe popeval and eval as boolean.
   *  @return def if stack is empty
   */
  inline bool popeval_bool(
      Stack *stack, const EvalContext &context, bool def) {
    auto x = popeval(stack, context, {});
    return x.isValid() ? x.toBool() : def;
  }
  /** /!\ This function assumes the stack isn't empty. */
  inline StackItem &top() {
    return _items.top();
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
  inline StackItem &operator=(const QVariant &constant_value) {
    _op = [constant_value](Stack *, const EvalContext &, const QVariant &) {
      return constant_value;
    };
    return *this;
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
  bool _right_to_left = false;
  // https://en.wikipedia.org/wiki/Operators_in_C_and_C%2B%2B#Operator_precedence
  // https://en.wikipedia.org/wiki/Order_of_operations#Programming_languages
  // at some extends: https://www.lua.org/manual/5.4/manual.html#3.4.8
  bool _last_arg_is_regexp = false;
  StackItemOperator _op;
  bool operator!() const { return _arity == -1; }
};

} // unnamed namespace

// static inline Utf8String tostr(QPartialOrdering o) {
//   if (o == QPartialOrdering::Equivalent) return "equivalent";
//   if (o == QPartialOrdering::Less) return "less";
//   if (o == QPartialOrdering::Greater) return "greater";
//   if (o == QPartialOrdering::Unordered) return "unordered";
//   return "default";
// }

static inline QPartialOrdering compareTwoOperands(
  Stack *stack, const EvalContext &context, bool pretends_invalid_is_empty) {
  auto y = stack->popeval(stack, context, {});
  auto x = stack->popeval(stack, context, {});
  //if (x.metaType().id() == QMetaType::QDateTime || !x.isValid())
  //qDebug() << "****** compareTwoOperands" << x << y << tostr(MathUtils::compareQVariantAsNumberOrString(x, y, pretends_invalid_is_empty));
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

static const StackItemOperator _percentOperator = [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant {
  return stack->popeval_utf8(stack, context, def) % context;
};

static const RadixTree<OperatorDefinition> _operatorDefinitions {
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
        return stack->popeval_utf8(stack, context, {}).utf8size();
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
          return false;
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
        //qDebug() << "seeing regexp at eval time:" << y;
        if (y.metaType().id() == QMetaType::QRegularExpression)//qMetaTypeId<QRegularExpression>())
          re = y.toRegularExpression();
        else {
          auto pattern = y.toString();
          re = _regexp_cache.get_or_create(pattern, [&](){
            auto re = QRegularExpression(pattern, _re_match_opts);
            re.optimize();
            return re;
          });
        }
        if (!re.isValid())
          return def;
        auto x = stack->popeval_utf8(stack, context, {});
        return re.match(x).hasMatch();
      } }, true },
  { "!=~", { 2, 10, false, true, [](Stack *stack, const EvalContext &context, const QVariant &def) -> QVariant  {
        auto y = stack->popeval(stack, context, {});
        QRegularExpression re;
        //qDebug() << "seeing regexp at eval time:" << y;
        if (y.metaType().id() == QMetaType::QRegularExpression)//qMetaTypeId<QRegularExpression>())
          re = y.toRegularExpression();
        else {
          auto pattern = y.toString();
          re = _regexp_cache.get_or_create(pattern, [&](){
            auto re = QRegularExpression(pattern, _re_match_opts);
            re.optimize();
            return re;
          });
        }
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
#if __cpp_lib_math_constants >= 201907L
        return std::numbers::pi;
#else
        return 3.141592653589793238462643383279502884;
#endif
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
  { "<metatypeid>", { 1, -1, false, false, [](Stack *stack, const EvalContext &context, const QVariant &) -> QVariant {
        auto x = stack->popeval(stack, context, false);
        return x.metaType().id();
      } }, true },
};

static const QMap<Utf8String, OperatorDefinition> _operatorDefinitionsMap {
  _operatorDefinitions.toUtf8Map()
};

class ParamsFormulaData : public QSharedData {
public:
  Utf8String _expr;
  FormulaDialect _dialect;
  Stack _stack = {};
  explicit ParamsFormulaData(
      Utf8String expr = {}, FormulaDialect dialect = ParamsFormula::InvalidFormula)
    : _expr(expr), _dialect(dialect) {}
  QVariant eval(const EvalContext &context, const QVariant &def) const;
};

void ParamsFormula::init_rpn(
    ParamsFormulaData *data, const Utf8StringList &list,
    const Utf8String &expr) {
  data->_dialect = RpnWithPercents;
  data->_expr = expr;
  bool previous_was_constant = false;
  for (qsizetype i = 0, len = list.size(); i < len; ++i) {
    auto item = list[i];
    auto operator_definition = _operatorDefinitionsMap.value(item);
    if (!!operator_definition) {
      if (operator_definition._last_arg_is_regexp && previous_was_constant) {
        // if possible, compile regular expression now rather than at eval time
        // a string can be substituted with a QRegularExpression provided
        // previous item was a QVariant (not an operator) and we already know
        // its value (it has not to be %-evaluated at eval time)
        auto pattern = list[i-1].toUtf16();
        auto re = _regexp_cache.get_or_create(pattern, [&](){
          auto re = QRegularExpression(pattern, _re_match_opts);
          re.optimize();
          return re;
        });
        data->_stack.top() = re;
        //qDebug() << "compiling regexp at parse time:" << list[i-1];
      }
      data->_stack.push(operator_definition._op);
      previous_was_constant = false;
      continue;
    }
    // LATER support ::int[eger] ::double ::bool[ean] etc. suffixes or prefixes list:: ::
    if (PercentEvaluator::is_independent(item)) {
      data->_stack.push(PercentEvaluator::eval_utf8(item));
      previous_was_constant = true;
    } else {
      data->_stack.push(item);
      data->_stack.push(_percentOperator);
      previous_was_constant = false;
    }
  }
}

void ParamsFormula::init_percent(
    ParamsFormulaData *data, const Utf8String &expr) {
  data->_dialect = PercentExpression;
  data->_expr = expr;
  if (PercentEvaluator::is_independent(data->_expr)) {
    data->_stack.push(PercentEvaluator::eval_utf8(data->_expr));
  } else {
    data->_stack.push(data->_expr);
    data->_stack.push(_percentOperator);
  }
}

ParamsFormula::ParamsFormula(const Utf8String &expr, FormulaDialect dialect) {
  auto data = new ParamsFormulaData;
  d = data;
  if (expr.isNull())
    return;
  switch(dialect) {
    case PercentExpression:
      init_percent(data, expr);
      break;
    case RpnWithPercents: {
      init_rpn(data, expr.split_headed_list(), expr);
      break;
    }
    case InvalidFormula:
      ;
  }
}

ParamsFormula::ParamsFormula(
    const Utf8StringList &list, const Utf8String &expr, FormulaDialect dialect){
  auto data = new ParamsFormulaData;
  d = data;
  if (expr.isNull())
    return;
  switch(dialect) {
    case PercentExpression:
      init_percent(data, expr);
      break;
    case RpnWithPercents: {
      init_rpn(data, list, expr);
      break;
    }
    case InvalidFormula:
      ;
  }
}

ParamsFormula::ParamsFormula(const ParamsFormula &other) noexcept
  : d{other.d} {
}

ParamsFormula::ParamsFormula(ParamsFormula &&other) noexcept
  : d{std::move(other.d)} {
}

ParamsFormula &ParamsFormula::operator=(const ParamsFormula &other) noexcept {
  if (this != &other)
    d = other.d;
  return *this;
}

ParamsFormula &ParamsFormula::operator=(ParamsFormula &&other) noexcept {
  if (this != &other)
    d = std::move(other.d);
  return *this;
}

ParamsFormula::~ParamsFormula() noexcept {
}

Utf8String ParamsFormula::expr() const noexcept {
  return d->_expr;
}

FormulaDialect ParamsFormula::dialect() const noexcept {
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
