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
           << ParamSet({"x","4"}).evaluate("4: %{=rpn,'1,'2,==,'3,x,?:}")
           << ParamSet().evaluate("true: %{=rpn,'aabcdaa,'bc,=~}")
           << ParamSet().evaluate("false: %{=rpn,'aabcdaa,'bC,=~}")
           << ParamSet().evaluate("false: %{=rpn,'aabcdaa,'c$,=~}")
           << ParamSet().evaluate("true: %{=rpn,'aabcdaa,'a$,=~}")
    ;
  return 0;
}
