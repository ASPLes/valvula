/* 
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2014 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. 
 *
 *  For comercial support about integrating valvula or any other ASPL
 *  software production please contact as at:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcalá de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/valvula
 */

#ifndef __VALVULA_H__
#define __VALVULA_H__

/**
 * \addtogroup valvula
 * @{
 */

/* define default socket pool size for the VALVULA_IO_WAIT_SELECT
 * method. If you change this value, you must change the
 * following. This value must be synchronized with FD_SETSIZE. This
 * has been tested on windows to work properly, but under GNU/Linux,
 * the GNUC library just rejects providing this value, no matter where
 * you place them. The only solutions are:
 *
 * [1] Modify /usr/include/bits/typesizes.h the macro __FD_SETSIZE and
 *     update the following values: FD_SETSIZE and VALVULA_FD_SETSIZE.
 *
 * [2] Use better mechanism like poll or epoll which are also available 
 *     in the platform that is giving problems.
 * 
 * [3] The last soluction could be the one provided by you. Please report
 *     any solution you may find.
 **/
#ifndef VALVULA_FD_SETSIZE
#define VALVULA_FD_SETSIZE 1024
#endif
#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif

/* External header includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Axl library headers */
#include <axl.h>

/* Direct portable mapping definitions */
#if defined(AXL_OS_UNIX)

/* Portable definitions while using Valvula Library */
#define VALVULA_EINTR           EINTR
#define VALVULA_EWOULDBLOCK     EWOULDBLOCK
#define VALVULA_EINPROGRESS     EINPROGRESS
#define VALVULA_EAGAIN          EAGAIN
#define VALVULA_SOCKET          int
#define VALVULA_INVALID_SOCKET  -1
#define VALVULA_SOCKET_ERROR    -1
#define valvula_close_socket(s) do {if ( s >= 0) {close (s);}} while (0)
#define valvula_getpid          getpid
#define valvula_sscanf          sscanf
#define valvula_is_disconnected (errno == EPIPE)
#define VALVULA_FILE_SEPARATOR "/"

#endif /* end defined(AXL_OS_UNIX) */

#if defined(AXL_OS_WIN32)

/* additional includes for the windows platform */

/* _WIN32_WINNT note: If the application including the header defines
 * the _WIN32_WINNT, it must include the bit defined by the value
 * 0x400. */
#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x400
#endif
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <time.h>

#define VALVULA_EINTR           WSAEINTR
#define VALVULA_EWOULDBLOCK     WSAEWOULDBLOCK
#define VALVULA_EINPROGRESS     WSAEINPROGRESS
#define VALVULA_EAGAIN          WSAEWOULDBLOCK
#define SHUT_RDWR              SD_BOTH
#define SHUT_WR                SD_SEND
#define VALVULA_SOCKET          SOCKET
#define VALVULA_INVALID_SOCKET  INVALID_SOCKET
#define VALVULA_SOCKET_ERROR    SOCKET_ERROR
#define valvula_close_socket(s) do {if ( s >= 0) {closesocket (s);}} while (0)
#define valvula_getpid          _getpid
#define valvula_sscanf          sscanf
#define uint16_t               u_short
#define valvula_is_disconnected ((errno == WSAESHUTDOWN) || (errno == WSAECONNABORTED) || (errno == WSAECONNRESET))
#define VALVULA_FILE_SEPARATOR "\\"

/* no link support windows */
#define S_ISLNK(m) (0)

#endif /* end defined(AXL_OS_WINDOWS) */

#if defined(AXL_OS_UNIX)
#include <sys/types.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#endif

/* additional headers for poll support */
#if defined(VALVULA_HAVE_POLL)
#include <sys/poll.h>
#endif

/* additional headers for linux epoll support */
#if defined(VALVULA_HAVE_EPOLL)
#include <sys/epoll.h>
#endif

/* Check gnu extensions, providing an alias to disable its precence
 * when no available. */
#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8)
#  define GNUC_EXTENSION __extension__
#else
#  define GNUC_EXTENSION
#endif

/* define minimal support for int64 constants */
#ifndef _MSC_VER 
#  define INT64_CONSTANT(val) (GNUC_EXTENSION (val##LL))
#else /* _MSC_VER */
#  define INT64_CONSTANT(val) (val##i64)
#endif

/* check for missing definition for S_ISDIR */
#ifndef S_ISDIR
#  ifdef _S_ISDIR
#    define S_ISDIR(x) _S_ISDIR(x)
#  else
#    ifdef S_IFDIR
#      ifndef S_IFMT
#        ifdef _S_IFMT
#          define S_IFMT _S_IFMT
#        endif
#      endif
#       ifdef S_IFMT
#         define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#       endif
#    endif
#  endif
#endif

/* check for missing definition for S_ISREG */
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
# define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif 

/** 
 * @brief Returns the minimum from two values.
 * @param a First value to compare.
 * @param b Second value to compare.
 */
#define VALVULA_MIN(a,b) ((a) > (b) ? b : a)

/** 
 * @brief Allows to check the reference provided, and returning the
 * return value provided.
 * @param ref The reference to be checke for NULL.
 * @param return_value The return value to return in case of NULL reference.
 */
#define VALVULA_CHECK_REF(ref, return_value) do { \
	if (ref == NULL) {   		         \
             return return_value;                \
	}                                        \
} while (0)

/** 
 * @brief Allows to check the reference provided, returning the return
 * value provided, also releasing a second reference with a custom
 * free function.
 * @param ref The reference to be checke for NULL.
 * @param return_value The return value to return in case of NULL reference.
 * @param ref2 Second reference to be released
 * @param free2_func Function to be used to release the second reference.
 */
#define VALVULA_CHECK_REF2(ref, return_value, ref2, free2_func) do { \
        if (ref == NULL) {                                          \
               free2_func (ref);                                    \
	       return return_value;                                 \
	}                                                           \
} while (0)

#if defined(AXL_OS_WIN32)
#include <valvula_win32.h>
#endif

#include <errno.h>

#if defined(AXL_OS_WIN32)
/* errno redefinition for windows platform. this declaration must
 * follow the previous include. */
#ifdef  errno
#undef  errno
#endif
#define errno (WSAGetLastError())
#endif

/* console debug support:
 *
 * If enabled, the log reporting is activated as usual. If log is
 * stripped from valvula building all instructions are removed.
 */
#if defined(ENABLE_VALVULA_LOG)
# define valvula_log(l, m, ...)   do{_valvula_log  (ctx, __AXL_FILE__, __AXL_LINE__, l, m, ##__VA_ARGS__);}while(0)
# define valvula_log2(l, m, ...)   do{_valvula_log2  (ctx, __AXL_FILE__, __AXL_LINE__, l, m, ##__VA_ARGS__);}while(0)
#else
# if defined(AXL_OS_WIN32) && !( defined(__GNUC__) || _MSC_VER >= 1400)
/* default case where '...' is not supported but log is still
 * disabled */
#   define valvula_log _valvula_log
#   define valvula_log2 _valvula_log2
# else
#   define valvula_log(l, m, ...) /* nothing */
#   define valvula_log2(l, m, message, ...) /* nothing */
# endif
#endif

/** 
 * @internal The following definition allows to find printf like wrong
 * argument passing to nopoll_log function. To activate the depuration
 * just add the following header after this comment.
 *
 * #define SHOW_FORMAT_BUGS (1)
 */
#if defined(SHOW_FORMAT_BUGS)
# undef  valvula_log
# define valvula_log(l, m, ...)   do{printf (m, ##__VA_ARGS__);}while(0)
#endif

/** 
 * @internal Allows to check a condition and return if it is not meet.
 * 
 * @param expr The expresion to check.
 */
#define v_return_if_fail(expr) \
if (!(expr)) {return;}

/** 
 * @internal Allows to check a condition and return the given value if it
 * is not meet.
 * 
 * @param expr The expresion to check.
 *
 * @param val The value to return if the expression is not meet.
 */
#define v_return_val_if_fail(expr, val) \
if (!(expr)) { return val;}

/** 
 * @internal Allows to check a condition and return if it is not
 * meet. It also provides a way to log an error message.
 * 
 * @param expr The expresion to check.
 *
 * @param msg The message to log in the case a failure is found.
 */
#define v_return_if_fail_msg(expr,msg) \
if (!(expr)) {valvula_log (VALVULA_LEVEL_CRITICAL, "%s: %s", __AXL_PRETTY_FUNCTION__, msg); return;}

/** 
 * @internal Allows to check a condition and return the given value if
 * it is not meet. It also provides a way to log an error message.
 * 
 * @param expr The expresion to check.
 *
 * @param val The value to return if the expression is not meet.
 *
 * @param msg The message to log in the case a failure is found.
 */
#define v_return_val_if_fail_msg(expr, val, msg) \
if (!(expr)) { valvula_log (VALVULA_LEVEL_CRITICAL, "%s: %s", __AXL_PRETTY_FUNCTION__, msg); return val;}


BEGIN_C_DECLS

/* Internal includes and external includes for Valvula API
 * consumers. */
#include <valvula_types.h>
#include <valvula_handlers.h>
#include <valvula_support.h>
#include <valvula_io.h>
#include <valvula_hash.h>
#include <valvula_ctx.h>
#include <valvula_thread.h>
#include <valvula_thread_pool.h>
#include <valvula_reader.h>
#include <valvula_listener.h>
#include <valvula_connection.h>

axl_bool valvula_init_ctx             (ValvulaCtx * ctx);

axl_bool valvula_init_check           (ValvulaCtx * ctx);

void     valvula_exit_ctx             (ValvulaCtx * ctx, 
				      axl_bool    free_ctx);

axl_bool valvula_is_exiting           (ValvulaCtx * ctx);

axl_bool valvula_log_is_enabled       (ValvulaCtx * ctx);

axl_bool valvula_log2_is_enabled      (ValvulaCtx * ctx);

void     valvula_log_enable           (ValvulaCtx * ctx, 
				      axl_bool    status);

void     valvula_log2_enable          (ValvulaCtx * ctx, 
				      axl_bool    status);

axl_bool valvula_color_log_is_enabled (ValvulaCtx * ctx);

void     valvula_color_log_enable     (ValvulaCtx * ctx, 
				      axl_bool    status);

#if defined(__COMPILING_VALVULA__) && defined(__GNUC__)
/* makes gcc happy, by prototyping functions which aren't exported
 * while compiling with -ansi. Really uggly hack, please report
 * any idea to solve this issue. */
int  setenv  (const char *name, const char *value, int overwrite);
#endif

/** internal definitions **/
void _valvula_log (ValvulaCtx        * ctx,
		   const char        * file,
		   int                 line,
		   ValvulaDebugLevel   log_level,
		   const char        * message,
		   ...);

void _valvula_log2 (ValvulaCtx        * ctx,
		    const char        * file,
		    int                 line,
		    ValvulaDebugLevel   log_level,
		    const char        * message,
		    ...);


const char * valvula_get_sender_domain (ValvulaRequest * request);

const char * valvula_get_recipient_domain (ValvulaRequest * request);

axl_bool     valvula_is_authenticated (ValvulaRequest * request);

const char * valvula_get_sasl_user (ValvulaRequest * request);

axl_bool     valvula_address_rule_match (ValvulaCtx * ctx, const char * rule, const char * address);

const char * valvula_get_domain (const char * address);

long         valvula_now (void);

long         valvula_get_day (void);

long         valvula_get_month (void);

END_C_DECLS

/* @} */
#endif
