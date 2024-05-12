/* Copyright 2024 Grégoire Barbier and others.
 * This file is part of qron, see <http://qron.eu/>.
 * Qron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Qron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with qron. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef OPENSSHCOMMAND_H
#define OPENSSHCOMMAND_H

#include <QProcess>
#include "util/paramset.h"

class LIBP6CORESHARED_EXPORT OpensshCommand : public QProcess {
  Q_OBJECT
  Utf8String _command, _hostname;
  ParamSet _params;
  QMap<Utf8String,Utf8String> _env_vars;
  Utf8String _log_task;
  quint64 _log_exec_id;

public:
  explicit OpensshCommand(
      const Utf8String &command, const Utf8String &hostname = {},
      const ParamSet &params = {},
      const QMap<Utf8String,Utf8String> &env_vars = {},
      const Utf8String &log_task = {}, quint64 log_exec_id = 0,
      QObject *parent = 0);
  void start(ParamsProvider *params_evaluation_context = 0,
        const Utf8String &hostname = {});
  ParamSet params() const { return _params; }
};

#endif // OPENSSHCOMMAND_H
