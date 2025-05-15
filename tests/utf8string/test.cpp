#include "util/paramset.h"
#include <QtDebug>

int main(void) {
  p6::log::init();
  p6::log::add_console_logger(Log::Debug, false, stdout);
  Utf8String s("§foo§bar§baz§§§");
  qDebug() << s.size() << s.utf8size();
  qDebug() << s << s.split('o') << s.split("§"_u8);
  qDebug() << s.split_headed_list();
  qDebug() << "/foo/bar/baz///"_u8.split_headed_list();
  qDebug() << s.left(4) << "-" << s.utf8left(6) << "-" << s.utf8mid(4,3) << "-" << s.utf8right(4); 
  s = "\xef\xbb\xbf\xef\xbb\xbf\xef\xbb\xbf§foo§bar§baz§\xef\xbb\xbf§§"_u8;
  auto sc = s.cleaned();
  qDebug() << s.size() << s.utf8size() << sc.size() << sc.utf8size() << sc;
  qDebug() << "j k  l   m "_u8.split(' ') << "j k  l   m "_u8.split(' ', Qt::SkipEmptyParts)
           << "jjj k"_u8.split(' ', Qt::SkipEmptyParts) << ""_u8.split(' ');
  qDebug() << "ab\xef\xbb\xbf"_u8.cleaned();
  qDebug() << s.split_headed_list();
  qDebug() << "  f   oo\n\rbar\vbaz"_u8.split(Utf8String::AsciiWhitespace);
  const char *p = "foo";
  qDebug() << Utf8String(false) << "-" << Utf8String(p != s) << "-"
           << Utf8String(42) << "-" << Utf8String(p) << "-"
           << Utf8String{} << "-" << Utf8String(0);
  qDebug() << Utf8String{}.isNull() << ""_u8.isNull();
  p6::log::debug() << s << " - " << s.split('o') << " - " << s.split("§"_u8);
  Utf8StringList l = { "foo", "bar", "baz" };
  qDebug() << PercentEvaluator::eval_utf8("%0,%{-1},%2,%8=foo bar baz,,bar,", &l);
  qDebug() << l << '|' << l.join(' ') << '|' << QVariant(l).toString();
  s = "aéÉb€¢\u03c3\u03c2\u03a3øœ×o'z"_u8;
  qDebug() << s << s.toUpper() << s.toLower() << s.toTitle() << s.isLower() << Utf8String::toTitle(0x01c6);
  s = "abcdabababaaacda";
  qDebug() << s.remove("ab") << "=cdaaacda";
  qDebug() << Utf8String::fromCEscaped("\\\\a\\x40\\60\\u00a7\\xa7\\U0001F968\\u8D8A\\U00008D8Aa\x1\x0001"_u8)
              +"=\\a\x40\60\u00a7\xa7\U0001f968\u8D8A\U00008D8Aa\x1\x0001";
  ParamSet ps { "foo", "1", "bar", "2" };
  auto bar = "%bar"_u8;
  bar %= ps;
  qDebug() << ps.paramValue("foo");
  qDebug() << "%foo"_u8 % ps << "%foo"_u8 % &ps << bar;
  qDebug() << "42"_u8.toNumber<long long>() << "0x1b"_u8.toNumber<int>() << "1e6M"_u8.toNumber<double>();
  ps = { "foo", "12345"};
  qDebug() << Utf8String::elide(0,false,"foobar",5,"§") << "fo§ar"_u8
           << Utf8String::elide(-1,true,"foo§ar",4,"") << "§ar"_u8
           << Utf8String::pad(-1,false,"fo§",6,"+") << "+++fo§"_u8
           << Utf8String::pad(-1,true,"fo§",6,"+") << "++fo§"_u8
           << Utf8String::pad(0,false,"hi!",7," ") << "  hi!  "_u8
           << Utf8String::pad(0,false,"fo§",6,"12345") << "1fo§23"_u8
              ;
  qDebug() << Utf8String().isNull() << Utf8String(QVariant()).isNull()
           << Utf8String(QString()).isNull() << Utf8String(QByteArray()).isNull()
           << Utf8String(QAnyStringView(QString())).isNull()
           << Utf8String(QByteArrayView(QByteArray())).isNull()
           << Utf8String(QVariant(QString())).isNull()
           << Utf8String(QVariant(QByteArray())).isNull()
           << Utf8String((const char *)0, -1).isNull()
           << QString().toUtf8().isNull()
              ; // everything should be true here
  qDebug() << Utf8String(u""_s).isNull() << Utf8String(QVariant(u""_s)).isNull()
           << u""_s.toUtf8().isNull()
              ; // everything should be false here
  qDebug() << Utf8String("9223372036854775807.0").toULongLong() << "= 0" // > 2**53 would be rounded to 922...808
           << Utf8String("9223372036854775807").toULongLong() << "= 9223372036854775807"
           << Utf8String("9007199254740992.0").toULongLong() << "= 9007199254740992" // 2**53
           << Utf8String("9007199254740995.0").toULongLong() << "= 0" // > 2**53 would be rounded to 900...996
           << Utf8String("1e3").toULongLong() << "= 1000"
              ;
  qDebug() << "abc§越🥨"_u8 << "abc§越🥨"_u8.utf8right(3) << ""_u8.utf8right(2) << "abc§越🥨"_u8.utf8right(0);
  qDebug() << "abcdef"_u8.utf8chopped(3) << ""_u8.utf8chopped(3) << "abc§越🥨"_u8.utf8chopped(3) << Utf8String{}.utf8chopped(3);
  qDebug() << Utf8String("1e3k").toDouble() << "= 1e+06"
           << Utf8String("1000k").toLongLong() << "= 1000000"
           << Utf8String("3.14u").toDouble() << "= 3.14e-06"
           << Utf8String("3.14µ").toDouble() << "= 3.14e-06"
           << Utf8String("3.14P").toDouble() << "= 3.14e+15"
           << Utf8String("8M").toLongLong() << "= 8000000"
           << Utf8String("8m").toLongLong() << "= 8000000"
           << Utf8String("8G").toLongLong() << "= 8000000000"
           << Utf8String("8b").toLongLong() << "= 8000000000"
              ;
  qDebug()
      << Utf8String((char)0xc2)+Utf8String((char)0xa7) // §
      << "\xc2\xa7"_u8 // §
      << "a'bc§♯越🥨"_u8.cEscaped()
      << "a'bc§♯越🥨"_u8.asciiCEscaped()
      << Utf8String::cEscaped("§"_u8[0]) // \xc2 first byte of §
      << Utf8String::cEscaped('a')
      << Utf8String::cEscaped('\a')
      << Utf8String::cEscaped('\n')
      << Utf8String::cEscaped(0)
      << Utf8String::asciiCEscaped(U'a')
      << Utf8String::asciiCEscaped(U'\a')
      << Utf8String::asciiCEscaped('\a')
      << Utf8String::asciiCEscaped('\b')
      << Utf8String::asciiCEscaped(0)
      << Utf8String::asciiCEscaped(U'§')
      << Utf8String::asciiCEscaped(U'🥨');

  ps = { "empty", "" };
  qDebug()
      << "%{=coalesce:%{=rpn,a}:ø}=a "
         "%{=coalesce:%{=rpn,<null>}:ø}=ø "
         "%{=coalesce:%{=rpn,a,~~}:ø}=ø " // cannot convert to int -> invalid
         "%{=coalesce:%{=rpn,%notdefined}:ø}=ø "
         "%{=coalesce:%{=rpn,}:ø}=ø " // empty list is invalid
         "%{=coalesce:%{=rpn,,}:ø}= "
         "%{=coalesce:%{=rpn,%empty}:ø}= "_u8 % ps;
  return 0;
}
