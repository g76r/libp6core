#include "util/containerutils.h"
#include <QtDebug>
#include <QList>

bool tree(int a, int b) {
  switch (a) {
    case 2006:
    case 2008:
      return b == 2004;
    case 2004:
      return b == 2003;
    case 2005:
    case 2003:
    case 2007:
      return b == 2002;
    case 2002:
    case 2001:
      return b == 2000;
    case 2000:
      ;
  }
  return false;
}

bool dag(int a, int b) {
  return tree(a, b)
    || (a == 2002 && b == 2001)
    ;
}

bool cycle(int a, int b) {
  return tree(a, b)
    || (a == 2003 && b == 2004)
    ;
}

int main(void) {
  // test 1
  QList<int> list = {2008,2006,2003,2001,2007,2002,2005,2000,2004};
  p6::stable_topological_sort(list.begin(), list.end(), dag);
  qDebug() << list;
  if (list != QList<int>{2000,2001,2002,2003,2004,2008,2006,2007,2005})
    return 1;

  // test 1 assuming injective: same result but one more iteration
  list = {2008,2006,2003,2001,2007,2002,2005,2000,2004};
  p6::stable_topological_sort<false,true>(list.begin(), list.end(), dag);
  qDebug() << list;
  if (list != QList<int>{2000,2001,2002,2003,2004,2008,2006,2007,2005})
    return 1;

  // test 1 with tree topology (2002 doesn't depends on 2001)
  list = {2008,2006,2003,2001,2007,2002,2005,2000,2004};
  p6::stable_topological_sort(list.begin(), list.end(), tree);
  qDebug() << list;
  if (list != QList<int>{2000,2002,2003,2004,2008,2006,2001,2007,2005})
    return 1;

  // test 2
  list = {2008,2006,2000,2007,2004,2005,2002,2003,2001};
  p6::stable_topological_sort(list.begin(), list.end(), dag);
  qDebug() << list;
  if (list != QList<int>{2000,2001,2002,2003,2004,2008,2006,2007,2005})
    return 1;

  // test 3
  list = {2007,2002,2006,2000,2004,2001,2003,2008,2005};
  p6::stable_topological_sort(list.begin(), list.end(), dag);
  qDebug() << list;
  if (list != QList<int>{2000,2001,2002,2007,2003,2004,2006,2008,2005})
    return 1;

  // test 4
  list = {2008,2004,2007,2006,2000,2005,2002,2003,2001};
  p6::stable_topological_sort(list.begin(), list.end(), dag);
  qDebug() << list;
  if (list != QList<int>{2000,2001,2002,2003,2004,2008,2007,2006,2005})
    return 1;

  // test 4 with cyclic dependencies
  list = {2008,2004,2007,2006,2000,2005,2002,2003,2001};
  bool cycle_detected = false;
  p6::stable_topological_sort(list.begin(), list.end(), cycle, &cycle_detected);
  // would crash with infinie recursive with <true>(list.begin(), list.end(), cycle);
  qDebug() << list;
  if (list != QList<int>{2004,2008,2000,2002,2007,2006,2005,2003,2001})
    return 1;
  if (!cycle_detected)
    return 1;

  qDebug() << "ok";
  return 0;
}
