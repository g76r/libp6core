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
  qDebug() << s.left(4) << "-" << s.utf8Left(6) << "-" << s.utf8Mid(4,3) << "-" << s.utf8Right(4); 
  s = "\xef\xbb\xbf\xef\xbb\xbf\xef\xbb\xbf§foo§bar§baz§\xef\xbb\xbf§§"_u8;
  qDebug() << s.size() << s.utf8Size();
  qDebug() << s.splitByLeadingChar();
  qDebug() << "  f   oo\n\rbar\vbaz"_u8.split(Utf8String::AsciiWhitespace);
  const char *p = "foo";
  qDebug() << Utf8String(false) << "-" << Utf8String(p != s) << "-"
           << Utf8String(42) << "-" << Utf8String(p) << "-"
           << Utf8String{} << "-" << Utf8String(0);
  qDebug() << Utf8String{}.isNull() << ""_u8.isNull();
  Log::debug() << s << " - " << s.split('o') << " - " << s.split("§"_u8);
  Utf8StringList l = { "foo", "bar", "baz" };
  qDebug() << ParamSet().evaluate("%0,%{-1},%2,%8=foo bar baz,,bar,", false, &l);
  return 0;
}
