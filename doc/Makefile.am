EXTRA_DIST = valvula.doxygen \
	     valvula.doxygen.in \
	     valvula-footer.html \
	     valvula-header.html \
	     main-page-logo.png \
	     valvula.css \
	     body_back.gif \
	     aspl-logo-header.png \
	     component-separation.png \
	     threaded-design.png \
	     port-separation.png \
	     automatic-conf.png \
	     robust-implementation.png \
	     stats-support.png  \
	     administrator-friendly.png \
	     adding-modules.png \
	     valvulad-open-source.png \
	     hacha-100x171.png \
	     valvulad-init.d \
             valvulad-rpm-init.d

bin_SCRIPTS = initial_build_doc

help:
	@echo 
	@echo " Documentation not build!:"
	@echo " ~~~~~~~~~~~~~~~~~~~~~~~~~"
	@echo " --> Doxygen documentation wasn't built because an stamp was detected: doc.stamp"
	@echo " --> To regenerate or force doc building type make build_doc or remove doc.stamp";
	@echo

initial_build_doc: 
	@echo "checking documenation stamp..";
	@test -e doc.stamp & make help
	@test -e doc.stamp || make build_doc
	@test -e doc.stamp || touch doc.stamp

build_doc: prepare_xmls
	rm -rf html/
	@echo "Generating module documentation..";
	doxygen valvula.doxygen
	cp $(top_srcdir)/doc/main-page-logo.png html
	cp $(top_srcdir)/doc/aspl-logo-header.png html
	cp $(top_srcdir)/doc/valvula.css html
	cp $(top_srcdir)/doc/body_back.gif html
	cp $(top_srcdir)/doc/component-separation.png html
	cp $(top_srcdir)/doc/threaded-design.png html
	cp $(top_srcdir)/doc/port-separation.png html
	cp $(top_srcdir)/doc/automatic-conf.png html
	cp $(top_srcdir)/doc/robust-implementation.png html
	cp $(top_srcdir)/doc/stats-support.png html
	cp $(top_srcdir)/doc/administrator-friendly.png html
	cp $(top_srcdir)/doc/adding-modules.png html
	cp $(top_srcdir)/doc/valvulad-open-source.png html
	cp $(top_srcdir)/doc/hacha-100x171.png html

prepare_xmls:
	axl-knife -i ../server/valvula.example.conf -o valvula.example.conf.xml -e -p fragment
	axl-knife -i ../plugins/mod-mw/mod-mw.example.conf -o mod-mw.example.conf.xml -e -p fragment

all: build_doc

