/* 
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2013 Advanced Software Production Line, S.L.
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

/* global includes */
#include <valvula.h>
#include <signal.h>

/* private include */
#include <valvula_private.h>

#define LOG_DOMAIN "valvula"

/* Ugly hack to have access to vsnprintf function (secure form of
 * vsprintf where the output buffer is limited) but unfortunately is
 * not available in ANSI C. This is only required when compile valvula
 * with log support */
#if defined(ENABLE_VALVULA_LOG)
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif

#if !defined(AXL_OS_WIN32)
void __valvula_sigpipe_do_nothing (int _signal)
{
	/* do nothing sigpipe handler to be able to manage EPIPE error
	 * returned by write. read calls do not fails because we use
	 * the valvula reader process that is waiting for changes over
	 * a connection and that changes include remote peer
	 * closing. So, EPIPE (or receive SIGPIPE) can't happen. */
	

	/* the following line is to ensure ancient glibc version that
	 * restores to the default handler once the signal handling is
	 * executed. */
	signal (SIGPIPE, __valvula_sigpipe_do_nothing);
	return;
}
#endif

/** 
 * @brief Allows to get current status for log debug info to console.
 * 
 * @param ctx The context that is required to return its current log
 * activation configuration.
 * 
 * @return axl_true if console debug is enabled. Otherwise axl_false.
 */
axl_bool      valvula_log_is_enabled (ValvulaCtx * ctx)
{
#ifdef ENABLE_VALVULA_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return axl_false;

	/* check if the debug function was already checked */
	if (! ctx->debug_checked) {
		/* not checked, get the value and flag as checked */
		ctx->debug         = valvula_support_getenv_int ("VALVULA_DEBUG") > 0;
		ctx->debug_checked = axl_true;
	} /* end if */

	/* return current status */
	return ctx->debug;

#else
	return axl_false;
#endif
}

/** 
 * @brief Allows to get current status for second level log debug info
 * to console.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @return axl_true if console debug is enabled. Otherwise axl_false.
 */
axl_bool      valvula_log2_is_enabled (ValvulaCtx * ctx)
{
#ifdef ENABLE_VALVULA_LOG	

	/* no context, no log */
	if (ctx == NULL)
		return axl_false;

	/* check if the debug function was already checked */
	if (! ctx->debug2_checked) {
		/* not checked, get the value and flag as checked */
		ctx->debug2         = valvula_support_getenv_int ("VALVULA_DEBUG2") > 0;
		ctx->debug2_checked = axl_true;
	} /* end if */

	/* return current status */
	return ctx->debug2;

#else
	return axl_false;
#endif
}

/** 
 * @brief Enable console valvula log.
 *
 * You can also enable log by setting VALVULA_DEBUG environment
 * variable to 1.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param status axl_true enable log, axl_false disables it.
 */
void     valvula_log_enable       (ValvulaCtx * ctx, axl_bool      status)
{
#ifdef ENABLE_VALVULA_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return;

	ctx->debug         = status;
	ctx->debug_checked = axl_true;
	return;
#else
	/* just return */
	return;
#endif
}

/** 
 * @brief Enable console second level valvula log.
 * 
 * You can also enable log by setting VALVULA_DEBUG2 environment
 * variable to 1.
 *
 * Activating second level debug also requires to call to \ref
 * valvula_log_enable (axl_true). In practical terms \ref
 * valvula_log_enable (axl_false) disables all log reporting even
 * having \ref valvula_log2_enable (axl_true) enabled.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param status axl_true enable log, axl_false disables it.
 */
void     valvula_log2_enable       (ValvulaCtx * ctx, axl_bool      status)
{
#ifdef ENABLE_VALVULA_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return;

	ctx->debug2 = status;
	ctx->debug2_checked = axl_true;
	return;
#else
	/* just return */
	return;
#endif
}


/** 
 * @brief Allows to check if the color log is currently enabled.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @return axl_true if enabled, axl_false if not.
 */
axl_bool      valvula_color_log_is_enabled (ValvulaCtx * ctx)
{
#ifdef ENABLE_VALVULA_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return axl_false;
	if (! ctx->debug_color_checked) {
		ctx->debug_color_checked = axl_true;
		ctx->debug_color         = valvula_support_getenv_int ("VALVULA_DEBUG_COLOR") > 0;
	} /* end if */

	/* return current result */
	return ctx->debug_color;
#else
	/* always return axl_false */
	return axl_false;
#endif
	
}


/** 
 * @brief Enable console color log. 
 *
 * Note that this doesn't enable logging, just selects color messages if
 * logging is already enabled, see valvula_log_enable().
 *
 * This is mainly useful to hunt messages using its color: 
 *  - red:  errors, critical 
 *  - yellow: warnings
 *  - green: info, debug
 *
 * You can also enable color log by setting VALVULA_DEBUG_COLOR
 * environment variable to 1.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param status axl_true enable color log, axl_false disables it.
 */
void     valvula_color_log_enable (ValvulaCtx * ctx, axl_bool      status)
{

#ifdef ENABLE_VALVULA_LOG
	/* no context, no log */
	if (ctx == NULL)
		return;
	ctx->debug_color_checked = status;
	ctx->debug_color = status;
	return;
#else
	return;
#endif
}

/** 
 * @internal Internal common log implementation to support several levels
 * of logs.
 * 
 * @param ctx The context where the operation will be performed.
 * @param file The file that produce the log.
 * @param line The line that fired the log.
 * @param log_level The level of the log
 * @param message The message 
 * @param args Arguments for the message.
 */
void _valvula_log_common (ValvulaCtx        * ctx,
			 const       char * file,
			 int                line,
			 ValvulaDebugLevel   log_level,
			 const char       * message,
			 va_list            args)
{

#ifndef ENABLE_VALVULA_LOG
	/* do no operation if not defined debug */
	return;
#else
	/* log with mutex */
	struct timeval stamp;
	char   buffer[1024];

	/* if not VALVULA_DEBUG FLAG, do not output anything */
	if (! valvula_log_is_enabled (ctx)) {
		return;
	} /* end if */


	/* get current stamp */
	gettimeofday (&stamp, NULL);

	/* print the message */
	vsnprintf (buffer, 1023, message, args);
				
	/* drop a log according to the level */
#if defined (__GNUC__)
	if (valvula_color_log_is_enabled (ctx)) {
		switch (log_level) {
		case VALVULA_LEVEL_DEBUG:
			fprintf (stdout, "\e[1;36m(%d.%d proc %d)\e[0m: (\e[1;32mdebug\e[0m) %s:%d %s\n", 
				 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
			break;
		case VALVULA_LEVEL_WARNING:
			fprintf (stdout, "\e[1;36m(%d.%d proc %d)\e[0m: (\e[1;33mwarning\e[0m) %s:%d %s\n", 
				 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
			break;
		case VALVULA_LEVEL_CRITICAL:
			fprintf (stdout, "\e[1;36m(%d.%d proc %d)\e[0m: (\e[1;31mcritical\e[0m) %s:%d %s\n", 
				 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
			break;
		}
	} else {
#endif /* __GNUC__ */
		switch (log_level) {
		case VALVULA_LEVEL_DEBUG:
			fprintf (stdout, "(%d.%d proc %d): (debug) %s:%d %s\n", 
				 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
			break;
		case VALVULA_LEVEL_WARNING:
			fprintf (stdout, "(%d.%d proc %d): (warning) %s:%d %s\n", 
				 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
			break;
		case VALVULA_LEVEL_CRITICAL:
			fprintf (stdout, "(%d.%d proc %d): (critical) %s:%d %s\n", 
				 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
			break;
		}
#if defined (__GNUC__)
	} /* end if */
#endif
	/* ensure that the log is dropped to the console */
	fflush (stdout);
	
#endif /* end ENABLE_VALVULA_LOG */


	/* return */
	return;
}

/** 
 * @internal Log function used by valvula to notify all messages that are
 * generated by the core. 
 *
 * Do no use this function directly, use <b>valvula_log</b>, which is
 * activated/deactivated according to the compilation flags.
 * 
 * @param ctx The context where the operation will be performed.
 * @param file The file that produce the log.
 * @param line The line that fired the log.
 * @param log_level The message severity
 * @param message The message logged.
 */
void _valvula_log (ValvulaCtx        * ctx,
		  const       char * file,
		  int                line,
		  ValvulaDebugLevel   log_level,
		  const char       * message,
		  ...)
{

#ifndef ENABLE_VALVULA_LOG
	/* do no operation if not defined debug */
	return;
#else
	va_list   args;

	/* call to common implementation */
	va_start (args, message);
	_valvula_log_common (ctx, file, line, log_level, message, args);
	va_end (args);

	return;
#endif
}

/** 
 * @internal Log function used by valvula to notify all second level
 * messages that are generated by the core.
 *
 * Do no use this function directly, use <b>valvula_log2</b>, which is
 * activated/deactivated according to the compilation flags.
 * 
 * @param ctx The context where the log will be dropped.
 * @param file The file that contains that fired the log.
 * @param line The line where the log was produced.
 * @param log_level The message severity
 * @param message The message logged.
 */
void _valvula_log2 (ValvulaCtx        * ctx,
		   const       char * file,
		   int                line,
		   ValvulaDebugLevel   log_level,
		   const char       * message,
		  ...)
{

#ifndef ENABLE_VALVULA_LOG
	/* do no operation if not defined debug */
	return;
#else
	va_list   args;

	/* if not VALVULA_DEBUG2 FLAG, do not output anything */
	if (!valvula_log2_is_enabled (ctx)) {
		return;
	} /* end if */
	
	/* call to common implementation */
	va_start (args, message);
	_valvula_log_common (ctx, file, line, log_level, message, args);
	va_end (args);

	return;
#endif
}

/**
 * \defgroup valvula Valvula: Main Valvula Library Module (initialization and exit stuff)
 */

/**
 * \addtogroup valvula
 * @{
 */


/** 
 * @brief Context based valvula library init. Allows to init the valvula
 * library status on the provided context object (\ref ValvulaCtx).
 *
 * To init valvula library use: 
 * 
 * \code
 * ValvulaCtx * ctx;
 * 
 * // create an empty context 
 * ctx = valvula_ctx_new ();
 *
 * // init the context
 * if (! valvula_init_ctx (ctx)) {
 *      printf ("failed to init the library..\n");
 * } 
 *
 * // do API calls before this function 
 * 
 * // terminate the context 
 * valvula_exit_exit (ctx);
 *
 * // release the context 
 * valvula_ctx_free (ctx);
 * \endcode
 * 
 * @param ctx An already created context where the library
 * initialization will take place.
 * 
 * @return axl_true if the context was initialized, otherwise axl_false is
 * returned.
 *
 * NOTE: This function is not thread safe, that is, calling twice from
 * different threads on the same object will cause improper
 * results. You can use \ref valvula_init_check to ensure if you
 * already initialized the context.
 */
axl_bool    valvula_init_ctx (ValvulaCtx * ctx)
{
	int          thread_num;

	v_return_val_if_fail (ctx, axl_false);

	/**** valvula_io.c: init io module */
	valvula_io_init (ctx);

#if ! defined(AXL_OS_WIN32)
	/* install sigpipe handler */
	signal (SIGPIPE, __valvula_sigpipe_do_nothing);
#endif

#if defined(AXL_OS_WIN32)
	/* init winsock API */
	valvula_log (VALVULA_LEVEL_DEBUG, "init winsocket for windows");
	if (! valvula_win32_init (ctx))
		return axl_false;
#endif

	/* init axl library */
	axl_init ();

	/* init reader subsystem */
	valvula_log (VALVULA_LEVEL_DEBUG, "starting valvula reader..");
	if (! valvula_reader_run (ctx))
		return axl_false;
	
	/* init writer subsystem */
	/* valvula_log (VALVULA_LEVEL_DEBUG, "starting valvula writer..");
	   valvula_writer_run (); */

	/* init thread pool (for query receiving) */
	thread_num = valvula_thread_pool_get_num ();
	valvula_log (VALVULA_LEVEL_DEBUG, "starting valvula thread pool: (%d threads the pool have)..",
		    thread_num);
	valvula_thread_pool_init (ctx, thread_num);

	/* flag this context as initialized */
	ctx->valvula_initialized = axl_true;

	/* register the valvula exit function */
	return axl_true;
}

/** 
 * @brief Allows to check if the provided ValvulaCtx is initialized
 * (\ref valvula_init_ctx).
 * @param ctx The context to be checked for initialization.
 * @return axl_true if the context was initialized, otherwise axl_false is returned.
 */
axl_bool valvula_init_check (ValvulaCtx * ctx)
{
	if (ctx == NULL || ! ctx->valvula_initialized) {
		return axl_false;
	}
	return axl_true;
}


/** 
 * @brief Terminates the valvula library execution on the provided
 * context.
 *
 * Stops all internal valvula process and all allocated resources
 * associated to the context. It also close all channels for all
 * connection that where not closed until call this function.
 *
 * This function is reentrant, allowing several threads to call \ref
 * valvula_exit_ctx function at the same time. Only one thread will
 * actually release resources allocated.
 *
 * @param ctx The context to terminate. The function do not dealloc
 * the context provided. 
 *
 * @param free_ctx Allows to signal the function if the context
 * provided must be deallocated (by calling to \ref valvula_ctx_free).
 *
 * <b>Notes about calling to terminate valvula from inside its handlers:</b>
 *
 * Currently this is allowed and supported only in the following handlers:
 *
 * - \ref ValvulaOnFrameReceived (\ref valvula_channel_set_received_handler)
 * - \ref ValvulaConnectionOnCloseFull (\ref valvula_connection_set_on_close_full)
 *
 * The rest of handlers has being not tested.
 */
void valvula_exit_ctx (ValvulaCtx * ctx, axl_bool  free_ctx)
{

	/* check context is initialized */
	if (! valvula_init_check (ctx))
		return;

	/* check if the library is already started */
	if (ctx == NULL || ctx->valvula_exit)
		return;

	valvula_log (VALVULA_LEVEL_DEBUG, "shutting down valvula library, ValvulaCtx %p", ctx);

	valvula_mutex_lock (&ctx->exit_mutex);
	if (ctx->valvula_exit) {
		valvula_mutex_unlock (&ctx->exit_mutex);
		return;
	}
	/* flag other waiting functions to do nothing */
	valvula_mutex_lock (&ctx->ref_mutex);
	ctx->valvula_exit = axl_true;
	valvula_mutex_unlock (&ctx->ref_mutex);
	
	/* unlock */
	valvula_mutex_unlock  (&ctx->exit_mutex);

	/* flag the thread pool to not accept more jobs */
	valvula_thread_pool_being_closed (ctx);

	/* stop valvula writer */
	/* valvula_writer_stop (); */

	/* stop valvula reader process */
	valvula_reader_stop (ctx);

#if defined(AXL_OS_WIN32)
	WSACleanup ();
	valvula_log (VALVULA_LEVEL_DEBUG, "shutting down WinSock2(tm) API");
#endif

	/* clean up valvula modules */
	valvula_log (VALVULA_LEVEL_DEBUG, "shutting down valvula xml subsystem");

	/* Cleanup function for the XML library. */
	valvula_log (VALVULA_LEVEL_DEBUG, "shutting down xml library");
	axl_end ();

	/* unlock listeners */
	valvula_log (VALVULA_LEVEL_DEBUG, "unlocking valvula listeners");
	valvula_listener_unlock (ctx);

	valvula_log (VALVULA_LEVEL_DEBUG, "valvula library stopped");

	/* stop the valvula thread pool: 
	 * 
	 * In the past, this call was done, however, it is showed that
	 * user applications on top of valvula that wants to handle
	 * signals, emitted to all threads running (including the pool)
	 * causes many non-easy to solve problem related to race
	 * conditions.
	 * 
	 * At the end, to release the thread pool is not a big
	 * deal. */
	valvula_thread_pool_exit (ctx); 

	/* lock/unlock to avoid race condition */
	valvula_mutex_lock  (&ctx->exit_mutex);
	valvula_mutex_unlock  (&ctx->exit_mutex);
	valvula_mutex_destroy (&ctx->exit_mutex);

	/* release the ctx */
	if (free_ctx)
		valvula_ctx_free2 (ctx, "end ctx");

	return;
}

/** 
 * @brief Allows to check if valvula engine started on the provided
 * context is finishing (a call to \ref valvula_exit_ctx was done).
 *
 * @param ctx The context to check if it is exiting.
 *
 * @return axl_true in the case the context is finished, otherwise
 * axl_false is returned. The function also returns axl_false when
 * NULL reference is received.
 */
axl_bool valvula_is_exiting           (ValvulaCtx * ctx)
{
	axl_bool result;
	if (ctx == NULL)
		return axl_false;
	valvula_mutex_lock (&ctx->ref_mutex);
	result = ctx->valvula_exit;
	valvula_mutex_unlock (&ctx->ref_mutex);
	return result;
}

/* @} */


