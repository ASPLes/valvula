if ENABLE_VALVULA_DOC
VALVULA_DOC_DIR = doc
endif

SUBDIRS = lib server plugins test $(ENABLE_VALVULA_DOC)
EXTRA_DIST = VERSION 

all-local: update-defs

update-defs: 
	cd lib; make update-def; cd ..

install-exec-hook:
	mkdir -p ${DESTDIR}$(sysconfdir)/valvula/mods-available
	mkdir -p ${DESTDIR}$(sysconfdir)/valvula/mods-enabled
	chmod o-rwx ${DESTDIR}$(sysconfdir)/valvula
