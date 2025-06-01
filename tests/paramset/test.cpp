#include <QtDebug>
#include "util/paramset.h"
#include "util/paramsprovidermerger.h"

int main(void) {
  ParamSet p { "foo", "bar", "x", "1.5", "s1", "\xef\xbb\xbf\xef\xbb\xbf\xef\xbb\xbf§foo§bar§baz§\xef\xbb\xbf§§"_u8,
               "s2", "%{=left:%foo:1}", "baz", "42", "fooz", "%bar", "foozz", "%%bar",
               "h1", "at http://1.2.3.4/\nthere's something", "empty", "", "i", "0x1f",
               "func1", "%{=uppercase:%1}", "func2", "%{=uppercase:%1}%{=lowercase:%2}",
               "tosqlin", "('%{=sub:%1:/ +/','/g}')", "ooks", "%baz", "x42", "43" };
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
                "%{=rpn,0x20,%x,+}=33.5 ", &p);
  qDebug() << PercentEvaluator::eval_utf8(
              "*** %foozz=%%bar foo=foo %{=rawvalue!fooz}=%%bar "
              "%{=rawvalue!fooz!e}=%%%%bar "
              "%ooks=42 %{%ooks}= %{=eval:ooks}=42 %{x%ooks}= %{=eval:x%ooks}=43 "
              "%{=rpn,%%foo}=%%foo %{=rpn,foo}=foo %{=rpn,%foo}=bar ***", &p);
  qDebug() << PercentEvaluator::eval_utf8(
                "%{=rawvalue!fooz!baz!e}=%%%%bar "
                "%{=rawvalue!notexist!baz!e}=42 "
                "%{=rawvalue!empty!baz!ehun}= "
                "%{=rawvalue!fooz!baz!}=%%bar "
                "%{=rawvalue!notexist!baz}= " // "baz" is processed as flags
                , &p);
  qDebug() << PercentEvaluator::eval_utf8("%{=rawvalue:h1:hun}", &p);
  qDebug() << PercentEvaluator::eval_utf8(
                "%{=htmlencode|%{=rawvalue:h1}|un}", &p);
  qDebug() << PercentEvaluator::eval_utf8(
                "%{=int64:-3.14}=-3 %{=int64:blurp:%baz}=42 "
                "%{=uint64:3.14:2.71}=3 %{=uint64:-3.14:2.72}=2 "
                "%{=int64:1e3}=1000 %{=int64:1k}=1000", &p);
  qDebug() << PercentEvaluator::eval_utf8(
                "%{=rpn,%{=int64:-3.14},%{=uint64:3.14},%{=double:3.14},%{=bool:3.14},3.14,%s2,%does_not_exists,<etvs>}"
                "=i8{-3},u8{3},f8{3.14},b{true},\"3.14\",\"b\",null{}", &p);
  qDebug() << PercentEvaluator::eval_utf8(
                "%{=int64:1e50}= %{=int64:10000P}= " // both overflows
                "%{=rpn,4G,4G,*}=16000000000000000000 " // still fit in an unsigned8
                "%{=rpn,-4G,4G,*}= " // overflows a signed8
                "%{=rpn,4.0G,4G,*}=1.6e+19"
                "%{=rpn,8G,4G,*}= " // overflows an unsigned8
                "%{=rpn,8.0G,4G,*}=3.2e+19 ");
  qDebug() << PercentEvaluator::eval_utf8("%{=uppercase:fooǆ}|%{=lowercase:fooǆ}|%{=titlecase:fooǆ}");
  qDebug() << PercentEvaluator::eval_utf8("%{=sub;Foo_Barǆ;/_/-/g↑}|%{=sub;Foo_Barǆ;/_/-/g↓}");
  qDebug() << PercentEvaluator::eval_utf8("%{=sub;Foo_Bar;/O/z/gi}=Fzz_Bar %{=sub;Foo_Bar;/O/z/g}=Foo_Bar %{=sub;Foo_Bar;/(?i)O/z/g}=Fzz_Bar");
  qDebug() << PercentEvaluator::eval_utf8(
                "%{=formatint64:31:16:0000}=001f 0x%{=formatint64:31:16}=0x1f %{=formatint64:%i::%j}=31 "
                "%{=formatuint64:0xffffffff:16:0000000000:ø}=00ffffffff "
                "%{=formatint64:2e3::000000:ø}=002000 %{=formatint64:0xffffffffffffffff:16::ø}=ø "
                "%{=formatuint64:0xffffffffffffffff:16::ø}=ffffffffffffffff "
                "%{=formatdouble:1M:e}=1.000000e+06 %{=formatdouble:1::2}=1.00 "
                "%{=formatboolean:1M}=true %{=formatboolean:0}=false %{=formatboolean:true}=true "
                "%{=formatboolean:Z}= %{=formatboolean:Z::false}=false", &p);
  qDebug() << PercentEvaluator::eval_utf8(
                "%{=apply:func1:a}=A %{=apply:func2:a:B}=Ab %{=apply:tosqlin:foo bar baz}=('foo','bar','baz')", &p);
  qDebug() << PercentEvaluator::eval_utf8(
                "'%{=box:foo:6}'='   foo' '%{=box:foo:6:r}'='foo   ' '%{=box:foo:6:c}'=' foo  '");
  p.insert("foo", "12345");
  qDebug() << PercentEvaluator::eval_utf8(
                "%{=box:%foo:6::0}=012345 %{=box:%foo:8:r:+}=12345+++ "
                "%{=box:%foo:8:r:.,}=12345.,. %{=box:%foo:8:c:+}=+12345++ "
                "%{=box:%foo:8::+}=+++12345 %{=box:  bar::t}=bar %{=box:  bar:🥨:t}=bar "
                "%{=box:%foo:3}=123 %{=box:%foo:3:l}=345 %{=box:%foo:3:m}=145 "
                "%{=box:%foo:3:m::…}=1…5 %{=box:%foo:3:mb::…}=… "
                "%{=box:%foo:3:m::🥨}=1🥨5 %{=box:%foo:3:r::🥨}=12🥨 "
                "%{=box:%foo%foo:8:m::...}=12...345 %{=box:%foo%foo:7:m::...}=12...45 "
                "%{=box:%foo%foo:6:m::...}=1...45 "
                "%{=box:%foo:4:::...}=1... "
                "%{=box:%foo:4:l::...}=...1 %{=box:%foo:4:m::...}=...1 "
                "%{=box:%foo:3:::abc}=abc %{=box:%foo:3:::abcdef}=abc "
                , &p);
  qDebug() << PercentEvaluator::eval_utf8(
                "%{=elideright:%foo%foo:6}=123... %{=elideleft:%foo%foo:6}=...345 "
                "%{=elidemiddle:%foo%foo:6}=1...45 "
                "%{=elideright:%foo%foo:6:…}=12345… %{=elideleft:%foo%foo:6:…}=…12345 "
                "%{=elidemiddle:%foo%foo:6:…}=12…345 "
                , &p);
  return 0;
}
