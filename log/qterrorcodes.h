/* Copyright 2013-2025 Hallowyn, Gregoire Barbier and others.
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
#ifndef QTERRORCODES_H
#define QTERRORCODES_H

#include "libp6core_global.h"
#include <QNetworkReply>

namespace p6 {

/** Decoding QNetworkReply error codes. */
QString LIBP6CORESHARED_EXPORT network_error_as_text(
    QNetworkReply::NetworkError code);

} // ns p6

#endif // QTERRORCODES_H
