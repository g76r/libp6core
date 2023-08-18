#include <QtDebug>
#include "util/paramset.cpp"
#include "log/log.h"

int main(void) {
  Log::init();
  ParamSet p { "foo", "bar", "x", "1.5", "s1", "\xef\xbb\xbf\xef\xbb\xbf\xef\xbb\xbf§foo§bar§baz§\xef\xbb\xbf§§"_u8,
               "s2", "%{=left:%foo:1}", "baz", "42", "fooz", "%bar", "foozz", "%%bar",
               "h1", "at http://1.2.3.4/\nthere's something", "empty", "" };
  qDebug() << p.value("s2");
  qDebug() << p.evaluate("%{=mid:%foo:1:1} %{=mid:%s1:3:3} %{=mid:%s1:11:5:b}");
  qDebug() << p.evaluate("%{=base64|login:password} %{=date🥨yyyy🥨2009-04-01Z"
              "🥨UTC} %{=hex!%{=frombase64:-_9h:u}!} %{=base64:§} "
              "%{=fromhex!25:62/61 7a!} %{=md5:%%baz} %{=rpn,'0x20,x,+} ");
  qDebug() << p.evaluate("%{=escape!%{=frombase64:JWJhcg==}}=%%%%bar "
              "%{=escape!%foozz}=%%%%bar "
              "%foozz=%%bar %{'foo}=foo %{=rawvalue!fooz}=%%bar "
              "%{=rawvalue!fooz!e}=%%%%bar "
              "%{=rpn,'%foo}=%%foo %{=rpn,'foo}=foo %{=rpn,foo}=bar ");
  qDebug() << p.evaluate("%{=rawvalue:h1:hun}");
  qDebug() << p.evaluate("%{=htmlencode|%{=rawvalue:h1}|un}");
  return 0;
}
