EXTRA_DIST = mod-ticket.xml

INCLUDES = -Wall -g -ansi -I ../../lib -I ../../server -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS)

lib_LTLIBRARIES      = mod-ticket.la
mod_ticket_la_SOURCES  = mod-ticket.c
mod_ticket_la_LDFLAGS  = -module -ldl $(top_builddir)/lib/libvalvula.la $(top_builddir)/server/libvalvulad.la

# reconfigure module installation directory
libdir = $(prefix)/lib/valvulad/modules

# configure site module installation
modconfdir   = $(sysconfdir)/valvulad/mods-available
modconf_DATA = mod-ticket.xml
