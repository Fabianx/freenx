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

"""Unittest for nxparser.base"""

import os
import StringIO
import sys
import unittest

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))

import nxlog
import nxparser.base

__author__ = 'diamond@google.com (Stephen Shirley)'
__copyright__ = 'Copyright 2008 Google Inc.'

class NxParserBaseUnitTest(unittest.TestCase):

  class MockStringIO(StringIO.StringIO):
    """Mock file object to check to log what's written and whether it's flushed"""

    def __init__(self, buf=None):
      if buf:
        StringIO.StringIO.__init__(self, buf)
      else:
        StringIO.StringIO.__init__(self)
      self.flushed = False

    def flush(self):
      StringIO.StringIO.flush(self)
      self.flushed = True

  def MockLog(self, *args):
    """Mock logging function to replace nxlog.log and store all arguments"""

    self.logged_data.append(args)

  def MockLogFlush(self, *args):
    """Wipe the MockLog() history"""

    self.logged_data = []

  def utilCheckInitialAttributes(self, p, input, output, version, program):
    """Check if the nxparser.base instance has initialised correctly"""

    self.assertEquals(input, p.input)
    self.assertEquals(output, p.output)
    self.assertEquals(105, p.state)
    self.assertEquals(True, p.running)
    self.assertEquals(version, p.version)
    self.assertEquals(program, p.program)
    self.assertEquals((nxlog.LOG_DEBUG,
      "Version: %(version)s Program: %(program)s" % locals()),
      self.logged_data.pop(0))
    self.assertEquals([], self.logged_data)

  def utilCheckOutput(self, out, exp_output, flush=True, log=True,
      log_level=nxlog.LOG_DEBUG):
    """Check if output has been written, flushed, and/or logged"""

    self.assertEquals(exp_output, out.getvalue())
    if flush:
      self.assertEquals(flush, out.flushed)
    if log:
      self.assertEquals((log_level, "Sent: %(exp_output)r\n" % locals()),
        self.logged_data.pop(0))
    else:
      self.assertEquals(0, len(self.logged_data))

  def setUp(self):
    """Before every test substitute Mocklog for nxlog.log"""

    self.nxlog_log_old = nxlog.log
    nxlog.log = self.MockLog
    self.MockLogFlush()

  def tearDown(self):
    """Restore the original nxlog.log"""

    nxlog.log = self.nxlog_log_old

  def testInitDefaults(self):
    """Does an instance with default args initialise correctly?"""

    p = nxparser.base.parser(sys.stdin, sys.stdout)

    self.utilCheckInitialAttributes(p, sys.stdin, sys.stdout, p.DEFAULT_VERSION,
      p.DEFAULT_PROGRAM)

  def testInitVersion(self):
    """Does an instance with an overridden version string initialise correctly?"""

    ver_name = "testverstring"
    p = nxparser.base.parser(sys.stdin, sys.stdout, version=ver_name)

    self.utilCheckInitialAttributes(p, sys.stdin, sys.stdout, ver_name,
      p.DEFAULT_PROGRAM)

  def testInitProgram(self):
    """Does an instance with an overridden program name initialise correctly?"""

    prog_name = "testprogstring"
    p = nxparser.base.parser(sys.stdin, sys.stdout, program=prog_name)

    self.utilCheckInitialAttributes(p, sys.stdin, sys.stdout, p.DEFAULT_VERSION,
      prog_name)

  def testInitVersionProgram(self):
    """Does an instance with an overridden program name and version string
    initialise correctly?"""

    ver_name = "testverstring"
    prog_name = "testprogstring"
    p = nxparser.base.parser(sys.stdin, sys.stdout,
        version=ver_name, program=prog_name)

    self.utilCheckInitialAttributes(p, sys.stdin, sys.stdout, ver_name, prog_name)

  def testBannerDefaults(self):
    """Does base.banner() output correctly with no arguments?"""

    out = self.MockStringIO()
    exp_output = "HELLO NXBASE - Version 3.0.0 - GPL\n"
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    p.banner()

    self.utilCheckOutput(out, exp_output)

  def testBannerVersionProgram(self):
    """Does base.banner() output correctly with a specified version string
    and program name?"""

    ver_name = "testverstring"
    prog_name = "testprogstring"
    exp_output = "HELLO %(prog_name)s - Version %(ver_name)s - GPL\n" % locals()
    out = self.MockStringIO()
    p = nxparser.base.parser(sys.stdin, out,
        version=ver_name, program=prog_name)
    self.MockLogFlush()

    p.banner()

    self.utilCheckOutput(out, exp_output)

  def testPromptDefaults(self):
    """Does base.prompt() output correctly with default arguments?"""

    state = 101
    out = self.MockStringIO()
    exp_output = "NX> %(state)d " % locals()
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    p.prompt(state)

    self.utilCheckOutput(out, exp_output)

  def testPromptMessage(self):
    """Does base.prompt() output correctly with a message?"""

    state = 101
    send = "Hello, World!"
    exp_output = "NX> %(state)d %(send)s\n" % locals()
    out = self.MockStringIO()
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    p.prompt(state, send)

    self.utilCheckOutput(out, exp_output)

  def testPromptOverrideNewlineTrueWithMsg(self):
    """Does base.prompt() output correctly with a message and forced newline?"""

    state = 101
    send = "Hello, World!"
    out = self.MockStringIO()
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    exp_output = "NX> %(state)d %(send)s\n" % locals()
    p.prompt(state, send, override_newline=True)
    self.utilCheckOutput(out, exp_output)

  def testPromptOverrideNewlineFalseWithMsg(self):
    """Does base.prompt() output correctly with a message and forced no newline?"""

    state = 101
    send = "Hello, World!"
    out = self.MockStringIO()
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    exp_output = "NX> %(state)d %(send)s" % locals()
    p.prompt(state, send, override_newline=False)
    self.utilCheckOutput(out, exp_output)

  def testPromptOverrideNewlineTrueWithoutMsg(self):
    """Does base.prompt() output correctly without a message and forced newline?"""

    state = 101
    out = self.MockStringIO()
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    exp_output = "NX> %(state)d \n" % locals()
    p.prompt(state, override_newline=True)
    self.utilCheckOutput(out, exp_output)

  def testPromptOverrideNewlineFalseWithoutMsg(self):
    """Does base.prompt() output correctly without a message and forced no newline?"""

    state = 101
    send = "Hello, World!"
    out = self.MockStringIO()
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    exp_output = "NX> %(state)d " % locals()
    p.prompt(state, override_newline=False)
    self.utilCheckOutput(out, exp_output)

  def testWriteDefaults(self):
    """Does base.write() output correctly with a message?"""

    send = "Hello, World!"
    exp_output = "%(send)s\n" % locals()
    out = self.MockStringIO()
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    p.write(send)

    self.utilCheckOutput(out, exp_output)

  def testWriteEmptyMsg(self):
    """Does base.write() output correctly with an empty message?"""

    send=""
    exp_output = "\n"
    out = self.MockStringIO()
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    p.write(send)

    self.utilCheckOutput(out, exp_output)

  def testWriteEmptyMsgNoNewline(self):
    """Does base.write() output correctly with an empty message and no newline?"""

    send = exp_output = ""
    out = self.MockStringIO()
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    p.write(send, newline=False)

    self.utilCheckOutput(out, exp_output)

  def testWriteNoNewline(self):
    """Does base.write() output correctly with a message and no newline?"""

    send = exp_output = "Hello, World!"
    out = self.MockStringIO()
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    p.write(send, newline=False)

    self.utilCheckOutput(out, exp_output)

  def testWriteNoFlush(self):
    """Does base.write() output correctly with a message and without flushing?"""

    send = "Hello, World!"
    exp_output = "%(send)s\n" % locals()
    out = self.MockStringIO()
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    p.write(send, flush=False)

    self.utilCheckOutput(out, exp_output, flush=False)

  def testWriteNoLog(self):
    """Does base.write() output correctly with a message and without logging?"""

    self.args = None
    send = "Hello, World!"
    exp_output = "%(send)s\n" % locals()
    out = self.MockStringIO()
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    p.write(send, log=False)

    self.utilCheckOutput(out, exp_output, log=False)

  def testWriteLogLevel(self):
    """Does base.write() output correctly with a message and a specific log level?"""

    self.args = None
    send = "Hello, World!"
    exp_output = "%(send)s\n" % locals()
    out = self.MockStringIO()
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    p.write(send, log_level=nxlog.LOG_ERR)

    self.utilCheckOutput(out, exp_output, log_level=nxlog.LOG_ERR)

  def testWriteFd(self):
    """Does base.write() output correctly with a message and a specific fd?"""

    self.args = None
    send = "Hello, World!"
    exp_output = "%(send)s\n" % locals()
    out = self.MockStringIO()
    p = nxparser.base.parser(sys.stdin, out)
    self.MockLogFlush()

    p.write(send, fd=out)

    self.utilCheckOutput(out, exp_output)

  def testParseArgsNoArgsDefaults(self):
    """Does base.parse_args() parse correctly with no extra args?"""

    sys.argv = ['argv0']
    p = nxparser.base.parser(sys.stdin, sys.stdout)

    p.parse_args()

    self.assertEquals(None, p.version)
    self.assertEquals(None, p.program)

  def testParseArgsNoArgsVersionProgram(self):
    """Does base.parse_args() parse correctly with no extra args and specified
    default version string and program name?"""

    sys.argv = ['argv0']
    ver_name = "testverstring"
    prog_name = "testprogstring"
    p = nxparser.base.parser(sys.stdin, sys.stdout)

    p.parse_args(version=ver_name, program=prog_name)

    self.assertEquals(ver_name, p.version)
    self.assertEquals(prog_name, p.program)

#Need to figure out how to test this:
#  def testParseArgsWithArgsDefaults(self):
#    sys.argv = ['argv0', '--help']
#    p = nxparser.base.parser(sys.stdin, sys.stdout)
#
#    p.parse_args()
#

  def testParseArgsWithArgsVersionProgram(self):
    """Does base.parse_args() parse correctly with version string and program 
    name specified by both args and defaults?"""

    ver_name = "testverstring"
    prog_name = "testprogstring"
    sys.argv = ['argv0', '--proto', ver_name, '--program', prog_name]
    p = nxparser.base.parser(sys.stdin, sys.stdout)

    p.parse_args()

    self.assertEquals(ver_name, p.version)
    self.assertEquals(prog_name, p.program)


if __name__ == '__main__':
  unittest.main()
