#include "tst_mathexpr.h"
#include "util/mathexpr.h"
#include "util/paramset.h"
#include <QDateTime>
#include <QTimeZone>

void tst_MathExpr::rpnBasics() {
  qint64 msecs = 1'662'821'797'014LL;
  QString tss { "2022-09-10T16:56:37.014" };
  QString tsutcs { "2022-09-10T16:56:37.014Z" };
  QString ts2s { "2022-09-10T16:56:37.014+02:00" };
  auto ts = QDateTime::fromMSecsSinceEpoch(msecs);
  auto tsutc = QDateTime::fromString(tsutcs, Qt::ISODateWithMs);
  auto ts2 = QDateTime::fromString(ts2s, Qt::ISODateWithMs);
  qulonglong small_ullong = ULLONG_MAX/2, huge_ullong = ULLONG_MAX/2+1;
  RawParamsProvider params{ { "a", 1 }, { "b", 2 }, { "nil", QVariant() },
                            { "as", "1"}, { "bs", "2" }, { "empty", "" },
                            { "ts", ts }, { "tsutc", tsutc }, { "ts2", ts2 },
                            { "tss", tss }, { "tsutcs", tsutcs },
                            { "ts2s", ts2s }, { "msecs", msecs },
                            { "yes", true }, { "no", false },
                            { "half", .5 }, { "halfs", "0.5" },
                            { "small_ullong", small_ullong },
                            { "huge_ullong", huge_ullong },
                            };
  MathExpr e { ",'1", MathExpr::CharacterSeparatedRpn };
  QVariant v;

  e = MathExpr{ ",a,b,+", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, 3);

  e = MathExpr{ ",'1,'2,+", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, 3);

  e = MathExpr{ ",'1,',+", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, QVariant());

  e = MathExpr{ ",a,',+", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, QVariant());

  e = MathExpr{ ",a,'0x20,+", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, 33);

  e = MathExpr{ ",a,'0x20,+,half,+", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, 33.5);

  e = MathExpr{ ",'1,'2,==,'3,'4,?:", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, 4);

  e = MathExpr{ ",a,nil,+", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, QVariant());

  e = MathExpr{ ",'1,'true,+", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, 2);

  e = MathExpr{ ",'1,'true,&&", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",a,b,&&", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",a,b,&&,yes,==", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",'42,!!,'true,==", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",a,b,..", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, "12");

  e = MathExpr{ ",a,b,==", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, false);

  e = MathExpr{ ",a,b,!=", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",empty,nil,==", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",',unknownvariable,==", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",empty,nil,==*", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, false);

  e = MathExpr{ ",empty,unknownvariable,==*", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, false);

  e = MathExpr{ ",ts,msecs,==", MathExpr::CharacterSeparatedRpn }; // datetime to longlong
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",ts,tss,==", MathExpr::CharacterSeparatedRpn }; // datetime to timestamp
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",ts2,ts2s,==", MathExpr::CharacterSeparatedRpn }; // w/ tz
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",tsutc,tsutcs,==", MathExpr::CharacterSeparatedRpn }; // w/ utc tz
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",'aabcdaa,'bc,~=", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",'aabcdaa,'bC,~=", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, false);

  e = MathExpr{ ",'aabcdaa,'c$,~=", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, false);

  e = MathExpr{ ",'aabcdaa,'a$,~=", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",'aabcdaa,'a$,!~=", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, false);

  e = MathExpr{ ",", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, QVariant());

  e = MathExpr{ ",'", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, "");

  e = MathExpr{ ",','", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, QVariant());

  e = MathExpr{ ",==", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, QVariant());

  e = MathExpr{ ",x,b,??,a,??,y,??", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, 2);

  e = MathExpr{ ",','x,??", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, "x");

  e = MathExpr{ ",','x,??*", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, "");

  e = MathExpr{ ",',?-", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, false);

  e = MathExpr{ ",',?*", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",nil,?-", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, false);

  e = MathExpr{ ",nil,?*", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, false);

  e = MathExpr{ ",nil,?*", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, false);

  e = MathExpr{ ",nil,?*", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, false);

  e = MathExpr{ ",a,b,<?", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, 1);

  e = MathExpr{ ",a,b,>?", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, 2);

  e = MathExpr{ ",ts,'2038-01,<?", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, ts);

  e = MathExpr{ ",ts,'2038-01,>?", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, "2038-01");

  e = MathExpr{ ",a,b,<", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, true);

  e = MathExpr{ ",a,b,<=>", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, -1);

  e = MathExpr{ ",a,'a,<=>", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, -1); // "1" < "a"

  e = MathExpr{ ",a,',<=>", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, 1); // "1" > ""

  e = MathExpr{ ",a,nil,<=>", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, QVariant());

  e = MathExpr{ ",'0,'1,-", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, -1);

  e = MathExpr{ ",'0,'1,-,~", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, 0);

  e = MathExpr{ ",'-4,small_ullong,+", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, small_ullong-4); // ok since the ULL can still be converted to LL

  e = MathExpr{ ",'-4,huge_ullong,+", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, QVariant()); // ULL cannot be a LL and -4 cannot be a ULL

  e = MathExpr{ ",'4,huge_ullong,+", MathExpr::CharacterSeparatedRpn };
  v = e.evaluate(&params);
  QCOMPARE(v, huge_ullong+4); // both can be a ULL

  QCOMPARE(ParamSet().evaluate("%{=rpn,'1,'2,==,'3,'4,?:}"), QVariant(4LL));
}

QTEST_MAIN(tst_MathExpr)
#include "tst_mathexpr.moc"
