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
    NXConfig (name)

    * name is the name of a configuration file on ~/.gnx/

    Attributes:

    host:        the host to connect to using nxssh
    port:        the port in which the ssh server is listening
    username:    the username to start the session with
    password:    the password for said user

    sshkey:      path for the ssh private key for ssh key auth

    session:     NXSession instance containing information about
                 the session to be started/resumed

    """

    def __init__ (self, name = None):
        # for use with configuration frontends
        self.modified = False

        self.host = None
        self.port = 22
        self.username = None
        self.password = None
        self.sshkey = '%s/.nx/id_dsa' % (HOME)

        self.session = None

        if name:
            self._load (name)

    def _load (self, name):
        self.name = name
        self.session = NXSession (name, 'unix-gnome')
        
        # FIXME: this can be failing for reasons other than the
        # file not existing
        try:
            f = open ('%s/.gnx/%s' % (HOME, name))
            lines = f.readlines ()
            f.close ()
        except IOError:
            return
        
        conf = {}
        for line in lines:
            key, value = line.split ('=')
            conf[key] = value.strip ()

        self.host = conf['host']
        self.port = int(conf['port'])
        self.username = conf['username']
        self.password = conf['password']
        self.sshkey = conf['sshkey']

        self.session.stype = conf['type']
        self.session.geometry = conf['geometry']

    def save (self):
        session = self.session
        name = self.name

        f = open ('%s/.gnx/%s' % (HOME, name), 'w')

        f.write ('host=%s\n' % (self.host))
        f.write ('port=%s\n' % (self.port))
        f.write ('username=%s\n' % (self.username))
        f.write ('password=%s\n' % (self.password))
        f.write ('sshkey=%s\n' % (self.sshkey))

        f.write ('type=%s\n' % (session.stype))
        f.write ('cache=%s\n' % (session.cache))
        f.write ('images_cache=%s\n' % (session.images_cache))
        f.write ('link=%s\n' % (session.link))
        f.write ('geometry=%s\n' % (session.geometry))

        f.close ()
    
if __name__ == '__main__':
    nc = NXConfig ('gnome_local')
    nc.session.name = 'bleh'
    nc.save ()
