EXTRA_DIST = 

if ENABLE_VALVULA_LOG
INCLUDE_VALVULA_LOG=-DENABLE_VALVULA_LOG
endif

if ENABLE_POLL_SUPPORT
INCLUDE_VALVULA_POLL=-DVALVULA_HAVE_POLL=1
endif

if ENABLE_EPOLL_SUPPORT
INCLUDE_VALVULA_EPOLL=-DVALVULA_HAVE_EPOLL=1
endif

if DEFAULT_EPOLL
INCLUDE_DEFAULT_EPOLL=-DDEFAULT_EPOLL 
endif

if DEFAULT_POLL
INCLUDE_DEFAULT_POLL=-DDEFAULT_POLL 
endif

INCLUDES = $(compiler_options) -I$(top_srcdir) -D__COMPILING_VALVULA__ -D__axl_disable_broken_bool_def__  -D_GNU_SOURCE \
	$(AXL_CFLAGS) $(INCLUDE_VALVULA_LOG) $(PTHREAD_CFLAGS) \
	-DVERSION=\""$(VALVULA_VERSION)"\" \
	-DPACKAGE_DTD_DIR=\""$(datadir)"\" \
	-DPACKAGE_TOP_DIR=\""$(top_srcdir)"\" $(INCLUDE_VALVULA_POLL) $(INCLUDE_VALVULA_EPOLL) $(INCLUDE_DEFAULT_EPOLL) $(INCLUDE_DEFAULT_POLL)

libvalvula_includedir = $(includedir)/valvula

lib_LTLIBRARIES = libvalvula.la

libvalvula_la_SOURCES =	valvula.c \
	valvula_reader.c \
	valvula_io.c \
	valvula_thread.c \
	valvula_thread_pool.c \
	valvula_ctx.c \
	valvula_support.c \
	valvula_listener.c \
	valvula_connection.c \
	valvula_hash.c

libvalvula_include_HEADERS = valvula.h \
	valvula_reader.h \
	valvula_support.h \
	valvula_ctx.h \
	valvula_private.h \
	valvula_io.h \
	valvula_handlers.h \
	valvula_thread.h \
	valvula_thread_pool.h \
	valvula_types.h \
	valvula_listener.h \
	valvula_connection.h \
	valvula_hash.h

libvalvula_la_LIBADD = \
	$(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS)

libvalvula_la_LDFLAGS = -no-undefined -export-symbols-regex '^(valvula|__valvula|_valvula).*'

libvalvula.def: update-def

update-def:
	echo "EXPORTS" > libvalvula.def
	cat .libs/libvalvula.exp | grep -v io_waiting_poll | grep -v io_waiting_epoll | grep -v __valvula >> libvalvula.def
	echo "__valvula_connection_set_not_connected" >> libvalvula.def
	echo "gettimeofday" >> libvalvula.def
