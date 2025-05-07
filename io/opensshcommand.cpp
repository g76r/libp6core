/* Copyright 2024-2025 Grégoire Barbier and others.
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
#include "opensshcommand.h"
#include "log/log.h"
#include "util/paramsprovidermerger.h"

OpensshCommand::OpensshCommand(
    const Utf8String &command, const Utf8String &hostname,
    const ParamSet &params, const QMap<Utf8String, Utf8String> &env_vars,
    const Utf8String &log_task, quint64 log_exec_id, QObject *parent)
  : QProcess(parent), _command(command), _hostname(hostname), _params(params),
    _env_vars(env_vars), _log_task(log_task), _log_exec_id(log_exec_id) {
}

void OpensshCommand::start(
    ParamsProvider *context, const Utf8String &start_hostname) {
  auto ppm = ParamsProviderMerger(context)(_params);

  QStringList cmdline, sshCmdline;
  const auto raw_hostname =
      start_hostname | _params.paramRawUtf8("ssh.hostname") | _hostname;
  const auto hostname = (raw_hostname % ppm).toString();
  const auto username = _params.paramUtf8("ssh.username", context);
  const auto port = _params.paramNumber<int>("ssh.port", -1, context);
  const auto ignoreknownhosts =
      _params.paramBool("ssh.ignoreknownhosts", true, context);
  const auto identity = _params.paramUtf8("ssh.identity", context);
  const auto options = _params.paramUtf8("ssh.options", context)
                       .split(Utf8String::AsciiWhitespace, Qt::SkipEmptyParts);
  const auto disablepty = _params.paramBool("ssh.disablepty", false, context);
  const auto shell = _params.paramUtf8("command.shell", context);
  sshCmdline << "-oLogLevel=ERROR" << "-oEscapeChar=none"
             << "-oServerAliveInterval=10" << "-oServerAliveCountMax=3"
             << "-oIdentitiesOnly=yes" << "-oKbdInteractiveAuthentication=no"
             << "-oBatchMode=yes" << "-oConnectionAttempts=3"
             << "-oTCPKeepAlive=yes" << "-oPasswordAuthentication=false";
  if (!disablepty)
    sshCmdline << "-t" << "-t";
  if (ignoreknownhosts)
    sshCmdline << "-oUserKnownHostsFile=/dev/null"
               << "-oGlobalKnownHostsFile=/dev/null"
               << "-oStrictHostKeyChecking=no";
  if (port > 0 && port < 65536)
    sshCmdline << "-oPort="+QString::number(port);
  if (!identity.isEmpty())
    sshCmdline << "-oIdentityFile=" + identity;
  for (const auto &option: options)
    sshCmdline << "-o" + option;
  if (!username.isEmpty())
    sshCmdline << "-oUser=" + username;
  sshCmdline << "--";
  sshCmdline << hostname;
  for (const auto &[key, value]: _env_vars.asKeyValueRange())
    cmdline << key+"='"+value.remove('\'')+"'";
  if (!shell.isEmpty()) {
    cmdline << shell << "-c";
    // must quote command line because remote user default shell will parse and
    // interpretate it and we want to keep it as is in -c argument to choosen
    // shell
    cmdline << '\'' + PercentEvaluator::eval_utf16(_command, &ppm)
               .replace("'", "'\\''")+ '\'';
  } else {
    // let remote user default shell interpretate command line
    cmdline << PercentEvaluator::eval_utf16(_command, &ppm);
  }
  Log::info(_log_task, _log_exec_id)
      << "exact command line to be executed (through ssh on host "
      << hostname <<  "): " << cmdline;
  sshCmdline << cmdline;
  QProcess::start("ssh", sshCmdline);
}
