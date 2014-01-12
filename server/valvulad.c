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
#include <valvulad.h>
#include <signal.h>

#ifndef mkstemp
int mkstemp (char *template);
#endif


/* global context */
ValvuladCtx * ctx = NULL;

#define HELP_HEADER "ValvulaD: a high performance policy daemon\n\
Copyright (C) 2014  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER "\n\
If you have question, bugs to report, patches, you can reach us\n\
at <vortex@lists.aspl.es>."

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

void install_arguments (int argc, char ** argv)
{
	/* install headers for help */
	exarg_add_usage_header  (HELP_HEADER);
	exarg_add_help_header   (HELP_HEADER);
	exarg_post_help_header  (POST_HEADER);
	exarg_post_usage_header (POST_HEADER);

	/* install default debug options. */
	exarg_install_arg ("version", "v", EXARG_NONE,
			   "Show Valvulad version.");

	/* install default debug options. */
	exarg_install_arg ("debug", "d", EXARG_NONE,
			   "Activates debug information to be showed in the console (terminal).");

	/* install default debug options. */
	exarg_install_arg ("verbose", "o", EXARG_NONE,
			   "Makes valvula server to produce some logs while operating.");

	/* install exarg options */
	exarg_install_arg ("config", "c", EXARG_STRING, 
			   "Server configuration location.");

	/* call to parse arguments */
	exarg_parse (argc, argv);

	/* check for version request */
	if (exarg_is_defined ("version")) {
		printf ("%s\n", VERSION);
		/* terminates exarg */
		exarg_end ();
		return;
	}

	return;
}


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

	if (exarg_is_defined ("verbose")) {
		ctx->console_enabled = axl_true;
		ctx->console_color_debug = axl_true;
	}
	if (exarg_is_defined ("debug")) {
		valvula_log_enable (ctx->ctx, axl_true);
		valvula_color_log_enable (ctx->ctx, axl_true);
	}

	msg ("Valvulad context initialized");

	/* setup result */
	(*result) = ctx;
	return axl_true;
}

void valvulad_exit (ValvuladCtx * ctx)
{
	/* release all context resources */
	axl_doc_free (ctx->config);
	axl_free (ctx->config_path);
	axl_free (ctx);

	return;
}



void valvulad_signal (int _signal)
{
	ValvuladCtx * ctxd     = ctx;
	ValvulaCtx  * temp_ctx = ctx->ctx;
	ValvulaCtx  * ctx      = temp_ctx;
	char        * bt_file  = NULL;
	char        * cmd;

	/* unlock listener */
	if (_signal == SIGINT || _signal == SIGTERM) 
		valvula_listener_unlock (ctx);
	else if (_signal == SIGSEGV || _signal == SIGABRT) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "Critical signal received: %d", _signal);
		bt_file = valvulad_support_get_backtrace (ctxd, getpid ());
		if (bt_file && valvula_support_file_test (bt_file, FILE_EXISTS)) {
			cmd = axl_strdup_printf ("cat %s", bt_file);
			system (cmd);
			axl_free (cmd);
		} /* end if */
		axl_free (bt_file);
	} /* end if */
	

	return;
}

int main (int argc, char ** argv) 
{
	axl_bool      result;

	/* parse arguments */
	install_arguments (argc, argv);

	/* init here valvula library and valvulaD context */
	if (! valvulad_init (&ctx)) {
		error ("Failed to initialize ValvulaD context, unable to start server");
		exit (-1);
	} /* end if */

	/* parse configuration file */
	if (exarg_is_defined ("config"))
		result = valvulad_config_load (ctx, exarg_get_string ("config"));
	else
		result = valvulad_config_load (ctx, "/etc/valvula/valvula.conf");

	if (! result) {
		error ("Failed to load configuration file");
		exit (-1);
	}

	if (! valvulad_run_config (ctx)) {
		error ("Failed to start configuration, unable to start the server");
		exit (-1);
	} /* end if */

	/* install signal handling */
	signal (SIGINT,  valvulad_signal); 		
	signal (SIGSEGV, valvulad_signal);
	signal (SIGABRT, valvulad_signal);
	signal (SIGTERM, valvulad_signal); 

#if defined(AXL_OS_UNIX)
/*	signal (SIGKILL, signal_handler); */
	signal (SIGQUIT, valvulad_signal);

	/* check for sighup */
	signal (SIGHUP,  valvulad_signal);
#endif

	/* now wait for requests */
	msg ("Valvula server started, processing requests..");
	valvula_listener_wait (ctx->ctx);

	msg ("Valvula server is finishing, releasing resources..");
	valvula_exit_ctx (ctx->ctx, axl_true);

	/* free valvula server context */
	valvulad_exit (ctx);

	/* finalize daemon */
	exarg_end ();

	return 0;
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
