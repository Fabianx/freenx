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
# Authors: alriddoch@google.com (Alistair Riddoch)
#          diamond@google.com (Stephen Shirley)

"""nxparser.base module for handling the nx protocol.
"""

import optparse
import re
import traceback
import sys

import nxlog

__author__ = 'alriddoch@google.com (Alistair Riddoch)'
__copyright__ = 'Copyright 2007 Google Inc.'

class parser:
  """Base parser for NX protocol parsers.

  This class handles breaking up the messages into components,
  and dispatching them.
  """

  DEFAULT_VERSION = '3.0.0'
  DEFAULT_PROGRAM = 'NXBASE'

  NX_PROMPT = 'NX>'
  NX_COMMANDS = ['hello', 'login', 'bye', 'set', 'listsession',
                 'restoresession', 'startsession', 'terminate']
  NX_PARAM_RX = re.compile(r'^--(?P<key>[a-z]+)="(?P<value>.+)"')

  def __init__(self, input, output, version=DEFAULT_VERSION,
               program=DEFAULT_PROGRAM):
    """base_parser constructor

    Args:
      input: The file object to read parser input from.
      output: The file object to write the results to.
      version: The version string used to negotiate the protocol.
      program: The program name used in the protocol banner.
    """

    self.input = input
    self.output = output
    self.state = 105
    self.running = True
    self.parse_args(version=version, program=program)
    nxlog.log(nxlog.LOG_DEBUG, "Version: %s Program: %s" %
        (self.version, self.program))

  def banner(self):
    """Write the protocol banner to the output."""

    self.write('HELLO %s - Version %s - GPL' % (self.program, self.version))

  def prompt(self, state, message='', override_newline=None):
    """Write the protocol prompt to the output.

    If no message is given, just print the prompt & state, no newline.
    If a message is provided, by default append a newline.

    Args:
      state: The state number to put after the NX> prompt
      message: Optional message to print after the state
      override_newline: Optional param to force a trailing newline on/off"""


    newline = False
    if override_newline is not None:
      newline = override_newline
    elif message:
      newline=True
    self.write('%s %d %s' % (self.NX_PROMPT, state, message), newline=newline)

  def loop(self):
    """Write the protocol prompt to the output, and accept commands."""

    try:
      while self.running:
        self.prompt(self.state)
        line = self.input.readline()
        if not line:
          nxlog.log(nxlog.LOG_DEBUG, "Exiting due to EOF")
          return
        line = line.rstrip()
        nxlog.log(nxlog.LOG_DEBUG, 'Got %r' % line)
        command = line.split()
        if not command:
          # If the line was all whitespace this could happen.
          continue
        cmd = command[0].lower()
        if cmd == 'set':
          self.write("%s %s: %s" % (cmd.capitalize(), command[1].lower(),
            command[2].lower()))
        elif cmd == 'startsession':
          self.write("Start session with: %s" % " ".join(command[1:]))
        else:
          self.write(line.capitalize())
        if cmd not in self.NX_COMMANDS:
          self.prompt(503, 'Error: undefined command: \'%s\'' % cmd)
          continue
        handler_name = '_nx_%s_handler' % cmd
        try:
          handler_method = getattr(self, handler_name)
        except AttributeError:
          nxlog.log(nxlog.LOG_DEBUG, 'Unhandled nx command %r' % cmd)
          continue
        handler_method(command)
    except IOError, e:
      nxlog.log(nxlog.LOG_ERR, 'IOError. Connection lost: %s' % e)
    except Exception, e:
      trace = traceback.format_exc()
      nxlog.log(nxlog.LOG_ERR, 'Going down because exception caught '
                               'at the top level.')
      for line in trace.split('\n'):
        nxlog.log(nxlog.LOG_ERR, '%s' % line)
      self.prompt(500, 'Error: Fatal error in module %s, '
          'check log file for more details.' % self.program.lower())
      self.prompt(999, 'Bye.')

  def write(self, output, newline=True, flush=True, log=True,
      log_level=nxlog.LOG_DEBUG, fd=None):
    """Write given string to output, and optionally:
      - append a newline
      - flush output afterwards
      - log the output, with a specified log level."""

    if newline:
      output += '\n'
    use_fd = self.output
    if fd:
      use_fd = fd
    use_fd.write(output)
    if flush:
      use_fd.flush()
    if log:
      nxlog.log(log_level, 'Sent: %r\n' % output)

  def parse_args(self, version=None, program=None):
    """Parse cmdline arguments"""

    optparser = optparse.OptionParser()
    optparser.add_option("--proto", action="store", type="string",
        dest="version", default=version, metavar="PROTO_VER",
        help="use the PROTO_VER version of the NX protocol")
    optparser.add_option("--program", action="store", type="string",
        dest="program", default=program, metavar="PROG_NAME",
        help="the PROG_NAME name to announce")
    options, args = optparser.parse_args()
    self.version = options.version
    self.program = options.program

  def _diff_version(self, ver1, ver2):
    """Compare 2 version strings"""

    for i,j in zip(ver1.split('.', 2), ver2.split('.', 2)):
      try:
        icomp = int(i)
        jcomp = int(j)
      except ValueError:
        icomp = i
        jcomp = j
      if icomp > jcomp:
        return 1
      elif icomp == jcomp:
        continue
      elif icomp < jcomp:
        return -1
    return 0

  def _parse_param(self, param):
    """Check that param is correctly formatted via the NX_PARAM_RX regex

    Args:
      param: parameter string to be checked, of the form --key="value"

    Returns:
      key,value tuple if param was correctly formatted, returns None,None
      otherwise.
    """

    m = self.NX_PARAM_RX.search(param)
    if m:
      key = m.group('key')
      value = m.group('value')
      nxlog.log(nxlog.LOG_DEBUG, 'Param matched: %r=%r' % (key, value))
    else:
      key = value = None
      nxlog.log(nxlog.LOG_WARNING, "Param didn't match: %r" % param)
    return key,value


if __name__ == '__main__':
  print 'This is a library. Please use it from python using import.'
  sys.exit(0)
