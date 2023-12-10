/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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

#include "pfoptions.h"
#include <QRegularExpression>

static const QRegularExpression _dupcolons { "::+" },
_illegalsAndUseless { "\\A:+|[^a-zA-Z0-9:]*|:+\\z" },
_nulls { "\\Anull\\z|null:|:null" };

Utf8String PfOptions::normalizeSurface(const Utf8String &surface) {
  QString s = surface;
  s.remove(_illegalsAndUseless)
      .replace(_dupcolons, QStringLiteral(":"))
      .remove(_nulls);
  return s;
}
