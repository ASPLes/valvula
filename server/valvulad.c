/* 
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
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
#include <valvulad.h>
#include <signal.h>

#ifndef mkstemp
int mkstemp (char *template);
#endif


/** 
 * @internal Simple macro to check if the console output is activated
 * or not.
 */
#define CONSOLE if (ctx->console_enabled || ignore_debug) fprintf

/** 
 * @internal Simple macro to check if the console output is activated
 * or not.
 */
#define CONSOLEV if (ctx->console_enabled || ignore_debug) vfprintf

/** 
 * @brief Allows to check if the debug is activated (\ref msg type).
 * 
 * @return axl_true if activated, otherwise axl_false is returned.
 */
axl_bool  valvulad_log_enabled (ValvuladCtx * ctx)
{
	/* get valvulad context */
	v_return_val_if_fail (ctx, axl_false);

	return ctx->console_debug;
}

/** 
 * @brief Allows to activate the valvulad console log (by default
 * disabled).
 * 
 * @param ctx The valvulad context to configure.  @param value The
 * value to configure to enable/disable console log.
 */
void valvulad_log_enable       (ValvuladCtx * ctx, 
				  int  value)
{
	v_return_if_fail (ctx);

	/* configure the value */
	ctx->console_debug = value;

	/* update the global console activation */
	ctx->console_enabled     = ctx->console_debug || ctx->console_debug2 || ctx->console_debug3;

	return;
}




/** 
 * @brief Allows to check if the second level debug is activated (\ref
 * msg2 type).
 * 
 * @return axl_true if activated, otherwise axl_false is returned.
 */
axl_bool  valvulad_log2_enabled (ValvuladCtx * ctx)
{
	/* get valvulad context */
	v_return_val_if_fail (ctx, axl_false);
	
	return ctx->console_debug2;
}

/** 
 * @brief Allows to activate the second level console log. This level
 * of debug automatically activates the previous one. Once activated
 * it provides more information to the console.
 * 
 * @param ctx The valvulad context to configure.
 * @param value The value to configure.
 */
void valvulad_log2_enable      (ValvuladCtx * ctx,
				  int  value)
{
	v_return_if_fail (ctx);

	/* set the value */
	ctx->console_debug2 = value;

	/* update the global console activation */
	ctx->console_enabled     = ctx->console_debug || ctx->console_debug2 || ctx->console_debug3;

	/* makes implicit activations */
	if (ctx->console_debug2)
		ctx->console_debug = axl_true;

	return;
}

/** 
 * @brief Allows to check if the third level debug is activated (\ref
 * msg2 with additional information).
 * 
 * @return axl_true if activated, otherwise axl_false is returned.
 */
axl_bool  valvulad_log3_enabled (ValvuladCtx * ctx)
{
	/* get valvulad context */
	v_return_val_if_fail (ctx, axl_false);

	return ctx->console_debug3;
}

/** 
 * @brief Allows to activate the third level console log. This level
 * of debug automatically activates the previous one. Once activated
 * it provides more information to the console.
 * 
 * @param ctx The valvulad context to configure.
 * @param value The value to configure.
 */
void valvulad_log3_enable      (ValvuladCtx * ctx,
				  int  value)
{
	v_return_if_fail (ctx);

	/* set the value */
	ctx->console_debug3 = value;

	/* update the global console activation */
	ctx->console_enabled     = ctx->console_debug || ctx->console_debug2 || ctx->console_debug3;

	/* makes implicit activations */
	if (ctx->console_debug3)
		ctx->console_debug2 = axl_true;

	return;
}

/** 
 * @brief Allows to configure if the console log produced is colorfied
 * according to the status reported (red: (error,criticals), yellow:
 * (warning), green: (info, debug).
 * 
 * @param ctx The valvulad context to configure.
 *
 * @param value The value to configure. This function could take no
 * effect on system where ansi values are not available.
 */
void valvulad_color_log_enable (ValvuladCtx * ctx,
				  int             value)
{
	v_return_if_fail (ctx);

	/* configure the value */
	ctx->console_color_debug = value;

	return;
}

/** 
 * @internal function that actually handles the console msg.
 */
void valvulad_msg (ValvuladCtx * ctx, const char * file, int line, const char * format, ...)
{
	/* get valvulad context */
	va_list            args;
	axl_bool           ignore_debug = axl_false;

	/* do not print if NULL is received */
	if (format == NULL || ctx == NULL)
		return;

	/* check extended console log */
	if (ctx->console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "(proc:%d) [\e[1;32mmsg\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [msg] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "\e[1;32mI: \e[0m");
		} else
#endif
			CONSOLE (stdout, "I: ");
	} /* end if */
	
	va_start (args, format);
	
	/* report to console */
	CONSOLEV (stdout, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	valvulad_log_report (ctx, LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);

	CONSOLE (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

/** 
 * @internal function that actually handles the console wrn.
 */
void valvulad_wrn (ValvuladCtx * ctx, const char * file, int line, const char * format, ...)
{
	/* get valvulad context */
	va_list            args;
	axl_bool           ignore_debug = axl_false;

	/* do not print if NULL is received */
	if (format == NULL || ctx == NULL)
		return;
	
	/* check extended console log */
	if (ctx->console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "(proc:%d) [\e[1;33m!!!\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [!!!] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "\e[1;33m!: \e[0m");
		} else
#endif
			CONSOLE (stdout, "!: ");
	} /* end if */
	
	va_start (args, format);

	CONSOLEV (stdout, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	valvulad_log_report (ctx, LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);
	va_start (args, format);

	valvulad_log_report (ctx, LOG_REPORT_WARNING, format, args, file, line);

	va_end (args);

	CONSOLE (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

/** 
 * @internal function that actually handles the console wrn_sl.
 */
void valvulad_wrn_sl (ValvuladCtx * ctx, const char * file, int line, const char * format, ...)
{
	/* get valvulad context */
	va_list            args;
	axl_bool           ignore_debug = axl_false;

	/* do not print if NULL is received */
	if (format == NULL || ctx == NULL)
		return;
	
	/* check extended console log */
	if (ctx->console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "(proc:%d) [\e[1;33m!!!\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [!!!] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "\e[1;33m!: \e[0m");
		} else
#endif
			CONSOLE (stdout, "!: ");
	} /* end if */
	
	va_start (args, format);

	CONSOLEV (stdout, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	valvulad_log_report (ctx, LOG_REPORT_ERROR | LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);

	fflush (stdout);
	
	return;
}

/** 
 * @internal function that actually handles the console error.
 *
 * @param ignore_debug Allows to configure if the debug configuration
 * must be ignored (bypassed) and drop the log. This can be used to
 * perform logging for important messages.
 */
void valvulad_error (ValvuladCtx * ctx, axl_bool ignore_debug, const char * file, int line, const char * format, ...)
{
	/* get valvulad context */
	va_list            args;

	/* do not print if NULL is received */
	if (format == NULL || ctx == NULL)
		return;
	
	/* check extended console log */
	if (ctx->console_debug3 || ignore_debug) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stderr, "(proc:%d) [\e[1;31merr\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stderr, "(proc:%d) [err] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stderr, "\e[1;31mE: \e[0m");
		} else
#endif
			CONSOLE (stderr, "E: ");
	} /* end if */

	va_start (args, format);

	/* report to the console */
	CONSOLEV (stderr, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	valvulad_log_report (ctx, LOG_REPORT_ERROR, format, args, file, line);

	va_end (args);
	va_start (args, format);

	valvulad_log_report (ctx, LOG_REPORT_GENERAL, format, args, file, line);
	
	va_end (args);

	CONSOLE (stderr, "\n");
	
	fflush (stderr);
	
	return;
}

/** 
 * @brief Allows to report in a consistent manner when an operation is
 * rejected.
 *
 * Message rejection will also be configured on the provided \ref
 * ValvulaRequest so it is also reported to postfix.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param request The request operation that was rejected.
 *
 * @param format A printf-like message to report along with the reject message.
 *
 * @parm ... Additional parameters to complete the request.
 */
void  valvulad_reject (ValvuladCtx * ctx, ValvulaState state, ValvulaRequest * request, const char * format, ...)
{
	va_list        args;
	char         * message;
	const char   * sasl_user = valvula_get_sasl_user (request);
	
	va_start (args, format);
	/* create the message */
	message = axl_stream_strdup_printfv (format, args);
	va_end (args);

	msg ("%s: %s -> %s%s%s%s%s, port %d, queue-id %s, from %s: %s", 
	     /* type of rejection */
	     valvula_support_state_str (state),
	     request->sender, request->recipient, 
	     /* drop SASL information */
	     sasl_user ? " (" : "",
	     sasl_user ? "sasl_user=" : "",
	     sasl_user ? sasl_user : "",
	     sasl_user ? ")" : "",
	     /* include message */
	     request->listener_port, 
	     request->queue_id ? request->queue_id : "<undef>" ,
	     request->client_address,
	     message);

	/* configure reject message into request to reply it in the
	 * case nothing is configured */
	if (! request->message_reply) {
		/* store message and let request to release it */
		request->message_reply = message;
	} else {
		/* release message */
		axl_free (message);
	}

	return;
}

void valvulad_cleanup_mysql (ValvulaCtx * _ctx)
{
	valvulad_db_cleanup_thread (NULL);

	return;
}

axl_bool valvulad_init (ValvuladCtx ** result) {
	ValvuladCtx * ctx;
	
	/* init context */
	ctx = axl_new (ValvuladCtx, 1);
	if (ctx == NULL)
		return axl_false;

	if (! valvulad_init_aux (ctx)) 
		return axl_false;

	/* setup result */
	(*result) = ctx;

	return axl_true;
}

void valvulad_report_final_state (ValvulaCtx        * lib_ctx,
				  ValvulaConnection * connection, 
				  ValvulaRequest    * request, 
				  ValvulaState        state, 
				  const char        * message,
				  axlPointer          _ctx)
{
	ValvuladCtx  * ctx = _ctx;
	const char   * sasl_user = valvula_get_sasl_user (request);

	/* do not report reject status because there where already reported */
	if (VALVULA_STATE_REJECT == state)
		return;

	/*          q=queue, ca=client address, ep=encryption protocol, ec=encryption cipher, ek=encryption keysize, m=message */
	/*                                                           q|ca |ep |ec |ek  |m */
	msg ("%s: %s -> %s%s%s%s%s, port %d, rcpt count=%d, queue-id %s%s%s%s%s%s%s%s%s%s%s",
	     valvula_support_state_str (state),
	     request->sender, request->recipient, 
	     /* drop SASL information */
	     sasl_user ? " (" : "",
	     sasl_user ? "sasl_user=" : "",
	     sasl_user ? sasl_user : "",
	     sasl_user ? ")" : "",
	     /* include message */
	     request->listener_port, 
	     request->recipient_count,
	     /* queue report */
	     request->queue_id ? request->queue_id : "<undef>" ,
	     /* client address */
	     request->client_address ? ", from " : "",
	     request->client_address ? request->client_address : "",
	     /* tls configuration */
	     (request->encryption_protocol && strlen (request->encryption_protocol) > 0) ? ", tls=" : ", no-tls",
	     (request->encryption_protocol && strlen (request->encryption_protocol) > 0)  ? request->encryption_protocol : "",
	     /* tls cipher */
	     (request->encryption_cipher && strlen (request->encryption_cipher) > 0) ? ", cipher=" : "",
	     (request->encryption_cipher && strlen (request->encryption_cipher) > 0)  ? request->encryption_cipher : "",
	     /* tls keysize */
	     (request->encryption_keysize && strlen (request->encryption_keysize) > 1) ? ", keysize=" : "",
	     (request->encryption_keysize && strlen (request->encryption_keysize) > 1)  ? request->encryption_keysize : "",
	     
	     message ? ": " : "",
	     message ? message : "");

	return;
}

void valvulad_log_engine (ValvulaCtx * _ctx, ValvulaDebugLevel level, const char * file, int line, const char * message, axlPointer ptr)
{
	ValvuladCtx * ctx = ptr;

	switch (level) {
	case VALVULA_LEVEL_CRITICAL:
		error ("%s:%d %s", file, line, message);
		break;
	case VALVULA_LEVEL_WARNING:
		wrn ("%s:%d %s", file, line, message);
		break;
	default:
		/* not reported, just failures */
		break;
	}
	return;
}

/** 
 * @brief Auxiliar initialization function that allows to provide the
 * \ref ValvuladCtx context.
 *
 * @param ctx The context where the initialization will take place.
 *
 * @return axl_true in the case initialization was ok, otherwise
 * axl_false is returned.
 */
axl_bool valvulad_init_aux (ValvuladCtx * ctx) {

	struct timeval start;

	/* create library context */
	ctx->ctx = valvula_ctx_new ();
	if (ctx->ctx == NULL)
		return axl_false;

	/* configure log handler */
	valvula_set_log_handler (ctx->ctx, valvulad_log_engine, ctx);

	if (! valvula_init_ctx (ctx->ctx)) 
		return axl_false;

	/* init list */
	ctx->listeners = axl_list_new (axl_list_always_return_1, NULL);

	/* setup clean function for threads */
	valvula_thread_pool_set_cleanup_func (ctx->ctx, valvulad_cleanup_mysql);

	/* default initialization function */
	if (! ctx->postfix_file)
		ctx->postfix_file = "/etc/postfix/main.cf";

	msg ("Valvulad context initialized");

	/* init modules */
	valvulad_module_init (ctx);

	/* flag context initialization */
	gettimeofday (&start, NULL);
	ctx->started_at = start.tv_sec;

	/* configure final state handler */
	valvula_ctx_set_final_state_handler (ctx->ctx, valvulad_report_final_state, ctx);

	return axl_true;
}

void valvulad_exit (ValvuladCtx * ctx)
{
	/* stop modules */
	msg ("Calling to stop modules..");
	valvulad_module_notify_close (ctx);

	msg ("Releasing modules (loaded: %d)..", axl_list_length (ctx->registered_modules));
	valvulad_module_cleanup (ctx);

	valvulad_db_cleanup (ctx);

	/* release on day change handlers */
	axl_list_free (ctx->on_day_change_handlers);
	ctx->on_day_change_handlers = NULL;

	/* release on month change handlers */
	axl_list_free (ctx->on_month_change_handlers);
	ctx->on_month_change_handlers = NULL;

	/* release valvulad local domains configuration */
	axl_free (ctx->ld_user);
	axl_free (ctx->ld_pass);
	axl_free (ctx->ld_host);
	axl_free (ctx->ld_dbname);
	axl_free (ctx->ld_query);
	axl_hash_free (ctx->ld_hash);

	axl_free (ctx->ls_user);
	axl_free (ctx->ls_pass);
	axl_free (ctx->ls_host);
	axl_free (ctx->ls_dbname);
	axl_free (ctx->ls_query);
	axl_hash_free (ctx->ls_hash);

	axl_free (ctx->la_user);
	axl_free (ctx->la_pass);
	axl_free (ctx->la_host);
	axl_free (ctx->la_dbname);
	axl_free (ctx->la_query);
	axl_hash_free (ctx->la_hash);
	

	/* release listeners */
	axl_list_free (ctx->listeners);

	/* release all context resources */
	axl_doc_free (ctx->config);
	axl_free (ctx->config_path);
	axl_free (ctx);

	return;
}



#define write_and_check(str, len) do {					\
	if (write (temp_file, str, len) != len) {			\
		error ("Unable to write expected string: %s", str);     \
		close (temp_file);					\
		axl_free (temp_name);					\
		axl_free (str_pid);					\
		return NULL;						\
	}								\
} while (0)

/** 
 * @brief Allows to get process backtrace (including all threads) of
 * the given process id.
 *
 * @param ctx The context where the operation is implemented.
 *
 * @param pid The process id for which the backtrace is requested. Use
 * getpid () to get current process id.
 *
 * @return A newly allocated string containing the path to the file
 * where the backtrace was generated or NULL if it fails.
 */
char          * valvulad_support_get_backtrace (ValvuladCtx * ctx, int pid)
{
#if defined(AXL_OS_UNIX)
	FILE               * file_handle;
	int                  temp_file;
	char               * temp_name;
	char               * str_pid;
	char               * command;
	int                  status;
	char               * backtrace_file = NULL;

	temp_name = axl_strdup ("/tmp/valvulad-backtrace.XXXXXX");
	temp_file = mkstemp (temp_name);
	if (temp_file == -1) {
		error ("Bad signal found but unable to create gdb commands file to feed gdb");
		return NULL;
	} /* end if */

	str_pid = axl_strdup_printf ("%d", getpid ());
	if (str_pid == NULL) {
		error ("Bad signal found but unable to get str pid version, memory failure");
		close (temp_file);
		return NULL;
	}
	
	/* write personalized gdb commands */
	write_and_check ("attach ", 7);
	write_and_check (str_pid, strlen (str_pid));

	axl_free (str_pid);
	str_pid = NULL;

	write_and_check ("\n", 1);
	write_and_check ("set pagination 0\n", 17);
	write_and_check ("thread apply all bt\n", 20);
	write_and_check ("quit\n", 5);
	
	/* close temp file */
	close (temp_file);
	
	/* build the command to get gdb output */
	while (1) {
		backtrace_file = axl_strdup_printf ("/tmp/valvulad-backtrace.%d.%d.%d.gdb", time (NULL), pid, getuid ());
		file_handle    = fopen (backtrace_file, "w");
		if (file_handle == NULL) {
			msg ("Changing path because path %s is not allowed to the current uid=%d", backtrace_file, getuid ());
			axl_free (backtrace_file);
			backtrace_file = axl_strdup_printf ("/tmp/valvulad-backtrace.%d.%d.%d.gdb", time (NULL), pid, getuid ());
		} else {
			fclose (file_handle);
			msg ("Checked that %s is writable/readable for the current usid=%d", backtrace_file, getuid ());
			break;
		} /* end if */

		/* check path again */
		file_handle    = fopen (backtrace_file, "w");
		if (file_handle == NULL) {
			error ("Failed to produce backtrace, alternative path %s is not allowed to the current uid=%d", backtrace_file, getuid ());
			axl_free (backtrace_file);
			return NULL;
		}
		fclose (file_handle);
		break; /* reached this point alternative path has worked */
	} /* end while */

	if (backtrace_file == NULL) {
		error ("Failed to produce backtrace, internal reference is NULL");
		return NULL;
	}

	/* place some system information */
	command  = axl_strdup_printf ("echo \"Valvulad backtrace at `hostname -f`, created at `date`\" > %s", backtrace_file);
	status   = system (command);
	msg ("Running: %s, exit status: %d", command, status);
	axl_free (command);

	/* get profile path id */
	command  = axl_strdup_printf ("echo \"Failure found at main process.\" >> %s", backtrace_file);
	status   = system (command);
	msg ("Running: %s, exit status: %d", command, status);
	axl_free (command);

	/* get place some pid information */
	command  = axl_strdup_printf ("echo -e 'Process that failed was %d. Here is the backtrace:\n--------------' >> %s", getpid (), backtrace_file);
	status   = system (command);
	msg ("Running: %s, exit status: %d", command, status);
	axl_free (command);
	
	/* get backtrace */
	command  = axl_strdup_printf ("gdb -x %s >> %s", temp_name, backtrace_file);
	status   = system (command);
	msg ("Running: %s, exit status: %d", command, status);

	/* remove gdb commands */
	unlink (temp_name);
	axl_free (temp_name);
	axl_free (command);

	/* return backtrace file created */
	return backtrace_file;

#elif defined(AXL_OS_WIN32)
	error ("Backtrace for Windows not implemented..");
	return NULL;
#endif			

}

void __valvulad_common_add_on_date_change (ValvuladCtx * ctx, ValvuladOnDateChange on_change, axlPointer ptr, ValvuladDateItem item_type)
{
	ValvuladHandlerPtr * data;

	if (ctx == NULL || on_change == NULL)
		return;

	/* create the list to store handlers */
	switch (item_type) {
	case VALVULAD_DATE_ITEM_DAY:
		if (! ctx->on_day_change_handlers) {
			ctx->on_day_change_handlers = axl_list_new (axl_list_equal_ptr, axl_free);

			/* check memory allocation */
			if (! ctx->on_day_change_handlers) {
				error ("Memory allocation for on day change handlers failed..");
				return;
			} /* end if */
		} /* end if */
		break;
	case VALVULAD_DATE_ITEM_MONTH:
		if (! ctx->on_month_change_handlers) {
			ctx->on_month_change_handlers = axl_list_new (axl_list_equal_ptr, axl_free);

			/* check memory allocation */
			if (! ctx->on_month_change_handlers) {
				error ("Memory allocation for on month change handlers failed..");
				return;
			} /* end if */
		} /* end if */

		break;
	}

	/* get holder */
	data = axl_new (ValvuladHandlerPtr, 1);
	if (! data) {
		error ("Memory allocation for on day/month change handlers failed (2)..");
		return;
	}

	/* store into list */
	data->handler = on_change;
	data->ptr     = ptr;

	switch (item_type) {
	case VALVULAD_DATE_ITEM_DAY:
		axl_list_append (ctx->on_day_change_handlers, data);
		break;
	case VALVULAD_DATE_ITEM_MONTH:
		axl_list_append (ctx->on_month_change_handlers, data);
		break;
	} /* end switch */

	msg ("On date change added (month handlers: %d, day handlers: %d)", 
	     axl_list_length (ctx->on_day_change_handlers), axl_list_length (ctx->on_month_change_handlers));

	return;
}

/** 
 * @brief Allows to add a handler to will be called every time a day
 * change is detected.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param on_day_change The change handler that will be called when
 * the event is detected.
 *
 * @param ptr Pointer to user defined data.
 */
void valvulad_add_on_day_change (ValvuladCtx * ctx, ValvuladOnDateChange on_day_change, axlPointer ptr)
{
	/* add handler as day change notifier */
	__valvulad_common_add_on_date_change (ctx, on_day_change, ptr, VALVULAD_DATE_ITEM_DAY);
	return;
}

/** 
 * @brief Allows to add a handler to will be called every time a month
 * change is detected.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param on_month_change The change handler that will be called when
 * the event is detected.
 *
 * @param ptr Pointer to user defined data.
 */
void valvulad_add_on_month_change (ValvuladCtx * ctx, ValvuladOnDateChange on_month_change, axlPointer ptr)
{
	/* add handler as month change notifier */
	__valvulad_common_add_on_date_change (ctx, on_month_change, ptr, VALVULAD_DATE_ITEM_MONTH);
	return;
}

/** 
 * @brief Allows to notify day change on currently registered
 * handlers.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param new_day The new day to notify.
 *
 * @param item_type that is being notified.
 */
void valvulad_notify_date_change (ValvuladCtx * ctx, long new_value, ValvuladDateItem item_type)
{
	ValvuladHandlerPtr   * ptr;
	int                    iterator;
	ValvuladOnDateChange   on_day_change;
	axlList              * list = NULL;

	if (ctx == NULL)
		return;

	switch (item_type) {
	case VALVULAD_DATE_ITEM_DAY:
		list = ctx->on_day_change_handlers;
		break;
	case VALVULAD_DATE_ITEM_MONTH:
		list = ctx->on_month_change_handlers;
		break;
	}

	/* check to avoid working with a null reference */
	if (! list) 
		return;

	/* iterate over all handlers */
	iterator = 0;
	while (iterator < axl_list_length (list)) {
		/* get valvulad handler */
		ptr = axl_list_get_nth (list, iterator);
		if (ptr && ptr->handler) {
			/* get the handler */
			on_day_change = ptr->handler;
			on_day_change (ctx, new_value, ptr->ptr);
		} /* end if */

		/* next iterator */
		iterator++;
	}

	return;
}

#if defined(AXL_OS_UNIX)

#define SYSTEM_ID_CONSUME_UNTIL_ZERO(line, fstab, delimiter)                       \
	if (fread (line + iterator, 1, 1, fstab) != 1 || line[iterator] == 0) {    \
	      fclose (fstab);                                                      \
	      return axl_false;                                                    \
	}                                                                          \
        if (line[iterator] == delimiter) {                                         \
	      line[iterator] = 0;                                                  \
	      break;                                                               \
	}                                                                          

axl_bool __valvulad_get_system_id_info (ValvuladCtx * ctx, const char * value, int * system_id, const char * path)
{
	FILE * fstab;
	char   line[512];
	int    iterator;

	/* set invalid value */
	if (system_id)
		(*system_id) = -1;

	fstab = fopen (path, "r");
	if (fstab == NULL) {
		error ("Failed to open file %s", path);
		return axl_false;
	}
	
	/* now read the file */
keep_on_reading:
	iterator = 0;
	do {
		SYSTEM_ID_CONSUME_UNTIL_ZERO (line, fstab, ':');

		/* next position */
		iterator++;
	} while (axl_true);
	
	/* check user found */
	if (! axl_cmp (line, value)) {
		/* consume all content until \n is found */
		iterator = 0;
		do {
			if (fread (line + iterator, 1, 1, fstab) != 1 || line[iterator] == 0) {
				fclose (fstab);
				return axl_false;
			}
			if (line[iterator] == '\n') {
				goto keep_on_reading;
				break;
			} /* end if */
		} while (axl_true);
	} /* end if */

	/* found user */
	iterator = 0;
	/* get :x: */
	if ((fread (line, 1, 2, fstab) != 2) || !axl_memcmp (line, "x:", 2)) {
		fclose (fstab);
		return axl_false;
	}
	
	/* now get the id */
	iterator = 0;
	do {
		SYSTEM_ID_CONSUME_UNTIL_ZERO (line, fstab, ':');

		/* next position */
		iterator++;
	} while (axl_true);

	(*system_id) = atoi (line);

	fclose (fstab);
	return axl_true;
}
#endif

/** 
 * @brief Allows to get system user id or system group id from the
 * provided string. 
 *
 * If the string already contains the user id or group id, the
 * function returns its corresponding integet value. The function also
 * checks if the value (that should represent a user or group in some
 * way) is present on the current system. get_user parameter controls
 * if the operation should perform a user lookup or a group lookup.
 * 
 * @param ctx The valvulad context.
 * @param value The user or group to get system id.
 * @param get_user axl_true to signal the value to lookup user, otherwise axl_false to lookup for groups.
 *
 * @return The function returns the user or group id or -1 if it fails.
 */
int valvulad_get_system_id  (ValvuladCtx * ctx, const char * value, axl_bool get_user)
{
#if defined (AXL_OS_UNIX)
	int system_id = -1;

	/* get user and group id associated to the value provided */
	if (! __valvulad_get_system_id_info (ctx, value, &system_id, get_user ? "/etc/passwd" : "/etc/group"))
		return -1;

	/* return the user id or group id */
	msg ("Resolved %s:%s to system id %d", get_user ? "user" : "group", value, system_id);
	return system_id;
#endif
	/* nothing defined */
	return -1;
}

/** 
 * \mainpage 
 *
 * \section intro Valvulad: a high performance policy daemon for Postfix
 *
 * <b>Valvula server</b> is an Open Source, high performance Postfix policy daemon written in ANSI C that provides very useful features that makes it suitable for commercial enviroments (massive mail servers, hosting providers and corporate mail servers). Some of its features are:
 *
 * - Component separation that provides more flexibility to implement several contexts or to embeed the product into another project. 
 * - Highly threaded design with port separation which allows providing Valvula services with different modules at different ports running everything within a single daemon
 * - Automatic postfix database detection (so your current Postfix configuration will be parsed by Valvula so it can also know what domains and accounts are local to make better decisions).
 * - Robust and well tested implementation checked by a strong regression test to ensure that the library and core server keep on working as new features are added across releases.
 * - See \ref valvulad_features "complete Valvula server feature lists".
 *
 * Valvula has been developed by <b>Advanced Software Production Line,
 * S.L. (ASPL) </b> (http://www.aspl.es). It is licensed under the GPL 2.0.
 *
 * \section valvulad_manuals Valvulad: manuals and documentation
 *
 * This manual is separated into different sections targeting different users:
 *
 * - For system administrators and developers: \ref valvulad_server_install
 * - For system administrators: \ref valvulad_administration_manual
 * - For developers: \ref valvulad_plugin_development_manual
 *
 * Documentation from modules:
 *
 * - \ref valvulad_mod_slm
 * - \ref valvulad_mod_mquota
 * - \ref valvulad_mod_bwl
 * - \ref valvulad_mod_ticket
 * - \ref valvulad_mod_mw
 *
 * \section contact_aspl Contact Us
 * 
 * You can reach us at the <b>Valvula mailing list:</b> at <a href="http://lists.aspl.es/cgi-bin/mailman/listinfo/valvula">Valvula users</a>
 * for any question you may find. 
 *
 * If you are interested on getting commercial support, you can also
 * contact us at: info@aspl.es.
 */

/** 
 * @page valvulad_server_install Valvula server installation
 *
 * \section valvulad_server_install_from_packges 1. Installing Valvula server from packages
 *
 * Please, before continue, check the following page to see if there are valvula package already available for your os:
 *
 * http://www.aspl.es/valvula/downloads.html
 *
 * For example, for debian / ubuntu system you can install it by running:
 *
 * \code
 * >> apt-get install valvulad-server
 * \endcode
 *
 * After that, you must install those modules you want. For example:
 *
 * \code
 * >> apt-get install valvulad-mod-ticket
 * \endcode
 *
 * Now, follow valvula administration manual to configure the server: \ref valvulad_administration_manual
 * 
 * \section valvula_server_install_from_sources 2. Installing Valvula server from latest stable source code release
 *
 * To fully install Valvula server you must have the following packages installed in your system:
 *
 * - MySQL client library (for example, in debian <b>libmysqlclient16</b>). Check your OS manual to install it.
 * - Axl Library for XML processing. Check http://www.aspl.es/xml to know if there are available packages for your OS.
 *
 * After that, get the latest Valvula release from http://www.aspl.es/valvula/downloads and then run:
 *
 * \code
 * >> tar xzvf valvula-1.X.X.tar.gz
 * >> cd valvula-1.X.X
 * >> ./configure
 * \endcode
 *
 * If everything went ok, compile valvula with the following to compile the project:
 *
 * \code
 * >> make
 * \endcode
 *
 * If everything went ok, install binaries and additional files by running:
 *
 * \code
 * >> make install
 * \endcode
 *
 * Then, to have working console administration tool (<b>valvulad-mgr.py</b>) you need to have python-axl installed. 
 * To have it installed, take a look at http://www.aspl.es/xml/html/axl_install.html
 * For debian systems, you can also install it by running:
 *
 * \code
 * >> apt-get install python-axl
 * \endcode
 *
 * Now see next section to know how to setup valvula to get it up and running.
 *
 * - \ref valvulad_administration_manual
 *
 */

/** 
 * \page valvulad_features Valvula server features
 *
 * \section valvulad_features_component_separation Server component separation
 *
 * 
 * <img src='component-separation.png' style='float: left; padding: 6px'> Valvula server is composed by several components which are a base
 * library (<b>libValvula</b>), a run time server (<b>Valvulad</b>)
 * and extension modules that provides the final end user features required by system administrators.
 *
 * <b>libValvula</b> allows to encapsulate all low level functions that
 * interacts with the postfix server to parse and reply requests. This
 * library allows to embed all functions into a different application
 * and/or creating different context where different handlers can be
 * configured to process incoming requests.
 *
 * <b>Valvulad</b> server is built on top of <b>libValvula</b>
 * providing all system administrator functions like port
 * configuration, modules to load, system user to run the server. This
 * server allows a system administrator to enable/disable Valvula
 * features, control its limits and receive logging about what's done
 * by the server.
 *
 * The administrator can enable different modules/plugins ready
 * available to provide support to different features like Sender
 * Login Mismatch handling, mail sending quotas, global/domain/account
 * whitelists and blacklists, ...
 *
 * \section valvulad_features_threaded_design Threaded design
 *
 * <img src='threaded-design.png' style='float: left; padding: 6px'> Valvula is highly threaded server that allows to handle thousand of
 * requests per minute with a single process. It also includes an
 * internal queue design to avoid overflooding the server when there
 * are a high number of requests.
 *
 * Even though there is a threading design in place, writing modules
 * is really easy because ValvulaD server takes cares of many details
 * so plugin writer only has to pay attention on the core features
 * its module has.
 *
 * \section valvulad_features_port_separation Port separation with different policies
 *
 * <img src='port-separation.png' style='float: left; padding: 6px'> Valvula has the hability to provide different policies (groups of
 * modules that are applied in a particular order) on different
 * ports. This way you can have the same Valvula server process
 * applying different policies at the different postfix sections you
 * require.
 *
 * Because you can have different combination of modules at different
 * ports, you are able to connect Valvula through postfix's
 * <b>check_policy_service</b> in a really flexible manner.
 *
 * \section valvulad_features_automatic_postfix Automatic postfix configuration detection
 *
 * <img src='automatic-conf.png' style='float: left; padding: 6px'> Valvula comes with a built-in postfix configuration parser that
 * allows automatic postfix database detection.  This way Valvula will
 * detect which domains and accounts are considered local by your
 * Postfix server, allowing valvula to make better decisions.
 *
 * This includes skipping some modules when the requests talks about a
 * local delivery or to skip a wrong white list rule that otherwise
 * will imply that your mail server becomes an open relay.
 *
 * This feature is provided through an API (\ref
 * valvulad_run_is_local_domain, \ref valvulad_run_is_local_address,
 * \ref valvulad_run_is_local_delivery) that modules can leverage to
 * create powerful plugins.
 *
 * \section valvulad_server_features_well_testes Robust and well tested implementation
 *
 * <img src='robust-implementation.png' style='float: left; padding: 6px'> Valvula suite is an ANSI C implementation checked with valgrind
 * (http://www.valgrind.org) and supervised by a strong regression
 * test to ensure the library, core server and modules keep on working
 * without any failure as the project moves forward.
 *
 * Currently it is being used intensively at many hosting providers
 * and in-house mail server solutions with high SMTP traffic.
 *
 * \section valvulad_server_features_real_time_stats Real time internal stats support
 *
 * <img src='stats-support.png' style='float: left; padding: 6px'>Valvula comes with support to provide system administrators with
 * real time process status and stats. This way it is possible to get
 * up to date information about requests being handled, pending tasks
 * or how long is taking Valvula server to process each requests (and
 * how much time each module takes to process requests).
 * 
 * To get stats, just run the following in a server where valvula server is running:
 * \code
 * >> valvulad -s
 * \endcode
 *
 * \section valvulad_server_features_administrator_friendly Valvula is administrator friendly
 *
 * <img src='administrator-friendly.png' style='float: left; padding: 6px'>We are system administrators too and we like it easy. Valvula has
 * various automatic processes that frees you from the database
 * burden, module activation and many other details.
 *
 * For example, you only have to create a MySQL database along with a
 * MySQL user, and after configuring it into valvula's conf, then
 * valvula will create for you all database tables, attributes, etc. It even will handle
 * database updates across releases automatically for you.
 *
 * \section valvulad_server_features_easy_module_design Easy module design
 *
 * <img src='adding-modules.png' style='float: left; padding: 6px'> Adding new valvula modules is really easy. You only have to
 * implement a set of handlers that let you process requestes received. 
 *
 * Along with this handlers you have a featureful API that includes
 * many functions to help you write concise modules quickly and
 * easily.
 *
 * Take a look at the valvula mod-test implementation to get a first glance: https://dolphin.aspl.es/svn/publico/valvula/plugins/mod-test/mod-test.c
 *
 * \section valvulad_server_features_open_source And don't forget, Valvula is open source!
 *
 * <img src='valvulad-open-source.png' style='float: left; padding: 6px'> Valvula is provided to the world with an open source license (GPL)
 * that allows integrating it into any enviroment without any cost. 
 *
 * 
 */

/** 
 * \page valvula_api Valvula API
 *
 * The following is a reference manual for the API provides by
 * <b>libValvula</b> and <b>Valvula server</b>. These functions are
 * only required in the case you want to create new Valvula server
 * plugins/modules. 
 *
 * From a module writer's point of view both apis are used the same
 * way as if they come from the same source. The only point to know is
 * that part of the API is provided by the base library and part of
 * the API is provided by the server it self.
 *
 * \section valvula_api_libvalvula libValvula API
 *
 * - \ref valvula
 * - \ref valvula_handlers
 * - \ref valvula_types
 * - \ref valvula_ctx
 * - \ref valvula_connection
 * - \ref valvula_listener
 *
 * \section valvula_api_valvulad ValvulaD API
 *
 * - \ref valvulad_moddef
 * - \ref valvulad_run
 *
 * 
 *
 */


/** 
 * \page valvulad_plugin_development_manual Valvulad plugin development manual
 *
 * \section valvulad_plugin_intro Introduction to valvula plugin developement
 *
 * Writing a Valvulad server plugin consists of creating a C module
 * that is loaded at run time by the server. To write that module
 * you'll have to use \ref valvula_api. 
 *
 * The recommended way to start is to take mod-test.c as a base
 * example to extend upon. To that end, grab a copy from https://dolphin.aspl.es/svn/publico/valvula/plugins/mod-test/mod-test.c
 *
 * Also, grab a copy of the Makefile.am (https://dolphin.aspl.es/svn/publico/valvula/plugins/mod-test/Makefile.am) to bootstrap 
 * your module compilation. On that Makefile.am you'll see the essentials about getting that module compiled. 
 *
 * Finally, you'll need a module pointer, which is a small xml file,
 * which is installed at /etc/valvula/mods-available, that allows
 * valvula server to locate your compiled plugin. Take a look at
 * https://dolphin.aspl.es/svn/publico/valvula/plugins/mod-test/mod-test.xml
 * to have a copy too.
 *
 * So, to summarize, you'll need the following files:
 *
 * - The module itself (mod-test.c) : https://dolphin.aspl.es/svn/publico/valvula/plugins/mod-test/mod-test.c
 * - A makefile to get it compiled : https://dolphin.aspl.es/svn/publico/valvula/plugins/mod-test/Makefile.am
 * - A module pointer : https://dolphin.aspl.es/svn/publico/valvula/plugins/mod-test/mod-test.xml
 *
 * Of course, you don't want your module to be called mod-test right? Change that into something meaningful to the module's function.
 *
 * \section valvulad_plugin_activation Plugin run-time activation
 *
 * Before getting into the details about each handler inside a module
 * let's consider how the code will flow. Assuming you have everything
 * compiled and activated, your module will be called like this:
 * 
 * - 1. First, the \ref ModInitFunc (in our case test_init) function will be called first at valvula startup. There, you'll have to place all initialization code you need to make the module work. This function is only called once.
 *
 * - 2. Then, for every request postfix passes to Valvula, and in the case your module is connected to the listener port (see \ref valvulad_server_configuration), then your module will be called at \ref ValvulaProcessRequest (in our case \ref test_process_request). That function will have to make a decision about that request by returning any of the available values found at: \ref ValvulaState. Optionally you can also return a message to postfix so he can report that message to the user (and record it into the log). We will get into more details about this function later.
 *
 * - 3. At the end, in the case valvula is closing/stoping, \ref ModCloseFunc (in our case test_close) will be called. There you'll have to release resources and save state for later recovery (if any).
 *
 * That's the very basic function. Then, your module can receive a call to \ref ModReconfFunc requesting to reload configuration (a kind of run-time init function). This is received when the user/administrator sends a SIGHUP signal to reload the process. This handler is optional but recommended.
 *
 * Finally, your module may receive a call to \ref ModUnloadFunc indicating the module is going to be unloaded (a kind of run-time close function). There, you'll have to implement all resource release code.
 *
 *
 * \section valvulad_plugin_installation Plugin installation notes
 *
 * Now, before getting into details about how modules are created, a few notes about installing new modules. This is as easy as:
 *
 * - 1. Copy the compiled module into (mod-your-mod.so), for example, /usr/lib/valvulad/modules.
 *
 * - 2. Then, copy your updated module pointer (mod-your-mod.xml) into /etc/valvula/mod-available. Make your the content of that file actually points to the full path of your .so
 * - 3. After that, you shoul be able to run the following command to check if the module is detected (at least), by the configuration:
 *
 * \code
 * >> valvulad-mgr.py -o
 * \endcode
 *
 * - 4. After that, to test the module, you must connect it to a valvula listener. See (see \ref valvulad_server_configuration).
 *
 * - 5. Finally, don't forget restart valvula to have it load your new brand module:
 *
 * \code
 * >> service valvulad restart
 * \endcode
 *
 * \section valvulad_plugin_testing Plugin testing notes (checking Valvula without having postfix running)
 *
 * To test your new module or any other modules, you can do the obvious
 * operation which is sending a test to postfix to see what happens.
 *
 * However, it is also possible to test valvula server directly without having postfix running. Use the following command to configure a 
 * request as if it were sent by the postfix server:
 *
 * \code
 * >> valvulad-mgr.py -t
 * \endcode
 *
 * That will ask for the required details and will connect to the port
 * indicated by the user, producing a network interation exactly the
 * same as if it were done by postfix.
 *
 * This command can also be used to check valvula is working (for a checker/monitor).
 *
 * \section valvulad_plugin_development_notes Plugin developement notes
 *
 * The following are general notes that you can use to solve common questions while using valvula API ( \ref valvula_api) :
 *
 * <b>How do I check if a user was autenticated?</b>
 *
 * Use \ref valvula_is_authenticated
 *
 * <b>How do I check if the sending user is a local user?</b>
 *
 * Use \ref valvulad_run_is_local_address
 *
 * <b>And a local domain?</b>
 *
 * Use \ref valvulad_run_is_local_domain
 * 
 *  
 *
 * 
 * 
 * 
 */

/** 
 * \page valvulad_administration_manual Valvulad server administration manual
 *
 * \section valvulad_server_install_index Index
 *
 * - \ref valvulad_server_configuration
 * - \ref valvulad_server_configuring_modules
 * - \ref valvulad_server_dont_panic
 *
 * \section valvulad_server_configuration 1. Valvulad server configuration
 *
 * Assuming you already have Valvulad server binaries installed in your system you must create a valvula.conf file at /etc/valvula
 *
 * 1) In general you can use as example the template bundled. For that run:
 *
 * \code
 * >> cp /etc/valvula/valvula.example.conf /etc/valvula/valvula.conf
 * \endcode
 *
 * 2) After that, create a mysql database and a user associated to
 * it. Check your OS documentation on how to do this. <i>Please, do not
 * use system administrator MySQL account directly with valvula. </i>
 *
 * 3) Now with you MySQL credentails, set them inside valvula.conf inside database section:
 *
 * \code
 *  <database>
 *    <!-- default mysql configuration -->
 *    <config driver="mysql" dbname="valvula" user="valvula" password="valvula" host="localhost" port="" />
 *  </database>
 * \endcode
 *
 * 4) After this, you can check if valvula is able to use your MySQL account by running the following. It should output that everything is working.
 *
 * \code
 * >> valvulad -b 
 * INFO: Database connection working OK
 * \endcode
 *
 * Now you have base Valvulad installed. Now you have to enable modules and connect them to postfix configuration. See next.
 *
 * \section valvulad_server_configuring_modules 2. Enabling Valvulad server modules
 *
 * \note you need python-axl installed in your system in order to have
 * valvulad-mgr.py working.
 * \note To have it installed, take a look at http://www.aspl.es/xml/html/axl_install.html
 * \note For debian systems, you can also install it by running:
 *
 * \note \code
 * >> apt-get install python-axl
 * \endcode
 * 
 * Please, read each module documentation to know more about them and
 * their features. Assuming you know what modules you want you have to:
 *
 * 1) Run the following command to list all modules available:
 *
 * \code
 * >> valvulad-mgr.py  -o
 * Module: mod-slm
 * Module: mod-mquota
 * Module: mod-bwl
 * \endcode
 *
 * 2) Now, you have to now what listeners/ports are already declared
 * by Valvula where you can run modules. These listeners are just
 * Valvulad server entry points where postfix can delegate policy. If
 * you don't undestand it, don't worry too much. Keep on reading and
 * you'll understand by example.
 *
 * \code
 * >> valvulad-mgr.py -l
 * \endcode
 *
 * 3) If there are not listeners added, you must add at least one. Do
 * it by running the following (that adds, for example, a valvulad listener at 3080 TCP/port):
 *
 * \code
 * >> valvulad-mgr.py -a 3080
 * \endcode
 *
 * NOTE: this will update /etc/valvula/valvula.conf file. Please, have a look at it to know what's going on.
 *
 * Now, this listener/port is an eligible place to run modules that will control/modify postfix decisions about mail passing through it.
 *
 * 4) Now, assuming you want to enable <b>mod-slm</b>, you enable it at a particular listener
 * by running the following command. 
 *
 * \code
 * >> valvulad-mgr.py -m mod-slm 3080
 * \endcode
 *
 * NOTE: this will update /etc/valvula/valvula.conf file. Please, have a look at it to know what's going on.
 *
 * 5) After that, restart valvula by running something like:
 *
 * \code
 * >> service valvulad restart
 * \endcode
 *
 * 6) Now, you must connect this module to postfix in a particular
 * section. The point here is that postfix will call valvula server,
 * at a particular listener, where you have configured a set of
 * modules. Also, the place where postfix is connect to valvula is
 * important. In general, it is recommended to connect valvulad server
 * at smtpd_recipient_restrictions section. If you want to know what
 * are the postfix section and a description run:
 *
 * \code
 * >> valvulad-mgr.py -s
 * \endcode
 *
 * 7) Assuming we want to connect postfix to delegate decisions to valvula at smtpd_recipient_restrictions on the port/listener 3080, just run:
 *
 * \code
 * >> valvulad-mgr.py -c smtpd_recipient_restrictions 3080 first
 * \endcode
 *
 * This command means that you are connecting valvula listener located
 * at 3080, at the postfix's section called
 * smtpd_recipient_restrictions. The <b>first</b> is telling
 * valvulad-mgr.py to make that connection to be <b>first</b> policy
 * executed by postfix on that section. You can also use <b>last</b>
 * to change the place accordingly.
 *
 * 8) Now you are done. Just reload postfix and watch postfix+valvula function by running:
 *
 * \code
 * >> service postfix reload
 * >> tail -f /var/log/mail.log
 * \endcode
 *
 * \section valvulad_server_dont_panic 3. Hey, it isn't working, what should I do!
 *
 * Don't panic. It depends on the error you are having but if it's
 * fatal, please, just comment out "check_policy_service" declaration
 * at /etc/postfix/main.cf that is conecting postfix to valvula so your server can keep going as it was
 * configured. After commeting that declaration reload postfix:
 *
 * \code
 * >> service postfix reload
 * \endcode
 *
 * Then, you can count on us at the mailing list to get some help: http://lists.aspl.es/cgi-bin/mailman/listinfo/valvula
 * 
 * 
 *
 */
