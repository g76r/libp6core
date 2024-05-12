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
#include "mailaddress.h"
#include <QRegularExpression>

// LATER strenghten this regexp (no double . etc.)
static const QRegularExpression _emailAddressRE
    { "\\A\\s*(?<address>(?<local>[a-zA-Z0-9!#$%&'*+/=?^_`.{|}~-]+)@"
     "(?<host>[a-zA-Z0-9_.-]+|\\[[0-9a-fA-F:]+\\]))\\s*\\z" };

static int staticInit() {
  qRegisterMetaType<MailAddress>();
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

class MailAddressData : public QSharedData {
public:
  QString _addr;
  MailAddressData(QString addr = QString()) : _addr(addr) { }
};

MailAddress::MailAddress(QString address) : d(0) {
  auto m = _emailAddressRE.match(address);
  if (!m.hasMatch())
    return;
  d = new MailAddressData(m.captured("address"));
}

MailAddress::MailAddress(const MailAddress &rhs) : d(rhs.d) {
}

MailAddress &MailAddress::operator=(const MailAddress &rhs) {
  if (this != &rhs)
    d.operator=(rhs.d);
  return *this;
}

MailAddress::~MailAddress() {
}

QString MailAddress::address() const {
  return d ? d->_addr : QString();
}
