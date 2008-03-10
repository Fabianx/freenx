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
# Author: alriddoch@google.com (Alistair Riddoch)

"""nxlog module for logging to syslog and stderr.

  __setup_syslog: Function for setting up logging to syslog.
  __log_syslog: Function for logging to syslog.
  __setup_stderr: Function for setting up logging to stderr.
  __log_stderr: Function for logging to stderr.
  __setup: Function for setting up the logging systems.

  setup: Function for setting the faciltiy name.
  set_log_level: Function for setting the log level.
  log: Function for writing logs.
"""

import os
import sys
import syslog

__author__ = 'alriddoch@google.com (Alistair Riddoch)'
__copyright__ = 'Copyright 2007 Google Inc.'

__pychecker__ = 'no-miximport'

from syslog import LOG_EMERG
from syslog import LOG_ALERT
from syslog import LOG_CRIT
from syslog import LOG_ERR
from syslog import LOG_WARNING
from syslog import LOG_NOTICE
from syslog import LOG_INFO
from syslog import LOG_DEBUG


__level = LOG_INFO
__handlers = []
__stderr_name = ''
_level_names = {
    'EMERG': LOG_EMERG,
    'ALERT': LOG_ALERT,
    'CRIT': LOG_CRIT,
    'ERR': LOG_ERR,
    'WARNING': LOG_WARNING,
    'NOTICE': LOG_NOTICE,
    'INFO': LOG_INFO,
    'DEBUG': LOG_DEBUG
  }



def __setup_syslog(name):
  """Initialize logging to the syslog.
  
  Args:
    name: The name to be used for the logging facility.

  Returns:
    None
  """

  syslog.openlog(name, syslog.LOG_PID)


def __log_syslog(level, message):
  """Write a message to syslog.

  Args:
    level: Number giving the level of this message.
    message: String giving the message to be logged.

  Returns:
    None
  """

  syslog.syslog(level, message)


def __setup_stderr(name):
  """Initialize loggin to stderr.
  
  Args:
    name: The name to be used for the logging facility.

  Returns:
    None
  """
  global __stderr_name

  __stderr_name = name


def __log_stderr(level, message):
  """Write a message to stderr.

  Args:
    level: Number giving the level of this message.
    message: String giving the message to be logged.

  Returns:
    None
  """
  global __stderr_name
  global __level

  if level <= __level:
    sys.stderr.write('%s: %s' % (__stderr_name, message))


def __setup(name):
  """Set up the library.
  
  Args:
    name: The name to be used for the logging facility.

  Returns:
    None
  """
  global __level

  level = os.getenv('LOG_LEVEL')
  if level is not None:
    __level = _name_to_level(level)

  __setup_syslog(name)
  __handlers.append(__log_syslog)

#  __setup_stderr(name)
#  __handlers.append(__log_stderr)


def setup(name):
  """Set the facility name for logging.

  Note: this can be called multiple times, to change the name.

  Args:
    name: The name to be used for the logging facility.

  Returns:
    None
  """

  __setup_syslog(name)
  __setup_stderr(name)
  

def set_log_level(level):
  """Set the log level.

  Args:
    level: Number giving the highest level to log at.

  Returns:
    None
  """
  global __level

  __level = _name_to_level(level)


def log(level, message):
  """Handle a log message with a given level.

  If the message is high enough level to be written, pass it to the
  configured log channels.

  Args:
    level: Integer or string giving the level of this message.
    message: String giving the message to be logged.

  Returns:
    None
  """
  global __level

  lev_num = _name_to_level(level)

  if lev_num <= __level:
    for handler in __handlers:
      handler(lev_num, message)


def _name_to_level(level):
  """Translate from level number or name to log level

  Args:
    level: Integer, number string, or name string for log level

  Returns:
    The corresponding log level
  """
  try:
    return int(level)
  except ValueError:
    pass
  # The value is not a number
  try:
    return _level_names[level]
  except KeyError:
    # ValueError makes more sense to raise for an invalid name
    raise ValueError('invalid name for log level: %s' % level)


if __name__ == '__main__':
  print 'This is a library. Please use it from python using import.'
  sys.exit(0)
else:
  __setup(__name__)

