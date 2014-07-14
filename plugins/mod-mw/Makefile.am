EXTRA_DIST = mod-mw.xml mod-mw.example.conf

INCLUDES = -Wall -g -ansi -I ../../lib -I ../../server -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS)

lib_LTLIBRARIES      = mod-mw.la
mod_mw_la_SOURCES  = mod-mw.c
mod_mw_la_LDFLAGS  = -module -ldl $(top_builddir)/lib/libvalvula.la $(top_builddir)/server/libvalvulad.la

# reconfigure module installation directory
libdir = $(prefix)/lib/valvulad/modules

# configure site module installation
modconfdir   = $(sysconfdir)/valvula/mods-available
modconf_DATA = mod-mw.xml
