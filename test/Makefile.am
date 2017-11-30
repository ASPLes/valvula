EXTRA_DIST = \
	test_01.conf \
	test_03.conf \
	test_02b.postfix.cf test_02b.postfix.variables.cf test_02b.postfix.variables.old-interface.cf \
	test_02b.conf \
	test_03.conf \
	test_05.conf \
	test_02.conf.ref \
	test_01_multi_first_comment2.base.ref \
	test_01_multi_first_comment.base.ref \
	test_01_multi_first.base.ref \
	test_01.base.ref \
	test_01_last.base.ref \
	test-02b-mysql-domains.cf  \
	test-02b-mysql-accounts.cf \
	test-02b-mysql-aliases.cf \
	test_06.conf \
	test_06.same-domain.conf \
	test_06.same-domain.02.conf \
	test_06.valid-from.conf \
	test_07.conf \
	test_07a.conf \
	test_07a2.conf \
	test_07a3.conf


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

INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/server -I$(top_srcdir)/plugins/mod-mquota  $(AXL_CFLAGS)  $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS) \
	-I$(READLINE_PATH)/include $(compiler_options) -D__axl_disable_broken_bool_def__   \
        -DVERSION=\""$(VALVULA_VERSION)"\" -I$(top_srcdir)/src $(INCLUDE_VALVULA_POLL) $(INCLUDE_VALVULA_EPOLL) $(INCLUDE_VALVULA_LOG) $(EXARG_FLAGS)

LIBS            = $(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS) $(NOPOLL_LIBS) $(SQLITE3_LIBS)

test_01_SOURCES = test_01.c
test_01_LDADD = $(AXL_LIBS) $(VALVULA_LIBS) $(top_srcdir)/server/libvalvulad.la $(top_srcdir)/lib/libvalvula.la $(top_srcdir)/plugins/mod-mquota/mod-mquota.la $(top_srcdir)/plugins/mod-bwl/mod-bwl.la

