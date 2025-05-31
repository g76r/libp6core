#include "util/paramset.h"
#include "util/paramsformula.h"
#include "util/mathutils.h"
#include <QDateTime>

QDebug operator<<(QDebug dbg, std::partial_ordering po) {
  if (po == std::partial_ordering::equivalent)
    return dbg << "equivalent";
  if (po == std::partial_ordering::less)
    return dbg << "less";
  if (po == std::partial_ordering::greater)
    return dbg << "greater";
  return dbg << "unordered";
}

int main(void) {
  QVariant x(ULLONG_MAX/2);
  QVariant y(-132);
  ParamSet e;
  ParamSet x1 { "x", "1" };
  ParamSet x1_5 { "x", "1.5" };
  ParamSet x4 { "x", "4" };
  ParamSet x5 { "x", "5" };
  ParamSet xa_d { "x", "a$" };
  qDebug() << PercentEvaluator::eval_utf8("3: %{=rpn,1,2,+}");
  qDebug() << PercentEvaluator::eval("%{=rpn,1,2,+}");
  qDebug() << PercentEvaluator::eval_utf8("6: %{=rpn,1,%x,+}", &x5);
  qDebug() << PercentEvaluator::eval_utf8("51: %{=rpn,1,%x,@}", &x5);
  qDebug() << PercentEvaluator::eval_utf8(": %{=rpn,1,,+}");
  qDebug() << PercentEvaluator::eval("%{=rpn,1,,+}");
  qDebug() << PercentEvaluator::eval_utf8("2: %{=rpn,1,true,+}");
  qDebug() << "true" << PercentEvaluator::eval("%{=rpn,1,true,&&}");
  qDebug() << "null" << PercentEvaluator::eval("%{=rpn,<null>,true,&&}");
  //qDebug() << "false" << PercentEvaluator::eval("%{=rpn,<null>,true,&&*}");
  qDebug() << "160 (0xa0): %{=rpn,0xaa,0xf0,&}" % e;
  qDebug() << "250 (0xfa): %{=rpn,0xaa,0xf0,|}" % e;
  qDebug() << "90 (0x5a): %{=rpn,0xaa,0xf0,^}" % e;
  qDebug() << "null" <<  PercentEvaluator::eval("%{=rpn,0xaa,<null>,|}");
  qDebug() << PercentEvaluator::eval_utf8("true: %{=rpn,1,true,==}");
  qDebug() << PercentEvaluator::eval_utf8("false: %{=rpn,42,true,==}");
  qDebug() << PercentEvaluator::eval_utf8("true: %{=rpn,42,!!,true,==}");
  qDebug() << PercentEvaluator::eval_utf8("33: %{=rpn,0x20,%x,+}", &x1);
  qDebug() << PercentEvaluator::eval_utf8("33.5: %{=rpn,0x20,%x,+}", &x1_5);
  qDebug() << PercentEvaluator::eval_utf8("2001.5: %{=rpn,2k,%x,+}", &x1_5);
  qDebug() << PercentEvaluator::eval("%{=rpn,2k,%x,+}", &x1_5);
  qDebug() << 4 << "%{=rpn,2,1,==,3,%x,:?}" % x4;
  qDebug() << 4 << "%{=rpn,%x,3,2,1,==,?:}" % x4;
  qDebug() << "null" << "%{=rpn,%notexists,3,4,:?}" % e;
  qDebug() << "null" << "%{=rpn,4,3,%notexists,?:}" % e;
  qDebug() << 4 << "%{=rpn,%notexists,3,4,:?*}" % e;
  qDebug() << 4 << "%{=rpn,4,3,%notexists,?:*}" % e;
  qDebug() << PercentEvaluator::eval_utf8("true: %{=rpn,aabcdaa,bc,=~}");
  qDebug() << PercentEvaluator::eval_utf8("false: %{=rpn,aabcdaa,bC,=~}");
  qDebug() << PercentEvaluator::eval_utf8("false: %{=rpn,aabcdaa,c$,=~}");
  qDebug() << PercentEvaluator::eval_utf8("true: %{=rpn,aabcdaa,a$,=~}");
  qDebug() << PercentEvaluator::eval_utf8("true: %{=rpn,aabcdaa,%x,=~}", &xa_d);
  qDebug() << PercentEvaluator::eval_utf8("7: %{=rpn,foo§bar,#}");
  qDebug() << PercentEvaluator::eval_utf8("8: %{=rpn,foo§bar,##}");
  qDebug() << PercentEvaluator::eval_utf8("1: %{=rpn,5,4,-}");
  qDebug() << PercentEvaluator::eval_utf8("-1: %{=rpn,5,4,:=:,-}");
  qDebug() << PercentEvaluator::eval_utf8("16: %{=rpn,4,<dup>,*}");
  qDebug() << PercentEvaluator::eval_utf8(": %{=rpn,*}");
  qDebug() << "ø: "+ParamsFormula(",*", ParamsFormula::RpnWithPercents).eval_utf8({}, "ø");
  qDebug() << "ø: "+ParamsFormula(",<nil>,<nil>,@", ParamsFormula::RpnWithPercents).eval_utf8({}, "ø");
  qDebug() << "ø: "+ParamsFormula(",<nil>,a,@", ParamsFormula::RpnWithPercents).eval_utf8({}, "ø");
  qDebug() << "a: "+ParamsFormula(",<nil>,a,@*", ParamsFormula::RpnWithPercents).eval_utf8({}, "ø");
  qDebug() << "ø: "+ParamsFormula(",1,<nil>,*", ParamsFormula::RpnWithPercents).eval_utf8({}, "ø");
  qDebug() << "ø: "+ParamsFormula(",1,foo,*", ParamsFormula::RpnWithPercents).eval_utf8({}, "ø");
  ParamSet p { "foo", "bar", "empty", "", "x", "42" };
  qDebug() << PercentEvaluator::eval_utf8(
              "%{=rpn,%empty,?-}=false %{=rpn,%empty,?*}=true "
              "%{=rpn,%inexistent,?-}=false %{=rpn,%inexistent,?*}=false "
              "%{=rpn,%empty,%foo,??}=bar %{=rpn,%empty,%foo,??*}= "
              "%{=rpn,%inexistent,%foo,??}=bar %{=rpn,%inexistent,%foo,??*}=bar "
              "%{=rpn,%empty,%inexistent,==,ø,??*}=true %{=rpn,%empty,%inexistent,==*,ø,??*}=ø "
              "%{=rpn,%empty,%inexistent,!=,ø,??*}=false %{=rpn,%empty,%inexistent,!=*,ø,??*}=ø ", &p);
  qDebug() << PercentEvaluator::eval_utf8("%{=rpn,%foo,%inexistent,>?}=bar %{=rpn,%foo,%inexistent,>?*,ø,??*}=ø "
              "%{=rpn,0xffffffffffffffff,1,+,ø,??*}=ø %{=rpn,1,foo,+,ø,??*}=ø "
              "%{=rpn,0xfffffffffffffffe,1,+,ø,??*}=18446744073709551615 "
              "%{=rpn,abc,12,13,==,@}=falseabc ", &p);
  qDebug() << PercentEvaluator::eval_utf8("%{=rpn,%x,true,&&,ø,??*}=true %{=rpn,%x,%empty,&&,ø,??*}=ø "
              "%{=rpn,%x,%nonexistent,&&,ø,??*}=ø %{=rpn,<pi>}=3.141592653589793 "
              "%{=rpn,<null>}= %{=rpn,,?*}=true %{=rpn,<nil>,?*}=false", &p);
  qDebug() << PercentEvaluator::eval_utf8("%{=rpn,%foo}=bar %{=rpn,foo}=foo "
              "%{=rpn,%%foo}=%foo",
              &p);
  qDebug() << PercentEvaluator::eval_utf8("%{=rpn,%{=rpn;42;!!},z,@}=ztrue "
              "%{=rpn,%{=date@@2023-09-20},dt:,@}=dt:2023-09-20 00:00:00,000 "
              "%{=rpn,%{=date@@2023-09-20},dt:,@}=dt:2023-09-20 00:00:00,000 "
              "%{=rpn,1,2,+}=3", &p);
  TypedValue f1(std::sqrt(-1)), f2(0/0.0),
      f3(std::numeric_limits<double>::infinity()), f4(3.14), i1(42), s1("§§"),
      ts1(QDateTime::fromString("2023-09-20T13:14:00,760",Qt::ISODateWithMs)),
      f5(42.0);
  p.insert("ts1", ts1);
  p.insert("f1", f1); // don't insert because !f1
  p.insert("f3", f3);
  p.insert("f4", f4);
  p.insert("i1", i1);
  p.insert("f5", f5);
  qDebug() << "nan"_u8.toDouble() << "NAN"_u8.toDouble() << "NaN"_u8.toDouble()
           << "inf"_u8.toDouble() << "INF"_u8.toDouble() << "-inf"_u8.toDouble();
  qDebug() << "float8() TypedValue isnull isnan isfinite isinf";
  qDebug() << f1.float8() << f1 << !f1 << std::isnan(f1) << std::isfinite(f1) << std::isinf(f1);
  qDebug() << f2.float8() << f2 << !f2 << std::isnan(f2) << std::isfinite(f2) << std::isinf(f2);
  qDebug() << f3.float8() << f3 << !f3 << std::isnan(f3) << std::isfinite(f3) << std::isinf(f3);
  qDebug() << f4.float8() << f4 << !f4 << std::isnan(f4) << std::isfinite(f4) << std::isinf(f4);
  qDebug() << i1.float8() << i1 << !i1 << std::isnan(i1) << std::isfinite(i1) << std::isinf(i1);
  qDebug() << s1.float8() << s1 << !s1 << std::isnan(s1) << std::isfinite(s1) << std::isinf(s1);
  qDebug() << PercentEvaluator::eval_utf8(
                "%{=rpn,%ts1,42,~~,3.14,0,+,<etvs>}=ts{2023-09-20T13:14:00.760},i8{42},f8{3.14}   "
                "%{=rpn,%ts1,42,~~,3.14,0,+,<typecodes>}=ts,i8,f8   "
                "%{=rpn,3.14,0,+,<etv>}=f8{3.14}   "
                "%{=rpn,3.14,0,+,<typeid>}=5   ",
                &p);
  qDebug() << p.paramValue("f1") << f1 << "%f1"_u8 % p;
  qDebug() << PercentEvaluator::eval_utf8(
                "%{=switch:%f5:%i1:good:bad} %{=switch:%f4:3.14:good:bad} "
                "%{=switch:%f1::good:bad} %{=switch:::good:bad} "
                "%{=switch:%notexist::good:bad} %{=switch:%f3:inf:good:bad} "
                "%{=rpn,0,0.0,/}=nan %{=switch:%{=rpn,0,0.0,/}:foo:}=nan "
                "%{=switch:%{=rpn,0,0.0,/}:nan:good:bad} ",
                &p);
  qDebug() << TypedValue::compare_as_number_otherwise_string(42.0, 42, false)
           << TypedValue::compare_as_number_otherwise_string(42.0, 42, true)
           << TypedValue::compare_as_number_otherwise_string(42.0, "42", false)
           << TypedValue::compare_as_number_otherwise_string(42.0, "42", true)
           << TypedValue::compare_as_number_otherwise_string(42.000000001, "42", false)
           << TypedValue::compare_as_number_otherwise_string(42.000000001, "42", true)
           << TypedValue::compare_as_number_otherwise_string(42, std::vector<double>({42.0}), false)
           << TypedValue::compare_as_number_otherwise_string(42, std::vector<double>({42.0}), true)
           << TypedValue(42) << TypedValue(std::vector<double>({42.0}))
              ;
  qDebug() << TypedValue::compare_as_number_otherwise_string(f1, f1, false) // unordered because any null/nan result in unordered
           << TypedValue::compare_as_number_otherwise_string(f1, f1, true) // equivalent because two nans are equivalent when pretending null is empty
           << TypedValue::compare_as_number_otherwise_string(f1, "", false) // unordered
           << TypedValue::compare_as_number_otherwise_string(f1, "", true) // equivalent because we pretend nan is ""
           << TypedValue::compare_as_number_otherwise_string(f1, "nan", false) // unordered
           << TypedValue::compare_as_number_otherwise_string(f1, "nan", true) // equivalent because "nan" can be converted to a double
           << TypedValue::compare_as_number_otherwise_string(f1, "foo", true) // unordered
              ;
  //ts1(QDateTime::fromString("2023-09-20T13:14:00,760",Qt::ISODateWithMs))
  auto ts42 = QDateTime::fromString("1970-01-01T00:00:00,042Z",Qt::ISODateWithMs);
  qDebug() << TypedValue::compare_as_number_otherwise_string(42, ts42, false)
           << TypedValue::compare_as_number_otherwise_string(42, ts42, true)
           << TypedValue(42) << TypedValue(ts42) << TypedValue(ts42).as_utf8()
           << TypedValue(ts42).as_unsigned8() << TypedValue(ts42).as_float8()
           << TypedValue(ts42).as_signed8()
              ;
  return 0;
}
