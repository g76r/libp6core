#include "typedvaluelist.h"
#include "log/log.h"

namespace p6 {

Utf8String TypedValueList::as_etv(const Utf8String &separator) const {
  Utf8String etv;
  bool first = true;
  for (auto e: *this) {
    if (first)
      first = false;
    else
      etv += separator;
    etv += e.as_etv();
  }
  return etv;
}

QDebug operator<<(QDebug dbg, const TypedValueList &o) {
  return dbg.noquote() << o.as_etv();
}

log::LogHelper operator<<( log::LogHelper lh, const TypedValueList &o) {
  return lh << o.as_etv();
}

}
