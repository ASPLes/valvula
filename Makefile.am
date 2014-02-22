SUBDIRS = lib server plugins test
EXTRA_DIST = VERSION 

all-local: update-defs

update-defs: 
	cd lib; make update-def; cd ..

