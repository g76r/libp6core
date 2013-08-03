/* Copyright 2013 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
 * Libqtssu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libqtssu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef USERSDATABASE_SPI_H
#define USERSDATABASE_SPI_H

#include "usersdatabase.h"

class LIBQTSSUSHARED_EXPORT UserDataData : public QSharedData {
public:
  virtual ~UserDataData();
  virtual QString userId() const = 0;
  virtual QString userName() const = 0;
  virtual bool hasRole(QString role) const;
  virtual QSet<QString> roles() const = 0;
  virtual QString mainGroupId() const = 0;
  virtual QSet<QString> allGroupIds() const = 0;
};


#endif // USERSDATABASE_SPI_H
