#!/usr/bin/python
# -*- coding: utf-8 -*-

# gnx.py -- a simple client for FreeNX
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

import sys

import pygtk
pygtk.require ('2.0')
import gtk
from gtk import glade

from nxclient import NXClient, NOTCONNECTED, CONNECTING, CONNECTED, RUNNING, STARTING
from nxsession import NXSession

def _update_gui ():
    while gtk.events_pending ():
        gtk.main_iteration ()

class NXGUI:
    state_dialog = None
    state = None

    def __init__ (self):
        self._create_gui ()

    def _create_gui (self):
        gui = glade.XML ('gnxui.glade', 'main_window')

        get_widget = gui.get_widget

        self.main_window = get_widget ('main_window')
        self.main_window.connect ('delete-event', gtk.main_quit)

        self.user = get_widget ('user')
        self.password = get_widget ('password')

        connect_btn = get_widget ('connect_btn')
        connect_btn.connect ('clicked', self._connect_session_cb)

        self.gui = gui

    def _connect_session_cb (self, *args):
        self.main_window.hide ()
        _update_gui ()

        user = self.user.get_chars (0, -1)
        password = self.password.get_chars (0, -1)

        client = NXClient ('localhost', user, password)
        self.client = client

        client.log = None
        client._yes_no_dialog = self._yes_no_dialog
        client._update_connection_state = self._update_connection_state
        
        dialog_gui = glade.XML ('gnxui.glade', 'con_progress')

        # this dialog will be destroyed when the connection state
        # goes to 'RUNNING'"
        self.state_dialog = dialog_gui.get_widget ('con_progress')
        self.state = dialog_gui.get_widget ('state_label')

        self.state.set_text (_('Initializing...'))
        self.state_dialog.show_all ()
        _update_gui ()

        client.connect ()

        client.session = NXSession ('GNOME', 'unix-gnome')
        client.start_session ()

        _update_gui ()

        self.main_window.show ()

    def _yes_no_dialog (self, msg):
        ret = False
        
        print msg
        dialog = gtk.MessageDialog (self.main_window, 0,
                                    gtk.MESSAGE_QUESTION,
                                    gtk.BUTTONS_YES_NO,
                                    msg)
        response = dialog.run ()
        if response == gtk.RESPONSE_YES:
            ret = True

        dialog.destroy ()
        return ret

    def _update_connection_state (self, state_code):
        dialog = self.state_dialog
        state = self.state

        msg = _('Unknown')

        if state_code == RUNNING:
            dialog.destroy ()
            dialog = None
            state = None
            _update_gui ()
            return
        elif state_code == NOTCONNECTED:
            msg = _('Initializing...')
        elif state_code == CONNECTING:
            msg = _('Authenticating...')
        elif state_code == CONNECTED:
            msg = _('Conected...')
        elif state_code == STARTING:
            msg = _('Starting session...')

        state.set_text (msg)
        _update_gui ()

    def loop (self):
        gtk.main ()
    

if __name__ == '__main__':
    nxgui = NXGUI ()
    nxgui.loop ()
