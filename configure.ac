dnl  Valvula: a high performance policy daemon
dnl  Copyright (C) 2014 Advanced Software Production Line, S.L.
dnl
dnl  This program is free software; you can redistribute it and/or
dnl  modify it under the terms of the GNU General Public License as
dnl  published by the Free Software Foundation; either version 2.1 of
dnl  the License, or (at your option) any later version.
dnl
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl  General Public License for more details.
dnl
dnl  You should have received a copy of the GNU General Public License
dnl  along with this program; if not, write to the Free Software
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
dnl  02111-1307 USA
dnl  
dnl  You may find a copy of the license under this software is
dnl  released at COPYING file.
dnl
dnl  For comercial support about integrating valvula or any other ASPL
dnl  software production please contact as at:
dnl          
dnl      Postal address:
dnl
dnl         Advanced Software Production Line, S.L.
dnl         C/ Antonio Suarez Nº10, Edificio Alius A, Despacho 102
dnl         Alcalá de Henares 28802 Madrid
dnl         Spain
dnl
dnl      Email address: info@aspl.es - http://www.aspl.es/valvula
dnl

AC_INIT(lib/valvula.h)

configure_dir=`dirname "$0"`
VALVULA_VERSION=`cat $configure_dir/VERSION`
AC_SUBST(VALVULA_VERSION)

axl_string_version=axl-`axl-knife --version`
AC_SUBST(axl_string_version)

AC_CONFIG_AUX_DIR(.)
AM_INIT_AUTOMAKE(valvula, $VALVULA_VERSION)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_PROG_CC
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_LIBTOOL

compiler_options=""
STRICT_PROTOTYPES=""
if test "$compiler" = "gcc" ; then
   compiler_options="-Wstrict-prototypes -Wall -Werror -g -ansi -fstack-protector-all -Wstack-protector"
   echo "Detected gcc compiler: $compiler, adding options: $compiler_options"
fi
AC_SUBST(compiler_options)

AC_CHECK_PROG(PKG_CONFIG, pkg-config, "yes", "no")
if test "$PKG_CONFIG" = "no" ; then
  AC_MSG_ERROR([You need to install pkg-config to compile Valvula 1.1. See: http://pkgconfig.freedesktop.org/releases/])
fi

dnl
dnl Thread detection support mostly taken from the apache project 2.2.3.
dnl
dnl VALVULA_PTHREADS_TRY_RUN(actions-if-success)
dnl
dnl Try running a program which uses pthreads, executing the
dnl actions-if-success commands on success.
dnl
AC_DEFUN([VALVULA_PTHREADS_TRY_RUN], [
AC_TRY_RUN( [
#include <pthread.h>
#include <stddef.h>

void *thread_routine(void *data) {
    return data;
}

int main() {
    pthread_t thd;
    pthread_mutexattr_t mattr;
    pthread_once_t once_init = PTHREAD_ONCE_INIT;
    int data = 1;
    pthread_mutexattr_init(&mattr);
    return pthread_create(&thd, NULL, thread_routine, &data);
} ], [valvula_p_t_r=yes], [valvula_p_t_r=no], [valvula_p_t_r=no])

if test $valvula_p_t_r = yes; then
  $1
fi

])dnl


dnl
dnl VALVULA_PTHREADS_CHECK()
dnl
dnl Try to find a way to enable POSIX threads.  Sets the 
dnl pthreads_working variable to "yes" on success.
dnl
AC_DEFUN([VALVULA_PTHREADS_CHECK],[

AC_CACHE_CHECK([for CFLAGS needed for pthreads], [valvula_cv_pthreads_cflags],
[valvula_ptc_cflags=$CFLAGS
 for flag in -kthread -pthread -pthreads -mt -mthreads -Kthread -threads; do 
    CFLAGS=$valvula_ptc_cflags
    test "x$flag" != "xnone" && CFLAGS="$CFLAGS $flag"
    VALVULA_PTHREADS_TRY_RUN([
      valvula_cv_pthreads_cflags="$flag"
      break
    ])
 done
 CFLAGS=$valvula_ptc_cflags
])

# The CFLAGS may or may not be sufficient to ensure that libvalvula
# depends on the pthreads library: some versions of libtool
# drop -pthread when passed on the link line; some versions of
# gcc ignore -pthread when linking a shared object.  So always
# try and add the relevant library to LIBS too.

AC_CACHE_CHECK([for LIBS needed for pthreads], [valvula_cv_pthreads_lib], [
  valvula_ptc_libs=$LIBS
  for lib in -lpthread -lpthreads -lc_r; do
    LIBS="$valvula_ptc_libs $lib"
    VALVULA_PTHREADS_TRY_RUN([
      valvula_cv_pthreads_lib=$lib
      break
    ])
  done
  LIBS=$valvula_ptc_libs
])

if test "$pthreads_working" = "yes"; then
  threads_result="POSIX Threads found"
else
  threads_result="POSIX Threads not found"
fi
])dnl

dnl call to detect thread activation support
VALVULA_PTHREADS_CHECK

PTHREAD_CFLAGS="$valvula_cv_pthreads_cflags"
PTHREAD_LIBS="$valvula_cv_pthreads_lib"
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)

dnl check for console log
AC_ARG_ENABLE(valvula-log, [  --enable-valvula-log       Enable building Valvula Library console debug log support [default=yes]], 
	      enable_valvula_log="$enableval", 
	      enable_valvula_log=yes)
AM_CONDITIONAL(ENABLE_VALVULA_LOG, test "x$enable_valvula_log" = "xyes")

dnl check for poll support
AC_CHECK_HEADER(sys/poll.h, enable_poll=yes, enable_poll=no)
AM_CONDITIONAL(ENABLE_POLL_SUPPORT, test "x$enable_poll" = "xyes")

dnl Check for the Linux epoll interface; epoll* may be available in libc
dnl with Linux kernels 2.6.X
AC_CACHE_CHECK([for epoll(2) support], [enable_cv_epoll],
[AC_TRY_RUN([
#include <sys/epoll.h>
#include <unistd.h>

int main()
{
    return epoll_create(5) == -1;
}], [enable_cv_epoll=yes], [enable_cv_epoll=no], [enable_cv_epoll=no])])
AM_CONDITIONAL(ENABLE_EPOLL_SUPPORT, test "x$enable_cv_epoll" = "xyes")

dnl select the best I/O platform
if test x$enable_cv_epoll = xyes ; then
   default_platform="epoll"
elif test x$enable_poll = xyes ; then
   default_platform="poll"
else 
   default_platform="select"
fi
AM_CONDITIONAL(DEFAULT_EPOLL, test "x$default_platform" = "xepoll")
AM_CONDITIONAL(DEFAULT_POLL, test "x$default_platform" = "xpoll")

dnl LibAxl library support.
PKG_CHECK_MODULES(AXL, axl >= 0.6.4)
AC_SUBST(AXL_CFLAGS)
AC_SUBST(AXL_LIBS)

dnl general libries subsitution
dnl AC_SUBST(LIBRARIES_CFLAGS)
dnl AC_SUBST(LIBRARIES_LIBS)

dnl get current platform and add especific flags
case $host in
  *-*-beos*)
    echo "Found BEOS platform: $host.."
    ;;
  *-*-cygwin*)
    echo "Found cygwin platform: $host.."
    ;;
  *-*-mingw*)
    echo "Found mingw platform: $host.."
    ADDITIONAL_LIBS=-lws2_32
    ;;
  *)
    echo "Found platform: $host.."
    ;;
esac
AC_SUBST(ADDITIONAL_LIBS)

# check additional flags for exarg
AC_TRY_LINK([#define _GNU_SOURCE
#include <stdio.h>], 
[
  char * result;
  return vasprintf (&result, "This is a test: %d", NULL);
], [have_vasprintf=yes],[have_vasprintf=no])
echo "Checking vasprintf support: $have_vasprintf"

dnl support for vasprintf
case $have_vasprintf in
yes)
	EXARG_FLAGS="-DHAVE_VASPRINTF $EXARG_FLAGS"
	echo "vasprintf support found"
     ;;
esac

dnl check for windows platform
case $host in
*-*-mingw*)
	EXARG_FLAGS="-DOS_WIN32 $EXARG_FLAGS"
	echo "windows platform found"
	;;	
*)
	echo "platform found: $host"
    ;;
esac
AC_SUBST(EXARG_FLAGS)	

dnl mysql-support flags
MYSQL_CFLAGS=`mysql_config --cflags 2>/dev/null`
MYSQL_LIBS=`mysql_config --libs 2>/dev/null`
AC_SUBST(MYSQL_CFLAGS)
AC_SUBST(MYSQL_LIBS)
AC_TRY_COMPILE([#include <mysql/mysql.h>
#include <stdlib.h>],
  [mysql_init(NULL);],
  mysql_devel_found=yes,
  mysql_devel_found=no)
AC_MSG_RESULT([MySQL development headers status: $mysql_devel_found])
if test "x$mysql_devel_found" = "xno"; then
   AC_MSG_WARN([Cannot find mysql.h header or current MySQL development environment do not compile symbols required. On debian try to install libmysqlclient15-dev package.])
fi
AM_CONDITIONAL(ENABLE_MYSQL_SUPPORT, test "x$mysql_devel_found" = "xyes")

AC_OUTPUT([
Makefile
lib/Makefile
server/Makefile
plugins/Makefile
plugins/mod-test/Makefile
plugins/mod-ticket/Makefile
plugins/mod-bwl/Makefile
plugins/mod-slm/Makefile
test/Makefile
doc/valvula.doxygen
doc/Makefile
])

echo "-----------------------------------------------"
echo "--     VALVULA POLICY SERVER 1.1 SETTINGS    --"
echo "-----------------------------------------------"
echo "   Installation prefix:            [$prefix]"
echo "      select(2) support:           [yes]"
echo "      poll(2) support:             [$enable_poll]"
echo "      epoll(2) support:            [$enable_cv_epoll]"
echo "      default:                     [$default_platform]"
echo "      debug log support:           [$enable_valvula_log]"
echo "      pthread cflags=$PTHREAD_CFLAGS, libs=$PTHREAD_LIBS"
echo "      additional libs=$ADDITIONAL_LIBS"
if test x$enable_valvula_log = xyes ; then
echo "      NOTE: To disable log reporting use: "
echo "            --disable-valvula-log"               
fi
echo
mysql_msg=""
if test "x$mysql_devel_found" = "xno"; then
     mysql_msg=" (Support for MySQL, won't work)"
fi

echo "   Build with MySQL support:       [$mysql_devel_found]$mysql_msg"
if test "x$mysql_devel_found" = "xyes"; then
echo "     flags: $MYSQL_CFLAGS"
echo "     libs: $MYSQL_LIBS"
fi
echo ""
echo "   Axl installation: "
echo "      cflags: $AXL_CFLAGS"
echo "      libs:   $AXL_LIBS"
echo
echo
echo "------------------------------------------"
echo "--     NOW TYPE: make; make install     --"
echo "------------------------------------------"
