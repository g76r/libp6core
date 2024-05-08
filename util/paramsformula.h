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
#ifndef PARAMSFORMULA_H
#define PARAMSFORMULA_H

#include "util/percentevaluator.h"
#include "log/log.h"

class ParamsFormulaData;

/** Can evaluate expressions like:
 *  ",%foo,bar,@" -> "hellobar" if foo holds "hello"
 *  ",%{myprefix.%k.mysuffix},0,>=" -> returns true or false
 *
 * See %=rpn function in percent_evaluation.md for detailed syntax and full list
 * of operators (this class is the implementation of %=rpn function).
 *
 * This class replaces and enhances older MathExpr class.
 */
class LIBP6CORESHARED_EXPORT ParamsFormula final {
  QSharedDataPointer<ParamsFormulaData> d;

public:
  using EvalContext = PercentEvaluator::EvalContext;
  enum FormulaDialect {
    InvalidFormula = 0,
    PercentExpression, // e.g. "%{=uppercase:%foo}" -> "HELLO" if foo="hello"
    RpnWithPercents, // e.g. ",%foo,bar,@" -> "hellobar" if foo="hello"
    // LATER: percent infix expression e.g. "1+1", "%foo@bar", "(%foo +4)*  2 "
  };

  /** Creates a formula
   *  e.g.:
   *  ParamsFormula(",%foo,bar,@", ParamsFormula::RpnWithPercents)
   *  ParamsFormula("%{=uppercase:%foo}", ParamsFormula::PercentExpression)
   */
  ParamsFormula(const Utf8String &expr, FormulaDialect dialect);
  /** Optimized constructor if expr has already been parsed and splitted as a
   *  list.
   */
  ParamsFormula(const Utf8StringList &list, const Utf8String &expr,
                FormulaDialect dialect);
  ParamsFormula() : ParamsFormula(Utf8String{}, InvalidFormula) {}
  ParamsFormula(const ParamsFormula &other);
  ParamsFormula &operator=(const ParamsFormula &other);
  ~ParamsFormula();
  [[nodiscard]] inline bool isValid() const { return !d; }
  [[nodiscard]] inline bool operator!() const { return !isValid(); }
  [[nodiscard]] Utf8String expr() const;
  [[nodiscard]] FormulaDialect dialect() const;
  [[nodiscard]] QVariant eval(
      const EvalContext &context = {}, const QVariant &def = {}) const;
  [[nodiscard]] inline Utf8String eval_utf8(
      const EvalContext &context = {}, const QVariant &def = {}) const {
    return Utf8String{eval(context, def)};
  }

private:
  inline void init_rpn(ParamsFormulaData *data, const Utf8StringList &list, const Utf8String &expr);
  inline void init_percent(ParamsFormulaData *data, const Utf8String &expr);
};

Q_DECLARE_METATYPE(ParamsFormula)
Q_DECLARE_TYPEINFO(ParamsFormula, Q_MOVABLE_TYPE);

inline uint qHash(const ParamsFormula &f) { return qHash(f.expr()); }

QDebug LIBP6CORESHARED_EXPORT operator<<(
    QDebug dbg, const ParamsFormula &expr);

LogHelper LIBP6CORESHARED_EXPORT operator<<(
    LogHelper lh, const ParamsFormula &expr);

#endif // PARAMSFORMULA_H
