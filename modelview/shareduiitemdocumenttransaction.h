/* Copyright 2015 Hallowyn and others.
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
#ifndef SHAREDUIITEMDOCUMENTTRANSACTION_H
#define SHAREDUIITEMDOCUMENTTRANSACTION_H

#include <QPointer>
#include "shareduiitemlist.h"
#include "util/coreundocommand.h"

class SharedUiItemDocumentManager;

/** Transaction that can be used by changeItem()/prepareChangeItem()/
 * commitChangeItem() to create ChangeItemCommands and access to changes
 * performed within the transaction but not yet commited to the DM. */
class LIBQTSSUSHARED_EXPORT SharedUiItemDocumentTransaction
    : public CoreUndoCommand {
  SharedUiItemDocumentManager *_dm;
  QHash<QString,QHash<QString,SharedUiItem>> _newItems;
  QHash<QString,QSet<QString>> _deletedItems;

public:
  class LIBQTSSUSHARED_EXPORT ChangeItemCommand : public CoreUndoCommand {
    QPointer<SharedUiItemDocumentManager> _dm;
    SharedUiItem _newItem, _oldItem;
    QString _idQualifier;

  public:
    ChangeItemCommand(SharedUiItemDocumentManager *dm, SharedUiItem newItem,
                      SharedUiItem oldItem, QString idQualifier,
                      CoreUndoCommand *parent);
    void redo();
    void undo();
  };

  SharedUiItemDocumentTransaction(SharedUiItemDocumentManager *dm) : _dm(dm) { }
  void changeItem(SharedUiItem newItem, SharedUiItem oldItem,
                  QString idQualifier);
  SharedUiItem itemById(QString idQualifier, QString id);
  SharedUiItemList<> foreignKeySources(
      QString sourceQualifier, int sourceSection, QString referenceId);
};


#endif // SHAREDUIITEMDOCUMENTTRANSACTION_H
