/* Copyright 2024-2025 Gregoire Barbier and others.
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
  inline TypedValue popeval(
      Stack *stack, const EvalContext &context);
  /** Safe popeval and eval as text.
   *  @return def if stack is empty
   */
  inline Utf8String popeval_utf8(Stack *stack, const EvalContext &context) {
    return Utf8String{popeval(stack, context)};
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
= std::function<TypedValue(Stack *stack, const EvalContext &context)>;

class StackItem final {
  StackItemOperator _op;

public:
  StackItem(const StackItemOperator &op) : _op(op) { }
  StackItem(const TypedValue &constant_value = {})
    : _op([constant_value](Stack *, const EvalContext &) {
    return constant_value;
  }) { }
  StackItem(const Utf8String &constant_value)
    : _op([constant_value](Stack *, const EvalContext &) -> TypedValue {
    return constant_value;
  }) { }
  inline TypedValue operator()(Stack *stack, const EvalContext &context) {
    return _op(stack, context);
  }
  inline StackItem &operator=(const TypedValue &constant_value) {
    _op = [constant_value](Stack *, const EvalContext &) {
      return constant_value;
    };
    return *this;
  }
};

TypedValue Stack::popeval(Stack *stack, const EvalContext &context) {
  if (_items.isEmpty())
    return {};
  return _items.pop()(stack, context);
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

static const StackItemOperator _percentOperator = [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
  return stack->popeval_utf8(stack, context) % context;
};

static RadixTree<OperatorDefinition> _operatorDefinitions {
  { "<%>", { 1, 1, false, false, _percentOperator }, true },
  { "??*", { 2, 2, true, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        return x || y; // null coalescence
      } }, true },
  { "??", { 2, 2, true, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        return x.as_utf8().isEmpty() ? y : x; // empty (incl. null) coalescence
      } }, true },
  { "!", { 1, 3, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        auto x = stack->popeval(stack, context);
        return !x.as_bool1();
      } }, true },
  { "!!", { 1, 3, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        auto x = stack->popeval(stack, context);
        return x.as_bool1();
      } }, true },
  { "!*", { 1, 3, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        auto x = stack->popeval(stack, context);
        return !x; // is null
      } }, true },
  { "?*", { 1, 3, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        auto x = stack->popeval(stack, context);
        return !!x; // is not null
      } }, true },
  { "!-", { 1, 3, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        auto x = stack->popeval(stack, context);
        return !x || x.as_utf8().isEmpty(); // is empty (incl. null and nan)
      } }, true },
  { "?-", { 1, 3, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        auto x = stack->popeval(stack, context);
        return !!x && !x.as_utf8().isEmpty(); // is not empty (neither null or nan)
      } }, true },
  { "~", { 1, 3, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        bool ok;
        auto x = stack->popeval(stack, context);
        auto u = x.as_unsigned8(&ok);
        if (ok)
          return ~u;
        auto i = x.as_signed8(&ok);
        if (ok)
          return ~i;
        return {};
      } }, true },
  { "~~", { 1, 3, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        bool ok;
        auto x = stack->popeval(stack, context);
        auto u = x.as_unsigned8(&ok);
        if (ok)
          return u;
        auto i = x.as_signed8(&ok);
        if (ok)
          return i;
        return {};
      } }, true },
  { "#", { 1, 3, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        // LATER support lists
        return stack->popeval_utf8(stack, context).utf8size();
      } }, true },
  { "##", { 1, 3, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        return stack->popeval_utf8(stack, context).size();
      } }, true },
  { "*", { 2, 5, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        return TypedValue::mul(x, y);
      } }, true },
  { "/", { 2, 5, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        return TypedValue::div(x, y);
      } }, true },
  { "%", { 2, 5, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        return TypedValue::mod(x, y);
      } }, true },
  { "+", { 2, 6, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        return TypedValue::add(x, y);
      } }, true },
  { "-", { 2, 6, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        return TypedValue::sub(x, y);
      } }, true },
  { "@", { 2, 6, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        return TypedValue::concat(x, y);
      } }, true },
  { "@*", { 2, 6, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        return TypedValue::concat<true>(x, y);
      } }, true },
  { "<?", { 2, 7, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto y = stack->popeval(stack, context);
        auto x = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y);
        if (po == QPartialOrdering::Less)
          return x;
        if (po == QPartialOrdering::Equivalent)
          return x;
        if (po == QPartialOrdering::Greater)
          return y;
        return {};
      } }, true },
  { ">?", { 2, 7, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto y = stack->popeval(stack, context);
        auto x = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y);
        if (po == QPartialOrdering::Less)
          return y;
        if (po == QPartialOrdering::Equivalent)
          return x;
        if (po == QPartialOrdering::Greater)
          return x;
        return {};
      } }, true },
  { "<?*", { 2, 7, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y, true);
        if (po == QPartialOrdering::Less)
          return x;
        if (po == QPartialOrdering::Equivalent)
          return x;
        if (po == QPartialOrdering::Greater)
          return y;
        return {};
      } }, true },
  { ">?*", { 2, 7, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y, true);
        if (po == QPartialOrdering::Less)
          return y;
        if (po == QPartialOrdering::Equivalent)
          return x;
        if (po == QPartialOrdering::Greater)
          return x;
        return {};
      } }, true },
  { "<=>", { 2, 8, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y);
        if (po == QPartialOrdering::Less)
          return -1;
        if (po == QPartialOrdering::Equivalent)
          return 0;
        if (po == QPartialOrdering::Greater)
          return 1;
        return {};
      } }, true },
  { "<=", { 2, 9, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y);
        if (po == QPartialOrdering::Less)
          return true;
        if (po == QPartialOrdering::Equivalent)
          return true;
        if (po == QPartialOrdering::Greater)
          return false;
        return {};
      } }, true },
  { "<", { 2, 9, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y);
        if (po == QPartialOrdering::Less)
          return true;
        if (po == QPartialOrdering::Equivalent)
          return false;
        if (po == QPartialOrdering::Greater)
          return false;
        return {};
      } }, true },
  { ">=", { 2, 9, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y);
        if (po == QPartialOrdering::Less)
          return false;
        if (po == QPartialOrdering::Equivalent)
          return true;
        if (po == QPartialOrdering::Greater)
          return true;
        return {};
      } }, true },
  { ">", { 2, 9, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y);
        if (po == QPartialOrdering::Less)
          return false;
        if (po == QPartialOrdering::Equivalent)
          return false;
        if (po == QPartialOrdering::Greater)
          return true;
        return {};
      } }, true },
  { "<=>*", { 2, 8, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y, true);
        if (po == QPartialOrdering::Less)
          return -1;
        if (po == QPartialOrdering::Equivalent)
          return 0;
        if (po == QPartialOrdering::Greater)
          return 1;
        return {};
      } }, true },
  { "<=*", { 2, 9, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y, true);
        if (po == QPartialOrdering::Less)
          return true;
        if (po == QPartialOrdering::Equivalent)
          return true;
        if (po == QPartialOrdering::Greater)
          return false;
        return {};
      } }, true },
  { "<*", { 2, 9, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y, true);
        if (po == QPartialOrdering::Less)
          return true;
        if (po == QPartialOrdering::Equivalent)
          return false;
        if (po == QPartialOrdering::Greater)
          return false;
        return {};
      } }, true },
  { ">=*", { 2, 9, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y, true);
        if (po == QPartialOrdering::Less)
          return false;
        if (po == QPartialOrdering::Equivalent)
          return true;
        if (po == QPartialOrdering::Greater)
          return true;
        return {};
      } }, true },
  { ">*", { 2, 9, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y, true);
        if (po == QPartialOrdering::Less)
          return false;
        if (po == QPartialOrdering::Equivalent)
          return false;
        if (po == QPartialOrdering::Greater)
          return true;
        return {};
      } }, true },
  { "==*", { 2, 10, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y, true);
        if (po == QPartialOrdering::Less)
          return false;
        if (po == QPartialOrdering::Equivalent)
          return true;
        if (po == QPartialOrdering::Greater)
          return false;
        return {};
      } }, true },
  { "!=*", { 2, 10, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y, true);
        if (po == QPartialOrdering::Less)
          return true;
        if (po == QPartialOrdering::Equivalent)
          return false;
        if (po == QPartialOrdering::Greater)
          return true;
        return {};
      } }, true },
  { "==", { 2, 10, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y);
        if (po == QPartialOrdering::Less)
          return false;
        if (po == QPartialOrdering::Equivalent)
          return true;
        if (po == QPartialOrdering::Greater)
          return false;
        return {};
      } }, true },
  { "!=", { 2, 10, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto po = TypedValue::compare_as_number_otherwise_string(x, y);
        if (po == QPartialOrdering::Less)
          return true;
        if (po == QPartialOrdering::Equivalent)
          return false;
        if (po == QPartialOrdering::Greater)
          return true;
        return {};
      } }, true },
  { "=~", { 2, 10, false, true, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto y = stack->popeval(stack, context);
        QRegularExpression re;
        //qDebug() << "seeing regexp at eval time:" << y;
        if (y.type() == TypedValue::Regexp)
          re = y.regexp();
        else {
          auto pattern = y.as_utf16();
          re = _regexp_cache.get_or_create(pattern, [&](){
            auto re = QRegularExpression(pattern, _re_match_opts);
            re.optimize();
            return re;
          });
        }
        if (!re.isValid())
          return {};
        auto x = stack->popeval_utf8(stack, context);
        return re.match(x).hasMatch();
      } }, true },
  { "!=~", { 2, 10, false, true, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto y = stack->popeval(stack, context);
        QRegularExpression re;
        //qDebug() << "seeing regexp at eval time:" << y;
        if (y.type() == TypedValue::Regexp)
          re = y.regexp();
        else {
          auto pattern = y.as_utf16();
          re = _regexp_cache.get_or_create(pattern, [&](){
            auto re = QRegularExpression(pattern, _re_match_opts);
            re.optimize();
            return re;
          });
        }
        if (!re.isValid())
          return {};
        auto x = stack->popeval_utf8(stack, context);
        return !re.match(x).hasMatch();
      } }, true },
  { "&", { 2, 11, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto y = stack->popeval(stack, context);
        auto x = stack->popeval(stack, context);
        return TypedValue::bitwise_and(x, y);
      } }, true },
  { "^", { 2, 12, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto y = stack->popeval(stack, context);
        auto x = stack->popeval(stack, context);
        return TypedValue::bitwise_xor(x, y);
      } }, true },
  { "|", { 2, 13, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto y = stack->popeval(stack, context);
        auto x = stack->popeval(stack, context);
        return TypedValue::bitwise_or(x, y);
      } }, true },
  { "&&", { 2, 14, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        bool okx, oky;
        auto x = stack->popeval(stack, context).as_bool1(&okx);
        auto y = stack->popeval(stack, context).as_bool1(&oky);
        if (!okx || !oky)
          return {};
        return x && y;
      } }, true },
  { "^^", { 2, 15, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        bool okx, oky;
        auto x = stack->popeval(stack, context).as_bool1(&okx);
        auto y = stack->popeval(stack, context).as_bool1(&oky);
        if (!okx || !oky)
          return {};
        return x != y;
      } }, true },
  { "||", { 2, 16, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        bool okx, oky;
        auto x = stack->popeval(stack, context).as_bool1(&okx);
        auto y = stack->popeval(stack, context).as_bool1(&oky);
        if (!okx || !oky)
          return {};
        return x || y;
      } }, true },
  { "?:*", { 3, 17, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto z = stack->popeval(stack, context);
        return x.as_bool1() ? y : z;
      } }, true },
  { "?:", { 3, 17, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto x = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto z = stack->popeval(stack, context);
        if (!x)
          return {};
        return x.as_bool1() ? y : z;
      } }, true },
  { ":?*", { 3, 17, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto z = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto x = stack->popeval(stack, context);
        return x.as_bool1() ? y : z;
      } }, true },
  { ":?", { 3, 17, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue  {
        auto z = stack->popeval(stack, context);
        auto y = stack->popeval(stack, context);
        auto x = stack->popeval(stack, context);
        if (!x)
          return {};
        return x.as_bool1() ? y : z;
      } }, true },
  { { "<null>", "<nil>" }, { 0, 0, false, false, [](Stack *, const EvalContext &) STATIC_LAMBDA -> TypedValue {
        return {};
      } }, true },
  { "<nan>", { 0, 0, false, false, [](Stack *, const EvalContext &) STATIC_LAMBDA -> TypedValue {
        return std::nan("");
      } }, true },
  { "<pi>", { 0, 0, false, false, [](Stack *, const EvalContext &) STATIC_LAMBDA -> TypedValue {
#if __cpp_lib_math_constants >= 201907L
        return std::numbers::pi;
#else
        return 3.141592653589793238462643383279502884;
#endif
      } }, true },
  { { ":=:", "<swap>" }, { 2, -1, true, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        auto y = stack->popeval(stack, context);
        auto x = stack->popeval(stack, context);
        stack->push(y); // swapping x and y
        return x;
      } }, true },
  { "<dup>", { 1, -1, true, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        auto x = stack->popeval(stack, context);
        stack->push(x); // duplicating x
        return x;
      } }, true },
  { "<typeid>", { 1, -1, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        auto x = stack->popeval(stack, context);
        return x.type();
      } }, true },
  { "<etv>", { 1, -1, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        auto x = stack->popeval(stack, context);
        return x.as_etv();
      } }, true },
  { "<etvs>", { 1, -1, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        Utf8StringList list;
        while (!stack->is_empty()) {
          auto v = stack->popeval(stack, context);
          list.prepend(v.as_etv());
        }
        return list.join(',');
      } }, true },
  { "<typecodes>", { 1, -1, false, false, [](Stack *stack, const EvalContext &context) STATIC_LAMBDA -> TypedValue {
        Utf8StringList list;
        while (!stack->is_empty()) {
          auto v = stack->popeval(stack, context);
          list.prepend(v.typecode());
        }
        return list.join(',');
      } }, true },
};

static QMap<Utf8String, OperatorDefinition> _operatorDefinitionsMap {
  _operatorDefinitions.toUtf8Map()
};

void ParamsFormula::register_operator(
    const Utf8String &symbol, ParamsFormula::UnaryOperator op) {
  OperatorDefinition opdef =
  {1, 7, false, false, [op](Stack *stack, const EvalContext &context) {
     const auto &x = stack->popeval(stack, context);
     return op(context, x);
   }};
  _operatorDefinitions.insert(symbol, opdef, false);
  _operatorDefinitionsMap.insert(symbol, opdef);
}

void ParamsFormula::register_operator(
    const Utf8String &symbol, ParamsFormula::BinaryOperator op) {
  OperatorDefinition opdef =
  {1, 7, false, false, [op](Stack *stack, const EvalContext &context) {
     const auto &x = stack->popeval(stack, context);
     const auto &y = stack->popeval(stack, context);
     return op(context, x, y);
   }};
  _operatorDefinitions.insert(symbol, opdef, false);
  _operatorDefinitionsMap.insert(symbol, opdef);
}

void ParamsFormula::register_operator(
    const Utf8String &symbol, ParamsFormula::TernaryOperator op) {
  OperatorDefinition opdef =
  {1, 7, false, false, [op](Stack *stack, const EvalContext &context) {
     const auto &x = stack->popeval(stack, context);
     const auto &y = stack->popeval(stack, context);
     const auto &z = stack->popeval(stack, context);
     return op(context, x, y, z);
   }};
  _operatorDefinitions.insert(symbol, opdef, false);
  _operatorDefinitionsMap.insert(symbol, opdef);
}

class ParamsFormulaData : public QSharedData {
public:
  Utf8String _expr;
  FormulaDialect _dialect;
  Stack _stack = {};
  explicit ParamsFormulaData(
      Utf8String expr = {},
      FormulaDialect dialect = ParamsFormula::InvalidFormula)
    : _expr(expr), _dialect(dialect) {}
  TypedValue eval(const EvalContext &context) const;
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
        // previous item was a TypedValue (not an operator) and we already know
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

TypedValue ParamsFormula::eval(const EvalContext &context) const {
  return d->eval(context);
}

TypedValue ParamsFormulaData::eval(const EvalContext &context) const {
  auto stack = _stack;
  stack.detach();
  return stack.popeval(&stack, context);
}

QDebug operator<<(QDebug dbg, const ParamsFormula &formula) {
  dbg.nospace() << "{";
  dbg.space() << formula.expr() << "}";
  return dbg.space();
}

p6::log::LogHelper operator<<(
    p6::log::LogHelper lh, const ParamsFormula &formula) {
  lh << "{ ";
  return lh << formula.expr() << " }";
}
