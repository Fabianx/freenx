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

import os

# i18n will come!
def _ (x):
    return x

class NXSession:
    sname = None
    stype = 'unix-gnome'
    cache = '8M'
    images_cache = '32M'
    xcookie = None
    link = 'adsl'
    kbtype = 'pc102/us'
    nodelay = '1'
    backingstore = 'never'
    geometry = '800x600+112+59'
    media = '0'
    agent_server = ''
    agent_user = ''
    agent_password = ''
    screeninfo = '800x600x16+render'

    def __init__ (self, name, session_type = None):
        self.sname = name
        if session_type:
            self.stype = session_type

        f = os.popen ('xauth list :0 | awk \'{ print $3 }\'')
        self.xcookie = f.read ().strip ()
        f.close ()

        f = os.popen ('xprop -root | grep "^_XKB_RULES_NAMES(STRING)"')
        tmp = f.read ()
        f.close ()
        
        try:
            tmp = tmp.split ('"')
            self.kbtype = '%s/%s' % (tmp[3], tmp[5])
        except AttributeError, IndexError:
            # raise an exception warning about the keyboard not
            # being detected?
            pass

    def get_start_params (self):
        # FIXME: check if self.xcookie has contents, and raise
        # an exception if not
        pline = '--session="%s" --type="%s" ' % (self.sname, self.stype)
        pline += '--cache="%s" --images="%s" ' % (self.cache, self.images_cache)
        pline += '--cookie="%s" --link="%s" ' % (self.xcookie, self.link)
        pline += '--kbtype="%s" --nodelay="%s" ' % (self.kbtype, self.nodelay)
        pline += '--backingstore="%s" --geometry="%s" ' % (self.backingstore, self.geometry)
        pline += '--media="%s" --agent_server="%s" ' % (self.media, self.agent_server)
        pline += '--agent_user="%s" --agent_password="%s" ' % (self.agent_user, self.agent_password)
        pline += '--screeninfo="%s"' % (self.screeninfo)

        return pline

if __name__ == '__main__':
    s = NXSession ('teste-gnome')
    print s.get_start_params ()
