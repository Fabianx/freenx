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

"""nxloadconfig module unit tests"""

import copy
import unittest

import nxlog
import nxloadconfig

__author__ = 'diamond@google.com (Stephen Shirley)'
__copyright__ = 'Copyright 2007 Google Inc.'

nxlog.setup("nxloadconfig_test")


class NXLoadConifgUnitTest(unittest.TestCase):
  """Unit test for nxloadconfig module."""

  def setUp(self):
    # Not actually needed for every test, but it handy to have.
    self.conf = nxloadconfig.conf
    self.orig_conf = copy.copy(self.conf)

  def tearDown(self):
    # Cleanup after ourselves. See note on setUp()
    nxloadconfig.copy = copy.copy(self.orig_conf)

  def testWhich(self):
    self.assertEquals('/bin/bash', nxloadconfig.which("bash"))
    self.assertEquals('', nxloadconfig.which("somethingwhichdoesntexist"))

  def testCheckCommandVar(self):
    # Var is unset, default value is valid
    if 'TEST' in self.conf: del self.conf['TEST']
    ret = nxloadconfig.check_command_var("TEST", "/bin/bash")
    self.assertEquals(True, ret)
    self.assertEquals("/bin/bash", self.conf['TEST'])

    # Var points to unexecutable file, default value is valid.
    nxloadconfig.copy = copy.copy(self.orig_conf)
    self.conf['TEST'] = '/etc/fstab'
    ret = nxloadconfig.check_command_var("TEST", "/bin/bash")
    self.assertEquals(True, ret)
    self.assertEquals("/bin/bash", self.conf['TEST'])

    # Var points to executable dir, default value is valid.
    nxloadconfig.copy = copy.copy(self.orig_conf)
    self.conf['TEST'] = '/bin'
    ret = nxloadconfig.check_command_var("TEST", "/bin/bash")
    self.assertEquals(True, ret)
    self.assertEquals("/bin/bash", self.conf['TEST'])

    # Test valid value with valid default
    nxloadconfig.copy = copy.copy(self.orig_conf)
    self.conf['TEST'] = '/bin/sh'
    ret = nxloadconfig.check_command_var("TEST", "/bin/bash")
    self.assertEquals(True, ret)
    self.assertEquals("/bin/sh", self.conf['TEST'])

    # Test invalid value with invalid default
    nxloadconfig.copy = copy.copy(self.orig_conf)
    self.conf['TEST'] = '/etc/fstab'
    ret = nxloadconfig.check_command_var("TEST", "/etc/hosts")
    self.assertEquals(False, ret)
    self.assertEquals("/etc/fstab", self.conf['TEST'])


  def testCheckDirVar(self):
    # Var is unset, default value is valid
    if 'TEST' in self.conf: del self.conf['TEST']
    ret = nxloadconfig.check_dir_var("TEST", "/bin")
    self.assertEquals(True, ret)
    self.assertEquals("/bin", self.conf['TEST'])

    # Var points to file, default value is valid.
    nxloadconfig.copy = copy.copy(self.orig_conf)
    self.conf['TEST'] = '/etc/fstab'
    ret = nxloadconfig.check_dir_var("TEST", "/bin")
    self.assertEquals(True, ret)
    self.assertEquals("/bin", self.conf['TEST'])

    # Test valid value with valid default
    nxloadconfig.copy = copy.copy(self.orig_conf)
    self.conf['TEST'] = '/bin'
    ret = nxloadconfig.check_dir_var("TEST", "/usr")
    self.assertEquals(True, ret)
    self.assertEquals("/bin", self.conf['TEST'])

    # Test invalid value with invalid default
    nxloadconfig.copy = copy.copy(self.orig_conf)
    self.conf['TEST'] = '/etc/fstab'
    ret = nxloadconfig.check_dir_var("TEST", "/etc/hosts")
    self.assertEquals(False, ret)
    self.assertEquals("/etc/fstab", self.conf['TEST'])


if __name__ == '__main__':
  unittest.main()
