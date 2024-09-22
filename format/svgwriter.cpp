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
    _svg += "text-anchor=\"start\" ";
  else if (flags & Qt::AlignHCenter)
    _svg += "text-anchor=\"middle\" ";
  else if (flags & Qt::AlignRight)
    _svg += "text-anchor=\"end\" ";
  _svg += ">"+text+"</text>\n";
  // LATER enforce bounding box for real
}

void SvgWriter::startAnchor(const Utf8String &title) {
  _svg += "<a xlink:title=\""+title+"\">\n";
}

void SvgWriter::endAnchor() {
  _svg += "</a>\n";
}

void SvgWriter::comment(const Utf8String &text) {
  _svg += "<!-- "+text+" -->\n";
}

Utf8String SvgWriter::data() const {
  return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
         "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\""
         " \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n"
         "<svg width=\""+Utf8String::number(_viewport.width())
      +"px\" height=\""+Utf8String::number(_viewport.height())
      +"px\" xmlns=\"http://www.w3.org/2000/svg\" "
       "xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n"
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
  { "square", "m-4,-4 h8 v8 h-8 z" }, // ■
  { "osquare", "m-4,-4 h8,-8 v8 h8,-8 m8,0 h-8,8 v-8 h-8,8" }, // □
  { "board22", "v-4 h4 v4 h-8 v4 h4 z m-4,-4 h8 h-8 v8 m8,0 h-8 h8 v-8" }, // 🙾 2x2 checkered square
  { "circle", "m-4,0 a4,4 0 0,0 8,0 a4,4 0 0,0 -8,0" }, // ●
  // for arc-based paths, see:
  // https://www.w3.org/TR/SVG/paths.html#PathDataEllipticalArcCommands
  // https://stackoverflow.com/questions/5737975/circle-drawing-with-svgs-arc-path
  { "ginkgo", "m-4,0 a4,4 0 0,1 4,4 a4,4 0 0,1 4,-4 a4,4 0 0,0 -8,0 z" }, // ginkgo leaf
  { "droplet", "m-4,0 a4,4 0 0,0 8,0 l-4,-4 z" }, // 🌢
  { "pacman", "l2.83,-2.83 a4,4 0 1,0 0,5.66 z" },
  { "crescentl", "m0,-4 a4,4 0 0,0 0,8 a6,6 0 0,1 0,-8 z" }, // ☾ north waning crescent
  { "crescentr", "m0,-4 a4,4 0 0,1 0,8 a6,6 0 0,0 0,-8 z" }, // ☽ north waxing crescent
  { "heart", "m-3.5,0 a2,2 0 0,1 3.5,-3.5 a2,2 0 0,1 3.5,3.5 l-3.5,3.5 z" }, // ♥
  // TODO { "marriage", "" }, // ⚭
  // TODO coat of arms / shield 🛡
  { "diamond", "m0,-4 l4,4 l-4,4 l-4,-4 z" }, // ♦
  { "arrowu", "m0,-3 l-4,6 h8 z" }, // ▲
  { "arrowd", "m0,3 l-4,-6 h8 z" }, // ▼
  { "arrowr", "m3,0 l-6,-4 v8 z" }, // ►
  { "arrowl", "m-3,0 l6,-4 v8 z" }, // ◄
  { "blockedarrow", "m4,-4 h-6 l5,4 l-5,4 h6 z m-8,1 l4,3 l-4,3 l4-3 l-4,-3 v6 l4,-3 l-4,3 v-6" }, // arrow to right blocked by clog
  //{ "blockedarrow", "m4,-4 h-6 l5,4 l-5,4 h6 z m-8,1 l4,3 l-4,3 z" }, same with filled arrow
  { "crossedarrows", "m1,0 l3,-3 v6 z m-1,-1 l3,-3 h-6 z m-1,1 l-3,-3 v6 z m1,1 l3,3 h-6 z" },
  { "hourglass", "m -4 -4 h 8 l -8 8 h 8 z" }, // ⧗
  { "bowtie",    "m -4 -4 l 8 8 v -8 l -8 8 z" }, // ⧓
  { "times", "m -4 -4 l 8 8 m -8 0 l 8 -8" }, // ×
  { "equal", "m-3,-1 h6 m0,3 h-6" }, // =
  { "pause", "m-1,-3 v6 m3,0 v-6" }, // ║
  { "bars", "m -3 -3 h 6 m 0 3 h -6 m 0 3 h 6" }, // ≡
  // slashed
  // TODO { "scircle", "a 3 3 m -4 4 l 8 -8" }, // ∅
  // back-slashed
  // TODO { "bcircle", "a 3 3 m -4 -4 l 8 8" }, // ⦰
  // cross-slashed
  // TODO { "xcircle", "a 3 3 m -4 -4 l 8 8 m -8 0 l 8 -8" }, // ⦻
  // circled
  // TODO { "odash", "a 4 4 m -2 0 h 4" }, // ⊝
  // TODO { "oplus", "a 4 4 m -2 0 h 4 m -2 -2 v 4" }, // ⊕
  // TODO { "otimes", "a 4 4 m -2 -2 l 4 4 m -4 0 l 4 -4" }, // ⊗
  // TODO { "ocircle", "a 4 4 a 2 2" }, // ⦾
  // squared
  // TODO { "rdash", "v -4 h 4 v 8 h -8 v -8 z m 4 4 m -2 0 h 4" }, // ⊟
  // TODO { "rplus", "v -4 h 4 v 8 h -8 v -8 z m 4 4 m -2 0 h 4 m -2 -2 v 4" }, // ⊞
  // TODO { "rtimes", "v -4 h 4 v 8 h -8 v -8 z m 4 4 m -2 -2 l 4 4 m -4 0 l 4 -4" }, // ⊠
  // TODO { "rcircle", "v -4 h 4 v 8 h -8 v -8 z m 4 4 a 2 2" }, // ⊡
  // ideograms
  { "ground", "m -2 4 h 4 m -5 -2 h 6 m -7 -2 h 8 m -4 0 l 0 -4" }, // ⏚
  { "erlenmeyer", "m-1,-4 v4 l-3,4 h8 l-3,-4 v-4" },
  { "funnel", "m-1,4 v-4 l-3,-4 h8 l-3,4 v4" },
  // TODO { "warning", "m -4 l -4 8 h 8 z m 0 2 v 4 m 0 2 a 1 1" }, // ⚠
  // TODO compass rose (4 winds, 8 winds, circled) 🧭
  // TODO fleur de lis ⚜
  { "chuu", "m-4,-2 v4,-4 h8 v4,-4 h-8 v3 h8,-8 m4,-5 v8" }, // 中 center
  { "nin", "m0,-4 v3 a6,6 0 0,1 -4,5 a6,6 0 0,0 4,-5 a6,6 0 0,0 4,5 a6,6 0 0,1 -4,-5" }, // 人 human
  { "ka", "m0,-4 v3 a6,6 0 0,1 -4,5 a6,6 0 0,0 4,-5 a6,6 0 0,0 4,5 a6,6 0 0,1 -4,-5 m3,-2 a6,6 0 0,1 -2,2 m-2,0 l-2,-2" }, // 火 fire
  // LATER /\-+|z⦀⦵⦶⦷⧋ •⁙ ⸭ ⸪ ⸫ ⸬ ⁚ ⁛ ⁘ ⁖ ㊅ ⁑⁎⁕ ⫯⫰⫱ ⎋ ⧮⧯⧰⧱⧲⧳
  // LATER ⏏⏭†‡※☀★♠♣☻☺♯♮♭♬♫♪♩⚀⚁⚂⚃⚄⚅⚑⚮⚬♀♂♁☘☉🏁⚑
  // LATER 🜊⼟🗲⎔☩☨☦⛌⤫⤬⤭⨯⚔☁∪⊎⩈⼤⽊⟲⟳⥀⥁⭮⭯▦▩🉁❂
  // LATER https://www.compart.com/fr/unicode/block/U+25A0 ◧
  // LATER haglaz ᚺ haegl ᚻ wunjo ᚹ féhu ᚠ algiz ᛉ calc ᛣ mannaz ᛗ ing ᛝ tvimadur ᛯ gar ᚸ aleph א
  // LATER https://fr.wikipedia.org/wiki/Balisage
  // LATER segno 𝄋 fclef 𝄢 ᚋ ᚌ ᚍ ᚎ ᚏ
  // LATER https://en.wikipedia.org/wiki/Chemical_symbol#Daltonian_symbols
};

Utf8StringList SvgWriter::_iconNames = SvgWriter::_icons.keys();
