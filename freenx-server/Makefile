.PHONY: all install clean

SHELL = /bin/bash

SUBDIRS=nxredir nxviewer-passwd nxserver-helper nx-session-launcher
PROGRAMS=nxacl.sample nxcheckload.sample nxcups-gethost nxdesktop_helper nxdialog nxkeygen nxloadconfig nxnode nxnode-login nxprint nxserver nxserver-helper/nxserver-helper nxsetup nxviewer_helper nxviewer-passwd/nxpasswd/nxpasswd nx-session-launcher/nx-session-launcher nx-session-launcher/nx-session-launcher-suid

all:
	cd nxviewer-passwd && xmkmf && make Makefiles && make depend
	source nxloadconfig &&\
	export PATH_BIN PATH_LIB CUPS_BACKEND NX_VERSION &&\
	for i in $(SUBDIRS) ; \
	do\
		echo "making" all "in $$i..."; \
	        $(MAKE) -C $$i all || exit 1;\
	done

nxenv_install:
	install -m755 -d $(DESTDIR)/$$PATH_BIN/
	install -m755 -d $(DESTDIR)/$$PATH_LIB/
	install -m755 -d $(DESTDIR)/$$NX_ETC_DIR/
	install -m755 -d $(DESTDIR)/$$CUPS_BACKEND/
	for i in $(PROGRAMS) ;\
	do\
	        install -m755 $$i $(DESTDIR)/$$PATH_BIN/ || exit 1;\
	done
	install -m644 node.conf.sample $(DESTDIR)/$$NX_ETC_DIR/
	$(MAKE) -C nxredir install

clean:
	make -C nxviewer-passwd clean
	for i in $(SUBDIRS) ; \
	do\
		echo "making" clean "in $$i..."; \
	        $(MAKE) -C $$i clean || exit 1;\
	done

install:
	source nxloadconfig &&\
	export PATH_BIN PATH_LIB CUPS_BACKEND NX_VERSION &&\
	$(MAKE) nxenv_install
