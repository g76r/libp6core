/* Copyright 2012-2015 Hallowyn and others.
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
#ifndef TEMPLATINGHTTPHANDLER_H
#define TEMPLATINGHTTPHANDLER_H

#include "filesystemhttphandler.h"
#include "textview/textview.h"
#include <QPointer>

// LATER try to factorize code with HtmlItemDelegate
// LATER make all method thread-safe, incl. setters
// TODO document other markups than value and view
/** HttpHandler which serves filesystem or Qt resources files parsing some of
 * them for special markups such as <?value:foo?> or <?view:bar?> to replace
 * these markups with dynamic content.
 * Files to be parsed must be declared first, e.g. with addFilter("\\.html$").
 * Views are permanent objects registred through addView() whereas values are
 * request-time data avalaible through a ParamsProvider. */
class LIBQTSSUSHARED_EXPORT TemplatingHttpHandler
    : public FilesystemHttpHandler {
  Q_OBJECT
  Q_DISABLE_COPY(TemplatingHttpHandler)

public:
  enum TextConversion { AsIs, HtmlEscaping, HtmlEscapingWithUrlAsLinks };

private:
  QHash<QString,QPointer<TextView> > _views;
  QSet<QString> _filters;
  TextConversion _textConversion;
  static TextConversion _defaultTextConversion;
  int _maxValueLength;
  static int _defaultMaxValueLength;

public:
  explicit TemplatingHttpHandler(QObject *parent = 0,
                                 QString urlPathPrefix = "",
                                 QString documentRoot = ":docroot/");
  TemplatingHttpHandler *addView(QString label, TextView *view) {
    _views.insert(label, QPointer<TextView>(view));
    return this; }
  TemplatingHttpHandler *addView(TextView *view);
  TemplatingHttpHandler *addFilter(QString regexp) {
    _filters.insert(regexp);
    return this; }
  void setTextConversion(TemplatingHttpHandler::TextConversion textConversion) {
    _textConversion = textConversion; }
  static void setDefaultTextConversion(
      TemplatingHttpHandler::TextConversion defaultTextConversion) {
    _defaultTextConversion = defaultTextConversion; }
  /** Maximum length of text inside a cell, measured before HTML encoding if
   * any. Default: 200. */
  void setMaxValueLength(int length = 200) { _maxValueLength = length; }
  /** Maximum length of text inside a cell, measured before HTML encoding if
   * any. Default: 200. */
  static void setDefaultMaxValueLength(int length = 200) {
    _defaultMaxValueLength = length; }

protected:
  void sendLocalResource(HttpRequest req, HttpResponse res, QFile *file,
                         ParamsProviderMerger *processingContext);

private:
  void applyTemplateFile(HttpRequest req, HttpResponse res, QFile *file,
                         ParamsProviderMerger *processingContext,
                         QString *output);
  void convertData(QString *data, bool disableTextConversion) const;
};

#endif // TEMPLATINGHTTPHANDLER_H
