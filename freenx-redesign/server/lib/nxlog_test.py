#!/usr/bin/python2.4 -E
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

"""nxlog module unit tests for logging to syslog and stderr.

  NXLogUnitTest: Test log and set_log_level.
"""

import os
import syslog
import unittest

import nxlog

__author__ = 'alriddoch@google.com (Alistair Riddoch)'
__copyright__ = 'Copyright 2007 Google Inc.'


class NXLogUnitTest(unittest.TestCase):
  """Unit test for nxlog module."""

  def mock_syslog_openlog(self, ident, opt = None, facility = None):
    """Flag that this mock syslog function has been called"""

    self._openlog_called = True

  def mock_syslog_syslog(self, level, message = None):
    """Flag that this mock syslog function has been called"""

    self._syslog_called = True

  def setUp(self):
    """Install mock versions of the syscall functions."""

    self._saved_openlog = syslog.openlog
    self._saved_syslog = syslog.syslog
    syslog.openlog = self.mock_syslog_openlog
    syslog.syslog = self.mock_syslog_syslog
    self._openlog_called = False
    self._syslog_called = False

  def tearDown(self):
    """Remove the mocked versions of the syscall functions."""

    syslog.openlog = self._saved_openlog 
    syslog.syslog = self._saved_syslog 

  def testSetupSyslog(self):
    """Test the code path through the syslog setup function."""

    nxlog.__dict__['__setup_syslog']('syslog_test')

    self.failUnless(self._openlog_called)

  def testLogSyslog(self):
    """Test the code path through the syslog log function."""

    nxlog.__dict__['__log_syslog'](0, 'syslog_test_message')

    self.failUnless(self._syslog_called)

  def testSetupStderr(self):
    """Test the code path through the stderr setup function."""

    name = 'stderr_test_name'

    nxlog.__dict__['__setup_stderr'](name)

    self.assertEqual(name, nxlog.__dict__['__stderr_name'])

  def testLogStderr(self):
    """Test the code path through the stderr log function."""

    nxlog.__dict__['__log_stderr'](0, 'stderr_test_message')

  def testInternalSetup(self):
    """Test calling __setup()"""

    nxlog.__dict__['__setup']('setup_test_name')

    log_level = nxlog.__dict__['__level']

    self.failUnless(log_level in [nxlog.LOG_EMERG, nxlog.LOG_ALERT,
                                  nxlog.LOG_CRIT, nxlog.LOG_ERR,
                                  nxlog.LOG_WARNING, nxlog.LOG_NOTICE,
                                  nxlog.LOG_INFO, nxlog.LOG_DEBUG])

  def testInternalSetupEnvionment(self):
    """Test calling __setup() with a log level environment variable"""

    os.environ['LOG_LEVEL'] = 'ERR'

    nxlog.__dict__['__setup']('setup_test_name')

    log_level = nxlog.__dict__['__level']

    self.assertEqual(log_level, nxlog.LOG_ERR)

    os.unsetenv('LOG_LEVEL')

  def testLogLevelSet(self):
    """Test that the log level has been set to a valid value"""

    # Ensure that before set_log_level has been called, the internal value
    # of log level is sane.
    log_level = nxlog.__dict__['__level']

    self.failUnless(log_level in [nxlog.LOG_EMERG, nxlog.LOG_ALERT,
                                  nxlog.LOG_CRIT, nxlog.LOG_ERR,
                                  nxlog.LOG_WARNING, nxlog.LOG_NOTICE,
                                  nxlog.LOG_INFO, nxlog.LOG_DEBUG])

  def testLog(self):
    """Test calling log function"""

    nxlog.log(0, 'test log message')

  def testSetup(self):
    """Test calling external setup()"""

    nxlog.setup('nxlog_test')

  def testSetLogLevel(self):
    """Test the code path through set_log_level() with a valid level"""

    nxlog.set_log_level(nxlog.LOG_INFO)

  def testSetLogLevelRaises(self):
    """Test the code path through set_log_level() with an invalid level"""

    self.assertRaises(ValueError, nxlog.set_log_level, 'not_a_level')

  def testNameToLevelNameString(self):
    """Test calling _name_to_level with a string specifying a level"""

    self.assertEquals(nxlog.LOG_DEBUG, nxlog._name_to_level('DEBUG'))

  def testNametoLevelNumberString(self):
    """Test calling _name_to_level() with an integer represented as a string"""

    self.assertEquals(nxlog.LOG_DEBUG, nxlog._name_to_level('7'))

  def testNametoLevelInteger(self):
    """Test calling _name_to_level() with an integer."""

    self.assertEquals(nxlog.LOG_DEBUG, nxlog._name_to_level(nxlog.LOG_DEBUG))

  def testNametoLevelInvalidNameString(self):
    """Test calling _name_to_level() on a meaningless string."""

    self.assertRaises(ValueError, nxlog._name_to_level, 'BLARG')


if __name__ == '__main__':
  unittest.main()
