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

"""nxsession module unit tests

  GenUniqIdTest: Test gen_uniq_id().
  NXSessionUnitTest: Test nxsession.
"""

import time
import unittest

import nxsession

__author__ = 'alriddoch@google.com (Alistair Riddoch)'
__version__ = '$Revision: #4 $'
__copyright__ = 'Copyright 2008 Google Inc. All rights reserved.'


class GlobalsTest(unittest.TestCase):
  """Unit test for global variables in nxsession"""

  def testStateNames(self):
    """Test state names is not empty"""

    self.failUnless(nxsession.state_names)

  def testDefaultParams(self):
    """Test that default params is not empty"""

    # In later tests we use the contents without bounds checking
    # This is a canary to making those tests easier to debug if
    # something changes.
    self.failUnless(nxsession.default_params)


class GenUniqIdTest(unittest.TestCase):
  """Unit test for nxsession.gen_uniq_id"""

  def testGetUniqId(self):
    """Test case for nxsession.gen_uniq_id function"""

    self.failUnless(nxsession.gen_uniq_id())

  def testGetUniqIdUniq(self):
    """Test case for nxsession.gen_uniq_id to ensure ID is unique"""

    first = nxsession.gen_uniq_id()

    time.sleep(1)

    second = nxsession.gen_uniq_id()

    self.assertNotEqual(first, second)


class NXSessionUnitTest(unittest.TestCase):
  """Unit test for nxsession.nxsession"""

  def setUp(self): pass

  def tearDown(self): pass

  def testConstructor(self):
    """Test calling the nxsession constructor"""

    nxsession.nxsession('')

  def testConstructorBadParams(self):
    """Test calling the nxsession constructor with params containing no ="""

    self.assertRaises(ValueError, nxsession.nxsession, 'foo')

  def testConstructorUnknownParams(self):
    """Test calling the nxsession constructor with param not in defaults"""

    nxsession.nxsession('foo=bar')

  def testConstructorGoodParam(self):
    """Test calling the nxsession constructor with param which is ok"""

    key = nxsession.default_params.keys()[0]

    nxsession.nxsession('%s=bar' % key)

  def testConstructorSetsUpId(self):
    """Test an ID parameter is set up by the constructor"""

    o = nxsession.nxsession('')

    self.failUnless('id' in o.params)

  def testConstructorSetsUpId(self):
    """Test a default ID parameter is set up by the constructor"""

    o = nxsession.nxsession('')

    self.failUnless('id' in o.params)

  def testConstructorAcceptsId(self):
    """Test an ID parameter is accepted in those passed to constructor"""

    id = '23'

    o = nxsession.nxsession('id=%s' % id)

    self.assertEquals(id, o.params['id'])

  def testConstructorPopulatesFullId(self):
    """Test a full_id parameter is set up by the constructor"""

    o = nxsession.nxsession('')

    self.failUnless('full_id' in o.params)

  def testConstructorPopulatesReplacesId(self):
    """Test a full_id parameter provided in vars is overriden"""

    test_full_id = 'hostname-1-ID'

    o = nxsession.nxsession('full_id=%s' % test_full_id)

    self.failUnless('full_id' in o.params)
    self.assertNotEqual(o.params['full_id'], test_full_id)

  def testConstructorPopulatesCookie(self):
    """Test a cookie parameter is set up"""

    o = nxsession.nxsession('')

    self.failUnless('cookie' in o.params)

  def testConstructorCookieIsUniqId(self):

    old_id = nxsession.gen_uniq_id()

    time.sleep(1)

    # Make sure the id from the generator is now different
    self.assertNotEqual(old_id, nxsession.gen_uniq_id())

    o = nxsession.nxsession('')

    self.failUnless('cookie' in o.params)
    self.assertNotEqual(old_id, o.params['cookie'])

  def testConstructorAcceptsCookie(self):
    """Test a cookie paramete is read from vars"""

    cookie = nxsession.gen_uniq_id()

    time.sleep(1)

    # Make sure the id from the generator is now different
    self.assertNotEqual(cookie, nxsession.gen_uniq_id())

    o = nxsession.nxsession('cookie=%s' % cookie)

    self.failUnless('cookie' in o.params)
    self.assertEqual(cookie, o.params['cookie'])
    
  def testConstructorSetUser(self):
    """Test a user parameter is set by constructor"""

    o = nxsession.nxsession('')

    self.failUnless('user' in o.params)

  def testConstructorAcceptsUser(self):
    """Test a user paramater specified in vars is not replaced"""

    username = 'dummy_user'

    o = nxsession.nxsession('user=%s' % username)

    self.failUnless('user' in o.params)
    self.assertEquals(username, o.params['user'])

  def testSetState(self):
    """Test setting state on a session"""

    o = nxsession.nxsession('')

    o.set_state(nxsession.state_names[0])

  def testSetStateAllStates(self):

    o = nxsession.nxsession('')

    for state in nxsession.state_names:

      o.set_state(state)

      self.assertEquals(o.params['state'], state)

  def testSetInvalidState(self):

    o = nxsession.nxsession('')

    state = 'invalud_state_name'

    o.set_state(state)

    self.assertNotEquals(o.params['state'], state)

if __name__ == '__main__':
  unittest.main()
