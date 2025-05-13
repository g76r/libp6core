#ifndef TYPEDVALUELIST_H
#define TYPEDVALUELIST_H

#include "typedvalue.h"

namespace p6 {

namespace log {
class LogHelper;
}

struct TypedValueList : std::list<TypedValue> {
  TypedValueList() {}
  // TypedValueList(const TypedValueList &other) : std::list<TypedValue>(other) {}
  // TypedValueList(TypedValueList &&other) : std::list<TypedValue>(std::move(other)) {}
  TypedValueList(const std::list<TypedValue> &other) : std::list<TypedValue>(other) {}
  TypedValueList(std::list<TypedValue> &&other) : std::list<TypedValue>(std::move(other)) {}
  TypedValueList &operator=(const std::list<TypedValue> &other) {
    std::list<TypedValue>::operator =(other); return *this; }
  TypedValueList &operator=(std::list<TypedValue> &&other) {
    std::list<TypedValue>::operator =(other); return *this; }
  [[nodiscard]] Utf8String as_etv(const Utf8String &separator = " "_u8) const;
};

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const TypedValueList &o);

log::LogHelper LIBP6CORESHARED_EXPORT operator<<( log::LogHelper lh, const TypedValueList &o);

}

#endif // TYPEDVALUELIST_H
