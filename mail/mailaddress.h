/* Copyright 2022-2024 Gregoire Barbier and others.
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
#ifndef MAILADDRESS_H
#define MAILADDRESS_H

#include "libp6core_global.h"
#include <QSharedDataPointer>
#include <QMetaType>

class MailAddressData;

class LIBP6CORESHARED_EXPORT MailAddress {
  QSharedDataPointer<MailAddressData> d;

public:
  MailAddress(QString address = QString());
  MailAddress(const MailAddress &other);
  MailAddress &operator=(const MailAddress &other);
  ~MailAddress();
  [[nodiscard]] inline bool isNull() const { return !d; }
  [[nodiscard]] inline bool operator!() const { return isNull(); }
  QString address() const;
  operator QString() const { return address(); }
};

#endif // MAILADDRESS_H

Q_DECLARE_METATYPE(MailAddress)
Q_DECLARE_TYPEINFO(MailAddress, Q_RELOCATABLE_TYPE);
