#!/usr/bin/python2.4
#
# Copyright 2007 Google Inc.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
# Author: diamond@google.com (Stephen Shirley)

"""nxloadconfg module"""

__author__ = 'diamond@google.com (Stephen Shirley)'
__copyright__ = 'Copyright 2007 Google Inc.'

import os
import subprocess
import sys

import nxlog


__default_prefix = '/usr/freenx'
__conf_errors = False

conf={'PATH_BASE': __default_prefix,
  'PATH_ETC': os.path.join(__default_prefix, 'etc'),
  'PATH': os.getenv('PATH')}


def setup(conf_file=None):
  global __conf_errors
  __conf_errors = False

  __conf_load("general")
  if conf_file: __conf_load(conf_file)
  __check_command_vars()
  __check_dir_vars()

  if __conf_errors:
    nxlog.log(nxlog.LOG_CRIT, "Configuration errors, exiting\n")
    sys.exit(1)


def __conf_load(conf_file):
  conf_file_path = os.path.join(conf["PATH_ETC"], "%s.conf" % conf_file)
  if not os.path.exists(conf_file_path):
    nxlog.log(nxlog.LOG_DEBUG, "Requested file %s doesn't exist\n" % conf_file_path)
    return
  for line in subprocess.Popen('nxloadconfig-helper.sh %s' % conf_file_path,
    shell=True, stdout=subprocess.PIPE, env=conf).stdout:
    var, val = line.split('=')
    conf[var] = val.rstrip()


def __check_command_vars():
  check_command_var("COMMAND_START_KDE", which("startkde"))
  check_command_var("COMMAND_START_GNOME", which("gnome-session"))
  check_command_var("COMMAND_START_CDE", which("cdwm"))
  check_command_var("COMMAND_XTERM", which("xterm"))
  check_command_var("COMMAND_XAUTH", which("xauth"))
  check_command_var("COMMAND_SMBMOUNT", which("smbmount"))
  check_command_var("COMMAND_SMBUMOUNT", which("smbumount"))
  check_command_var("COMMAND_NETCAT", which("netcat"))
  check_command_var("COMMAND_SSH", which("ssh"))
  check_command_var("COMMAND_SSH_KEYGEN", which("ssh-keygen"))
  check_command_var("COMMAND_CUPSD", which("cupsd"))
  check_command_var("COMMAND_MD5SUM", which("md5sum"))


def __check_dir_vars():
  check_dir_var("PATH_BASE", conf.get("PATH_BASE"))
  check_dir_var("PATH_BIN", os.path.join(conf.get("PATH_BASE"), "bin"))
  check_dir_var("PATH_ETC", os.path.join(conf.get("PATH_BASE"), "etc"))
  check_dir_var("PATH_LIB", os.path.join(conf.get("PATH_BASE"), "lib"))


def check_command_var(varname, defval):
  def is_valid(path):
    try:
      cmd = path.split()[0]
    except IndexError:
      # Can happen if path is None, or "" etc
      cmd = path
    return cmd is not None and os.path.isfile(cmd) and os.access(cmd, os.X_OK)

  global __conf_errors
  varval = conf.get(varname)

  if varval is not None: # Is there a value set already?
    if is_valid(varval):
      return True # Everything checks out.
    else:
      nxlog.log(nxlog.LOG_WARNING, "Invalid command variable %s: \"%s\"\n" %
          (varname, varval))

  if defval == "" or is_valid(defval):
    # Everything ok now, we assume it's blank if deliberately unset
    # (or if the command isn't available
    conf[varname] = defval
    return True
  else:
    nxlog.log(nxlog.LOG_ERR, "Invalid default command variable %s: \"%s\"\n" %
        (varname, defval))
    __conf_errors = True
    return False


def check_dir_var(varname, defval):
  def is_valid(path):
    return path is not None and os.path.isdir(path)

  global __conf_errors
  varval = conf.get(varname)

  if varval is not None: # Is there a value set already?
    if is_valid(varval):
      return True # Everything checks out.
    else:
      nxlog.log(nxlog.LOG_WARNING, "Invalid directory variable %s: \"%s\"\n" %
          (varname, varval))

  if is_valid(defval):
    # Everything ok now
    conf[varname] = defval
    return True
  else:
    nxlog.log(nxlog.LOG_ERR, "Invalid default directory variable %s: \"%s\"\n" %
        (varname, defval))
    __conf_errors = True
    return False


def which(cmd):
  return os.popen("which %s" % cmd).read().rstrip()


if __name__ == '__main__':
  print 'This is a library. Please use it from python using import.'
  sys.exit(0)
