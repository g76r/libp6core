#include "util/paramset.h"
#include "util/mathexpr.h"
#include "util/mathutils.h"

int main(void) {
  QVariant x(ULLONG_MAX/2);
  QVariant y(-132);
  ParamSet x1 { "x", "1" };
  ParamSet x1_5 { "x", "1.5" };
  ParamSet x4 { "x", "4" };
  ParamSet x5 { "x", "5" };
  qDebug() << x.metaType().name() << y.metaType().name()
           << MathUtils::promoteToBestNumericType(&x, &y)
           << x.metaType().name() << y.metaType().name()
           << x.toLongLong() << y.toLongLong()
           << x.toDouble() << y.toDouble()
           << (MathUtils::compareQVariantAsNumberOrString(x, y) == QPartialOrdering::Greater)
           << (MathUtils::compareQVariantAsNumberOrString(x, y) == QPartialOrdering::Unordered)
           << PercentEvaluator::eval_utf8("3: %{=rpn,'1,'2,+}")
           << PercentEvaluator::eval("%{=rpn,'1,'2,+}")
           << PercentEvaluator::eval_utf8("6: %{=rpn,'1,x,+}", &x5)
           << PercentEvaluator::eval_utf8("15: %{=rpn,'1,x,..}", &x5)
           << PercentEvaluator::eval_utf8(": %{=rpn,'1,',+}")
           << PercentEvaluator::eval("%{=rpn,'1,',+}")
           << PercentEvaluator::eval_utf8("2: %{=rpn,'1,'true,+}")
           << PercentEvaluator::eval_utf8("true: %{=rpn,'1,'true,&&}")
           << PercentEvaluator::eval("%{=rpn,'1,'true,&&}")
           << PercentEvaluator::eval_utf8("true: %{=rpn,'1,'true,==}")
           << PercentEvaluator::eval_utf8("false: %{=rpn,'42,'true,==}")
           << PercentEvaluator::eval_utf8("true: %{=rpn,'42,!!,'true,==}")
           << PercentEvaluator::eval_utf8("33: %{=rpn,'0x20,x,+}", &x1)
           << PercentEvaluator::eval_utf8("33.5: %{=rpn,'0x20,x,+}", &x1_5)
           << PercentEvaluator::eval_utf8("2001.5: %{=rpn,'2k,x,+}", &x1_5)
           << PercentEvaluator::eval("%{=rpn,'2k,x,+}", &x1_5)
           << PercentEvaluator::eval_utf8("4: %{=rpn,'1,'2,==,'3,x,?:}", &x4)
           << PercentEvaluator::eval_utf8("true: %{=rpn,'aabcdaa,'bc,=~}")
           << PercentEvaluator::eval_utf8("false: %{=rpn,'aabcdaa,'bC,=~}")
           << PercentEvaluator::eval_utf8("false: %{=rpn,'aabcdaa,'c$,=~}")
           << PercentEvaluator::eval_utf8("true: %{=rpn,'aabcdaa,'a$,=~}")
           << PercentEvaluator::eval_utf8("7: %{=rpn,'foo§bar,#}")
           << PercentEvaluator::eval_utf8("8: %{=rpn,'foo§bar,##}")
  ;
  ParamSet p { "foo", "bar", "empty", "", "x", "42" };
  qDebug() << PercentEvaluator::eval_utf8(
              "%{=rpn,empty,?-}=false %{=rpn,empty,?*}=true "
              "%{=rpn,inexistent,?-}=false %{=rpn,inexistent,?*}=false "
              "%{=rpn,empty,foo,??}=bar %{=rpn,empty,foo,??*}= "
              "%{=rpn,inexistent,foo,??}=bar %{=rpn,inexistent,foo,??*}=bar "
              "%{=rpn,empty,inexistent,==,'ø,??*}=true %{=rpn,empty,inexistent,==*,'ø,??*}=ø "
              "%{=rpn,empty,inexistent,!=,'ø,??*}=false %{=rpn,empty,inexistent,!=*,'ø,??*}=ø ", &p);
  qDebug() << PercentEvaluator::eval_utf8("%{=rpn,foo,inexistent,>?}=bar %{=rpn,foo,inexistent,>?*,'ø,??*}=ø "
              "%{=rpn,'0xffffffffffffffff','1,+,'ø,??*}=ø %{=rpn,'1,'foo,+,'ø,??*}=ø "
              "%{=rpn,'0xfffffffffffffffe','1,+,'ø,??*}=18446744073709551615 "
              "%{=rpn,'abc,'12,'13,==,..}=abcfalse ", &p);
  qDebug() << PercentEvaluator::eval_utf8("%{=rpn,x,'true,&&,'ø,??*}=true %{=rpn,x,empty,&&,'ø,??*}=ø "
              "%{=rpn,x,nonexistent,&&,'ø,??*}=ø %{=rpn,<pi>}=3.141592653589793 "
              "%{=rpn,<null>}= %{=rpn,',?*}=true %{=rpn,<nil>,?*}=false", &p);
  qDebug() << PercentEvaluator::eval_utf8("%{=rpn,foo}=bar %{=rpn,'foo}=foo "
              "%{=rpn,'%foo}=%foo", // =rpn does not %evaluate it terms
              &p);
  qDebug() << PercentEvaluator::eval_utf8("%{=rpn,=rpn;'42;!!,'z,..}=truez "
              "%{=rpn,'dt: ,=date**2023,..}=dt: 2023-09-20 00:00:00,000", &p);
  return 0;
}
