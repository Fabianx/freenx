#!/usr/bin/python
# -*- coding: utf-8 -*-

# nxclient.py -- a simple client for FreeNX
# Copyright (C) 2005 Marco Sinhoreli and Gustavo Noronha Silva
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# i18n will come!
def _ (x):
    return x

import os, sys
import pexpect

from pexpect import EOF, TIMEOUT

CLIENTID = 'NXCLIENT - Version 1.4.0'
HOME = os.getenv ('HOME')

class SSHAuthError(Exception):
    """
    Attributes:

    errtype - key missing, auth failed?
    message - message explaining the problem

    errtype can be:
     0: the ssh private key is not accessible
     1: the ssh key authentication failed
    """
    
    def __init__ (self, errtype, message):
        self.errtype = errtype
        self.message = message

class SSHConnectionError(Exception):
    """
    Attributes:

    errtype - key missing, auth failed?
    message - message explaining the problem

    errtype can be:
     0: connection refused (port closed, for example)
    """
    
    def __init__ (self, errtype, message):
        self.errtype = errtype
        self.message = message

class ProtocolError(Exception):
    """
    Attributes:

    message - message explaining the problem
    """
    
    def __init__ (self, message):
        self.message = message

# state of the connection
NOTCONNECTED = 0
CONNECTING = 1
CONNECTED = 2

class NXClient:
    """
    NXClient class

    Instantiation:

    NXClient (host, username, password [, sshkey] [, port] [, session])

    * sshkey defaults to $HOME/.nx/id_dsa
    * port defaults to 22
    * session defaults to None

    Attributes:

    host:        the host to connect to using nxssh
    port:        the port in which the ssh server is listening
    username:    the username to start the session with
    password:    the password for said user

    sshkey:      path for the ssh private key for ssh key auth

    session:     NXSession instance containing information about
                 the session to be started/resumed

    connection:  file object used to communicate with the nxserver
    state:       state of the connection, one of the following:
                    NOTCONNECTED - nothing happened or connection
                                   was closed
                    CONNECTING   - ssh connection being stabilished
                                   and user authorization being
                                   negotiated with nxserver
                    CONNECTED    - ready to list for active sessions
                                   and start/restore sessions

    Methods:

    connect ():       starts the connection by connecting to the
                      host through nxssh and authenticating the
                      user with nxserver
    
    """

    def __init__ (self, host, username, password,
                  sshkey = '%s/.nx/id_dsa' % (HOME),
                  port = 22, session = None):
        self.host = host
        self.port = port
        self.username = username
        self.password = password
        self.sshkey = sshkey

        self.session = session

        self.state = NOTCONNECTED

        if not os.access (sshkey, os.R_OK):
            raise SSHAuthError (0, _('SSH key is inaccessible.'))

    def connect (self):
        try:
            self._connect ()
        except:
            self.state = NOTCONNECTED
            raise

    def _waitfor (self, code):
        connection = self.connection
        
        if code: size = len (code)
        else: size = 3

        found = False
        while not found:
            try:
                connection.expect (['NX> '])
                remote_code = connection.read (size)
                if not code or remote_code == code:
                    found = True
            except TIMEOUT, EOF:
                message = remote_code + connection.readline ()
                raise ProtocolError (_('Protocol error: %s') % \
                                     (message))

    def _connect (self):
        self.state = CONNECTING
        waitfor = self._waitfor
        
        connection = pexpect.spawn ('nxssh -nx -p %d -i %s nx@%s -2 -S' % \
                                    (self.port, self.sshkey, self.host))
        self.connection = connection
        connection.setlog (sys.stdout)

        send = connection.send

        try:
            # FIXME - "^.*?" may appear, meaning that
            # the ssh key of the host is not known, "yes"
            # or "no" should be given as response
            index = connection.expect (['HELLO', 'NX> 204 ', 'nxssh: '])
            if index == 0:
                waitfor ('105')
                send ('HELLO NXCLIENT - Version 1.4.0\n')
            elif index == 1:
                raise SSHAuthError (1, _('SSH key authentitcation failed.'))
            elif index == 2:
                raise SSHConnectionError (0, _('Connection refused.'))
            del index

            # check if protocol was accepted
            waitfor ('134')

            # wait for the shell
            waitfor ('105')
            send ('login\n')

            # user prompt
            waitfor ('101')
            send (self.username + '\n')

            # password prompt
            waitfor ('102')
            send (self.password + '\n')

            # check if all went fine
            waitfor (None)
            if connection.after[:3] == '404':
                raise ProtocolError (_('Failed to login: wrong username or password given. (404)'))
            elif connection.after[:3] != '103':
                raise ProtocolError (_('Protocol error: %s') % \
                                     (connection.after))

            # ok, we're in
            waitfor ('105')
            self.state = CONNECTED
            
        except (EOF, TIMEOUT):
            # raise our own?
            raise

    def start_session (self):
        session = self.session
        waitfor = self._waitfor
        connection = self.connection

        # FIXME: raise exception?
        if not session:
            return 1

        send = connection.send
        send ('startsession %s\n' % (session.get_start_params ()))

        waitfor ('700')
        line = connection.readline ()
        session.id = line.split (':', 2)[1].strip ()

        waitfor ('705')
        line = connection.readline ()
        session.display = line.split (':', 2)[1].strip ()

        waitfor ('701')
        line = connection.readline ()
        session.pcookie = line.split (':', 2)[1].strip ()

        waitfor ('1006')
        line = connection.readline ()
        session.status = line.split (':', 2)[1].strip ()

        os.mkdir ('%s/.nx/S-%s' % (HOME, session.id))
        f = open ('%s/.nx/S-%s/options' % (HOME, session.id), 'w')
        f.write ('cookie=%s,root=%s/.nx,session=%s,id=%s,connect=%s:%s' % \
                 (session.pcookie, HOME, session.sname, session.id, \
                  self.host, session.display))
        f.close ()

        os.system ('nxproxy -S options=%s/.nx/S-%s/options:%s' % (HOME, session.id, session.display))

if __name__ == '__main__':
    from nxsession import NXSession

    nc = NXClient ('200.207.4.57', 'kov', 'wsxedc')

    nc.connect ()

    nc.session = NXSession ('teste-gnome')
    nc.start_session ()

    nc.connection.send ('\n')
