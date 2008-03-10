CLEANFILES = *~

DOXYFILE = Doxyfile

docdir = $(prefix)/doc/$(PACKAGE)-$(VERSION)

EXTRA_DIST = html

SEDCMD1 = s/$$title/GNU nxcl documentation version $(VERSION)/g
SEDCMD2 = s/$$(VERSION)/$(VERSION)/g

all: html

html:
	doxygen

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(docdir)
	@for file in $(srcdir)/html/*; do \
		bn="`basename $$file`"; \
		echo " $(INSTALL_DATA) $$file $(DESTDIR)$(docdir)/$$bn"; \
		$(INSTALL_DATA) $$file $(DESTDIR)$(docdir)/$$bn; \
	done

uninstall-local:
	@for file in $(srcdir)/html/*; do \
		bn="`basename $$file`"; \
		echo " rm -f $(DESTDIR)$(docdir)/$$bn"; \
		rm -f $(DESTDIR)$(docdir)/$$bn; \
	done

