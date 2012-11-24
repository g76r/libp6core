#include "pfutils.h"
#include "pfinternals.h"

QString PfUtils::escape(const QString &string) {
  return pfescape(string);
}
