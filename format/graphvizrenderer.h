/* Copyright 2024 Hallowyn, Gregoire Barbier and others.
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
#ifndef GRAPHVIZRENDERER_H
#define GRAPHVIZRENDERER_H

#include "util/paramset.h"
#include <QMutexLocker>
#include <QProcess>

/** QProcess subclass for rendering graphviz graph using localy installed
 *  binaries (dot, neato, etc.).
 */
class LIBP6CORESHARED_EXPORT GraphvizRenderer : public QProcess {
  Q_OBJECT

public:
  enum Layout { Dot, Neato, TwoPi, Circo, Fdp, Sfdp, Osage };
  enum Format { Png, Svg, Svgz, Plain, Gv, Xdot };

private:
  Utf8String _source, _tmp, _stderr, _output;
  Layout _layout;
  Format _format;
  ParamSet _params;
  QMutex _mutex;

public:
  explicit GraphvizRenderer(
      const Utf8String &source, Layout layout = Dot,
      Format format = Plain, const ParamSet &params = {});
  GraphvizRenderer(
      const Utf8String &source, Format format, const ParamSet &params = {})
    : GraphvizRenderer(source, Dot, format, params) {}
  explicit GraphvizRenderer(
      Layout layout, Format format = Plain, const ParamSet &params = {})
    : GraphvizRenderer({}, layout, format, params) {}
  explicit GraphvizRenderer(Format format, const ParamSet &params = {})
    : GraphvizRenderer({}, Dot, format, params) {}
  GraphvizRenderer(const Utf8String &source, const ParamSet &params)
    : GraphvizRenderer(source, Dot, Plain, params) {}
  explicit GraphvizRenderer(const ParamSet &params)
    : GraphvizRenderer({}, Dot, Plain, params) {}
  GraphvizRenderer() : GraphvizRenderer(""_u8) {}
  /** Synchronously start and wait for process finished, then return output
   *  Thread-safe (by blocking and allowing only one rendering at a time).
   */
  Utf8String run(ParamsProvider *params_evaluation_context = 0,
                 const Utf8String &source = {});
  static Utf8String mime_type(Format format);
  static Format formatFromString(const Utf8String &s, Format def = Gv);
  static Utf8String formatAsString(Format format);
  static Layout layoutFromString(const Utf8String &s, Layout def = Dot);
  static Utf8String layoutAsString(Layout layout);

private:
  using QProcess::start; // make it private
  void do_start(ParamsProvider *params_evaluation_context,
                const Utf8String &source);
  void process_error(QProcess::ProcessError error);
  void process_finished(int exitCode, QProcess::ExitStatus exitStatus);
  void read_stdout();
  void read_stderr();
};

#endif // GRAPHVIZRENDERER_H
