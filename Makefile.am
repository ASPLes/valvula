if ENABLE_VALVULA_DOC
VALVULA_DOC_DIR = doc
endif

SUBDIRS = lib server plugins test $(VALVULA_DOC_DIR)
EXTRA_DIST = VERSION  valvula.pc.in valvulad.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = valvula.pc valvulad.pc

all-local: update-defs

update-defs: 
	cd lib; make update-def; cd ..

install-exec-hook:
	mkdir -p ${DESTDIR}$(sysconfdir)/valvula/mods-available
	mkdir -p ${DESTDIR}$(sysconfdir)/valvula/mods-enabled
	chmod o-rwx ${DESTDIR}$(sysconfdir)/valvula
