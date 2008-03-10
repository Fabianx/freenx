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

import os
import pwd
import socket
import subprocess
import sys

import nxlog
import nxparser
import nxsession

class parser(nxparser.base.parser):
  """Node parser for NX protocol

  This class handles the NX protocol messages required by a node.
  """

  DEFAULT_PROGRAM = 'NXNODE'

  class node_session:
    """Internal representation of a session

    This class is used by nxnode to store session parameters, to create the
    needed session args & options file, and to print out the parameters for
    transmission to nxserver-inner.
    """

    def __init__(self, id, args):
      """node_session constructor

      Args:
        args: The id of the session, followed by all the other
              parameters the client requested
      """

      self.id = id
      self.args = args
      self.display = self._gen_disp_num()
      self.hostname = socket.getfqdn()
      self.full_id = "%s-%s-%s" % (self.hostname, self.display, self.id)
      self.cookie = nxsession.gen_uniq_id()
      self.dir = os.path.join('/tmp/nx', 'S-%s' % self.full_id)
      #FIXME(diamond): needs error checking, maybe different mode
      os.makedirs(self.dir, 0755)
      self.opts_file_path = os.path.join(self.dir, 'options') 
      self.args_file_path = os.path.join(self.dir, 'args') 
      self.application = self.args.get('application')
      self.user = pwd.getpwuid(os.getuid())[0]
      self.name = self.args.get('session', "%s:%s" % (self.hostname, self.display))
      self.keyboard = self.args.get('keyboard', 'pc105/gb')
      self.geometry = self.args.get('geometry', '640x480')
      self.client = self.args.get('client', 'unknown')
      self.link = self.args.get('link', 'isdn')
      self.fullscreen = self.args.get('fullscreen', '0')
      #DEBUG, FIXME(diamond): all of these
      self.type = self.args.get('type', 'unix-default')
      self.options = '-----PSA' #FIXME(diamond): see note in self.info()
      self.depth = 24
      self.resolution = "640x480"
      #FIXME(diamond): Not ipv6 compatible
      self.proxyip = socket.gethostbyname(self.hostname)
      self.ssl = 1
      #End DEBUG/FIXME(diamond)

      if self.type == 'unix-application':
        assert(self.application)
        self.mode = '-R' # Run nxagent in rootless mode
      else:
        self.mode = '-D' # Run nxagent in desktop mode

      # We need to write the type without the 'unix-' prefix for nxagent.
      if self.type.startswith('unix-'):
        self.shorttype = self.type.split('-', 1)[1]
      else:
        self.shorttype = self.type

      self._write_args()
      self._write_opts()

    def __getitem__(self, item):
      """Allow node_session instances to be treated as dicts

      This is used in places like self.info(), to allow cleaner variable
      substitution of variables in strings.
      """

      return getattr(self, item)

    def _gen_disp_num(self):
      """Return an unused display number (corresponding to an unused port)"""
      return 20 #DEBUG, FIXME(diamond)

    def _write_args(self):
      """Create the session's 'args' file

      The args file is a newline-delimited list of arguments to be passed to
      nxagent
      """

      try:
        args_file = open(self.args_file_path, 'w')
        #DEBUG, FIXME(diamond):
        args_file.write("\n".join([self.mode, '-options', self.opts_file_path,
          '-name', 'FreeNX - %(user)s@%(hostname)s:%(display)s' % self,
          '-nolisten', 'tcp', ':%d' % self.display]))
        args_file.write("\n")
        args_file.close()
      except IOError, e:
        nxlog.log(nxlog.LOG_ERR, 'IOError when writing '
            'session args file: %s' % e)
      except OSError, e:
        nxlog.log(nxlog.LOG_ERR, 'OSError when writing '
            'session args file: %s' % e)

    def _write_opts(self):
      """Create the session's 'options' file

      The options file is a comma-delimited list of options that are read in
      by nxagent
      """

      try:
        opts_file = open(self.opts_file_path, 'w')
        opts = ['nx/nx', 'keyboard=%(keyboard)s' % self,
            'geometry=%(geometry)s' % self, 'client=%(client)s' % self,
            'cache=8M', 'images=32M', 'link=%(link)s' % self,
            'type=%(shorttype)s' % self, 'clipboard=both', 'composite=1',
            'cleanup=0', 'accept=127.0.0.1', 'product=Freenx-gpl', 'shmem=1',
            'backingstore=1', 'shpix=1', 'cookie=%s' % self.cookie,
            'id=%s' % self.full_id, 'strict=0']
        if self.type == 'unix-application':
          opts.append('application=%(application)s' % self)
        if self.fullscreen == '1':
          opts.append('fullscreen=%(fullscreen)s' % self)
        opts_file.write("%s:%d\n" % (",".join(opts), self.display))
        opts_file.close()
      except IOError, e:
        nxlog.log(nxlog.LOG_ERR, 'IOError when writing '
            'session options file: %s' % e)
      except OSError, e:
        nxlog.log(nxlog.LOG_ERR, 'OSError when writing '
            'session options file: %s' % e)

    def info(self): #This is for reporting back to nxserver
      """Return a string with all parameter values encoded into it"""
      # Needed for session list:
      #   Display number
      #   type
      #   id
      #   options(?) FRD--PSA (F=fullscreen, R=render,
      #              D=non-rootless(Desktop?), PSA?)
      #   depth
      #   resolution
      #   status
      #   name
      # Needed for session start:
      #   hostname
      #   cookie
      #   proxy ip
      #   ssl

      return ("display=%(display)d type=%(type)s id=%(id)s "
          "options=%(options)s depth=%(depth)d resolution=%(resolution)s "
          "name=%(name)s hostname=%(hostname)s cookie=%(cookie)s "
          "proxyip=%(proxyip)s ssl=%(ssl)s" % self)


  def __init__(self, input, output, version=nxparser.base.parser.DEFAULT_VERSION,
               program=DEFAULT_PROGRAM):
    """node_parser constructor

    Args:
      input: The file object to read parser input from.
      output: The file object to write the results to.
      version: The version string used to negotiate the protocol.
      program: The program name used in the protocol banner.
    """
    nxparser.base.parser.__init__(self, input, output, version=version, program=program)

  def banner(self):
    """Write the protocol banner to the output."""

    self.prompt(1000, '%s - Version %s' % (self.program.upper(), self.version))

  def _nx_startsession_handler(self, command):
    # Remove 'startsession' from the front of the list of args
    command.pop(0)

    id = command.pop(0)
    req = {}
    for param in command:
      key,val = self._parse_param(param)
      if key: req[key] = val
    sess = self.node_session(id, req)

    # FIXME(diamond): change number to something sensible
    self.write("NX> 8888 sessioncreate %s" % sess.info())
    # Let stdout go directly to our stdout, i.e. to nxserver
    # Check stderr for error messages if things go badly
    p = subprocess.Popen('/usr/freenx/bin/nxagent-helper',
                         stdin=subprocess.PIPE,
                         stderr=subprocess.PIPE,
                         shell=True)
    p.stdin.write('start %s\n' % sess.full_id)
    p.stdin.flush()
    nxlog.log(nxlog.LOG_DEBUG, 'Starting session')
    child_status = p.wait()
    if child_status != 0:
      lines = p.stderr.readlines()
      if not lines:
        out_msg = ", no output printed"
      else:
        out_msg = ", with %d lines of output (shown below):" % len(lines)
      nxlog.log(nxlog.LOG_ERR, 'Start session failed %d%s' %
          (child_status, out_msg))
      for line in lines:
        nxlog.log(nxlog.LOG_ERR, 'from nxagent-helper: %s' % line)
      self.prompt(500, 'Error: Startsession failed')
      self.running = False
      return
    nxlog.log(nxlog.LOG_ERR, 'Session completed %d' % child_status)
    self.running = False
    #FIXME(diamond): cleanup session dir here?

  def _nx_resumesession_handler(self, unused_command):
    nxlog.log(nxlog.LOG_DEBUG, 'Resuming session')


if __name__ == '__main__':
  print 'This is a library. Please use it from python using import.'
  sys.exit(0)
