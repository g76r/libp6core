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
#include "svgwriter.h"
#include <QFile>
#include "log/log.h"

void SvgWriter::drawLine(
    int x1, int y1, int x2, int y2, const Utf8String &brush_color, int pen_width) {
  _svg += "<line x1=\""+Utf8String::number(x1)+"\" x2=\""+Utf8String::number(x2)
          +"\" y1=\""+Utf8String::number(y1)+"\" y2=\""+Utf8String::number(y2)
          +"\" stroke=\""+(brush_color|"#000")
          +"\" stroke-width=\""+Utf8String::number(pen_width)+"\"/>\n";
}

void SvgWriter::drawPath(
    int x, int y, const Utf8String &path, const Utf8String &brush_color,
    const Utf8String &fill_color) {
  _svg += "<path d=\"M "+Utf8String::number(x)+" "+Utf8String::number(y)+" "
          +path+"\"";
  if (!brush_color.isEmpty())
    _svg += " stroke=\""+brush_color+"\"";
  if (!fill_color.isEmpty())
    _svg += " fill=\""+fill_color+"\"";
  _svg += "/>\n";
}

void SvgWriter::drawSmallIcon(
    int x, int y, const Utf8String &name, const Utf8String &brush_color,
    const Utf8String &fill_color) {
  drawPath(x, y, _icons.value(name), brush_color, fill_color);
}

void SvgWriter::drawText(
    const QRect &box, int flags, const Utf8String &text, const Utf8String &brush_color,
    const Utf8String &font_name, int font_size) {
  _svg += "<text x=\""+Utf8String::number(box.x())+"\" y=\""
          +Utf8String::number(box.y()+box.height())
          +"\" fill=\""+brush_color+"\" ";
  if (!font_name.isEmpty())
    _svg += "style=\"font-family:"+font_name+";font-size="
            +Utf8String::number(font_size)+"\" ";
  if (flags & Qt::AlignLeft)
    _svg += "text-anchor\"start\" ";
  else if (flags & Qt::AlignHCenter)
    _svg += "text-anchor\"center\" ";
  else if (flags & Qt::AlignRight)
    _svg += "text-anchor\"end\" ";
  _svg += ">"+text+"</text>\n";
  // LATER enforce bounding box for real
}

Utf8String SvgWriter::data() const {
  return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
         "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\""
         " \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n"
         "<svg width=\""+Utf8String::number(_viewport.width())
      +"px\" height=\""+Utf8String::number(_viewport.height())
      +"px\" xmlns=\"http://www.w3.org/2000/svg\">\n"
       "<g>\n"
      +_svg
      +"</g>\n</svg>\n";
}

bool SvgWriter::write(const Utf8String &filename) const {
  auto svg = data();
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
    Log::error() << "can't open file " << filename << " for writing, error "
                 << file.error() << " : " << file.errorString();
    return false;
  }
  if (file.write(svg) != svg.size()) {
    Log::error() << "can't write to file " << filename << ", error "
                 << file.error() << " : " << file.errorString();
    return false;
  }
  return true;
}

QMap<Utf8String,Utf8String> SvgWriter::_icons {
  { "square", "m -4 -4 h 8 v 8 h -8 z" }, // ■
  // FIXME { "circle", "a 4 4 0 0" }, // ●
  // FIXME { "marriage", "m -1 0 a 3 3 m 2 0 a 3 3" }, // ⚭
  { "diamond", "m 0 -4 l 4 4 l -4 4 l -4 -4 z" }, // ♦
  //{ "arrowr", "v -4 l 4 4 l -4 4 z" }, // ►
  { "arrowu", "m 0 -4 l -4 6 h 8 z" }, // ▲
  { "arrowr", "m 4 0 l -6 -4 v 8 z" }, // ►
  { "hourglass", "m -4 -4 h 8 l -8 8 h 8 z" }, // ⧗
  { "bowtie",    "m -4 -4 l 8 8 v -8 l -8 8 z" }, // ⧓
  // FIXME { "droplet", "m 0 -1 a 3 3 h -3 l 3 -4 l 3 4 z" }, // 🌢
  { "times", "m -4 -4 l 8 8 m -8 0 l 8 -8" }, // ×
  { "equal", "m -3 -3 h 6 m 0 6 h -6" }, // =
  // FIXME { "pause", "m -3 -3 v 6 m 6 0 v -6" }, // ║
  { "bars", "m -3 -3 h 6 m 0 3 h -6 m 0 3 h 6" }, // ≡
  { "ground", "m -2 4 h 4 m -5 -2 h 6 m -7 -2 h 8 m -4 0 l 0 -4" }, // ⏚
  // FIXME { "funnel", "m -1 0 l -3 -4 h 8 l -3 4 v 4 m -2 0 v -4" },
  { "erlenmeyer", "m -1 -4 v 4 l -3 4 h 8 l -3 -4 v -4" },
  // slashed
  // FIXME { "scircle", "a 3 3 m -4 4 l 8 -8" }, // ∅
  // back-slashed
  // FIXME { "bcircle", "a 3 3 m -4 -4 l 8 8" }, // ⦰
  // cross-slashed
  // FIXME { "xcircle", "a 3 3 m -4 -4 l 8 8 m -8 0 l 8 -8" }, // ⦻
  // circled
  // FIXME { "odash", "a 4 4 m -2 0 h 4" }, // ⊝
  // FIXME { "oplus", "a 4 4 m -2 0 h 4 m -2 -2 v 4" }, // ⊕
  // FIXME { "otimes", "a 4 4 m -2 -2 l 4 4 m -4 0 l 4 -4" }, // ⊗
  // FIXME { "ocircle", "a 4 4 a 2 2" }, // ⦾
  // squared
  // FIXME { "rdash", "v -4 h 4 v 8 h -8 v -8 z m 4 4 m -2 0 h 4" }, // ⊟
  // FIXME { "rplus", "v -4 h 4 v 8 h -8 v -8 z m 4 4 m -2 0 h 4 m -2 -2 v 4" }, // ⊞
  // FIXME { "rtimes", "v -4 h 4 v 8 h -8 v -8 z m 4 4 m -2 -2 l 4 4 m -4 0 l 4 -4" }, // ⊠
  // FIXME { "rcircle", "v -4 h 4 v 8 h -8 v -8 z m 4 4 a 2 2" }, // ⊡
  // ideograms
  // FIXME { "warning", "m -4 l -4 8 h 8 z m 0 2 v 4 m 0 2 a 1 1" }, // ⚠
  //{ "chuu", "m -4 -2 v 4 m 0 -4 h 8 m 0 0 v 4 m -8 -1 h 8 m -4 -5 v 8" }, // 中
  { "chuu", "m -4 3 v -4 h 8 v 4 v -1 h -8 m 4 -6 v 8" }, // 中
  // LATER /\-+|z⦀⦵⦶⦷⧋◄▼⧳ pacman •⁙ ⸭ ⸪ ⸫ ⸬ ⁚ ⁛ ⁘ ⁖ ㊅ ⁑⁎⁕ ⫯⫰⫱
  // LATER ⏏⏭†‡※☀★♠♣♦♥☻☺♯♮♭♬♫♪♩⚀⚁⚂⚃⚄⚅⚑⚮⚬♀♂♁☘☉🏁⚑
  // LATER ⽕🔥🜊⼟🗲⎔☩☨☦⛌⤫⤬⤭⨯⚔☁∪⊎⩈⼤⽊⟲⟳⥀⥁⭮⭯▦▩🉁	☾☽
};

Utf8StringList SvgWriter::_iconNames = SvgWriter::_icons.keys();
