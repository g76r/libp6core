/* Copyright 2012-2014 Hallowyn and others.
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

TemplatingHttpHandler::TemplatingHttpHandler(
    QObject *parent, QString urlPathPrefix, QString documentRoot)
  : FilesystemHttpHandler(parent, urlPathPrefix, documentRoot) {
}

void TemplatingHttpHandler::sendLocalResource(
    HttpRequest req, HttpResponse res, QFile *file,
    HttpRequestContext ctxt) {
  setMimeTypeByName(file->fileName(), res);
  foreach (QString filter, _filters) {
    QRegExp re(filter);
    if (re.indexIn(file->fileName()) >= 0) {
      QString output;
      applyTemplateFile(req, res, file, ctxt, &output);
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
    HttpRequest req, HttpResponse res, QFile *file, HttpRequestContext ctxt,
    QString *output) {
  QBuffer buf;
  buf.open(QIODevice::WriteOnly);
  IOUtils::copy(&buf, file);
  buf.close();
  QString input = QString::fromUtf8(buf.data());
  int pos = 0, markup;
  while ((markup = input.indexOf("<?", pos)) >= 0) {
    output->append(input.mid(pos, markup-pos));
    pos = markup+2;
    markup = input.indexOf("?>", pos);
    QString label = input.mid(pos, markup-pos).trimmed();
    int colon = label.indexOf(":");
    if (colon < 0) {
      Log::warning() << "TemplatingHttpHandler found incorrect markup '"
                     << label << "'";
      output->append("?");
    } else {
      QString data = label.mid(colon+1);
      label = label.left(colon);
      if (label == "view") {
        QPointer<TextView> view = _views.value(data);
        if (view)
          output->append(view.data()->text(&ctxt, req.url().toString()));
        else {
          Log::warning() << "TemplatingHttpHandler did not find view '"
                         << data << "' among " << _views.keys();
          output->append("?");
        }
      } else if (label == "value") {
        QString value = ctxt.paramValue(data).toString();
        if (!value.isNull())
          output->append(value);
        else {
          Log::warning() << "TemplatingHttpHandler did not find value: '"
                         << data << "' in context 0x"
                         << QString::number((long long)&ctxt, 16);
          output->append("?");
        }
      } else if (label == "include") {
        QString includePath = file->fileName();
        QRegularExpression dirSep("[/:]");
        includePath = includePath.left(includePath.lastIndexOf(dirSep));
        QFile included(includePath+"/"+data);
        if (included.open(QIODevice::ReadOnly)) {
          applyTemplateFile(req, res, &included, ctxt, output);
        } else {
          Log::warning() << "TemplatingHttpHandler couldn't include file: '"
                         << data << "' as '" << included.fileName()
                         << "' in context 0x"
                         << QString::number((long long)&ctxt, 16)
                         << " : " << included.errorString();
          output->append("?");
        }
      } else {
        Log::warning() << "TemplatingHttpHandler found incorrect markup: <?"
                       << label << ":" << data << "?>";
        output->append("?");
      }
    }
    pos = markup+2;
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
