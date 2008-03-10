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
# Authors: diamond@google.com (Stephen Shirley)
#          alriddoch@google.com (Alistair Riddoch)

import errno
import os
import re
import signal
import socket
import sys
import termios
import time
import traceback

import nxlog
import nxparser
import nxsession

class parser(nxparser.base.parser):
  """Server parser for NX protocol.

  This class handles the NX protocol messages required by a server.
  """

  DEFAULT_PROGRAM = 'NXSERVER'

  STATUS_CONNECTED, \
  STATUS_LOGGEDIN = range(2)

  SET_ACCEPTED_VARIABLES = ['AUTH_MODE', 'SHELL_MODE']

  LISTSESSION_ACCEPTED_PARAMETERS = ['user', 'status', 'geometry', 'type']
  LISTSESSION_COLUMNS = [["Display", 7, "display"], ["Type", 16, "type"],
      ["Session ID", 32, "id"], ["Options", 8, "options"],
      ["Depth", -5, "depth"], ["Screen", 14, "resolution"],
      ["Status", 11, "state"], ["Session Name", 30, 'name']]

  STARTSESSION_ACCEPTED_PARAMETERS = [ 'backingstore', 'cache', 'client',
                                       'composite', 'encryption', 'geometry',
                                       'images', 'keyboard', 'link',
                                       'media', 'screeninfo' 'session',
                                       'shmem', 'shpix', 'strict', 'type', ]

  def __init__(self, input, output, version=nxparser.base.parser.DEFAULT_VERSION,
               program=DEFAULT_PROGRAM):
    """server_parser constructor

    Args:
      input: The file object to read parser input from.
      output: The file object to write the results to.
      version: The version string used to negotiate the protocol.
      program: The program name used in the protocol banner.
    """
    nxparser.base.parser.__init__(self, input, output, version=version, program=program)
    self.port = 0

    username = os.getenv('NX_TRUSTED_USER')

    if username:
      self.status = self.STATUS_LOGGEDIN
      self.username = username
    else:
      self.status = self.STATUS_CONNECTED

    commfd = os.getenv('NX_COMMFD')

    if commfd:
      self.nxnode_commfd = int(commfd)
      self.nxnode_rfile = os.fdopen(self.nxnode_commfd, 'r')
      self.nxnode_wfile = os.fdopen(self.nxnode_commfd, 'w')
      nxlog.log(nxlog.LOG_DEBUG, 'Got commfd %d\n' % self.nxnode_commfd)

  def __del__(self):
    """Destructor for the server_parser class

    This is needed to cleanup after server_parser is done. In particular,
    the file descriptors used for comms with nxnode may be in an errored state
    if nxnode has exited.
    """

    for i in ['r', 'w']:
      var = "nxnode_%sfile" % i
      try:
        getattr(self, var).close()
      except IOError, e:
        if e.args[0] != errno.EBADF:
          nxlog.log(nxlog.LOG_WARNING, "Got error closing %s: %s\n" %
              (var, e))
      except AttributeError:
        pass # self.nxnode_(r|w)file doesn't exist

  def banner(self):
    """Write the protocol banner to the output."""

    if self.status == self.STATUS_LOGGEDIN:
      assert(hasattr(self, 'username'))
      self.prompt(103, 'Welcome to: %s user: %s' % (socket.getfqdn().lower(),
                                                      self.username))
    else:
      self.write('HELLO %s - Version %s - GPL' % (self.program, self.version))

  def _nx_bye_handler(self, unused_command):
    """Handle the bye NX command.

    'bye' signals the end of a commandline session. Currently it is ignored
    as the other end typically closes the connection, causing termination.
    It may be a good idea to explicitly exit.

    Args:
      command: The NX command and arguments invoked

    Returns:
      None
    """

    # Basic checking of the right status
    if self.status < self.STATUS_LOGGEDIN:
      self.prompt(554, 'Error: the command \'bye\' cannot '
          'be called before login')
      return
    self.prompt(999, 'Bye.')
    if self.port != 0:
      os.execve("/bin/netcat", ["netcat", "localhost", str(self.port)], {})

  def _nx_hello_handler(self, command):
    """Handle the hello NX command.

    'hello' is used to handshake the commandline session, and appears to
    support negotiation of the protocol version. We currently only
    accept versions which look like 3.x.x, matched using a regex.

    Args:
      command: The NX command and arguments invoked

    Returns:
      None
    """

    # Basic checking of the right status
    if self.status >= self.STATUS_LOGGEDIN:
      self.prompt(554, 'Error: the command \'hello\' cannot '
          'be called after login')
      return
    if len(command) < 5:
      nxlog.log(nxlog.LOG_DEBUG, 'Hello too short')
      return
    if not re.match('^3(\.[0-9]+(\.[0-9]+))', command[4]):
      nxlog.log(nxlog.LOG_DEBUG, 'Version too fucked')
      self.prompt(552, 'Protocol you requested is not supported')
      return
    # If the proffered version is the same as ours, or older..
    if self._diff_version(self.version, command[4]) < 1:
      accept_ver = self.version
    else:
      accept_ver = command[4]
    self.prompt(134, 'Accepted protocol: %s' % accept_ver)

  def _nx_login_handler(self, unused_command):
    """Handle the login NX command.

    'login' is used to start the process of authenticating to NX. The username
    and password is send in response to requests from the server. If no
    options have been set, nxserver will ask for the password, and will then
    ask for an 'MD5 Password' if no password is given. If
    'SET AUTH_MODE PASSWORD' has been sent by the client, it does not do this
     however. This code currently never requests the MD5 Password. It is
    possible that making use of this feature requires storing the users
    password and verifying it with the hash.

    Args:
      command: The NX command and arguments invoked

    Returns:
      None
    """

    # Basic checking of the right status
    if self.status >= self.STATUS_LOGGEDIN:
      self.prompt(554, 'Error: the command \'login\' cannot be '
          'called after login')
      return
    self.prompt(101, 'User: ', override_newline=False)
    line = self.input.readline()
    split_line = line.split()
    if not line or len(split_line) != 1:
      self.prompt(500, 'Error: Username is not in expected format')
      return
    self.write('') # Print newline after username
    self.username = split_line[0]
    nxlog.log(nxlog.LOG_DEBUG, 'Got user %r' % self.username)
    self.prompt(102, 'Password: ', override_newline=False)

    fd = self.input.fileno()
    nxlog.log(nxlog.LOG_DEBUG, 'Got fd %r' % fd)
    # Save the terminal settings
    try:
      old = termios.tcgetattr(fd)
      new = old[:]

      # Disable the echo flag
      new[3] = new[3] & ~termios.ECHO # 3 == 'lflags'
      try:
        termios.tcsetattr(fd, termios.TCSADRAIN, new)
        passwd = self.input.readline()
      finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old)
    except termios.error:
      passwd = self.input.readline()

    nxlog.log(nxlog.LOG_DEBUG, 'Got a passwd')

    self.write('\n')
    # FIXME(diamond): ssh to localhost to verify the username and password are
    # correct. Also store the authentication information we need in a secure
    # way.
    del passwd
    self.status = self.STATUS_LOGGEDIN
    self.banner()

  def _nx_set_handler(self, command):
    """Handle the SET NX command.

    'SET' is used to configure the session in various ways. Two variables
    have been seen.
    'SET SHELL_MODE SHELL' has no known effect.
    'SET AUTH_MODE PASSWORD' prevents nxserver fromm asking for an MD5
    password if no password has been given.

    Args:
      command: The NX command and arguments invoked

    Returns:
      None
    """
    if len(command) < 2:
      self.prompt(500, 'Error: missing parameter \'variable\'')
      return
    var = command[1].upper()
    if var not in self.SET_ACCEPTED_VARIABLES:
      self.prompt(500, 'Error: unknown variable \'%s\'' % var)
      return

  def _nx_listsession_handler(self, command):
    """Handle the listsession NX command.

    'listsession' requests a table of session information for the current
    user. It requires parameters be specified. The following parameters have
    been seen.
    '--user="alriddoch"'
    This seems to be ignored. No matter what is specified, the user given at
    login is used.
    '--status="suspended,running"'
    This seems to constrain the list to sessions in the given states.
    '--geometry="3840x1200x24+render"'
    This seems to specify the desired geometry.
    '--type="unix-gnome"'
    This seems to specify the desired type.
    The format of the returned data is somewhat freeform, and looks hard to
    parse. Currently this function returns an empty table, which forces the
    the client to always start a new session

    Args:
      command: The NX command and arguments invoked

    Returns:
      None
    """

    # Basic checking of the right status
    if self.status < self.STATUS_LOGGEDIN:
      self.prompt(554, 'Error: the command \'listsession\' cannot '
          'be called before login')
      return
    # Make sure the state is consistent
    assert(hasattr(self, 'username'))
    # Ask for parameters if none have been given
    if len(command) > 1:
      parameters = command[1:]
    else:
      self.prompt(106, 'Parameters: ', override_newline=False)
      response = self.input.readline()
      self.write('') # Do newline after parameters.
      parameters = response.split()
    req = {}
    # Check the parameters fit with the expected syntax
    for param in parameters:
      key,val = self._parse_param(param)
      if key: req[key] = val
    # Ignore --user, as per commercial implementation
    req['user'] = [self.username]
    if 'type' not in req:
      # Default to showing any type
      req['type'] = None
    else:
      req['type'] = req['type'].split(',')
    if 'status' not in req:
      # Default to showing any state
      req['type'] = None
    else:
      req['status'] = req['status'].split(',')
    self.prompt(127, 'Available sessions:')
    self.__print_list_session(nxsession.db_find_sessions(
      users=req['user'],
      states=req['status'],
      types=req['type']))
    self.write('') # Print newline.
    self.prompt(148, "Server capacity: not reached for user: %s" %
        self.username)

  def _nx_startsession_handler(self, command):
    """Handle the startsession NX command.

    'startsession' seems to request a new session be started.
    It requires parameters be specified. The following parameters have
    been seen.
    '--link="lan"'
    '--backingstore="1"'
    '--encryption="1"'
    '--cache="16M"'
    '--images="64M"'
    '--shmem="1"'
    '--shpix="1"'
    '--strict="0"'
    '--composite="1"'
    '--media="0"'
    '--session="localtest"'
    '--type="unix-gnome"'
    '--geometry="3840x1150"'
    '--client="linux"'
    '--keyboard="pc102/gb"'
    '--screeninfo="3840x1150x24+render"'
    Experiments with this command by directly invoked nxserver have not
    worked, as it refuses to create a session saying the unencrypted sessions
    are not supported. This is independent of whether the --encryption option
    has been set, so probably is related to the fact the nxserver has not
    been launched by sshd.

    Args:
      command: The NX command and arguments invoked

    Returns:
      None
    """

    # Basic checking of the right status
    if self.status < self.STATUS_LOGGEDIN:
      self.prompt(554, 'Error: the command \'%s\' cannot '
          'be called before login' % command[0])
      return
    # Make sure the state is consistent
    assert(hasattr(self, 'username'))
    # Ask for parameters if none have been given
    if len(command) > 1:
      parameters = command[1:]
    else:
      self.prompt(106, 'Parameters: ', override_newline=False)
      response = self.input.readline()
      self.write('') # Do newline after parameters.
      parameters = response.split()
    # Check the parameters fit with the expected syntax
    for param in parameters:
      key,val = self._parse_param(param)
      # FIXME(diamond): DO something with the params.
    # FIXME(diamond): Start the session.
    if not hasattr(self, 'nxnode_commfd'):
      nxlog.log(nxlog.LOG_ERR, 'Nxserver does not have an nxnode yet.')
      return
    # Send the command to the connected nxnode running
    # FIXME(diamond): Convert the arguments to the form expected by nxnode.
    sess_id = nxsession.gen_uniq_id()
    self.write('startsession %s %s' %
        (sess_id, " ".join(parameters)), fd=self.nxnode_wfile)

    if self.daemonize():
    # Two threads return here, one connected to the client, one connected to
    # nxnode.
      self.running = False
      return

    start_waiting = time.time()
    wait_time = 30 #FIXME(diamond): make configurable
    while True:
      if time.time() - start_waiting > wait_time:
        nxlog.log(nxlog.LOG_ERR, "Session %s has not appeared in session db "
            "within %d seconds\n" % (sess_id, wait_time))
        sys.exit(1)
        #FIXME(diamond): raise proper error
      sessions = nxsession.db_find_sessions(id=sess_id)
      if len(sessions) == 1:
        sess = sessions[0]
        nxlog.log(nxlog.LOG_DEBUG, "Session %s has appeared in session db\n" %
            sess.params['full_id'])
        break
      elif len(sessions) > 1:
        nxlog.log(nxlog.LOG_DEBUG, "Multiple sessions matching %d have been "
            "found in the session db: %r\n" % sess_id)
        #FIXME(diamond): raise proper error
        break
      else:
        time.sleep(1)

    self.__print_sess_info(sess)

    start_waiting = time.time()
    wait_time = 30 #FIXME(diamond): make configurable
    while True:
      if time.time() - start_waiting > wait_time:
        nxlog.log(nxlog.LOG_ERR, "Session %s has not achieved running status "
            "within %d seconds\n" % (sess_id, wait_time))
        sys.exit(1)
        #FIXME(diamond): raise proper error
      sess.reload()
      if sess.params['state'] == 'starting':
        break
      elif sess.params['state'] in ['terminating', 'terminated']:
        nxlog.log(nxlog.LOG_ERR, "Session %(full_id)s has status "
            "'%(state)s', exiting." % sess.params)
        self.prompt(500, "Error: Session %(full_id)s has status '%(state)s'." %
            sess.params)
        self.prompt(999, "Bye.")
        self.running = False
        return
        #FIXME(diamond): raise proper error
      else:
        time.sleep(1)
    self.prompt(710, 'Session status: %s' % sess.params['state'])
    #FIXME(diamond): use configurable offset
    self.port = int(sess.params['display']) + 4000

  def _nx_restoresession_handler(self, command):
    """Handle the restoresession NX command.

    'restoresession' requests an existing session be resume.
    It requires parameters be specified. The following parameters have
    been seen, at a minimum the session id must be specified:
    '--link="lan"'
    '--backingstore="1"'
    '--encryption="1"'
    '--cache="16M"'
    '--images="64M"'
    '--shmem="1"'
    '--shpix="1"'
    '--strict="0"'
    '--composite="1"'
    '--media="0"'
    '--session="localtest"'
    '--type="unix-gnome"'
    '--geometry="3840x1150"'
    '--client="linux"'
    '--keyboard="pc102/gb"'
    '--screeninfo="3840x1150x24+render"'
    --id="A28EBF5AAC354E9EEAFEEB867980C543"

    Args:
      command: The NX command and arguments invoked

    Returns:
      None
    """

    # Basic checking of the right status
    if self.status < self.STATUS_LOGGEDIN:
      self.prompt(554, 'Error: the command \'%s\' cannot '
          'be called before login' % command[0])
      return
    # Make sure the state is consistent
    assert(hasattr(self, 'username'))
    # Ask for parameters if none have been given
    if len(command) > 1:
      parameters = command[1:]
    else:
      self.prompt(106, 'Parameters: ', override_newline=False)
      response = self.input.readline()
      self.write('') # Do newline after parameters.
      parameters = response.split()
    req = {}
    # Check the parameters fit with the expected syntax
    for param in parameters:
      key,val = self._parse_param(param)
      if key: req[key] = val
      # FIXME(diamond): DO something with the params.
    if not req.has_key('id'):
      msg = "Restore session requested, but no session specified"
      nxlog.log(nxlog.LOG_ERR, "%s (args: %r)\n" % (msg, req))
      self.prompt(500, 'Error: %s. check log file for more details.' % msg)
      self.running = False
      return
    nxlog.log(nxlog.LOG_DEBUG, "Got id param: %s" % req['id'])
    sessions = nxsession.db_find_sessions(id=req['id'], users=[self.username],
        states=['suspended', 'running'])
    if len(sessions) != 1:
      nxlog.log(nxlog.LOG_ERR, "%d sessions found matching %s in "
          "session db %s\n" % (len(sessions), req['id'], sessions))
      self.prompt(500, 'Error: Fatal error in module %s, '
          'check log file for more details.' % self.program.lower())
      self.running = False
      return

    sess = sessions[0]
    nxlog.log(nxlog.LOG_DEBUG, "Session %s found in session db\n" %
        sess.params['full_id'])

    # Needed to get nxagent to open it's port again.
    try:
      os.kill(int(sess.params['agent_pid']), signal.SIGHUP)
    except OSError, e:
      nxlog.log(nxlog.LOG_WARNING, "Attempted to send SIGHUP to nxagent, "
          "got error from kill[%d]: %s\n" % e.args)
      self.prompt(500, 'Error: Fatal error in module %s, '
          'check log file for more details.' % self.program.lower())
      self.prompt(999, 'Bye.')
      self.running = False
      return
    except (TypeError, ValueError), e:
      nxlog.log(nxlog.LOG_WARNING, "Session does not have a valid nxagent pid "
          "stored (instead has %r), got error: %s" %
          (sess.params['agent_pid'], e))
      self.prompt(500, 'Error: Fatal error in module %s, '
          'check log file for more details.' % self.program.lower())
      self.prompt(999, 'Bye.')
      self.running = False
      return
    else:
      nxlog.log(nxlog.LOG_NOTICE, "Sent SIGHUP to nxagent\n")

    self.__print_sess_info(sess)
    self.prompt(710, 'Session status: %s' % sess.params['state'])
    #FIXME(diamond): use configurable offset
    self.port = int(sess.params['display']) + 4000

  def daemonize(self):
    """Drop into the background."""

    # I am assumuing this throws if fork fails.
    pid = os.fork()
    # In the parent, return.
    if pid != 0:
      # self.nxnode_rfile.close()
      # self.nxnode_wfile.close()
      os.close(self.nxnode_commfd)
      # del(self.nxnode_rfile)
      # del(self.nxnode_wfile)
      # del(self.nxnode_commfd)
      nxlog.setup('nxserver-outer')
      nxlog.log(nxlog.LOG_INFO, "Forked child to take care of nxsession stuff")
      return False

    # Dissociate from the nxserver terminal
    os.setsid()

    # If we need to change signal behavior, do it here.

    # Close the stdio fds.
    os.close(0)
    os.close(1)
    os.close(2)

    self.input = self.nxnode_rfile
    self.output = self.nxnode_wfile

    # I'm not sure what to do here with self.nxnode_rfile and self.nxnode_wfile
    # Closing the fd is enough, but the file objects would linger on.
    del(self.nxnode_rfile)
    del(self.nxnode_wfile)
    del(self.nxnode_commfd)

    nxlog.setup('nxserver-inner')
    nxlog.log(nxlog.LOG_INFO, "Successfully forked, "
        "taking care of nxsession stuff\n")
    try:
      self._session_read_loop()
    except Exception:
      trace = traceback.format_exc()
      nxlog.log(nxlog.LOG_ERR, 'Going down because exception caught '
                               'at the top level.')
      for line in trace.split('\n'):
        nxlog.log(nxlog.LOG_ERR, '%s' % line)
    return True

  def _session_read_loop(self):
    sess = None
    while True:
      line = self.input.readline()
      if not line:
        return
      line = line.rstrip()
      nxlog.log(nxlog.LOG_DEBUG, 'Got from nxnode %r\n' % line)
      # FIXME(diamond): change number to something sensible
      if line.startswith('NX> 8888 sessioncreate'):
        if sess:
          nxlog.log(nxlog.LOG_ERR, 'Nxnode tried to create a session when one '
              'already exists: %s\n' % line)
        else:
          args = line.split(' ', 3)[-1].replace(' ', '\n')
          sess = nxsession.nxsession(args)
      elif line.startswith('NX> 8888 agentpid:'):
        if not sess:
          nxlog.log(nxlog.LOG_ERR, 'Nxagent-helper tried to change session '
              'when none exists: %s\n' % line)
        else:
          agent_pid = line.rstrip().split(' ')[3]
          sess.params['agent_pid'] = agent_pid
          sess.save()
          nxlog.log(nxlog.LOG_DEBUG, "Agent pid set to '%s'\n" %
              sess.params['agent_pid'])
      elif line.startswith('NX> 1009 Session status:'):
        if not sess:
          nxlog.log(nxlog.LOG_ERR, 'Nxagent-helper tried to change session '
              'when none exists: %s\n' % line)
        else:
          state_name = line.rstrip().split(' ')[4]
          sess.set_state(state_name)
          sess.save()
          nxlog.log(nxlog.LOG_DEBUG, "Session state updated to '%s'\n" %
              sess.params['state'])
      elif line.startswith('NX> 500 Error:'):
        if sess:
          sess.set_state('terminated')
          sess.save()
          nxlog.log(nxlog.LOG_DEBUG, "Session state updated to '%s', exiting\n"
              % sess.params['state'])
          break

  def __print_list_session(self, sessions):
    """Given a list of sessions, print the listsession output

    Args:
      sessions: A list of nxsession.nxsesion() instances
    """

    # Print headers
    cols = []
    for header,width,param in self.LISTSESSION_COLUMNS:
      if width >= 0:
        text = header.ljust(width)
      else:
        text = header.rjust(abs(width))
      cols.append(text)
    self.write(" ".join(cols))

    # Print dashes
    cols = []
    for header,width,param in self.LISTSESSION_COLUMNS:
      cols.append("-" * abs(width))
    self.write(" ".join(cols))

    # Print sessions
    for sess in sessions:
      cols = []
      for header,width,param in self.LISTSESSION_COLUMNS:
        if width >= 0:
          text = sess.params[param].ljust(width)
        else:
          text = sess.params[param].rjust(abs(width))
        if param == 'state': text = text.capitalize()
        cols.append(text)
      self.write(" ".join(cols))

  def __print_sess_info(self, sess):
    """Print out session information to the client

    This is used for starting/resuming a session.
    """

    self.prompt(700, 'Session id: %(full_id)s' % sess.params)
    self.prompt(705, 'Session display: %(display)s' % sess.params)
    self.prompt(703, 'Session type: %(type)s' % sess.params)
    self.prompt(701, 'Proxy cookie: %(cookie)s' % sess.params)
    self.prompt(702, 'Proxy IP: %(proxyip)s' % sess.params)
    self.prompt(706, 'Agent cookie: %(cookie)s' % sess.params)
    self.prompt(704, 'Session cache: %(cache)s' % sess.params)
    self.prompt(707, 'SSL tunneling: %(ssl)s' % sess.params)
    self.prompt(708, 'Subscription: GPL')


if __name__ == '__main__':
  print 'This is a library. Please use it from python using import.'
  sys.exit(0)
