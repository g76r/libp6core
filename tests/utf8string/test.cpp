#include "util/utf8string.h"
#include "log/log.h"
#include <QtDebug>
#include <unistd.h>
#include "util/paramset.h"

int main(void) {
  Log::init();
  Log::addConsoleLogger(Log::Debug);
  Utf8String s("§foo§bar§baz§§§");
  qDebug() << s.size() << s.utf8Size();
  qDebug() << s << s.split('o') << s.split("§"_u8);
  qDebug() << s.splitByLeadingChar();
  qDebug() << "/foo/bar/baz///"_u8.splitByLeadingChar();
  qDebug() << s.left(4) << "-" << s.utf8left(6) << "-" << s.utf8mid(4,3) << "-" << s.utf8right(4); 
  s = "\xef\xbb\xbf\xef\xbb\xbf\xef\xbb\xbf§foo§bar§baz§\xef\xbb\xbf§§"_u8;
  auto sc = s.cleaned();
  qDebug() << s.size() << s.utf8Size() << sc.size() << sc.utf8Size() << sc;
  qDebug() << "j k  l   m "_u8.split(' ') << "j k  l   m "_u8.split(' ', Qt::SkipEmptyParts)
           << "jjj k"_u8.split(' ', Qt::SkipEmptyParts) << ""_u8.split(' ');
  qDebug() << "ab\xef\xbb\xbf"_u8.cleaned();
  qDebug() << s.splitByLeadingChar();
  qDebug() << "  f   oo\n\rbar\vbaz"_u8.split(Utf8String::AsciiWhitespace);
  const char *p = "foo";
  qDebug() << Utf8String(false) << "-" << Utf8String(p != s) << "-"
           << Utf8String(42) << "-" << Utf8String(p) << "-"
           << Utf8String{} << "-" << Utf8String(0);
  qDebug() << Utf8String{}.isNull() << ""_u8.isNull();
  Log::debug() << s << " - " << s.split('o') << " - " << s.split("§"_u8);
  Utf8StringList l = { "foo", "bar", "baz" };
  qDebug() << PercentEvaluator::eval_utf8("%0,%{-1},%2,%8=foo bar baz,,bar,", &l);
  s = "aéÉb€¢\u03c3\u03c2\u03a3øœ×o'z"_u8;
  qDebug() << s << s.toUpper() << s.toLower() << s.toTitle() << s.isLower() << Utf8String::toTitle(0x01c6);
  s = "abcdabababaaacda";
  qDebug() << s.remove("ab") << "=cdaaacda";
  qDebug() << Utf8String::fromCEscaped("\\\\a\\x40\\60\\u00a7\\xa7\\U0001F968\\u8D8A\\U00008D8Aa\x1\x0001"_u8)
              +"=\\a\x40\60\u00a7\xa7\U0001f968\u8D8A\U00008D8Aa\x1\x0001";
  ParamSet ps { "foo", "1", "bar", "2" };
  auto bar = "%bar"_u8;
  bar %= ps;
  qDebug() << "%foo"_u8 % ps << "%foo"_u8 % &ps << bar;
  return 0;
}
