#include <stdio.h>
#include <errno.h>

#include <sys/select.h>

#include <gtk/gtk.h>
#include <glib.h>

const gchar *homedir = NULL;

void
flush_buffer (gchar *buffer)
{
  g_print (buffer);
  g_free (buffer);
}

gchar*
read_code (gint fd)
{
  size_t bytes_read;
  gchar *buffer = g_malloc0 (sizeof(gchar)*8);

  bytes_read = read (fd, buffer, 7);
  if (bytes_read == -1)
    return NULL;

  return buffer;
}

gchar*
read_line (gint fd)
{
  fd_set rfds;
  struct timeval tv;

  gchar *buffer = g_malloc0 (sizeof(gchar)*2);
  gchar tmp[1];
  gint count;

  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);

  tv.tv_sec = 3;
  tv.tv_usec = 0;

  for (count = 2; tmp[0] != '\n'; count++)
    {
      if (!select (fd+1, &rfds, NULL, NULL, &tv))
	break;
      
      read (fd, tmp, 1);
      buffer = g_realloc (buffer, sizeof(gchar)*count);
      buffer[count - 2] = tmp[0];
    }
  buffer[count - 2] = '\0';
  
  return buffer;
}

gchar*
read_chars (gint fd, gint count)
{
  gchar *buffer = NULL;

  buffer = g_malloc0 (sizeof(gchar)*(count+1));

  read (fd, buffer, count);

  return buffer;
}

void
write_line (gint fd, gchar *line)
{
  gchar *buffer;

  buffer = g_strdup_printf ("%s\n", line);
  write (fd, buffer, strlen (buffer));
  flush_buffer (buffer);
}

void
drop_chars (gint fd, gint count)
{
  gchar *buffer;

  buffer = read_chars (fd, count);
  flush_buffer (buffer);
}

void
drop_line (gint fd)
{
  gchar *buffer = read_line (fd);
  flush_buffer (buffer);
}

void
protocol_error ()
{
  g_critical ("Protocol Error!\n");
  exit (1);
}

gchar*
input_dialog (gchar *message, gboolean visible)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *entry;

  gchar *retval;

  dialog = gtk_window_new (GTK_WINDOW_POPUP);
  
  vbox = gtk_vbox_new (TRUE, 6);
  gtk_container_add (GTK_CONTAINER(dialog), vbox);

  label = gtk_label_new (message);
  gtk_box_pack_start_defaults (GTK_BOX(vbox), label);

  entry = gtk_entry_new ();
  g_signal_connect (G_OBJECT(entry), "active",
		    G_CALLBACK(gtk_main_quit), NULL);
  gtk_entry_set_visibility (GTK_ENTRY(entry), visible);
  gtk_box_pack_start_defaults (GTK_BOX(vbox), entry);

  gtk_widget_show_all (dialog);

  gtk_main ();

  retval = g_strdup (gtk_entry_get_text (GTK_ENTRY(entry)));

  gtk_widget_destroy (dialog);
  
  return retval;
}

int
main (int argc, char **argv)
{
  gchar *confdir;

  gchar *host = g_strdup ("localhost");
  gchar *port = g_strdup ("22");
  gchar *sshkey = NULL;

  gchar *user = NULL;
  gchar *password = NULL;

  gchar *session = "teste-gnome";
  gchar *type = "unix-gnome";
  gchar *cookie = "bbfb441eea5fe6f54f749a05015e58d0";

  gchar *link = "adsl";
  gchar *kbdtype = "pc104/us_intl";

  gchar *geometry = "800x600";
  gchar *screeninfo = "1024x768x24+render";

  gchar *session_id = NULL;
  gchar *session_display = NULL;
  gchar *pcookie = NULL;
  
  gchar **nxssh_argv = (gchar**) g_malloc (sizeof(gchar*) * 9);

  gint in, out;

  gchar *buffer = NULL;

  GError *gerr = NULL;

  gtk_init (&argc, &argv);

  homedir = g_get_home_dir ();
  confdir = g_strdup_printf ("%s/.nx/", homedir);

  if (!g_file_test (confdir, G_FILE_TEST_EXISTS))
    {
      if (mkdir (confdir, 0777) == -1)
	g_critical ("Could not create directory %s: %s\n", 
		    confdir, strerror (errno));
    }

  sshkey = g_strdup_printf ("%s/.ssh/id_dsa", homedir);

  nxssh_argv[0] = g_strdup ("/usr/bin/nxssh");
  nxssh_argv[1] = g_strdup ("-nx");
  nxssh_argv[2] = g_strdup_printf ("-p%s", port);
  nxssh_argv[3] = g_strdup ("-i");
  nxssh_argv[4] = g_strdup (sshkey);
  nxssh_argv[5] = g_strdup_printf ("nx@%s", host);
  nxssh_argv[6] = g_strdup ("-2");
  nxssh_argv[7] = g_strdup ("-S");
  nxssh_argv[8] = NULL;

  g_spawn_async_with_pipes (homedir, nxssh_argv, NULL, 
			    G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, 
			    NULL, &in, &out, NULL, &gerr);

  if (gerr)
    {
      g_critical ("Error spawning nxssh: %s\n", gerr->message);
      g_error_free (gerr);
      gerr = NULL;
    }

  /* Handle initial hand-shaking */
  {
    gboolean ssh_authed = FALSE;

    while (!ssh_authed)
      {
	buffer = read_code (out);
	if (!strcmp (buffer, "NX> 205"))
	  {
	    drop_line (out);
	    drop_line (out);
	    drop_chars (out, 56);
	    write_line (in, "yes");
	    drop_line (out);
	    drop_line (out);
	  }
	else if (!strcmp (buffer, "NX> 204"))
	  {
	    g_critical ("Failed to authenticate to SSH using the public key!\n");
	  }
	else if (!strcmp (buffer, "HELLO N"))
	  {
	    /* OK, time to say HELLO! */
	    ssh_authed = TRUE;
	  }
	else
	  protocol_error ();

	flush_buffer (buffer);

	buffer = read_line (out);
	flush_buffer (buffer);
      }
  }

  /* Handle HELLO */
  buffer = read_code (out);
  if (!strcmp (buffer, "NX> 105"))
    {
      flush_buffer (buffer);
      drop_chars (out, 1);
      write_line (in, "HELLO NXCLIENT - Version 1.4.0");
      drop_line (out);
    }
  else
    protocol_error ();

  /* Handle Login */
  buffer = read_code (out);
  if (!strcmp (buffer, "NX> 134"))
    {
      flush_buffer (buffer);
      buffer = read_line (out);
      flush_buffer (buffer);
    }
  else
    protocol_error ();

  buffer = read_code (out);
  if (!strcmp (buffer, "NX> 105"))
    {
      flush_buffer (buffer);
      drop_chars (out, 1);
      write_line (in, "login");
      drop_line (out);
    }
  else
    protocol_error ();

  user = input_dialog ("Usuário", TRUE);

  buffer = read_code (out);
  if (!strcmp (buffer, "NX> 101"))
    {
      flush_buffer (buffer);
      drop_chars (out, 7);
      write_line (in, user);
      drop_line (out);
    }
  else
    protocol_error ();

  password = input_dialog ("Senha", FALSE);

  buffer = read_code (out);
  if (!strcmp (buffer, "NX> 102"))
    {
      flush_buffer (buffer);
      drop_chars (out, 11);
      write_line (in, password);
      drop_line (out);
    }
  else
    protocol_error ();

  buffer = read_code (out);
  if (!strcmp (buffer, "NX> 103"))
    {
      flush_buffer (buffer);
      drop_line (out);
    }
  else
    protocol_error ();

  buffer = read_code (out);
  if (!strcmp (buffer, "NX> 105"))
    {
      gchar *cmdline;

      flush_buffer (buffer);
      drop_chars (out, 1);

      cmdline = g_strdup_printf ("startsession --session=\"%s\" --type=\"%s\" --cache=\"8M\" --images=\"32M\" --cookie=\"%s\" --link=\"%s\" --kbtype=\"%s\" --nodelay=\"1\" --backingstore=\"never\" --geometry=\"%s\" --media=\"0\" --agent_server=\"\" --agent_user=\"\" --agent_password=\"\" --screeninfo=\"%s\"", session, type, cookie, link, kbdtype, geometry, screeninfo);
      write_line (in, cmdline);
      g_free (cmdline);

      drop_line (out);
      drop_line (out);
      drop_line (out);
    }
  else
    protocol_error ();

  /* Handle session start information */  
  gchar *
    get_session_info (gchar *code)
    {
      gchar *buffer;
      gchar *retval;

      buffer = read_code (out);
      if (!strcmp (buffer, code))
	{
	  gchar **split_str;
	  
	  flush_buffer (buffer);
	  buffer = read_line (out);
	  
	  split_str = g_strsplit (buffer, ":", 2);
	  retval = g_strdup (split_str[1]);
	  g_strstrip (retval);
	  g_strfreev (split_str);

	  return retval;
	}
      else
	{
	  g_warning ("Waiting for %s, got %s\n", code, buffer);
	  protocol_error ();
	}
    }

  session_id = get_session_info ("NX> 700");
  session_display = get_session_info ("NX> 705");
  drop_line (out); /* 703 (session type) */
  pcookie = get_session_info ("NX> 701");
  drop_line (out); /* 702 proxy ip */
  drop_line (out); /* 706 agent cookie */
  drop_line (out); /* 704 session cache */
  drop_line (out); /* 707 ssl tunneling */
  drop_line (out); /* 710 session status: running */
  drop_line (out); /* 1002 commit */
  drop_line (out); /* 1006 session status: running */

  read_code (out);
  drop_chars (out, 1);

  /* now prepare to run nxproxy */
  {
    FILE *options;

    gchar *dirname;
    gchar *fname;
    gchar *cmdline;

    dirname = g_strdup_printf ("%s/.nx/S-%s", homedir, session_id);
    fname = g_strdup_printf ("%s/options", dirname);

    g_print ("Dir: %s\nFname: %s\n", dirname, fname);

    if (mkdir (dirname, 0777) == -1)
      {
	/* BOMB or handle 'directory already exists' */
      }
    g_free (dirname);

    buffer = g_strdup_printf ("cookie=%s,root=%s/.nx,session=%s,id=%s,connect=%s:%s", pcookie, homedir, session, session_id, host, session_display);

    options = fopen (fname, "w");
    fwrite (buffer, sizeof(char), strlen (buffer), options);
    fclose (options);

    g_free (buffer);

    cmdline = g_strdup_printf ("/usr/bin/nxproxy -S options=%s:%s", fname, session_display);
    system (cmdline);
    g_free (cmdline);

    g_free (fname);
  }

  return 0;
}
