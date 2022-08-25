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

static bool convertStringToBestNumericTypeIfPossible(
  QVariant *a, int *ta, int *tta) {
  if (*ta == QMetaType::QByteArray) {
    a->setValue(QString::fromUtf8(a->toByteArray()));
    *ta = QMetaType::QString;
  }
  if (*ta == QMetaType::QString) {
    auto s = a->toString();
    bool ok;
    auto ll = s.toLongLong(&ok, 0);
    if (ok) {
      a->setValue(ll);
      *tta = QMetaType::LongLong;
      return true;
    }
    auto ull = s.toLongLong(&ok, 0);
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
  }
  return false;
}

bool MathUtils::promoteToBestNumericType(QVariant *a) {
  int ta = a->metaType().id();
  int tta = numericsPromotion(ta);
  convertStringToBestNumericTypeIfPossible(a, &ta, &tta);
  switch (tta) {
    case QMetaType::ULongLong:
    case QMetaType::LongLong:
    case QMetaType::Double:
      a->convert(QMetaType(tta));
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
  convertStringToBestNumericTypeIfPossible(a, &ta, &tta);
  convertStringToBestNumericTypeIfPossible(b, &tb, &ttb);
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
      b->setValue((qlonglong) a->toULongLong());
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

QPartialOrdering MathUtils::compareQVariantAsNumber(QVariant a, QVariant b) {
  if (!promoteToBestNumericType(&a, &b))
    return QPartialOrdering::Unordered;
  return QVariant::compare(a, b);
}

QVariant MathUtils::addQVariantAsNumber(QVariant a, QVariant b) {
  if (!promoteToBestNumericType(&a, &b))
    return QVariant();
  //qDebug() << "addQVariantAsNumber " << a.metaType().id();
  switch (a.metaType().id()) {
    case QMetaType::Double:
      return QVariant(a.toDouble() + b.toDouble());
    case QMetaType::LongLong:
      return QVariant(a.toLongLong() + b.toLongLong());
    case QMetaType::ULongLong:
      return QVariant(a.toULongLong() + b.toULongLong());
  };
  return QVariant();
}

QVariant MathUtils::subQVariantAsNumber(QVariant a, QVariant b) {
  if (!promoteToBestNumericType(&a, &b))
    return QVariant();
  switch (a.metaType().id()) {
    case QMetaType::Double:
      return QVariant(a.toDouble() - b.toDouble());
    case QMetaType::LongLong:
      return QVariant(a.toLongLong() - b.toLongLong());
    case QMetaType::ULongLong:
      return QVariant(a.toULongLong() - b.toULongLong());
  };
  return QVariant();
}

QVariant MathUtils::mulQVariantAsNumber(QVariant a, QVariant b) {
  if (!promoteToBestNumericType(&a, &b))
    return QVariant();
  switch (a.metaType().id()) {
    case QMetaType::Double:
      return QVariant(a.toDouble() * b.toDouble());
    case QMetaType::LongLong:
      return QVariant(a.toLongLong() * b.toLongLong());
    case QMetaType::ULongLong:
      return QVariant(a.toULongLong() * b.toULongLong());
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
      return QVariant((long long)a.toDouble() % (long long)b.toDouble());
    case QMetaType::LongLong:
      return QVariant(a.toLongLong() % b.toLongLong());
    case QMetaType::ULongLong:
      return QVariant(a.toULongLong() % b.toULongLong());
  };
  return QVariant();
}
