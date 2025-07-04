/* Copyright 2012-2025 Hallowyn, Gregoire Barbier and others.
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
#ifndef LIBP6CORE_GLOBAL_H
#define LIBP6CORE_GLOBAL_H

#include <QtGlobal>

#if defined(LIBP6CORE_LIBRARY)
#  define LIBP6CORESHARED_EXPORT Q_DECL_EXPORT
#else
#  define LIBP6CORESHARED_EXPORT Q_DECL_IMPORT
#endif

#if __cplusplus <= 201703L
#error "won't compile with C++ < 17, if you use MSVC please add /Zc:__cplusplus"
#endif

#if __cpp_static_call_operator >= 202207L && __cplusplus >= 202302
#define STATIC_LAMBDA static
#define CONST_IF_NOT_STATIC_FUNCTION_CALL_OPERATOR
#else
#define STATIC_LAMBDA
#define CONST_IF_NOT_STATIC_FUNCTION_CALL_OPERATOR const
#endif

namespace p6 {

#ifdef __cpp_concepts

template<typename T>
concept arithmetic = std::integral<T> || std::floating_point<T>;
template<typename T>
concept enumeration = std::is_enum_v<T>;
template<typename T>
concept integral_or_enum = std::integral<T> || std::is_enum_v<T>;

#endif

}

class QDebug;

QDebug operator<<(QDebug dbg, std::partial_ordering po);

#endif // LIBP6CORE_GLOBAL_H
