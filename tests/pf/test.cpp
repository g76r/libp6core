#include "pf/pfnode.h"

int main(void) {
  PfNode n { "root", "abéc€d🥨e", {
                       { "child1", "a" },
                       { "child2", "b" },
                       { "child1", "c" } } };
  qDebug() << n.toPf() << n["child1"] << n.contentAsUtf16() << n.contentAsUtf8();
  return 0;
}
