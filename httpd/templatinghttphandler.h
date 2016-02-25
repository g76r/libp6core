/* Copyright 2012-2016 Hallowyn and others.
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
#ifndef TEMPLATINGHTTPHANDLER_H
#define TEMPLATINGHTTPHANDLER_H

#include "filesystemhttphandler.h"
#include "textview/textview.h"
#include <QPointer>

// LATER try to factorize code with HtmlItemDelegate
// LATER make all method thread-safe, incl. setters
/** HttpHandler which serves filesystem or Qt resources files parsing some of
 * them for special markups such as <?value:foo?> or <?view:bar?> to replace
 * these markups with dynamic content.
 *
 * Files to be parsed must be declared first, e.g. with addFilter("\\.html$").
 * Views are permanent objects registred through addView() whereas values are
 * request-time data avalaible through a ParamsProvider.
 *
 * Currently, following markups are supported:
 *
 * <?[raw]value:variablename[:valueifnotdef[:valueifdef]]?>
 * intent: displays the value of a processing context variable, limiting its
 *         length to max value length (500 by default), and html-encoding
 *         (but for rawvalue markup or if text conversion has been set to AsIs)
 * params:
 * - valueifnotdef: replacement value if variablename is not defined, if not set
 *   "?" is used and a warning is logged
 * - valueifdef: replacement value if variablename is defined
 * see also: <?=?> to provide %-evaluation and no length limit
 * examples:
 * - <?value:userid:?> displays the authentified userid or an empty string
 * - <?value:11:-?> displays the field #11 of a SharedUiItem associated to
 *   processing context through SharedUiItemParamsProvider or a dash
 *
 * <?=formula?>
 * intent: displays the result of a ParamSet %-evaluated formula
 * see also: <?[raw]value?> and ParamSet
 * examples:
 * - <?=%{=ifneq:%31::<a href="taskdoc.html?taskid=%31">%31</a>}?> displays
 *   value of field #31 surrounded with an html link if field is not empty,
 *   or displays nothing
 * - <?=%{=ifneq:%27::<i class="icon-fire"></i>&nbsp;%27:}?> displays value of
 *   field #27, plus an icon if the field is not empty
 * - <?=%name?> displays variable name without length limitation nor html
 *   encoding
 * - %{=htmlencode:%name} displays variable name without length limitation but
 *   with html encoding
 * - %{=htmlencode:%{=elidemiddle!%name!100}} displays variable name with an
 *   arbitrary length limitation (100 characters before html encoding)
 *
 * <?view:viewname?>
 * intent: displays the current content of a view
 * examples:
 * - <?view:users?> displays the content of an associated TextView named users
 *
 * <?include:path_relative_to_current_file_dir?>
 * intent: includes the content of another template file
 * examples:
 * - <?include:header.html?> includes a header file
 */
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
    _views.insert(label, view); return this; }
  TemplatingHttpHandler *addView(TextView *view);
  TemplatingHttpHandler *addFilter(QString regexp) {
    _filters.insert(regexp);
    return this; }
  void setTextConversion(TemplatingHttpHandler::TextConversion textConversion) {
    _textConversion = textConversion; }
  static void setDefaultTextConversion(
      TemplatingHttpHandler::TextConversion defaultTextConversion) {
    _defaultTextConversion = defaultTextConversion; }
  /** Maximum length of text produced by <?[raw]value?> markups, measured before
   * HTML encoding if any. Default: defaultMaxValueLength().
   * 0 means unlimited. */
  void setMaxValueLength(int length = 500) { _maxValueLength = length; }
  /** Maximum length of text produced by <?[raw]value?> markups, measured before
   * HTML encoding if any. Default: 500. 0 means unlimited. */
  static void setDefaultMaxValueLength(int length = 500) {
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
