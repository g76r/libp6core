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
#include "coreundocommand.h"
//#include <QtDebug>

CoreUndoCommand::~CoreUndoCommand() {
  qDeleteAll(_children);
}

void CoreUndoCommand::redo() {
  //qDebug() << "CoreUndoCommand::redo" << _children.size();
  foreach (CoreUndoCommand *child, _children)
    child->redo();
}

void CoreUndoCommand::undo() {
  foreach (CoreUndoCommand *child, _children)
    child->undo();
}

int	CoreUndoCommand::id() const {
  return -1;
}

bool CoreUndoCommand::mergeWith(const CoreUndoCommand *command) {
  Q_UNUSED(command)
  return false;
}
