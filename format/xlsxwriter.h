/* Copyright 2023-2024 Gregoire Barbier and others.
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
#ifndef XLSXWRITER_H
#define XLSXWRITER_H

#include <util/utf8string.h>

class QFile;

/** Write data sequencially in a Open Office XML (OOXML, ECMA-376-5th)
 *  spreadsheet format. Which makes it openable by LibreOffice Calc and
 *  Microsoft Excel.
 *
 *  Supports most standard QVariant types.
 *  Deduplicate text strings (so called shared strings).
 *  Keep the less data possible in memory, by writing in temp files as soon as
 *  possible. The only scalability limit is temp disk space and text strings
 *  dictionay memory footprint (which is lightweight if the same strings are
 *  heavily repeated).
 */
class XlsxWriter {
public:
  explicit XlsxWriter(const Utf8String &workdir, bool autoclean = true);
  ~XlsxWriter();
  bool appendRow(const QVariantList &row, const Utf8String &sheet_title = {});
  size_t rowCount(const Utf8String &sheet_title = {}) const;
  bool write(Utf8String filename);
  bool failed() const { return !_success; }

private:
  class Sheet;
  QMap<Utf8String,Sheet*> _sheets;
  QHash<Utf8String,size_t> _strings;
  QFile *_strings_file = nullptr;
  size_t _strings_ref = 0;
  bool _success = true;
  Utf8String _workdir;
  bool _autoclean = true;
  bool _bool_as_text = false;

  /** empty title becomes "Sheet1", longer than 31 char titles are truncated. */
  inline static Utf8String normalized_sheet_name(const Utf8String &sheet_title);
  inline Sheet *get_or_create_sheet(const Utf8String &sheet_title);
  inline size_t share_string(const Utf8String &string, bool incr_counter);
};

#endif // XLSXWRITER_H
