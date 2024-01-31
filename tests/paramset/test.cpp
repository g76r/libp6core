#include <QtDebug>
#include "util/paramset.h"
#include "util/paramsprovidermerger.h"

int main(void) {
  ParamSet p { "foo", "bar", "x", "1.5", "s1", "\xef\xbb\xbf\xef\xbb\xbf\xef\xbb\xbf§foo§bar§baz§\xef\xbb\xbf§§"_u8,
               "s2", "%{=left:%foo:1}", "baz", "42", "fooz", "%bar", "foozz", "%%bar",
               "h1", "at http://1.2.3.4/\nthere's something", "empty", "" };
  auto ppm = ParamsProviderMerger(&p);
  qDebug() << p;
  qDebug() << p.paramUtf8("s2");
  qDebug() << PercentEvaluator::eval_utf8(
                "~~~ %{=mid:%foo:1:1}=a %{=mid:%s1:3:3}=o§b "
                "%{=mid:%s1:11:5:b}=foo§ ~~~", &p);
  qDebug() << PercentEvaluator::eval_utf8(
                "%{=base64|login:password}=bG9naW46cGFzc3dvcmQ= "
                "%{=date🥨yyyy🥨2009-04-01Z🥨UTC}=2009 "
                "%{=hex!%{=frombase64:-_9h:u}!}=fbff61 %{=base64:§}=wqc= "
                "%{=fromhex!25:62/61 7a!}=%%baz "
                "%{=md5:%%baz}=96ab86a37cef7e27d8d45af9c29dc974 "
                "%{=rpn,'0x20,x,+}=33.5 ", &p);
  qDebug() << PercentEvaluator::eval_utf8(
              "*** %foozz=%%bar %{'foo}=foo %{=rawvalue!fooz}=%%bar "
              "%{=rawvalue!fooz!e}=%%%%bar "
              "%{=rpn,'%foo}=%%foo %{=rpn,'foo}=foo %{=rpn,foo}=bar ***", &p);
  qDebug() << PercentEvaluator::eval_utf8("%{=rawvalue:h1:hun}", &p);
  qDebug() << PercentEvaluator::eval_utf8(
                "%{=htmlencode|%{=rawvalue:h1}|un}", &p);
  qDebug() << PercentEvaluator::eval_utf8(
                "%{=integer:-3.14}=-3 %{=integer:blurp:%baz}=42 "
                "%{=integer:1e3}=1000 %{=integer:1k}=1000", &p);
  qDebug() << PercentEvaluator::eval_utf8( // overflows
                "%{=integer:1e50}= %{=integer:10000P}= %{=rpn,'4G,'4G,*}= "
                "%{=rpn,'4.0G,'4G,*}=1.6e+19");
  qDebug() << PercentEvaluator::eval_function("'abcdef")
           << p.paramRawValue("'abcdef") << p.paramUtf8("'abcdef") << ppm.paramUtf8("'abcdef");
  qDebug() << PercentEvaluator::eval_utf8("%{=uppercase:fooǆ}|%{=lowercase:fooǆ}|%{=titlecase:fooǆ}");
  qDebug() << PercentEvaluator::eval_utf8("%{=sub;Foo_Barǆ;/_/-/g↑}|%{=sub;Foo_Barǆ;/_/-/g↓}");
  return 0;
}
