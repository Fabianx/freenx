.PHONY: all install

SHELL = /bin/bash

SUBDIRS=nxredir nxviewer-passwd nxserver-helper
PROGRAMS=nxacl.sample nxcheckload.sample nxcups-gethost nxdesktop_helper nxdialog nxkeygen nxloadconfig nxnode nxnode-login nxprint nxserver nxserver-helper/nxserver-helper nxsetup nxviewer_helper nxviewer-passwd/nxpasswd/nxpasswd

all:
	( cd nxviewer-passwd; xmkmf; make Makefiles; make depend; cd ..)
	for i in $(SUBDIRS) ; \
	do\
		echo "making" all "in $$i..."; \
	        $(MAKE) -C $$i  all;\
	done

install:
	. nxloadconfig;\
	install -m755 -d $(DESTDIR)/$$PATH_BIN/ ;\
	install -m755 -d $(DESTDIR)/$$PATH_LIB/ ;\
	install -m755 -d $(DESTDIR)/$$NX_ETC_DIR/ ;\
	install -m755 -d $(DESTDIR)/$$CUPS_BACKEND/ ;\
	for i in $(PROGRAMS) ; \
	do\
	        install -m755 $$i $(DESTDIR)/$$PATH_BIN/ ;\
	done;\
	export PATH_BIN PATH_LIB CUPS_BACKEND NX_VERSION;\
	install -m644 node.conf.sample $(DESTDIR)/$$NX_ETC_DIR/ ; \
	make -C nxredir install
