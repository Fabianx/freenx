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

import os, sys

import pygtk
pygtk.require ('2.0')
import gtk, gobject
from gtk import glade

from nxclient import NXClient, NOTCONNECTED, CONNECTING, CONNECTED, RUNNING, STARTING
from nxsession import NXSession
from nxconfig import NXConfig

HOME = os.getenv ('HOME')

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
        self.user.connect ('activate', self._connect_session_cb)

        self.password = get_widget ('password')
        self.password.connect ('activate', self._connect_session_cb)

        session = get_widget ('session')
        session.connect ('changed', self._session_changed_cb)
        self.session = session

        self._prepare_session_combo (session)
        self._update_sessions ()

        new_btn = get_widget ('new_btn')
        new_btn.connect ('clicked', self._new_session_cb)

        modify_btn = get_widget ('modify_btn')
        modify_btn.connect ('clicked', self._modify_session_cb)

        connect_btn = get_widget ('connect_btn')
        connect_btn.connect ('clicked', self._connect_session_cb)
        connect_btn.grab_focus ()

        self.gui = gui

    def _prepare_session_combo (self, session):
        cr = gtk.CellRendererText ()
        session.pack_start (cr, True)
        session.add_attribute (cr, 'text', 0)

    def _update_sessions (self):
        session = self.session

        model = session.get_model ()
        if model:
            model.clear ()
        
        model = gtk.TreeStore (gobject.TYPE_STRING)
        sessions = os.listdir ('%s/.gnx/' % (HOME))
        for s in sessions:
            if s[-1] == '~':
                continue
            iter = model.append (None)
            model.set (iter, 0, s)
        session.set_model (model)
        session.set_active (0)

    def _new_session_cb (self, *args):
        dialog = gtk.Dialog (_('Choose a name...'),
                             self.main_window, 0,
                             (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                              gtk.STOCK_OK, gtk.RESPONSE_OK))
        dialog.set_modal (True)
        dialog.set_has_separator (False)

        table = gtk.Table (2, 2, False)
        table.set_border_width (6)
        table.set_row_spacings (6)
        table.set_col_spacings (6)
        dialog.vbox.pack_start (table)

        image = gtk.image_new_from_stock (gtk.STOCK_DIALOG_QUESTION,
                                          gtk.ICON_SIZE_DIALOG)
        table.attach (image, 0, 1, 0, 2)

        label = gtk.Label (_('Please, choose a name for your new session:'))
        table.attach (label, 1, 2, 0, 1)
        
        entry = gtk.Entry ()
        table.attach (entry, 1, 2, 1, 2)

        dialog.show_all ()
        response = dialog.run ()
        if response != gtk.RESPONSE_OK:
            dialog.destroy ()
            return

        name = entry.get_text ()
        dialog.destroy ()

        self.config = NXConfig (name)

        modgui, conf_edit = self._new_session_editing_window ()
        
        conf_edit.show_all ()
        gtk.main ()

        self.config.save ()
        conf_edit.destroy ()

        self._update_sessions ()

    def _modify_session_cb (self, *args):
        config = self.config

        modgui, conf_edit = self._new_session_editing_window ()
        
        self._load_config_to_window (modgui)

        conf_edit.show_all ()
        gtk.main ()

        if config.modified:
            config.save ()

        conf_edit.destroy ()

        self._update_sessions ()

    def _new_session_editing_window (self):
        modgui = glade.XML ('gnxui.glade', 'conf_edit')

        conf_edit = modgui.get_widget ('conf_edit')
        conf_edit.connect ('delete-event', gtk.main_quit)
        conf_edit.set_transient_for (self.main_window)
        conf_edit.set_modal (True)

        close_btn = modgui.get_widget ('conf_close_btn')
        close_btn.connect ('clicked', gtk.main_quit)

        return modgui, conf_edit

    def _load_config_to_window (self, wgui):
        config = self.config
        
        conf_host = wgui.get_widget ('conf_host')
        conf_host.set_text (config.host)
        conf_host.connect ('changed', self._conf_changed_cb, 'host')

        conf_port = wgui.get_widget ('conf_port')
        conf_port.set_text (str(config.port))
        conf_port.connect ('changed', self._conf_changed_cb, 'port')

        conf_sshkey = wgui.get_widget ('conf_sshkey')
        conf_sshkey.set_text (config.sshkey)
        conf_sshkey.connect ('changed', self._conf_changed_cb, 'sshkey')

        geodict = {}
        geodict['1024x768'] = 0
        geodict['800x600'] = 1
        geodict['640x480'] = 2
        
        geometry_string = config.session.geometry.split ('+', 2)[0]

        conf_geometry = wgui.get_widget ('conf_geometry')
        conf_geometry.set_active (geodict[geometry_string])
        conf_geometry.connect ('changed', self._conf_changed_cb, 'geometry')

    def _conf_changed_cb (self, w, what):
        if what == 'host':
            self.config.host = w.get_text ()
        elif what == 'port':
            self.config.port = int (w.get_text ())
        elif what == 'sshkey':
            self.config.sshkey = w.get_text ()
        elif what == 'geometry':
            model = w.get_model ()
            iter = w.get_active_iter ()
            self.config.session.geometry = model.get_value (iter, 0)
            print self.config.session.geometry
        else:
            return # should not be reached

        self.config.modified = True

    def _session_changed_cb (self, *args):
        session = self.session
        model = session.get_model ()

        iter = session.get_active_iter ()
        name = model.get (iter, 0)[0]
        self.config = NXConfig (name)

        self.user.set_text (self.config.username)
        self.password.set_text (self.config.password)

    def _connect_session_cb (self, *args):
        config = self.config
        config.save ()

        self.main_window.hide ()
        _update_gui ()

        config.username = self.user.get_chars (0, -1)
        config.password = self.password.get_chars (0, -1)

        client = NXClient(config)
        self.client = client

        # FIXME, this should be recorded in a log
        client.log = sys.stdout
        client._yes_no_dialog = self._yes_no_dialog
        client._update_connection_state = self._update_connection_state
        
        dialog_gui = glade.XML ('gnxui.glade', 'con_progress')

        # this dialog will be destroyed when the connection state
        # goes to 'RUNNING'"
        self.state_dialog = dialog_gui.get_widget ('con_progress')
        self.state = dialog_gui.get_widget ('state_label')
        stop_btn = dialog_gui.get_widget ('stop_btn')
        stop_btn.connect ('clicked', self._cancel_connect_cb)

        self.state.set_text (_('Initializing...'))
        self.state_dialog.show_all ()
        _update_gui ()

        client.connect ()
        client.start_session ()

        gtk.main_quit ()

    def _cancel_connect_cb (self, *args):
        self.client.disconnect ()

    def _yes_no_dialog (self, msg):
        ret = False
        
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

        if state_code == RUNNING or state_code == NOTCONNECTED:
            dialog.destroy ()
            dialog = None
            state = None
            _update_gui ()
            return
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
