EXTRA_DIST = exarg.h exarg.c 

etcdir = $(sysconfdir)/valvula
etc_DATA = valvula.example.conf

if ENABLE_POLL_SUPPORT
INCLUDE_VALVULA_POLL=-DVALVULA_HAVE_POLL=1
endif

if ENABLE_EPOLL_SUPPORT
INCLUDE_VALVULA_EPOLL=-DVALVULA_HAVE_EPOLL=1
endif

if ENABLE_VALVULA_LOG
INCLUDE_VALVULA_LOG=-DENABLE_VALVULA_LOG
endif

noinst_PROGRAMS = valvulad

INCLUDES = -I$(top_srcdir)/lib  $(AXL_CFLAGS)  $(PTHREAD_CFLAGS) \
	-I$(READLINE_PATH)/include $(compiler_options) -D__axl_disable_broken_bool_def__   \
        -DVERSION=\""$(VALVULA_VERSION)"\" -I$(top_srcdir)/src $(INCLUDE_VALVULA_POLL) $(INCLUDE_VALVULA_EPOLL) $(INCLUDE_VALVULA_LOG) $(EXARG_FLAGS)

LIBS            = $(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS) $(NOPOLL_LIBS)

valvulad_SOURCES   = \
	valvulad.c valvulad.h \
	valvulad_config.c valvulad_config.h \
	valvulad_log.c valvulad_log.h \
	exarg.c \
	exarg.h

valvulad_LDADD     = $(LIBS) $(top_builddir)/lib/libvalvula.la


# copy exarg dependency to not force people to download a library with
# two files
exarg.h exarg.c: 
	test -e exarg.h || cp ~/af-arch/libexarg/src/exarg.h .
	test -e exarg.c || cp ~/af-arch/libexarg/src/exarg.c .

update-exarg:
	cp ~/af-arch/libexarg/src/exarg.[ch] .