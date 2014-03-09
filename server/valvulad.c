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
 * @brief Allows to report in a consistent maner when an operation is
 * rejected.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param request The request operation that was rejected.
 *
 * @param format A printf-like message to report along with the reject message.
 *
 * @parm ... Additional parameters to complete the request.
 */
void  valvulad_reject (ValvuladCtx * ctx, ValvulaRequest * request, const char * format, ...)
{
	va_list     args;
	char      * message;

	va_start (args, format);
	/* create the message */
	message = axl_stream_strdup_printfv (format, args);
	va_end (args);

	msg ("REJECT: %s -> %s : %s", request->sender, request->recipient, message);
	axl_free (message);

	return;
}

axl_bool valvulad_init (ValvuladCtx ** result) {
	ValvuladCtx * ctx;
	
	/* init context */
	ctx = axl_new (ValvuladCtx, 1);
	if (ctx == NULL)
		return axl_false;

	/* create library context */
	ctx->ctx = valvula_ctx_new ();
	if (ctx->ctx == NULL)
		return axl_false;

	if (! valvula_init_ctx (ctx->ctx))
		return axl_false;

	msg ("Valvulad context initialized");

	/* init modules */
	valvulad_module_init (ctx);

	/* setup result */
	(*result) = ctx;
	return axl_true;
}

void valvulad_exit (ValvuladCtx * ctx)
{
	/* stop modules */
	msg ("Releasing modules (loaded: %d)..", axl_list_length (ctx->registered_modules));
	valvulad_module_cleanup (ctx);

	valvulad_db_cleanup (ctx);

	/* release on day change handlers */
	axl_list_free (ctx->on_day_change_handlers);
	ctx->on_day_change_handlers = NULL;

	/* release on month change handlers */
	axl_list_free (ctx->on_month_change_handlers);
	ctx->on_month_change_handlers = NULL;
	
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
