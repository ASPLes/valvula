/* 
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2017 Advanced Software Production Line, S.L.
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

/** 
 * \addtogroup valvula
 * @{
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
 * @brief Allows to configure a log handler that is called every time
 * a log is produced by the libValvula engine.
 *
 * @param ctx The context to configure.
 *
 * @param log_handler The log handler that is being configured.
 *
 * @param user_data User data pointer passed to the log_handler
 */
void     valvula_set_log_handler      (ValvulaCtx * ctx,
				       ValvulaLogHandler log_handler, 
				       axlPointer        user_data)
{
	if (ctx == NULL)
		return;

	ctx->log_handler      = log_handler;
	ctx->log_handler_data = user_data;

	return;
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

	/* if not VALVULA_DEBUG FLAG and no log handler was found, do
	 * not output anything */
	if ((! valvula_log_is_enabled (ctx)) && ctx->log_handler == NULL) 
		return;


	/* get current stamp */
	gettimeofday (&stamp, NULL);

	/* print the message */
	vsnprintf (buffer, 1023, message, args);

	/* if log handler is enabled, route all messages there */
	if (ctx->log_handler) {
		ctx->log_handler (ctx, log_level, file, line, buffer, ctx->log_handler_data);
		return;
	} /* end if */

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

	/* if not VALVULA_DEBUG2 FLAG and log handler is not defined, do not output anything */
	if ((!valvula_log2_is_enabled (ctx)) && ctx->log_handler == NULL) {
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

/** 
 * @brief Support function that allows getting sender domain for the
 * provided request.
 *
 * @param request The request that is being queried for its sender domain.
 *
 * @return A reference to the sender domain or NULL if it fails or it
 * is not defined.
 */
const char * valvula_get_sender_domain (ValvulaRequest * request)
{
	int iterator;

	if (request == NULL || request->sender == NULL)
		return NULL;
	
	/* next iterator */
	iterator = 0;
	while (request->sender[iterator] && request->sender[iterator] != '@')
		iterator++;

	/* report the sender */
	if (request->sender[iterator] == '@')
		return &(request->sender[iterator+1]);
	return request->sender;
}

/** 
 * @brief Allows to check if the provided request is authenticated
 * (SASL user enabled).
 *
 * @param request The request that is being checked.
 *
 * @return axl_true In the case the request represents an
 * authenticated transaction, otherwise axl_false is returned. 
 */
axl_bool     valvula_is_authenticated (ValvulaRequest * request)
{
	const char * sasl_user = valvula_get_sasl_user (request);

	/* check that is defined and has content */
	return (sasl_user && strlen (sasl_user) > 0);
}

/** 
 * @brief Allows to get sasl_user associated to the request.
 *
 * @param request that is being queried.
 *
 * @return A reference to the sasl user or NULL if the request
 * received is not autenticated.
 */ 
const char * valvula_get_sasl_user (ValvulaRequest * request)
{
	if (request == NULL || request->sasl_method == NULL || request->sasl_username == NULL)
		return NULL;

	/* return current sasl username */
	return request->sasl_username;
}

/** 
 * @brief Allows to check if the provided address matches the provided
 * rule.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param rule The rule that is being attempted to match.
 *
 * @param address The address that is being matched.
 *
 * Examples:
 * \code
 * rule=NULL            address=anything        MATCH
 * rule=''              address=anything        MATCH
 * rule=test.com        address=test@test.com   MATCH
 * rule=test2.com       address=test@test.com   NOT MATCH
 * rule=test.com        address=test.com        MATCH
 * rule=test@test.com   address=test@test.com   MATCH       -- match by domain
 * rule=test2@test.com  address=test@test.com   NOT MATCH
 * rule=test@           address=test@test.com   MATCH       -- match by local-part
 * rule=com             address=test@test.com   MATCH       -- match TLD .com with @test.com
 * \endcode
 *    
 * @return The function returns axl_true in the case everything
 * matches, otherwise axl_false is reported. The function also returns
 * axl_false every time address is NULL and/or ctx is NULL.
 */
axl_bool     valvula_address_rule_match (ValvulaCtx * ctx, const char * rule, const char * address)
{
	char * local_part;
	char * local_part_aux2;

	/* check rule and address */
	if (ctx == NULL || address == NULL)
		return axl_false;
	if (rule == NULL || strlen (rule) == 0)
		return axl_true;
	if (axl_casecmp (rule, address))
		return axl_true;

	if (!strstr (rule, "@") && !strstr(rule, ".")) {
		/* rule not has @ and . in it, so we have a TLD case */
		/* check if domain matches: that is rule=aspl.es == get_domain(test@aspl.es) */
		if (axl_casecmp (rule, valvula_get_tld_extension (address)))
			return axl_true;
	} else if (!strstr (rule, "@")) {
		/* rule not has @ in it */
		/* check if domain matches: that is rule=aspl.es == get_domain(test@aspl.es) */
		if (axl_casecmp (rule, valvula_get_domain (address)))
			return axl_true;
	} else {
		/* rule has @ in it, try to check for for rule=web@ == get_local_part (web@aspl.es) */
		if (valvula_get_domain (rule) == NULL || strlen (valvula_get_domain (rule)) == 0) {
			local_part      = valvula_get_local_part (rule);
			local_part_aux2 = valvula_get_local_part (address);
			if (axl_casecmp (local_part, local_part_aux2)) {
				/* matches */
				axl_free (local_part);
				axl_free (local_part_aux2);
				return axl_true;
			} /* end if */

			axl_free (local_part);
			axl_free (local_part_aux2);
		}
	} /* end if */

	/* reporting it doesn't match */
	return axl_false;
}

/** 
 * @brief Allows to get the domain part of the provided address.
 *
 * @param address The address that is being queried to report its domain part.
 *
 * @return A reference to the domain or NULL if it fails. If the
 * function receives a domain, the function reports a domain.
 */
const char * valvula_get_domain (const char * address)
{
	int iterator = 0;
	if (! address)
		return NULL;

	/* find the end of the string or @ */
	while (address[iterator] && address[iterator] != '@')
		iterator++;

	if (address[iterator] == '@')
		return address + iterator + 1;

	return address;
}

/** 
 * @brief Allows to get the domain extension part of the provided
 * address (TLD).
 *
 * Examples:
 * 
 * \code
 * valvula_get_tld_extension (someaccount@domain-example.com) -> com
 * valvula_get_tld_extension (domain-example.org) -> org
 * \endcode
 *
 * @param address The address/domain that is being queried to report its domain extension (TLD)
 *
 * @return A reference to the domain or NULL if it fails. If the
 * function receives a domain, the function reports a domain.
 */
const char * valvula_get_tld_extension  (const char * address)
{
	int iterator = 0;
	if (! address)
		return NULL;

	/* startup iterator */
	iterator = strlen (address) - 1;

	/* find the end of the string or @ */
	while (address[iterator] && address[iterator] != '.')
		iterator--;

	if (address[iterator] == '.')
		return address + iterator + 1;

	return address;
}

/** 
 * @brief Allows to get local part from the provided domain [local-part]\@domain
 *
 * @param address The address that is being queried to report its local part.
 *
 * @return A reference to the domain or NULL if it fails. If the
 * function receives a domain, the function reports NULL. Please note
 * this function is different from valvula_get_domain in the sense it
 * returns a pointer that should be released with axl_free
 */
char * valvula_get_local_part (const char * address)
{
	char * local_part;
	int    iterator = 0;

	if (! address)
		return NULL;

	/* find the end of the string or @ */
	while (address[iterator] && address[iterator] != '@')
		iterator++;

	if (address[iterator] == '@') {
		local_part = axl_new (char, iterator + 1);
		memcpy (local_part, address, iterator);
		return local_part;
	}

	/* return NULL */
	return NULL;
}

/** 
 * @brief Allows to get recipient domain defined at the valvula
 * request.
 *
 * @param request The request operation that is being checked to
 * report recipient domain.
 *
 * @return Recipient domain or NULL if it fails.
 */
const char * valvula_get_recipient_domain (ValvulaRequest * request)
{
	int iterator;

	if (request == NULL || request->recipient == NULL)
		return NULL;
	
	/* next iterator */
	iterator = 0;
	while (request->recipient[iterator] && request->recipient[iterator] != '@')
		iterator++;

	/* report the recipient */
	if (request->recipient[iterator] == '@')
		return &(request->recipient[iterator+1]);
	return request->recipient;
}

/** 
 * @brief Allows to get local part associated to the sender of the
 * provided request.
 *
 * @param request The request to get local-part from sender
 *
 * @return A newly allocated string holding local part or NULL (if
 * sender is not defined or just domain is received). You must call to
 * axl_free (result) when no longer needed.
 */
char       * valvula_get_sender_local_part (ValvulaRequest * request)
{
	if (request == NULL || request->sender == NULL)
		return NULL;
	
	return valvula_get_local_part (request->sender);
}

/** 
 * @brief Allows to get local part associated to the recipient of the
 * provided request.
 *
 * @param request The request to get local-part from recipient
 *
 * @return A newly allocated string holding local part or NULL (if
 * recipient is not defined or just domain is received). You must call to
 * axl_free (result) when no longer needed.
 */
char       * valvula_get_recipient_local_part (ValvulaRequest * request)
{
	if (request == NULL || request->recipient == NULL)
		return NULL;
	
	return valvula_get_local_part (request->recipient);
}

/** 
 * @brief Allows to get current epoch (now).
 *
 * @return Number of seconds that represents epoch now.
 */
long     valvula_now (void)
{
	struct timeval       now;
	gettimeofday (&now, NULL);

	/* report now tv_sec */
	return now.tv_sec;
}

/** 
 * @brief Allows to get current day.
 *
 * @return Current day reported.
 */
long         valvula_get_day (void)
{
	 time_t      t;
	 struct tm * tmp;
	 
	 t = time(NULL);
	 tmp = localtime (&t);
	 return tmp->tm_mday;
}

/** 
 * @brief Allows to get current hour.
 *
 * @return Current hour reported.
 */
long         valvula_get_hour (void)
{
	 time_t      t;
	 struct tm * tmp;
	 
	 t = time(NULL);
	 tmp = localtime (&t);
	 return tmp->tm_hour;
}

/** 
 * @brief Allows to get current minute.
 *
 * @return Current minute reported.
 */
long         valvula_get_minute (void)
{
	 time_t      t;
	 struct tm * tmp;
	 
	 t = time(NULL);
	 tmp = localtime (&t);
	 return tmp->tm_min;
}

/** 
 * @brief Allows to get current month.
 *
 * @return Current month reported.
 */
long         valvula_get_month (void)
{
	time_t      t;
	struct tm * tmp;
	
	t = time(NULL);
	tmp = localtime (&t);
	
	return tmp->tm_mon;
}

/* @} */


