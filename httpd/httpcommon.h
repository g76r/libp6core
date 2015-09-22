/* Copyright 2013 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
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
#ifndef HTTPCOMMON_H
#define HTTPCOMMON_H

// TODO create unit test for these regexp since they may be buggy
#define RFC2616_TOKEN_OCTET_RE "[ -'*-+\\-.0-9A-Z^-z\\|\\~]"
#define RFC6265_COOKIE_OCTET_RE "[!#-+\\--\\:\\<-\\[\\]-\\~]"
#define RFC6265_PATH_VALUE_RE "[ -\\:\\<-\\|\\}\\~]+"
// following is rather relaxed domain name RE
#define INTERNET_DOMAIN_RE "[0-9a-zA-Z.\\-]+"

#endif // HTTPCOMMON_H
