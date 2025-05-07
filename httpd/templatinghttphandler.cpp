/* Copyright 2012-2025 Hallowyn, Gregoire Barbier and others.
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
#include "templatinghttphandler.h"
#include "io/ioutils.h"
#include "log/log.h"
#include "format/stringutils.h"
#include <QFile>
#include <QBuffer>

static const QRegularExpression _directorySeparatorRE("[/:]");

int TemplatingHttpHandler::_defaultMaxValueLength(500);
TemplatingHttpHandler::TextConversion
TemplatingHttpHandler::_defaultTextConversion(HtmlEscapingWithUrlAsLinks);

TemplatingHttpHandler::TemplatingHttpHandler(
    QObject *parent, const QByteArray &urlPathPrefix,
    const Utf8String &documentRoot)
  : FilesystemHttpHandler(parent, urlPathPrefix, documentRoot),
    _textConversion(_defaultTextConversion),
    _maxValueLength(_defaultMaxValueLength) {
}

void TemplatingHttpHandler::sendLocalResource(
    HttpRequest &req, HttpResponse &res, QFile *file,
    ParamsProviderMerger &context) {
  setMimeTypeByName(file->fileName().toUtf8(), res);
  for (const auto &filter: _filters) {
    if (QRegularExpression(filter).match(file->fileName()).hasMatch()) {
      Utf8String output;
      computePathToRoot(req, context);
      applyTemplateFile(req, res, file, context, output);
      res.set_content_length(output.size());
      if (req.method() != HttpRequest::HEAD) {
        QBuffer buf(&output);
        buf.open(QIODevice::ReadOnly);
        IOUtils::copy(res.output(), &buf);
      }
      return;
    }
  }
  if (!handleCacheHeadersAndSend304(file, req, res)) {
    res.set_content_length(file->size());
    IOUtils::copy(res.output(), file);
  }

}

void TemplatingHttpHandler::computePathToRoot(
    HttpRequest &req, ParamsProviderMerger &context) const {
  // note that FileSystemHttpHandler enforces that:
  // - the path never contains several adjacent / (would have been redirected)
  // - the path never points on a directory (would've been redirected to index)
  if (context.overridingParams().paramContains("!pathtoroot"))
    return;
  auto prefix = urlPathPrefix();
  auto path = req.path().mid(urlPathPrefix().length());
  bool ignoreOneSlash = prefix.isEmpty() ? path.startsWith('/')
                                         : !prefix.endsWith('/');
  int depth = path.count('/') - (ignoreOneSlash ? 1 : 0);
  auto pathToRoot = depth ? "../"_u8.repeated(depth) : "./"_u8;
  context.overrideParamValue("!pathtoroot"_u8, pathToRoot);
}

void TemplatingHttpHandler::applyTemplateFile(
    HttpRequest &req, HttpResponse &res, QFile *file,
    ParamsProviderMerger &context, Utf8String &output) {
  ParamsProviderMergerRestorer restorer(context);
  if (context.paramUtf8("!pathtoroot"_u8).isNull())
    computePathToRoot(req, context);
  QBuffer buf;
  buf.open(QIODevice::WriteOnly);
  IOUtils::copy(&buf, file);
  buf.close();
  Utf8String input = buf.data();
  int pos = 0, markupPos;
  while ((markupPos = input.indexOf("<?", pos)) >= 0) {
    output.append(input.mid(pos, markupPos-pos));
    pos = markupPos+2;
    markupPos = input.indexOf("?>", pos);
    auto markupContent = input.mid(pos, markupPos-pos).trimmed();
    int separatorPos = 0;
    while (markupContent.size() > separatorPos
           && ::isalpha(markupContent.at(separatorPos)))
      ++separatorPos;
    if (separatorPos >= markupContent.size()) {
      Log::warning() << "TemplatingHttpHandler found incorrect markup '"
                     << markupContent << "'";
      output.append('?');
    } else {
      auto markupId = markupContent.left(separatorPos);
      if (markupContent.at(0) == '=') { // syntax: <?=percent_expression?>
        output.append(markupContent.mid(1) % context);
      } else if (markupId == "view") { // syntax: <?view:viewname?>
        auto markupData = markupContent.mid(separatorPos+1);
        TextView *view = _views.value(markupData);
        if (view) {
          output.append(view->text(&context, req.url()));
        } else {
          Log::warning() << "TemplatingHttpHandler did not find view '"
                         << markupData << "' among " << _views.keys();
          [[unlikely]] output.append('?');
        }
      } else if (markupId == "value" || markupId == "rawvalue") {
        // syntax: <?[raw]value:variablename[:valueifnotdef[:valueifdef]]?>
        // rawvalue disables html encoding (escaping special chars and links
        // beautifying
        auto markupParams = markupContent.split_headed_list(separatorPos);
        auto value = context.paramUtf8(markupParams.value(0))
                     .toUtf16();
        if (!value.isNull()) {
          value = markupParams.value(2, value);
        } else {
          if (markupParams.size() < 2) {
            Log::debug() << "TemplatingHttpHandler did not find value: '"
                         << markupParams.value(0) << "'";
            [[unlikely]] value = u"?"_s;
          } else {
            value = markupParams.value(1);
          }
        }
        convertData(&value, markupId == "rawvalue");
        output.append(value.toUtf8());
      } else if (markupId == "include") {
        // syntax: <?include:path_relative_to_current_file_dir?>
        auto markupData = markupContent.mid(separatorPos+1);
        auto includePath = file->fileName();
        includePath =
            includePath.left(includePath.lastIndexOf(_directorySeparatorRE));
        QFile included(includePath+"/"_u8+markupData);
        // LATER detect include loops
        if (included.open(QIODevice::ReadOnly)) {
          applyTemplateFile(req, res, &included, context, output);
        } else {
          Log::warning() << "TemplatingHttpHandler couldn't include file: '"
                         << markupData << "' as '" << included.fileName()
                         << "' : " << included.errorString();
          [[unlikely]] output.append('?');
        }
      } else if (markupId == "override") {
        // syntax: <?override:key:value?>
        auto markupParams = markupContent.split_headed_list(separatorPos);
        auto key = markupParams.value(0);
        if (key.isEmpty()) {
          [[unlikely]];
          Log::debug() << "TemplatingHttpHandler cannot set parameter with "
                          "null key in file " << file->fileName();
        } else {
          auto value = markupParams.value(1) % context;
          context.overrideParamValue(key, value);
        }
      } else {
        Log::warning() << "TemplatingHttpHandler found unsupported markup: <?"
                       << markupContent << "?>";
        [[unlikely]] output.append('?');
      }
    }
    pos = markupPos+2;
  }
  output.append(input.right(input.size()-pos));
}

TemplatingHttpHandler *TemplatingHttpHandler::addView(TextView *view) {
  if (!view) {
    qWarning() << "TemplatingHttpHandler::addView(null)";
    [[unlikely]] return this;
  }
  Utf8String label = view->objectName().toUtf8();
  if (label.isEmpty()) {
    qWarning() << "TemplatingHttpHandler::addView(TextView*) called with empty "
                  "TextView::objectName():" << view;
    [[unlikely]] return this;
  }
  _views.insert(label, view);
  return this;
}

void TemplatingHttpHandler::convertData(
    QString *data, bool disableTextConversion) const {
  if (!data)
    return;
  *data = StringUtils::elideMiddle(*data, _maxValueLength);
  if (disableTextConversion)
    return;
  switch (_textConversion) {
  case HtmlEscaping:
    *data = StringUtils::htmlEncode(*data, false, false);
    break;
  case HtmlEscapingWithUrlAsLinks:
    *data = StringUtils::htmlEncode(*data, true, true);
    break;
  case AsIs:
    ;
  }
}
