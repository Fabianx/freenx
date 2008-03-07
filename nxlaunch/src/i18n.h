/***************************************************************************
                   nxlaunch: A GTK NX Client based on nxcl
                   ---------------------------------------
    begin                : September 2007
    copyright            : (C) 2007 Embedded Software Foundry Ltd. (U.K.)
                         :     Author: Sebastian James
    email                : seb@esfnet.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef __NXLAUNCH_I18N__
#  define __NXLAUNCH_I18N__
#  ifdef HAVE_CONFIG_H
#    include <config.h>
#  endif
#  ifdef ENABLE_NLS
#    include "../lib/gettext.h"
#    define _(String) gettext (String)
#    define gettext_noop(String) String
#    define N_(String) gettext_noop (String)
#  else
#    define _(String) (String)
#    define N_(String) String
#    define textdomain(Domain) (Domain)
#    define gettext(String) (String)
#    define dgettext(Domain,String) (String)
#    define dcgettext(Domain,String,Type) (String)
#    define bindtextdomain(Domain, Directory) (Domain) 
#    define bind_textdomain_codeset(Domain,Codeset) (Codeset) 
#  endif /* ENABLE_NLS */
#endif /* __NXLAUNCH_I18N__ */

