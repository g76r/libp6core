/* Copyright 2022 Gregoire Barbier and others.
 * This file is part of libpumpkin, see <http://libpumpkin.g76r.eu/>.
 * Libpumpkin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libpumpkin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libpumpkin.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "mathutils.h"
#include <QHash>
#include <QDateTime>
#include <cmath>

/** following constants are not compliant to C++ standard since they assume that
 * integers are implemented with 2's complement
 * however this is compliant with real world
 * see proposal P0907R4
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0907r4.html
 */
#define INT_MAX_AS_LL ((1LL<<(8*sizeof(int)-1))-1)
#define INT_MIN_AS_LL (-(1LL<<(8*sizeof(int)-1)))
#define INT_MAX_AS_ULL ((1ULL<<(8*sizeof(int)-1))-1)
#define UINT_MAX_AS_LL ((1LL<<(8*sizeof(int)))-1)
#define UINT_MAX_AS_ULL ((1ULL<<(8*sizeof(int)))-1)

static int numericsPromotion(int typeId) {
  switch(typeId) {
    case QMetaType::Bool:
    case QMetaType::Int:
    case QMetaType::UInt:
    //case QMetaType::QChar: // leave it alone because toDouble() does not works with QChar
    case QMetaType::Long:
    case QMetaType::LongLong:
    case QMetaType::Short:
    case QMetaType::Char:
    case QMetaType::Char16:
    case QMetaType::Char32:
    case QMetaType::ULong:
    case QMetaType::UShort:
    case QMetaType::SChar:
    case QMetaType::UChar:
      return QMetaType::LongLong;
    case QMetaType::Double:
    case QMetaType::Float:
      return QMetaType::Double;
    case QMetaType::ULongLong:
      return QMetaType::ULongLong;
  }
  return QMetaType::UnknownType;
}

static bool convertOtherTypesToBestNumericTypeIfPossible(
  QVariant *a, int *ta, int *tta) {
  if (*ta == QMetaType::QByteArray) {
    a->setValue(QString::fromUtf8(a->toByteArray()));
    *ta = QMetaType::QString;
  }
  switch(*ta) {
    case QMetaType::QString: {
      auto s = a->toString().trimmed();
      bool ok;
      // LATER support kmb and kMGTP suffixes
      auto ll = s.toLongLong(&ok, 0);
      if (ok) {
        a->setValue(ll);
        *tta = QMetaType::LongLong;
        return true;
      }
      auto ull = s.toULongLong(&ok, 0);
      if (ok) {
        a->setValue(ull);
        *tta = QMetaType::ULongLong;
        return true;
      }
      auto d = s.toDouble(&ok);
      if (ok) {
        a->setValue(d);
        *tta = QMetaType::Double;
        return true;
      }
      if (s.compare("true", Qt::CaseInsensitive) == 0) {
        a->setValue(1LL);
        *tta = QMetaType::LongLong;
        return true;
      }
      if (s.compare("false", Qt::CaseInsensitive) == 0) {
        a->setValue(0LL);
        *tta = QMetaType::LongLong;
        return true;
      }
    }
    break;
    case QMetaType::QDate:
    case QMetaType::QDateTime: {
      auto dt = a->toDateTime();
      if (dt.isValid()) {
        a->setValue(dt.toMSecsSinceEpoch());
        *tta = QMetaType::LongLong;
      }
    }
    break;
    case QMetaType::QTime:  {
      auto t = a->toTime();
      if (t.isValid()) {
        a->setValue(t.msecsSinceStartOfDay());
        *tta = QMetaType::LongLong;
      }
    }
    break;
  }
  return false;
}

bool MathUtils::promoteToBestNumericType(QVariant *a) {
  int ta = a->metaType().id();
  int tta = numericsPromotion(ta);
  convertOtherTypesToBestNumericTypeIfPossible(a, &ta, &tta);
  switch (tta) {
    case QMetaType::ULongLong:
    case QMetaType::LongLong:
    case QMetaType::Double:
      a->convert(QMetaType(tta));
      return true;
  }
  return false;
}

bool MathUtils::convertToString(QVariant *a) {
  if (!a)
    return false;
  switch(a->metaType().id()) {
    case QMetaType::Bool:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::QChar:
    case QMetaType::Long:
    case QMetaType::LongLong:
    case QMetaType::Short:
    case QMetaType::Char:
    case QMetaType::Char16:
    case QMetaType::Char32:
    case QMetaType::ULong:
    case QMetaType::UShort:
    case QMetaType::SChar:
    case QMetaType::UChar:
    case QMetaType::Double:
    case QMetaType::Float:
    case QMetaType::ULongLong:
    case QMetaType::QDateTime:
    case QMetaType::QDate:
    case QMetaType::QTime:
      *a = a->toString();
      return true;
    case QMetaType::QString:
      return true;
    case QMetaType::QStringList:
      *a = a->toStringList().join(" ");
      return true;
  }
  return false;
}

bool MathUtils::promoteToBestNumericType(QVariant *a, QVariant *b) {
  if (!a || !b)
    return false;
  int ta = a->metaType().id();
  int tb = b->metaType().id();
  int tta = numericsPromotion(ta);
  int ttb = numericsPromotion(tb);
  convertOtherTypesToBestNumericTypeIfPossible(a, &ta, &tta);
  convertOtherTypesToBestNumericTypeIfPossible(b, &tb, &ttb);
  //qDebug() << "promoteToBestNumericType " << ta << " " << tb << " " << tta
  //         << " " << ttb;
  if (tta == QMetaType::UnknownType || ttb == QMetaType::UnknownType)
    return false;
  if (tta == QMetaType::Double || ttb == QMetaType::Double) {
    a->convert(QMetaType(QMetaType::Double));
    b->convert(QMetaType(QMetaType::Double));
    return true;
  }
  if (tta == QMetaType::ULongLong && ttb == QMetaType::ULongLong)
    return true; // both are already ULL
  if (tta == QMetaType::ULongLong && ttb == QMetaType::LongLong) {
    if (a->toULongLong() <= (ULLONG_MAX >> 1)) {
      // can convert ULL to LL because it's less than LL positive max
      a->setValue((qlonglong) a->toULongLong());
      b->convert(QMetaType(QMetaType::LongLong));
      return true;
    } else if (b->toLongLong() >= 0) {
      // can convert LL to ULL because it's > 0
      b->setValue((qulonglong) b->toLongLong());
      return true;
    }
    // cannot convert LL and ULL to any common integer types
    return false;
  }
  if (tta == QMetaType::LongLong && ttb == QMetaType::ULongLong) {
    // same, swaping a and b
    if (b->toULongLong() <= (ULLONG_MAX >> 1)) {
      a->convert(QMetaType(QMetaType::LongLong));
      b->setValue((qlonglong) b->toULongLong());
      return true;
    } else if (a->toLongLong() >= 0) {
      a->setValue((qulonglong) a->toLongLong());
      return true;
    }
    return false;
  }
  // only one remaining case : LL on both sides
  if (tta == QMetaType::LongLong && ttb == QMetaType::LongLong) {
    a->convert(QMetaType(QMetaType::LongLong));
    b->convert(QMetaType(QMetaType::LongLong));
    return true;
  }
  // this cannot happen
  return false;
}

QPartialOrdering MathUtils::compareQVariantAsNumber(
  QVariant a, QVariant b, bool anyStringRepresentation) {
  auto a0 = a, b0 = b;
  //qDebug() << "compareQVariantAsNumber" << a << b;
  if (promoteToBestNumericType(&a, &b)) {
    //qDebug() << "  promoted" << a << b;
    return QVariant::compare(a, b);
  }
  if (anyStringRepresentation) {
    convertToString(&a0);
    convertToString(&b0);
    auto c = a0.toString().compare(b0.toString());
    //qDebug() << "  stringified(any)" << a0 << b0 << c;
    return c > 0 ? QPartialOrdering::Greater
           : c < 0 ? QPartialOrdering::Less
                   : QPartialOrdering::Equivalent;
  }
  if (convertToString(&a0) && convertToString(&b0)) {
    //qDebug() << "  stringified" << a0 << b0;
    return QVariant::compare(a0, b0);
  }
  //qDebug() << "  unordered";
  return QPartialOrdering::Unordered;
}

QVariant MathUtils::addQVariantAsNumber(QVariant a, QVariant b) {
  if (!promoteToBestNumericType(&a, &b))
    return QVariant();
  //qDebug() << "addQVariantAsNumber " << a.metaType().id();
#if __has_builtin(__builtin_add_overflow)
  qlonglong i;
  qulonglong u;
  bool o = false;
#else
#warning MathUtils::addQVariantAsNumber() without overflow handling
#endif
  switch (a.metaType().id()) {
    case QMetaType::Double:
      return QVariant(a.toDouble() + b.toDouble());
    case QMetaType::LongLong:
#if __has_builtin(__builtin_add_overflow)
      o = __builtin_saddll_overflow(a.toLongLong(), b.toLongLong(), &i);
      return o ? QVariant() : i;
#else
      return QVariant(a.toLongLong() + b.toLongLong());
#endif
    case QMetaType::ULongLong:
#if __has_builtin(__builtin_add_overflow)
      o = __builtin_uaddll_overflow(a.toULongLong(), b.toULongLong(), &u);
      return o ? QVariant() : u;
#else
      return QVariant(a.toULongLong() + b.toULongLong());
#endif
  };
  return QVariant();
}

QVariant MathUtils::subQVariantAsNumber(QVariant a, QVariant b) {
  if (!promoteToBestNumericType(&a, &b))
    return QVariant();
#if __has_builtin(__builtin_sub_overflow)
  qlonglong i;
  qulonglong u;
  bool o = false;
#else
#warning MathUtils::subQVariantAsNumber() without overflow handling
#endif
  switch (a.metaType().id()) {
    case QMetaType::Double:
      return QVariant(a.toDouble() - b.toDouble());
    case QMetaType::LongLong:
#if __has_builtin(__builtin_sub_overflow)
      o = __builtin_ssubll_overflow(a.toLongLong(), b.toLongLong(), &i);
      return o ? QVariant() : i;
#else
      return QVariant(a.toLongLong() - b.toLongLong());
#endif
    case QMetaType::ULongLong:
#if __has_builtin(__builtin_sub_overflow)
      o = __builtin_usubll_overflow(a.toULongLong(), b.toULongLong(), &u);
      return o ? QVariant() : u;
#else
      return QVariant(a.toULongLong() - b.toULongLong());
#endif
  };
  return QVariant();
}

QVariant MathUtils::mulQVariantAsNumber(QVariant a, QVariant b) {
  if (!promoteToBestNumericType(&a, &b))
    return QVariant();
#if __has_builtin(__builtin_mul_overflow)
  qlonglong i;
  qulonglong u;
  bool o = false;
#else
#warning MathUtils::mulQVariantAsNumber() without overflow handling
#endif
  switch (a.metaType().id()) {
    case QMetaType::Double:
      return QVariant(a.toDouble() * b.toDouble());
    case QMetaType::LongLong:
#if __has_builtin(__builtin_mul_overflow)
      o = __builtin_smulll_overflow(a.toLongLong(), b.toLongLong(), &i);
      return o ? QVariant() : i;
#else
      return QVariant(a.toLongLong() * b.toLongLong());
#endif
    case QMetaType::ULongLong:
#if __has_builtin(__builtin_mul_overflow)
      o = __builtin_umulll_overflow(a.toULongLong(), b.toULongLong(), &u);
      return o ? QVariant() : u;
#else
      return QVariant(a.toULongLong() * b.toULongLong());
#endif
  };
  return QVariant();
}

QVariant MathUtils::divQVariantAsNumber(QVariant a, QVariant b) {
  if (!promoteToBestNumericType(&a, &b))
    return QVariant();
  switch (a.metaType().id()) {
    case QMetaType::Double:
      return QVariant(a.toDouble() / b.toDouble());
    case QMetaType::LongLong:
      return QVariant(a.toLongLong() / b.toLongLong());
    case QMetaType::ULongLong:
      return QVariant(a.toULongLong() / b.toULongLong());
  };
  return QVariant();
}

QVariant MathUtils::modQVariantAsNumber(QVariant a, QVariant b) {
  if (!promoteToBestNumericType(&a, &b))
    return QVariant();
  switch (a.metaType().id()) {
    case QMetaType::Double:
      return QVariant(std::fmod(a.toDouble(), b.toDouble()));
    case QMetaType::LongLong:
      return QVariant(a.toLongLong() % b.toLongLong());
    case QMetaType::ULongLong:
      return QVariant(a.toULongLong() % b.toULongLong());
  };
  return QVariant();
}
