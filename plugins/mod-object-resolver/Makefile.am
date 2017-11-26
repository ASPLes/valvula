EXTRA_DIST = mod-object-resolver.xml

INCLUDES = -Wall -g -ansi -I ../../lib -I ../../server -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS)

lib_LTLIBRARIES      = mod-object-resolver.la
mod_object_resolver_la_SOURCES  = mod-object-resolver.c
mod_object_resolver_la_LDFLAGS  = -module -ldl $(top_builddir)/lib/libvalvula.la $(top_builddir)/server/libvalvulad.la

# reconfigure module installation directory
libdir = $(prefix)/lib/valvulad/modules

# configure site module installation
modconfdir   = $(sysconfdir)/valvula/mods-available
modconf_DATA = mod-object-resolver.xml
