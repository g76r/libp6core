/* Copyright 2024 Gregoire Barbier and others.
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
#ifndef SVGWRITER_H
#define SVGWRITER_H

#include "util/utf8stringlist.h"
#include <QRect>

/** Write a SVG file with simplified vector drawing instructions.
 *  @see QSvgGenerator for more features (if you can afford QtGui and QtSvg) */
class LIBP6CORESHARED_EXPORT SvgWriter {
public:
  SvgWriter() {}
  void setViewport(const QRect &viewport) { _viewport = viewport; }
  inline void setViewport(int x, int y, int w, int h) {
    setViewport({x,y,w,h});
  }
  void drawLine(int x1, int y1, int x2, int y2, const Utf8String &brush_color = {},
                int pen_width = 1);
  void drawPath(
      int x, int y, const Utf8String &path, const Utf8String &brush_color = {},
      const Utf8String &fill_color = {});
  void drawSmallIcon(
      int x, int y, const Utf8String &name, const Utf8String &brush_color = {},
      const Utf8String &fill_color = {});
  void drawText(
      const QRect &box, int flags, const Utf8String &text,
      const Utf8String &brush_color,
      const Utf8String &font_name = {}, int font_size = 12);
  inline void drawText(
      int x, int y, int w, int h, int flags, const Utf8String &text,
      const Utf8String &brush_color,
      const Utf8String &font_name = {}, int font_size = 12) {
    drawText({x, y, w, h}, flags, text, brush_color, font_name, font_size);
  }
  /** start <a> element, must be followed by matching endAnchor()
   *  @param title set xlink:title attribute (used as tooltip by web browsers)
   */
  void startAnchor(const Utf8String &title);
  void endAnchor();
  void comment(const Utf8String &text);
  static inline Utf8StringList iconNames() { return _iconNames; }
  bool write(const Utf8String &filename) const;
  Utf8String data() const;

private:
  Utf8String _svg;
  QRect _viewport;
  static QMap<Utf8String,Utf8String> _icons;
  static Utf8StringList _iconNames;
};

#endif // SVGWRITER_H
