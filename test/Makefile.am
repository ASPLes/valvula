EXTRA_DIST = 

if ENABLE_POLL_SUPPORT
INCLUDE_VALVULA_POLL=-DVALVULA_HAVE_POLL=1
endif

if ENABLE_EPOLL_SUPPORT
INCLUDE_VALVULA_EPOLL=-DVALVULA_HAVE_EPOLL=1
endif

if ENABLE_VALVULA_LOG
INCLUDE_VALVULA_LOG=-DENABLE_VALVULA_LOG
endif

noinst_PROGRAMS = test_01

INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/server  $(AXL_CFLAGS)  $(PTHREAD_CFLAGS) \
	-I$(READLINE_PATH)/include $(compiler_options) -D__axl_disable_broken_bool_def__   \
        -DVERSION=\""$(VALVULA_VERSION)"\" -I$(top_srcdir)/src $(INCLUDE_VALVULA_POLL) $(INCLUDE_VALVULA_EPOLL) $(INCLUDE_VALVULA_LOG) $(EXARG_FLAGS)

LIBS            = $(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS) $(NOPOLL_LIBS)

test_01_SOURCES = test_01.c
test_01_LDADD = $(AXL_LIBS) $(VALVULA_LIBS) $(top_srcdir)/server/libvalvulad.la $(top_srcdir)/lib/libvalvula.la

