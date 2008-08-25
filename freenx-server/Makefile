.PHONY: all install clean nxenv_install suid_install

SHELL = /bin/bash

SUBDIRS=nxredir nxviewer-passwd nxserver-helper nxserver-suid nx-session-launcher
PROGRAMS=nxacl.sample nxcheckload.sample nxcups-gethost nxdesktop_helper nxdialog nxkeygen nxloadconfig nxnode nxnode-login nxprint nxserver nxserver-helper/nxserver-helper nxsetup nxviewer_helper nxviewer-passwd/nxpasswd/nxpasswd nx-session-launcher/nx-session-launcher nx-session-launcher/nx-session-launcher-suid nxserver-usermode nxserver-suid/nxserver-suid

all:
	cd nxviewer-passwd && xmkmf && make Makefiles && make depend
	source nxloadconfig &&\
	export PATH_BIN PATH_LIB CUPS_BACKEND NX_VERSION NX_ETC_DIR &&\
	for i in $(SUBDIRS) ; \
	do\
		echo "making" all "in $$i..."; \
	        $(MAKE) -C $$i all || exit 1;\
	done

suid_install:
	chown nx:root $(DESTDIR)/$$PATH_BIN/nxserver-suid
	chmod 4755 $(DESTDIR)/$$PATH_BIN/nxserver-suid

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
	# uncomment the following line to make
	# nxserver-suid suid nx
	#$(MAKE) suid_install

clean:
	make -C nxviewer-passwd clean
	for i in $(SUBDIRS) ; \
	do\
		echo "making" clean "in $$i..."; \
	        $(MAKE) -C $$i clean || exit 1;\
	done

install:
	source nxloadconfig &&\
	export PATH_BIN PATH_LIB CUPS_BACKEND NX_VERSION NX_ETC_DIR &&\
	$(MAKE) nxenv_install
