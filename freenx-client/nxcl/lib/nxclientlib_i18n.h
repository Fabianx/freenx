extern "C" {
#ifndef __NXCLIENTLIB_I18N__
#  define __NXCLIENTLIB_I18N__

#define _(String) (String)
#define N_(String) String


/*
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
#  endif
*/
#endif /* __NXCLIENTLIB_I18N__ */
}
