#include "util/paramset.h"
#include "util/mathexpr.h"
#include "util/mathutils.h"

int main(void) {
  QVariant x(ULLONG_MAX/2);
  QVariant y(-132);
  qDebug() << x.metaType().name() << y.metaType().name()
           << MathUtils::promoteToBestNumericType(&x, &y)
           << x.metaType().name() << y.metaType().name()
           << x.toLongLong() << y.toLongLong()
           << x.toDouble() << y.toDouble()
           << (MathUtils::compareQVariantAsNumber(x, y) == QPartialOrdering::Greater)
           << (MathUtils::compareQVariantAsNumber(x, y) == QPartialOrdering::Unordered)
           << ParamSet().evaluate("3: %{=rpn,'1,'2,+}")
           << ParamSet({"x","5"}).evaluate("6: %{=rpn,'1,x,+}")
           << ParamSet({"x","5"}).evaluate("15: %{=rpn,'1,x,..}")
           << ParamSet().evaluate(": %{=rpn,'1,',+}")
           << ParamSet().evaluate("2: %{=rpn,'1,'true,+}")
           << ParamSet().evaluate("true: %{=rpn,'1,'true,&&}")
           << ParamSet().evaluate("true: %{=rpn,'1,'true,==}")
           << ParamSet().evaluate("false: %{=rpn,'42,'true,==}")
           << ParamSet().evaluate("true: %{=rpn,'42,!!,'true,==}")
           << ParamSet({"x","1"}).evaluate("33: %{=rpn,'0x20,x,+}")
           << ParamSet({"x","1.5"}).evaluate("33.5: %{=rpn,'0x20,x,+}")
           << ParamSet({"x","1.5"}).evaluate("2001.5: %{=rpn,'2k,x,+}")
           << ParamSet({"x","4"}).evaluate("4: %{=rpn,'1,'2,==,'3,x,?:}")
           << ParamSet().evaluate("true: %{=rpn,'aabcdaa,'bc,=~}")
           << ParamSet().evaluate("false: %{=rpn,'aabcdaa,'bC,=~}")
           << ParamSet().evaluate("false: %{=rpn,'aabcdaa,'c$,=~}")
           << ParamSet().evaluate("true: %{=rpn,'aabcdaa,'a$,=~}")
    ;
  ParamSet p { "foo", "bar", "empty", "", "x", "42" };
  qDebug() << p.evaluate("%{=rpn,empty,?-}=false %{=rpn,empty,?*}=true "
              "%{=rpn,inexistent,?-}=false %{=rpn,inexistent,?*}=false "
              "%{=rpn,empty,foo,??}=bar %{=rpn,empty,foo,??*}= "
              "%{=rpn,inexistent,foo,??}=bar %{=rpn,inexistent,foo,??*}=bar "
              "%{=rpn,empty,inexistent,==,'ø,??*}=true %{=rpn,empty,inexistent,==*,'ø,??*}=ø "
              "%{=rpn,empty,inexistent,!=,'ø,??*}=false %{=rpn,empty,inexistent,!=*,'ø,??*}=ø ");
  qDebug() << p.evaluate("%{=rpn,foo,inexistent,>?}=bar %{=rpn,foo,inexistent,>?*,'ø,??*}=ø "
              "%{=rpn,'0xffffffffffffffff','1,+,'ø,??*}=ø %{=rpn,'1,'foo,+,'ø,??*}=ø "
              "%{=rpn,'0xfffffffffffffffe','1,+,'ø,??*}=18446744073709551615 "
              "%{=rpn,'abc,'12,'13,==,..}=abcfalse ");
  qDebug() << p.evaluate("%{=rpn,x,'true,&&,'ø,??*}=true %{=rpn,x,empty,&&,'ø,??*}=ø %{=rpn,x,nonexistent,&&,'ø,??*}=ø");
  return 0;
}
