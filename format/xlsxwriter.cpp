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
#include "xlsxwriter.h"
#include "log/log.h"
#include <QFile>
#include <QDir>
#include <QProcess>

class XlsxWriter::Sheet {
public:
  Utf8String _title;
  size_t _index;
  QFile *_file;
  size_t _rowcount = 0;
  bool _success = true;
  Sheet(const Utf8String &title, size_t index, const Utf8String &workdir)
    : _title(title), _index(index) {
    _file = new QFile(workdir+"/"+"sheet"+Utf8String::number(_index)+".xml");
    if (!_file->open(QIODevice::Append|QIODevice::WriteOnly
                     |QIODevice::Truncate)
        || _file->write(
          "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
          "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/"
          "2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocum"
          "ent/2006/relationships\">\n"
          "<sheetData>\n"
          ) < 0) {
      Log::error() << "cannot create file: " << _file->fileName() << " : "
                   << _file->errorString();
      _success = false;
    }
  }
  ~Sheet() {
    if (_file)
      delete _file;
  }
};

inline Utf8String html_protect(
    const Utf8String &s, bool *has_spaces = nullptr) {
  Utf8String q;
  size_t n = s.size();
  for (size_t i = 0; i < n; ++i)
    switch (s[i]) {
      case '<':
        q += "&lt;"_u8;
        break;
//      case '>':
//        q += "&gt;"_u8;
//        break;
      case '&':
        q += "&amp;"_u8;
        break;
      case '"':
        q += "&#34;"_u8;
        break;
//      case '\'':
//        q += "&#39;"_u8;
//        break;
      case ' ':
      case '\r':
      case '\n':
      case '\t':
        if (has_spaces)
          *has_spaces = true;
        [[fallthrough]];
      default:
        q += s[i];
    }
  return q;
}

inline Utf8String cell_ref(size_t rownum, size_t colnum) {
  return Utf8String::bijectiveBaseNumber(
        colnum, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"_ba) + Utf8String::number(rownum);
}

static inline double to_excel_datetime(QVariant v) {
  // excel pretends to use 1900-1-1 as an epoch, but...
  // for excel 1900-1-1 is day 1 (not 0)
  // for excel 1900-2-29 exists
  // therefore the virtual gregorian epoch is 2 days before 1900-1-1
  // (a.k.a. 1970-1-1 is day 25569)
  static const QDate _excel_epoch(1899, 12, 30);
  auto days_since_1899 = _excel_epoch.daysTo(v.toDate());
  if (days_since_1899 < 61)
    days_since_1899 = 0; // everything before 1900-3-1 is inconsistent
  auto days_since_midnight = v.toTime().msecsSinceStartOfDay()/86'400'000.0;
  return days_since_1899+days_since_midnight;
}

XlsxWriter::XlsxWriter(const Utf8String &workdir, bool autoclean)
  : _workdir(workdir), _autoclean(autoclean) {
  if (!QDir().mkpath(workdir)) {
    Log::error() << "cannot create directory: " << workdir;
    _success = false;
    return;
  }
  _strings_file = new QFile(_workdir+"/strings.xml");
  if (!_strings_file->open(QIODevice::Append|QIODevice::WriteOnly
                           |QIODevice::Truncate)) {
    Log::error() << "cannot create file: " << _strings_file->fileName()
                 << " : " << _strings_file->errorString();
    _success = false;
    return;
  }
  if (_strings_file->write(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<sst xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\""
        " count=                                            "
        " uniqueCount=                                                >\n"
        ) < 0) {
    Log::error() << "cannot write header to file: " << _strings_file->fileName()
                 << " : " << _strings_file->errorString();
    _success = false;
    return;
  }
}

Utf8String XlsxWriter::normalized_sheet_name(const Utf8String &title) {
  if (title.isEmpty())
    return "Sheet1"_u8;
  if (title.utf8size() >= 32)
    return title.utf8left(31);
  return title;
}

XlsxWriter::Sheet *XlsxWriter::get_or_create_sheet(
    const Utf8String &original_title) {
  auto title = normalized_sheet_name(original_title);
  if (_sheets.contains(title))
    return _sheets[title];
  Sheet *sheet = new Sheet(title, _sheets.size()+1, _workdir);
  _sheets.insert(title, sheet);
  if (!sheet->_success)
    _success = false;
  return sheet;
}

size_t XlsxWriter::share_string(
    const Utf8String &original_string, bool incr_counter) {
  auto string = original_string.null_coalesced();
  auto i = _strings.value(string, SIZE_MAX);
  if (i == SIZE_MAX) {
    i = _strings.size();
    _strings.insert(string, i);
    bool has_spaces = false;
    auto s = html_protect(string, &has_spaces);
    if (_strings_file->write((has_spaces ? "<si><t xml:space=\"preserve\">"
                              : "<si><t>")+s+"</t></si>\n") < 0) {
      Log::error() << "cannot write to file: " << _strings_file->fileName()
                   << " : " << _strings_file->errorString();
      _success = false;
    }
  }
  if (incr_counter)
    ++_strings_ref;
  return i;
}

bool XlsxWriter::appendRow(
    const QVariantList &row, const Utf8String &sheet_title) {
  if (!_success)
    return false;
  Sheet *sheet = this->get_or_create_sheet(sheet_title);
  auto rownum = sheet->_rowcount+1;
  Utf8String bytes = "  <row r=\""+Utf8String::number(rownum)
                     +"\" spans=\"1:"+Utf8String::number(row.size())
                     +"\">\n";
  size_t colnum = 1;
  for (auto v: row) {
    auto id = v.metaType().id();
    bytes += "<c r=\""+cell_ref(rownum, colnum)+"\"";
    if (id == QMetaType::Double || id == QMetaType::Float
        || id == QMetaType::Float16)
      bytes += " s=\"4\"><v>"+Utf8String::number(v.toDouble(), 'g', 16)
               +"</v></c>\n";
    else if (id == QMetaType::QDateTime)
      bytes += " s=\"1\"><v>"+Utf8String::number(to_excel_datetime(v), 'g', 16)
               +"</v></c>\n";
    else if (id == QMetaType::QDate)
      bytes += " s=\"2\"><v>"+Utf8String::number(to_excel_datetime(v), 'g', 16)
               +"</v></c>\n";
    else if (id == QMetaType::QTime)
      bytes += " s=\"3\"><v>"+Utf8String::number(to_excel_datetime(v), 'g', 16)
               +"</v></c>\n";
    else if (id == QMetaType::ULongLong
             || id == QMetaType::ULong || id == QMetaType::UInt
             || id == QMetaType::UShort || id == QMetaType::UChar)
      bytes += " s=\"5\"><v>"+Utf8String::number(v.toULongLong())
               +"</v></c>\n";
    else if (id == QMetaType::LongLong
             || id == QMetaType::Long || id == QMetaType::Int
             || id == QMetaType::Short || id == QMetaType::Char)
      bytes += " s=\"5\"><v>"+Utf8String::number(v.toLongLong())
               +"</v></c>\n";
    else if (id == QMetaType::Bool && !_bool_as_text)
      bytes += " t=\"b\" s=\"6\"><v>"+(v.toBool() ? "1"_u8 : "0"_u8)
               +"</v></c>\n";
    else { // will be handled as a string
      auto s = v.value<Utf8String>();
      if (s.isEmpty())
        bytes += " t=\"str\"><v/></c>\n"; // FIXME str or number ?
      else // LATER should we use str instead of s below some minimal size ?
        bytes += " t=\"s\"><v>"
                 +Utf8String::number(share_string(s, true))
                 +"</v></c>\n";
    }
    ++colnum;
  }
  bytes += "  </row>\n";
  if (sheet->_file->write(bytes) < 0) {
    Log::error() << "cannot write to file: " << sheet->_file->fileName()
                 << " : " << sheet->_file->errorString();
    return _success = false;
  }
  sheet->_rowcount++;
  return true;
}

size_t XlsxWriter::rowCount(const Utf8String &original_title) const {
  auto title = normalized_sheet_name(original_title);
  auto sheet = _sheets[title];
  return sheet ? sheet->_rowcount : 0;
}

bool XlsxWriter::write(Utf8String filename) {
  if (!_success)
    return false;
  // closing sheet files
  Utf8String sheets_in_book, sheets_in_rels, sheets_in_content_types;
  for (auto [title,_]: _sheets.asKeyValueRange()) {
    auto sheet = _sheets[title];
    auto id_as_utf8 = Utf8String::number(sheet->_index);
    sheets_in_book += "    <sheet name=\""+html_protect(title)+"\" sheetId=\""
                      +id_as_utf8+"\" r:id=\"rId"+id_as_utf8+"\"/>\n";
    sheets_in_rels += "  <Relationship Id=\"rId"+id_as_utf8
                      +"\" Type=\"http://schemas.openxmlformats.org/officeDocum"
                       "ent/2006/relationships/worksheet\" Target=\""
                       "sheet"+id_as_utf8+".xml\"/>\n";
    sheets_in_content_types += "  <Override PartName=\"/sheet"+id_as_utf8
                               +".xml\" ContentType=\"application/vnd.openxmlfo"
                                "rmats-officedocument.spreadsheetml.worksheet+x"
                                "ml\"/>\n";
    if (sheet->_file->write(
          "</sheetData>\n"
          "</worksheet>\n"
          ) < 0) {
      Log::error() << "cannot write footer to file: "
                   << sheet->_file->fileName() << " : "
                   << sheet->_file->errorString();
      return _success = false;
    }
    sheet->_file->close();
  }

  // closing shared strings file
  if (_strings_file->write("</sst>\n") < 0) {
    Log::error() << "cannot write footer to file: " << _strings_file->fileName()
                 << " : " << _strings_file->errorString();
    return _success = false;
  }
  _strings_file->close();

  // updating shared strings file header
  if (!_strings_file->open(QIODevice::ReadWrite|QIODevice::ExistingOnly)) {
    Log::error() << "cannot reopen file: " << _strings_file->fileName()
                 << " : " << _strings_file->errorString();
    return _success = false;
  }
  auto ba = _strings_file->read(16384);
  auto i = ba.indexOf("count="_ba);
  if (!_strings_file->seek(i+6)
      || _strings_file->write("\""+Utf8String::number(_strings_ref)+"\"") < 0) {
    Log::error() << "cannot update header of file: "
                 << _strings_file->fileName() << " : "
                 << _strings_file->errorString();
    return _success = false;
  }
  i = ba.indexOf("uniqueCount="_ba);
  if (!_strings_file->seek(i+12)
      || _strings_file->write("\""+Utf8String::number(_strings.size())
                              +"\"") < 0) {
    Log::error() << "cannot update header of file: "
                 << _strings_file->fileName() << " : "
                 << _strings_file->errorString();
    return _success = false;
  }
  _strings_file->close();

  // writing book file
  QFile file(_workdir+"/workbook.xml");
  if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Append)
      || file.write(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006"
        "/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/"
        "2006/relationships\">\n"
        "  <workbookPr/>\n"
        "  <sheets>\n"
        +sheets_in_book+
        "  </sheets>\n"
        "</workbook>\n"
        ) < 0) {
    Log::error() << "cannot write file: " << file.fileName() << " : "
                 << file.errorString();
    return _success = false;
  }
  file.close();

  // writing style sheet
  file.setFileName(_workdir+"/styles.xml");
  if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Append)
      || file.write(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/20"
        "06/main\" >\n"
        "<numFmts count=\"9\">\n"
        // predefined numFmtIds:
        // 0  "general"
        // 1  0
        // 2  0.00
        // 3  #,##0
        // 4  #,##0.00
        // 5  $#,##0;\-$#,##0
        // 6  $#,##0;[Red]\-$#,##0
        // 7  $#,##0.00;\-$#,##0.00
        // 8  $#,##0.00;[Red]\-$#,##0.00
        // 9  0%
        // 10 0.00%
        // 11 0.00E+00
        // 12 # ?/?
        // 13 # ??/??
        // 14 mm-dd-yy
        // 15 d-mmm-yy
        // 16 d-mmm
        // 17 mmm-yy
        // 18 h:mm AM/PM
        // 19 h:mm:ss AM/PM
        // 20 h:mm
        // 21 h:mm:ss
        // 22 m/d/yy h:mm
        // 27 [$-404]e/m/d
        // 30 m/d/yy
        // 36 [$-404]e/m/d
        // 37 #,##0 ;(#,##0)
        // 38 #,##0 ;[Red](#,##0)
        // 39 #,##0.00;(#,##0.00)
        // 40 #,##0.00;[Red](#,##0.00)
        // 44 _("$"* #,##0.00_);_("$"* \(#,##0.00\);_("$"* "-"??_);_(@_)
        // 45 mm:ss
        // 46 [h]:mm:ss
        // 47 mmss.0
        // 48 ##0.0E+0
        // 49 @
        // 50 [$-404]e/m/d
        // 57 [$-404]e/m/d
        // 59 t0
        // 60 t0.00
        // 61 t#,##0
        // 62 t#,##0.00
        // 67 t0%
        // 68 t0.00%
        // 69 t# ?/?
        // 70 t# ??/??
        // custom numFmtIds start at 164:
        "  <numFmt numFmtId=\"164\" formatCode=\"yyyy\\-mm\\-dd\\ hh:mm:ss\"/>\n"
        //"  <numFmt numFmtId=\"165\" formatCode=\"yyyy\\-mm\\-dd\\ hh:mm:ss,000\"/>\n"
        "  <numFmt numFmtId=\"166\" formatCode=\"yyyy\\-mm\\-dd\"/>\n"
        "  <numFmt numFmtId=\"167\" formatCode=\"hh:mm:ss\"/>\n"
        //"  <numFmt numFmtId=\"168\" formatCode=\"hh:mm:ss,000\"/>\n"
        "  <numFmt numFmtId=\"169\" formatCode=\"#,##0;\\-\\ #,##0\"/>\n"
        //"  <numFmt numFmtId=\"170\" formatCode=\"#,##0.00;\\-\\ #,##0.00\"/>\n"
        "  <numFmt numFmtId=\"171\" formatCode=\"#,##0.00;[Red]\\-\\ #,##0.00\"/>\n"
        "  <numFmt numFmtId=\"172\" formatCode=\"00000\"/>\n"
        "</numFmts>\n"
        "<fonts count=\"1\">\n"
        "  <font/>\n"
        "</fonts>\n"
        "<fills count=\"1\">\n"
        "  <fill/>\n"
        "</fills>\n"
        "<borders count=\"1\">\n"
        "  <border/>\n"
        "</borders>\n"
        "<cellStyleXfs count=\"1\">\n"
        "  <xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/>\n"
        "</cellStyleXfs>\n"
        "<cellXfs count=\"7\">\n"
        "  <xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/>\n"
        "  <xf numFmtId=\"164\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyNumberFormat=\"1\"/>\n"
        "  <xf numFmtId=\"166\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyNumberFormat=\"1\"/>\n"
        "  <xf numFmtId=\"167\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyNumberFormat=\"1\"/>\n"
        "  <xf numFmtId=\"171\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyNumberFormat=\"1\"/>\n"
        "  <xf numFmtId=\"169\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyNumberFormat=\"1\"/>\n"
        "  <xf numFmtId=\"172\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyNumberFormat=\"1\"/>\n"
        "</cellXfs>\n"
        "</styleSheet>\n"
        ) < 0) {
    Log::error() << "cannot write file: " << file.fileName() << " : "
                 << file.errorString();
    return _success = false;
  }
  file.close();

  // writing workbook relations file
  QDir().mkdir(_workdir+"/_rels");
  file.setFileName(_workdir+"/_rels/workbook.xml.rels");
  if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Append)
      || file.write(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
        +sheets_in_rels+
        "  <Relationship Id=\"rIdS\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/sharedStrings\" Target=\"strings.xml\"/>\n"
        "  <Relationship Id=\"rIdY\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>\n"
        "</Relationships>\n"
        ) < 0) {
    Log::error() << "cannot write file: " << file.fileName() << " : "
                 << file.errorString();
    return _success = false;
  }
  file.close();

  // writing .rels relations file
  file.setFileName(_workdir+"/_rels/.rels");
  if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Append)
      || file.write(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
        "  <Relationship Id=\"rIdB\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"workbook.xml\"/>\n"
        "</Relationships>\n"
        ) < 0) {
    Log::error() << "cannot write file: " << file.fileName() << " : "
                 << file.errorString();
    return _success = false;
  }
  file.close();

  // writing content types file
  file.setFileName(_workdir+"/[Content_Types].xml");
  if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Append)
      || file.write(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
        "  <Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
        "  <Default Extension=\"xml\" ContentType=\"application/xml\"/>\n"
        "  <Override PartName=\"/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>\n"
        +sheets_in_content_types+
        "  <Override PartName=\"/strings.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml\"/>\n"
        "  <Override PartName=\"/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>\n"
        "</Types>\n"
        ) < 0) {
    Log::error() << "cannot write file: " << file.fileName() << " : "
                 << file.errorString();
    return _success = false;
  }
  file.close();

  // zip
  QProcess zip;
  if (!filename.startsWith('/'))
    filename = Utf8String(QDir::currentPath())+"/"+filename;
  zip.setWorkingDirectory(_workdir);
  zip.start("zip", { "-rX6qm", filename, "." });
  while (zip.state() != QProcess::NotRunning)
    zip.waitForFinished();
  if (zip.exitStatus() != QProcess::NormalExit || zip.exitCode() != 0) {
    Log::error() << "cannot write spreadsheet archive: " << filename << " : "
                 << zip.exitCode();
    return _success = false;
  }

  // remove temp working dir
  if (_autoclean) {
    if (!QDir(_workdir).removeRecursively()) {
      Log::error() << "cannot remove temp working dir: " << _workdir;
      return _success = false;
    }
  }
  return true;
}

XlsxWriter::~XlsxWriter() {
  if (_strings_file)
    delete _strings_file;
}
