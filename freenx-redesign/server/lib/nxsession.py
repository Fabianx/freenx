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

"""nxsession module for handling nx sessions."""

import copy
import glob
import md5
import os
import pwd
import socket
import time

import nxlog

DB_TOPLEVEL="/usr/freenx/var/sessdb"

state_names = ['created', 'starting', 'running', 'suspending',
    'suspended', 'terminating', 'terminated']

# FIXME(diamond): make sure all defaults are sane
default_params = {
    'agent_pid': None,
    'cache': None,
    'cookie': None,
    'depth': 24,
    'display': 1,
    'full_id': None,
    'fullscreen': None,
    'hostname': socket.getfqdn(),
    'id': None,
    'name': "No name given",
    'options': None,
    #FIXME(diamond): Not ipv6 compatible:
    'proxyip': socket.gethostbyname(socket.getfqdn()),
    'resolution': None,
    'ssl': '1',
    'state': state_names[0],
    'subscription': 'GPL',
    'type': None,
    'updated': None,
    'user': None}


def gen_uniq_id():
  """Generate a unique 32-character ID

  This uses uses md5 to create a hash of the hostname, the time, and the 
  process ID.

  Return:
    The generated ID
  """

  #FIXME(diamond): time.time() can return the same time multiple times if
  #called quickly enough, so we need to maybe keep a counter, add it
  return md5.md5(socket.getfqdn() + str(time.time()) +
      str(os.getpid())).hexdigest().upper()


class nxsession:
  """nxsession database

  This class handles parsing session parameters, and saving them into the
  session database"""

  def __init__(self, parameters):
    """nxsession constructor

    Args:
      parameters: A list of key=value strings
    """

    self.__set_vars(parameters)

  def __set_vars(self, parameters):
    """Set instances parameters from key=value string list

    Args:
      parameters: A list of key=value strings
    """
    self.params = copy.deepcopy(default_params)

    # Read values from supplied set
    for pair in parameters.split('\n'):
      if not pair: continue
      name,val = pair.split('=', 1)
      if name not in default_params:
        nxlog.log(nxlog.LOG_ERR, "Invalid session parameter passed in: %s" % pair)
      else:
        self.params[name] = val

    if not self.params['id']:
      self.params['id'] = gen_uniq_id()
    # Always generate full_id
    self.params['full_id'] = "%(hostname)s-%(display)s-%(id)s" % self.params
    if not self.params['cookie']:
      self.params['cookie'] = gen_uniq_id()
    if not self.params['user']:
      self.params['user'] = pwd.getpwuid(os.getuid())[0]
    if not self.params['cache']:
      type = self.params['type']
      if type.startswith('unix-'):
        type = type.split('-', 1)[0]
      self.params['cache'] = "cache-%s" % type

  def set_state(self, name):
    """Set the state of the session

    Does some sanity checking to make sure the new state is valid

    Args:
      name: Name of the state to set the session to.
    Returns:
      None
    """

    if name not in state_names:
      nxlog.log(nxlog.LOG_ERR, "Invalid state name passed in: %r" % name)
      # FIXME(diamond): handle error better
    else:
      self.params['state'] = name

  def save(self):
    """Save the session to the database

    Sets the 'updated' parameter while saving
    """

    sess_path = os.path.join(DB_TOPLEVEL, self.params['full_id'])
    sess_save_path = sess_path + '.saving'
    self.params['updated'] = int(time.time())
    f = open(sess_save_path, 'w')
    for name,val in self.params.iteritems():
      f.write('%s=%s\n' % (name, val))
    f.close()
    os.rename(sess_save_path, sess_path)

  def reload(self):
    self.__set_vars(_db_read_session(self.params['full_id']))

  def get_params(self):
    """Return an iterator over (key, value) the session parameters"""

    return self.params.iteritems()


def db_list():
  """List all sessions in the database"""

  return os.listdir(DB_TOPLEVEL)

def db_find_sessions(id=None, users=None, states=None, types=None):
  """Find sessions that match the specified parameters

  Args:
    id: 32-character unique session id
    users: List of session owners
    states: List of session states; 'running', 'suspended', etc

  Returns:
    A list of nxsession instances for any matching sessions in the database.
    If none are found, the list is empty.
  """

  found = []
  file_glob = "*"
  if id:
    file_glob += id

  for sess_full_id in map(os.path.basename,
      glob.glob(os.path.join(DB_TOPLEVEL, file_glob))):
    sess = db_get_session(sess_full_id)
    if users and sess.params['user'] not in users:
        continue
    if states and sess.params['state'] not in states:
        continue
    if types and sess.params['type'] not in types:
        continue
    found.append(sess)

  return found

def _db_read_session(full_id):
  """Read in specified session from the database

  Args:
    full_id: The full session id (hostname-displaynum-id)

  Returns:
    A string containing the contents of the session's entry in the database.
  """

  contents = open(os.path.join(DB_TOPLEVEL, full_id), 'r').read()
  return contents


def db_get_session(full_id):
  """Retrieve the specified session from the database

  Args:
    full_id: The full session id (hostname-displaynum-id)

  Returns:
    An instance of nxsession
  """

  return nxsession(_db_read_session(full_id))
