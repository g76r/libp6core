/* Copyright 2012-2013 Hallowyn and others.
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
#include <QFile>
#include <QBuffer>
#include "util/ioutils.h"
#include "log/log.h"

TemplatingHttpHandler::TemplatingHttpHandler(
    QObject *parent, const QString urlPrefix, const QString documentRoot) :
  FilesystemHttpHandler(parent, urlPrefix, documentRoot) {
}

void TemplatingHttpHandler::sendLocalResource(
    HttpRequest req, HttpResponse res, QFile *file,
    ParamsProvider *values, QString scope) {
  Q_UNUSED(req)
  Q_UNUSED(scope)
  setMimeTypeByName(file->fileName(), res);
  foreach (QString filter, _filters) {
    QRegExp re(filter);
    if (re.indexIn(file->fileName()) >= 0) {
      QBuffer buf;
      buf.open(QIODevice::WriteOnly);
      IOUtils::copyAll(&buf, file);
      buf.close();
      QString input = QString::fromUtf8(buf.data()), output;
      int pos = 0, markup;
      while ((markup = input.indexOf("<?", pos)) >= 0) {
        output.append(input.mid(pos, markup-pos));
        pos = markup+2;
        markup = input.indexOf("?>", pos);
        QString label = input.mid(pos, markup-pos).trimmed();
        int colon = label.indexOf(":");
        if (colon < 0) {
          Log::warning() << "TemplatingHttpHandler found incorrect markup '"
                         << label << "'";
          output.append("?");
        } else {
          QString data = label.mid(colon+1);
          label = label.left(colon);
          if (label == "view") {
            QWeakPointer<TextView> view = _views.value(data);
            if (view)
              output.append(view.data()->text(values, data));
            else {
              Log::warning() << "TemplatingHttpHandler did not find view '"
                             << data << "' among " << _views.keys();
              output.append("?");
            }
          } else if (label == "value") {
            QString value(values ? values->paramValue(data).toString()
                                 : QString());
            if (!value.isNull())
              output.append(value);
            else {
              Log::warning() << "TemplatingHttpHandler did not find value: '"
                             << data << "' among paramset 0x"
                             << QString::number((long long)values, 16);
              output.append("?");
            }
          } else {
            Log::warning() << "TemplatingHttpHandler found incorrect markup: <?"
                           << label << ":" << data << "?>";
            output.append("?");
          }
        }
        pos = markup+2;
      }
      output.append(input.right(input.size()-pos));
      QByteArray ba = output.toUtf8();
      res.setContentLength(ba.size());
      buf.setData(ba);
      buf.open(QIODevice::ReadOnly);
      IOUtils::copyAll(res.output(), &buf);
      return;
    }
  }
  res.setContentLength(file->size());
  IOUtils::copyAll(res.output(), file);
}
