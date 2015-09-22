/* Copyright 2015 Hallowyn and others.
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
#ifndef COREUNDOCOMMAND_H
#define COREUNDOCOMMAND_H

#include <QString>
#include <QList>

/** Like QUndoCommand without QtWidget and QtGui dependency.
 * @see QUndoCommand */
class CoreUndoCommand {
  Q_DISABLE_COPY(CoreUndoCommand)

  CoreUndoCommand *_parent;
  QString _text;
  QList<CoreUndoCommand*> _children;

public:
  explicit CoreUndoCommand(CoreUndoCommand *parent = 0)
    : CoreUndoCommand(QString(), parent) { }
  explicit CoreUndoCommand(const QString &text, CoreUndoCommand *parent = 0)
    : _parent(parent), _text(text) {
    if (parent)
      parent->_children.append(this);
  }
  virtual ~CoreUndoCommand();
  QString	actionText() const;
  const CoreUndoCommand *child(int index) const {
    return (index >= 0 && index < _children.size()) ? _children.at(index) : 0; }
  int	childCount() const { return _children.size(); }
  void setText(const QString &text) { _text = text; }
  QString text() const { return _text; }
  virtual void redo();
  virtual void undo();
  virtual int id() const;
  virtual bool mergeWith(const CoreUndoCommand *command);
};

#endif // COREUNDOCOMMAND_H
