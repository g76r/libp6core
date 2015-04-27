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
#include "templatinghttphandler.h"
#include <QRegExp>
#include <QRegularExpression>
#include <QFile>
#include <QBuffer>
#include "util/ioutils.h"
#include "log/log.h"
#include <QtDebug>
#include "util/characterseparatedexpression.h"
#include <QRegularExpression>
#include "util/htmlutils.h"

int TemplatingHttpHandler::_defaultMaxValueLength(200);
TemplatingHttpHandler::TextConversion
TemplatingHttpHandler::_defaultTextConversion(HtmlEscapingWithUrlAsLinks);

TemplatingHttpHandler::TemplatingHttpHandler(
    QObject *parent, QString urlPathPrefix, QString documentRoot)
  : FilesystemHttpHandler(parent, urlPathPrefix, documentRoot),
    _textConversion(_defaultTextConversion) {
}

void TemplatingHttpHandler::sendLocalResource(
    HttpRequest req, HttpResponse res, QFile *file,
    ParamsProviderMerger *processingContext) {
  setMimeTypeByName(file->fileName(), res);
  foreach (QString filter, _filters) {
    QRegExp re(filter);
    if (re.indexIn(file->fileName()) >= 0) {
      QString output;
      applyTemplateFile(req, res, file, processingContext, &output);
      QByteArray ba = output.toUtf8();
      res.setContentLength(ba.size());
      QBuffer buf(&ba);
      buf.open(QIODevice::ReadOnly);
      IOUtils::copy(res.output(), &buf);
      return;
    }
  }
  if (!handleCacheHeadersAndSend304(file, req, res)) {
    res.setContentLength(file->size());
    IOUtils::copy(res.output(), file);
  }

}

void TemplatingHttpHandler::applyTemplateFile(
    HttpRequest req, HttpResponse res, QFile *file,
    ParamsProviderMerger *processingContext,
    QString *output) {
  static QRegularExpression templateMarkupIdentifierEndRE("[^a-z]");
  static QRegularExpression directorySeparatorRE("[/:]");
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
    int separatorPos = markupContent.indexOf(templateMarkupIdentifierEndRE);
    if (separatorPos < 0) {
      Log::warning() << "TemplatingHttpHandler found incorrect markup '"
                     << markupContent << "'";
      output->append("?");
    } else {
      QString markupId = markupContent.left(separatorPos);
      if (markupContent.at(0) == '=') {
        // syntax: <?=paramset_evaluable_expression?>
        output->append(
              ParamSet().evaluate(markupContent.mid(1), processingContext));
      } else if (markupId == "view") {
        // syntax: <?view:viewname?>
        QString markupData = markupContent.mid(separatorPos+1);
        QPointer<TextView> view = _views.value(markupData);
        if (view)
          output->append(view.data()
                         ->text(processingContext, req.url().toString()));
        else {
          Log::warning() << "TemplatingHttpHandler did not find view '"
                         << markupData << "' among " << _views.keys();
          output->append("?");
        }
      } else if (markupId == "value" || markupId == "rawvalue") {
        // syntax: <?[raw]value:variablename[:valueifnotdef[:valueifdef]]?>
        // value is htmlencoded provided textConversion != AsIs
        // rawvalue is not htmlencoded regardless textConversion setting
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
        convertData(&value, markupId == "rawvalue");
        output->append(value);
      } else if (markupId == "include") {
        // syntax: <?include:path_relative_to_current_file_dir?>
        QString markupData = markupContent.mid(separatorPos+1);
        QString includePath = file->fileName();
        includePath =
            includePath.left(includePath.lastIndexOf(directorySeparatorRE));
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
      } else {
        Log::warning() << "TemplatingHttpHandler found unsupported markup: <?"
                       << markupContent << "?>";
        output->append("?");
      }
    }
    pos = markupPos+2;
  }
  output->append(input.right(input.size()-pos));
  return;
}

TemplatingHttpHandler *TemplatingHttpHandler::addView(TextView *view) {
  QString label = view ? view->objectName() : QString();
  if (label.isEmpty())
    qWarning() << "TemplatingHttpHandler::addView(TextView*) called with empty "
                  "TextView::objectName():" << view;
  else
    _views.insert(label, QPointer<TextView>(view));
  return this;
}

void TemplatingHttpHandler::convertData(
    QString *data, bool disableTextConversion) const {
  if (!data)
    return;
  if (_maxValueLength > 0 && data->size() > _maxValueLength) {
    *data = data->left(_maxValueLength/2-1) + "..."
        + data->right(_maxValueLength/2-2);
  }
  if (disableTextConversion)
    return;
  switch (_textConversion) {
  case HtmlEscaping:
    *data = HtmlUtils::htmlEncode(*data, false, false);
    break;
  case HtmlEscapingWithUrlAsLinks:
    *data = HtmlUtils::htmlEncode(*data, true, true);
    break;
  case AsIs:
    ;
  }
}
