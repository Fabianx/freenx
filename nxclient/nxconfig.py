#!/usr/bin/python
# -*- coding: utf-8 -*-

# nxclient.py -- a simple client for FreeNX
#
# Copyright (C) 2005 Instituto de Estudos e Pesquisas dos Trabalhadores
# no Setor Energético, Marco Sinhoreli and Gustavo Noronha Silva
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

import os
from nxsession import NXSession

HOME = os.getenv ('HOME')

class NXConfig:
    """
    NXConfig (host, username, password [, sshkey] [, port] [, session])

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
    
