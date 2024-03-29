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
#ifndef MATHEXPR_H
#define MATHEXPR_H

#include "percentevaluator.h"
#include "log/log.h"

class MathExprData;

class LIBP6CORESHARED_EXPORT MathExpr {
  QSharedDataPointer<MathExprData> d;

public:
  using EvalContext = PercentEvaluator::EvalContext;

  enum MathDialect { Infix, CharacterSeparatedRpn };
  MathExpr(const Utf8String expr, MathDialect dialect);
  MathExpr() : MathExpr(Utf8String(), Infix) { }
  MathExpr(const MathExpr &other);
  MathExpr &operator=(const MathExpr &other);
  ~MathExpr();
  bool isValid() const { return !d; }
  const Utf8String expr() const;
  const QVariant eval(
      const EvalContext &context, const QVariant &def = {}) const;
};

Q_DECLARE_METATYPE(MathExpr)
Q_DECLARE_TYPEINFO(MathExpr, Q_MOVABLE_TYPE);

inline uint qHash(const MathExpr &i) { return qHash(i.expr()); }

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const MathExpr &expr);

LogHelper LIBP6CORESHARED_EXPORT operator<<(LogHelper lh,
           const MathExpr &expr);

#endif // MATHEXPR_H
