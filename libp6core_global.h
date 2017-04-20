/* Copyright 2012-2017 Hallowyn, Gregoire Barbier and others.
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

#include <QtCore/qglobal.h>

#if defined(LIBPUMPKIN_LIBRARY)
#  define LIBPUMPKINSHARED_EXPORT Q_DECL_EXPORT
#else
#  define LIBPUMPKINSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // LIBP6CORE_GLOBAL_H
