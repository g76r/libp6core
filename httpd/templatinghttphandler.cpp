/* Copyright 2012-2017 Hallowyn, Gregoire Barbier and others.
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
#include <QRegularExpression>
#include <QFile>
#include <QBuffer>
#include "util/ioutils.h"
#include "log/log.h"
#include <QtDebug>
#include "util/characterseparatedexpression.h"
#include <QRegularExpression>
#include "format/stringutils.h"

static const QRegularExpression _templateMarkupIdentifierEndRE("[^a-z]");
static const QRegularExpression _directorySeparatorRE("[/:]");

int TemplatingHttpHandler::_defaultMaxValueLength(500);
TemplatingHttpHandler::TextConversion
TemplatingHttpHandler::_defaultTextConversion(HtmlEscapingWithUrlAsLinks);

TemplatingHttpHandler::TemplatingHttpHandler(
    QObject *parent, QString urlPathPrefix, QString documentRoot)
  : FilesystemHttpHandler(parent, urlPathPrefix, documentRoot),
    _textConversion(_defaultTextConversion),
    _maxValueLength(_defaultMaxValueLength) {
}

void TemplatingHttpHandler::sendLocalResource(
    HttpRequest req, HttpResponse res, QFile *file,
    ParamsProviderMerger *processingContext) {
  setMimeTypeByName(file->fileName(), res);
  foreach (QString filter, _filters) {
    if (QRegularExpression(filter).match(file->fileName()).hasMatch()) {
      QString output;
      applyTemplateFile(req, res, file, processingContext, &output);
      QByteArray ba = output.toUtf8();
      res.setContentLength(ba.size());
      if (req.method() != HttpRequest::HEAD) {
        QBuffer buf(&ba);
        buf.open(QIODevice::ReadOnly);
        IOUtils::copy(res.output(), &buf);
      }
      return;
    }
  }
  if (!handleCacheHeadersAndSend304(file, req, res)) {
    res.setContentLength(file->size());
    IOUtils::copy(res.output(), file);
  }

}

QString TemplatingHttpHandler::computePathToRoot(const HttpRequest &req, ParamsProviderMerger *processingContext) const {
  // note that FileSystemHttpHandler enforces that:
  // - the path never contains several adjacent / (would have been redirected)
  // - the path never points on a directory (would've been redirected to index)
  const QString prefix = urlPathPrefix();
  const QString path = req.url().path().mid(urlPathPrefix().length());
  bool ignoreOneSlash = prefix.isEmpty() ? path.startsWith('/')
                                         : !prefix.endsWith('/');
  int depth = path.count('/') - (ignoreOneSlash ? 1 : 0);
  QString pathToRoot = depth ? QStringLiteral("../").repeated(depth)
                             : QStringLiteral("./");
  if (processingContext)
    processingContext->overrideParamValue(QStringLiteral("!pathtoroot"),
                                          pathToRoot);
  return pathToRoot;
}

void TemplatingHttpHandler::applyTemplateFile(
    HttpRequest req, HttpResponse res, QFile *file,
    ParamsProviderMerger *processingContext,
    QString *output) {
  HttpRequestPseudoParamsProvider hrpp = req.pseudoParams();
  ParamsProviderMergerRestorer restorer(processingContext);
  processingContext->append(&hrpp);
  if (!processingContext->paramValue(QStringLiteral("!pathtoroot")).isValid())
    computePathToRoot(req, processingContext);
  QBuffer buf;
  buf.open(QIODevice::WriteOnly);
  IOUtils::copy(&buf, file);
  buf.close();
  QString input = QString::fromUtf8(buf.data());
  int pos = 0, markupPos;
  while ((markupPos = input.indexOf("<?", pos)) >= 0) {
    output->append(input.mid(pos, markupPos-pos));
    pos = markupPos+2;
    markupPos = input.indexOf("?>", pos);
    QString markupContent = input.mid(pos, markupPos-pos).trimmed();
    int separatorPos = markupContent.indexOf(_templateMarkupIdentifierEndRE);
    if (separatorPos < 0) {
      Log::warning() << "TemplatingHttpHandler found incorrect markup '"
                     << markupContent << "'";
      output->append("?");
    } else {
      QString markupId = markupContent.left(separatorPos);
      if (markupContent.at(0) == '=') {
        // syntax: <?=paramset_evaluable_expression?>
        output->append(
              processingContext->overridingParams()
              .evaluate(markupContent.mid(1), processingContext));
      } else if (markupId == QStringLiteral("view")) {
        // syntax: <?view:viewname?>
        QString markupData = markupContent.mid(separatorPos+1);
        TextView *view = _views.value(markupData);
        if (view)
          output->append(view->text(processingContext, req.url().toString()));
        else {
          Log::warning() << "TemplatingHttpHandler did not find view '"
                         << markupData << "' among " << _views.keys();
          output->append("?");
        }
      } else if (markupId == QStringLiteral("value")
                 || markupId == QStringLiteral("rawvalue")) {
        // syntax: <?[raw]value:variablename[:valueifnotdef[:valueifdef]]?>
        CharacterSeparatedExpression markupParams(markupContent, separatorPos);
        QString value = processingContext
            ->paramValue(markupParams.value(0)).toString();
        if (!value.isNull()) {
          value = markupParams.value(2, value);
        } else {
          if (markupParams.size() < 2) {
            Log::debug() << "TemplatingHttpHandler did not find value: '"
                         << markupParams.value(0) << "' in context 0x"
                         << QString::number((long long)processingContext, 16);
            value = "?";
          } else {
            value = markupParams.value(1);
          }
        }
        convertData(&value, markupId == QStringLiteral("rawvalue"));
        output->append(value);
      } else if (markupId == QStringLiteral("include")) {
        // syntax: <?include:path_relative_to_current_file_dir?>
        QString markupData = markupContent.mid(separatorPos+1);
        QString includePath = file->fileName();
        includePath =
            includePath.left(includePath.lastIndexOf(_directorySeparatorRE));
        QFile included(includePath+"/"+markupData);
        // LATER detect include loops
        if (included.open(QIODevice::ReadOnly)) {
          applyTemplateFile(req, res, &included, processingContext, output);
        } else {
          Log::warning() << "TemplatingHttpHandler couldn't include file: '"
                         << markupData << "' as '" << included.fileName()
                         << "' in context 0x"
                         << QString::number((long long)processingContext, 16)
                         << " : " << included.errorString();
          output->append("?");
        }
      } else if (markupId == QStringLiteral("override")) {
        // syntax: <?override:key:value?>
        CharacterSeparatedExpression markupParams(markupContent, separatorPos);
        QString key = markupParams.value(0);
        if (key.isEmpty()) {
          Log::debug() << "TemplatingHttpHandler cannot set parameter with "
                          "null key in file " << file->fileName();
        } else {
          QString value = processingContext->overridingParams().evaluate(
                markupParams.value(1), processingContext);
          processingContext->overrideParamValue(key, value);
        }
      } else {
        Log::warning() << "TemplatingHttpHandler found unsupported markup: <?"
                       << markupContent << "?>";
        output->append("?");
      }
    }
    pos = markupPos+2;
  }
  output->append(input.right(input.size()-pos));
}

TemplatingHttpHandler *TemplatingHttpHandler::addView(TextView *view) {
  QString label = view ? view->objectName() : QString();
  if (label.isEmpty())
    qWarning() << "TemplatingHttpHandler::addView(TextView*) called with empty "
                  "TextView::objectName():" << view;
  else
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
