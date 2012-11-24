#ifndef PFDOMHANDLER_H
#define PFDOMHANDLER_H

#include "pfhandler.h"
#include "pfnode.h"
#include <QList>

/** Handler for loading the whole PF document into memory (except lazy-loaded
  * binary fragments).
  * This class is usefull for manipulating PF content without bothering with
  * event-oriented parsing: all the data is loaded into memory (excepted binary
  * fragments if they are lazy-loaded) and can be manipuled as PfNode trees.
  */
class PfDomHandler : public PfHandler {
protected:
  QList<PfNode> _path;
  QList<PfNode> _roots;
  bool _deleteNodesOnDelete;

public:
  // LATER add option to enable (or disable) loading of arrays as children
  /** @param ignoreComments if set, won't receive comment() calls
    */
  PfDomHandler();
  ~PfDomHandler();
  bool startDocument(const PfOptions &options);
  bool startNode(const QList<QString> &names);
  bool text(const QString &text);
  bool binary(QIODevice *device, qint64 length, qint64 offset,
              const QString &surface);
  bool binary(const QByteArray &data, const QString &surface);
  bool array(const PfArray &array);
  bool endNode(const QList<QString> &names);
  bool comment(const QString &content);
  bool endDocument();
  QList<PfNode> roots() const { return _roots; }
};

#endif // PFDOMHANDLER_H
