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
#include <valvula_private.h>
#include <signal.h>

/* add missing declartion */
int kill (pid_t pid, int sig);


/* global context */
ValvuladCtx * ctx = NULL;

/** 
 * Default pid file configured.
 */
const char * pid_file_path  = "/var/run/valvulad.pid";
const char * valvula_status = "/var/run/valvulad.status";

#define HELP_HEADER "ValvulaD: a high performance policy daemon\n\
Copyright (C) 2014  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER "\n\
If you have question, bugs to report, patches, you can reach us\n\
at <vortex@lists.aspl.es>."

axl_bool valvulad_report_status_foreach (axlPointer key, axlPointer data, axlPointer _fstatus)
{
	ValvulaRequestRegistry * registry = key;
	FILE * fstatus = _fstatus;

	/* processing stats */
	fprintf (fstatus, "  <section title='Processing stats for %s (in ms)' />\n", registry->identifier);
	fprintf (fstatus, "  <attr name='avg request processing time' value='%ld' />\n", registry->avg_processing / 1000);
	fprintf (fstatus, "  <attr name='min request processing time' value='%ld' />\n", registry->min_processing / 1000);
	fprintf (fstatus, "  <attr name='max request processing time' value='%ld' />\n", registry->max_processing / 1000);


	return axl_false; /* iterate over all nodes */
}

void valvulad_report_status (void) {
	FILE * fstatus;
	int                 running_threads = 0;
	int                 waiting_threads = 0;
	int                 pending_tasks = 0;
	struct timeval  now;

	/* open valvula status */
	fstatus = fopen (valvula_status, "w");
	if (fstatus == NULL) {
		error ("Unable to open %s to dump current status, errno=%d : %s", errno, strerror (errno));
		return;
	} /* end if */


	fprintf (fstatus, "<valvula-state>\n");
	fprintf (fstatus, "  <section title='General stats' />\n");
	fprintf (fstatus, "  <attr name='valvula pid' value='%d' />\n", getpid ());
	fprintf (fstatus, "  <attr name='operation stamp' value='%ld' />\n", (long) time (NULL));
	fprintf (fstatus, "  <attr name='pending in reader queue' value='%d' />\n", valvula_async_queue_items (ctx->ctx->reader_queue));
	fprintf (fstatus, "  <attr name='handlers registered' value='%d' />\n", valvula_hash_size (ctx->ctx->process_handler_registry));
	fprintf (fstatus, "  <attr name='requests handled' value='%d' />\n", ctx->ctx->requests_handled);

	gettimeofday (&now, NULL);
	fprintf (fstatus, "  <attr name='seconds_running' value='%ld' />\n", now.tv_sec - ctx->started_at );

	fprintf (fstatus, "  <section title='TCP stats' />\n");
	fprintf (fstatus, "  <attr name='current listeners' value='%d' />\n", axl_list_length (ctx->ctx->srv_list));
	fprintf (fstatus, "  <attr name='current connections' value='%d' />\n", axl_list_length (ctx->ctx->conn_list));

	/* thread status and pending tasks */

	fprintf (fstatus, "  <section title='Work load' />\n");
	valvula_thread_pool_stats (ctx->ctx, &running_threads, &waiting_threads, &pending_tasks);
	fprintf (fstatus, "  <attr name='running threads' value='%d' />\n", running_threads);
	fprintf (fstatus, "  <attr name='waiting threads' value='%d' />\n", waiting_threads);
	fprintf (fstatus, "  <attr name='pending tasks' value='%d' />\n", pending_tasks);

	/* processing stats */
	fprintf (fstatus, "  <section title='Processing stats (in ms)' />\n");
	fprintf (fstatus, "  <attr name='avg request processing time' value='%ld' />\n", ctx->ctx->avg_processing / 1000);
	fprintf (fstatus, "  <attr name='min request processing time' value='%ld' />\n", ctx->ctx->min_processing / 1000);
	fprintf (fstatus, "  <attr name='max request processing time' value='%ld' />\n", ctx->ctx->max_processing / 1000);

	/* now show processing stats for each module */

	valvula_hash_foreach (ctx->ctx->process_handler_registry, valvulad_report_status_foreach, fstatus);

	fprintf (fstatus, "</valvula-state>\n");

	fclose (fstatus);
	
	return;
}

void show_current_processes (ValvuladCtx * ctx)
{
	ValvulaCtx           * _ctx = ctx->ctx;
	int                    iterator;
	ValvulaReaderProcess * process;
	const char           * sasl_user;
	int                    now;
	struct timeval         tv;
	ValvulaRequest       * request;

	/* get current stamp */
	gettimeofday (&tv, NULL);
	now = tv.tv_sec;

	/* call to acquire mutex */
	valvula_mutex_lock (&_ctx->op_mutex);

	if (axl_list_length (_ctx->request_in_process) == 0) {

		error ("No process was in processing state..");

		/* release lock */
		valvula_mutex_unlock (&_ctx->op_mutex);
		return;
	} /* end if */

	error ("Current processes being handled and not finished");
	
	iterator = 0;
	while (iterator < axl_list_length (_ctx->request_in_process)) {
		/* get next process */
		process   = axl_list_get_nth (_ctx->request_in_process, iterator);
		request   = process->request;
		sasl_user = valvula_get_sasl_user (process->request);
		
		error ("%d) %s : %s -> %s%s%s%s%s, port %d, queue-id %s, from %s (processing during %d secs)", 
		       iterator + 1, process->handler_name, 
		       /* request status */
		       request->sender, request->recipient, 
		       /* drop SASL information */
		       sasl_user ? " (" : "",
		       sasl_user ? "sasl_user=" : "",
		       sasl_user ? sasl_user : "",
		       sasl_user ? ")" : "",
		       /* include message */
		       request->listener_port, 
		       request->queue_id ? request->queue_id : "<undef>" ,
		       request->client_address ? request->client_address : "<undef>",
		       now - process->stamp);

		/* next iterator */
		iterator++;
	}

	valvula_mutex_unlock (&_ctx->op_mutex);
	return;
}

char ** __valvulad_ref_argv;

char ** __valvulad_add_skip_pid_check (char ** arguments) 
{
	int      iterator = 0;
	char  ** result;

	/* check if skip pid check is found; if it is found, just
	 * return without doing any modification */
	while (arguments[iterator]) {

		if (axl_cmp (arguments[iterator], "--skip-pid-check"))
			return arguments;

		iterator++;
	} /* end if */

	/* create new pointer */
	result = (char **) axl_new (char **, iterator + 1);
	result[iterator] = "--skip-pid-check";

	while (iterator > 0) {
		iterator--;
		result[iterator] = __valvulad_ref_argv[iterator];
	} /* end */
		
	return result;
}

void valvulad_signal (int _signal)
{
	ValvulaCtx        * _ctx      = ctx->ctx;
	char              * bt_file  = NULL;
	char              * cmd;
	ValvulaAsyncQueue * queue;
	axlNode           * node;
	int                 iterator;
	ValvulaConnection * listener;

	/* unlock listener */
	if (_signal == SIGINT || _signal == SIGTERM) 
		valvula_listener_unlock (_ctx);
	else if (_signal == SIGSEGV || _signal == SIGABRT) {
		error ("Valvula received a critical signal: %d", _signal);

		/* call to show current processes */
		show_current_processes (ctx);

		/* get signal handling */
		node = axl_doc_get (ctx->config, "/valvula/global-settings/signal");
		if (HAS_ATTR_VALUE (node, "action", "hold")) {
			error ("Holding process as declared by signal action...");

			/* create waiting queue */
			queue = valvula_async_queue_new ();
			/* hold it */
			valvula_async_queue_pop (queue);

		} else if (HAS_ATTR_VALUE (node, "action", "backtrace")) {
			error ("Getting process backtrace...");
			bt_file = valvulad_support_get_backtrace (ctx, getpid ());
			error ("Backtrace found at %s (pid %s)...", bt_file, getpid ());
			if (bt_file && valvula_support_file_test (bt_file, FILE_EXISTS)) {
				cmd = axl_strdup_printf ("cat %s", bt_file);
				if (system (cmd)) 
					printf ("ERROR: command %s failed..\n", cmd);
				axl_free (cmd);
			} /* end if */
			axl_free (bt_file);
		} else if (HAS_ATTR_VALUE (node, "action", "reexec") || HAS_ATTR_VALUE (node, "action", "re-exec")) {
			/* report what are the requests that are in place */
			error ("Restarting the server (removing pid file: %s).. ", pid_file_path);
			if (unlink (pid_file_path) == -1) 
				error ("Failed to remove pid file %s, errno=%d (%s)", pid_file_path, errno, strerror (errno));


			error ("Finishing server as clean as possible..");
			/* free valvula server context */
			ctx->ctx->skip_thread_pool_wait = axl_true;
			ctx->ctx->skip_reader_stop = axl_true;
			valvula_exit_ctx (ctx->ctx, axl_true);
			/* valvulad_exit (ctx); */
			iterator = 0;
			while (iterator < axl_list_length (ctx->listeners)) {

				/* get next iterator */
				listener = axl_list_get_nth (ctx->listeners, iterator);
				valvula_connection_close (listener);

				/* next iterator */
				iterator++;
			}

			error ("Restarting valvula..");
			__valvulad_ref_argv = __valvulad_add_skip_pid_check (__valvulad_ref_argv);
			execv (__valvulad_ref_argv[0], __valvulad_ref_argv);

			error ("execv() call failed, errno=%d", errno);
			exit (-1);
		} /* end if */

		error ("Ending valvula process...");
		exit (_signal);
		/* now kill the process */
		/* that is, just do nothing */
	} else if (_signal == SIGUSR1) {
		msg ("Received request to report status");
		valvulad_report_status ();
		signal (SIGUSR1, valvulad_signal); 
	} /* end if */

	return;
}

/**
 * @internal Implementation to detach turbulence from console.
 */
void valvulad_detach_process (void)
{

	msg ("Detaching from console, creating child process (parent: %d)", getpid ());

	pid_t   pid;
	/* fork */
	pid = fork ();
	switch (pid) {
	case -1:
		error ("unable to detach process, failed to executing child process");
		exit (-1);
	case 0:
		/* running inside the child process */
		msg ("running child created in detached mode (child pid: %d)", getpid ());
		return;
	default:
		/* child process created (parent code) */
		break;
	} /* end switch */

	/* terminate current process */
	msg ("finishing parent process (created child: %d, parent: %d)..", pid, getpid ());
	exit (0);
	return;
}

void valvulad_show_current_server_status (void)
{
	axlDoc   * doc;
	axlError * err = NULL;
	axlNode  * node;
	int        pid = -1;

	doc = axl_doc_parse_from_file (valvula_status, &err);
	if (doc == NULL) {
		printf ("ERROR: unable to open file %s, error was: %s\n",
			valvula_status, axl_error_get (err));
		axl_error_free (err);
		exit (-1);
	}  /* end if */

	/* find the pid file to send the signal */
	node = axl_doc_get (doc, "/valvula-state/attr");
	while (node) {
		if (HAS_ATTR_VALUE (node, "name", "valvula pid")) {
			/* get pid */
			pid = valvula_support_strtod (ATTR_VALUE (node, "value"), NULL);
			break;
		}

		/* get next node */
		node = axl_node_get_next_called (node, "attr");
	} /* end while */

	/* release document */
	axl_doc_free (doc);

	if (pid == -1) {
		printf ("ERROR: unable to find running pid..\n");
		exit (-1);
	} /* end if */

	printf ("--== Found pid %d, sending signal..==--\n", pid); 
	kill (pid, SIGUSR1);
	
	sleep (1);

	/* reparse file */
	doc = axl_doc_parse_from_file (valvula_status, &err);
	if (doc == NULL) {
		printf ("ERROR: unable to open file %s, error was: %s\n",
			valvula_status, axl_error_get (err));
		axl_error_free (err);
		exit (-1);
	}  /* end if */

	/* find the pid file to send the signal */
	node = axl_doc_get (doc, "/valvula-state");
	node = axl_node_get_first_child (node);
	while (node) {
		/* show values */
		if (NODE_CMP_NAME (node, "attr"))
			printf ("%s: %s\n", ATTR_VALUE (node, "name"), ATTR_VALUE (node, "value"));
		else if (NODE_CMP_NAME (node, "section"))
			printf ("\n--== %s ==--\n\n", ATTR_VALUE (node, "title"));

		/* get next node */
		node = axl_node_get_next (node);
	}
	printf ("\n");

	exit (0);

	return;
}

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

	exarg_install_arg ("detach", NULL, EXARG_NONE,
			   "Makes valvulad to detach from console, starting in background.");

	/* install exarg options */
	exarg_install_arg ("config", "c", EXARG_STRING, 
			   "Server configuration location.");

	/* install exarg options */
	exarg_install_arg ("debug-queries", "q", EXARG_NONE, 
			   "Makes valvulad server to show all database queries executed into the log (WARNING: user privacy may be violated with this option).");

	/* install exarg options */
	exarg_install_arg ("check-db", "b", EXARG_NONE, 
			   "Ask valvulad server to check current database connection.");

	exarg_install_arg ("skip-pid-check", "f", EXARG_NONE, 
			   "Makes valvula to skip pid checking on startup.");

	/* install exarg options */
	exarg_install_arg ("is-local-destination", "l", EXARG_STRING, 
			   "Ask valvula to check if the provided domain or account is a local destination for this server. Please note this option only works when valvula is able to detect local postfix configuration to the right query. If it does not work, please check valvula's server documentation.");

	/* install status option */
	exarg_install_arg ("status", "s", EXARG_NONE, 
			   "Dump status of the server.");


	/* call to parse arguments */
	exarg_parse (argc, argv);

	/* check for version request */
	if (exarg_is_defined ("version")) {
		printf ("%s\n", VERSION);
		/* terminates exarg */
		exarg_end ();
		return;
	}

	if (exarg_is_defined ("status")) {
		/* call to show status */
		valvulad_show_current_server_status ();
		return;
	}

	return;
}

/**
 * @internal Places current process identifier into the file provided
 * by the user.
 */
void valvulad_place_pidfile (ValvuladCtx * ctx)
{
	FILE    * pid_file = NULL;
	int       pid      = getpid ();
	char      buffer[20];
	int       size;
	axlNode * node;
	int       gid, uid;

	/* check if pid file exists */
	if  (! exarg_is_defined ("skip-pid-check")) {
		if (valvula_support_file_test (pid_file_path, FILE_EXISTS)) {
			abort_error ("Unable to start server, found pid file in place %s. There is a valvula server running. If not, remove file %s",
				     pid_file_path, pid_file_path);
			exit (-1);
			return;
		} /* end if */ 
	} else {
		wrn ("Skipping pid file checking..");
	} /* end if */

	/* open pid file or create it to place the pid file */
	pid_file = fopen (pid_file_path, "w");
	if (pid_file == NULL) {
		abort_error ("Unable to open pid file at: %s", pid_file_path);
		return;
	} /* end if */
	
	/* stringfy pid */
	size = axl_stream_printf_buffer (buffer, 20, NULL, "%d", pid);
	msg ("signaling PID %d at %s", pid, pid_file_path);
	fwrite (buffer, size, 1, pid_file);

	fclose (pid_file);

	pid_file = fopen (valvula_status, "w");
	if (pid_file == NULL) {
		abort_error ("Unable to open valvula status file at: %s (errno=%d)", valvula_status, errno);
		return;
	} /* end if */

	fprintf (pid_file, "<valvula-state>\n");
	fprintf (pid_file, "  <attr name='valvula pid' value='%d' />\n", getpid ());
	fprintf (pid_file, "</valvula-state>\n");

	fclose (pid_file);

	/* now change permissions (if required) */
	node = axl_doc_get (ctx->config, "/valvula/global-settings/running");
	if (node && ATTR_VALUE (node, "user") && ATTR_VALUE (node, "group") && HAS_ATTR_VALUE (node, "enabled", "yes")) {
		/* change group first */
		gid = valvulad_get_system_id (ctx, ATTR_VALUE (node, "group"), axl_false);
		uid = valvulad_get_system_id (ctx, ATTR_VALUE (node, "user"), axl_true);
		msg ("Attempting to update pid ownership to %d:%d", uid, gid);
		if (gid > 0 && uid > 0) {
			if (chown (valvula_status, uid, gid) != 0) {
				error ("Unable to change permissions to file %s (%d:%d), error was errno=%d (%s)",
				       valvula_status, uid, gid, errno, strerror (errno));
			} /* end if */

			if (chown (pid_file_path, uid, gid) != 0) {
				error ("Unable to change permissions to file %s (%d:%d), error was errno=%d (%s)",
				       valvula_status, uid, gid, errno, strerror (errno));
			} /* end if */

		} /* end if */
	} /* end if */

	return;
}


int main (int argc, char ** argv) 
{
	axl_bool      result;

	/* record a reference to arguments received */
	__valvulad_ref_argv = argv;

	/* parse arguments */
	install_arguments (argc, argv);

	/* check detach operation */
	if (exarg_is_defined ("detach")) {
		valvulad_detach_process ();
		/* caller do not follow */
	} /* end if */

	/* init here valvula library and valvulaD context */
	if (! valvulad_init (&ctx)) {
		error ("Failed to initialize ValvulaD context, unable to start server");
		exit (-1);
	} /* end if */

	if (exarg_is_defined ("verbose")) {
		ctx->console_enabled = axl_true;
		ctx->console_color_debug = axl_true;
	}
	if (exarg_is_defined ("debug")) {
		valvula_log_enable (ctx->ctx, axl_true);
		valvula_color_log_enable (ctx->ctx, axl_true);
	}
	if (exarg_is_defined ("debug-queries"))
		ctx->debug_queries = axl_true;

	/* parse configuration file */
	if (exarg_is_defined ("config"))
		result = valvulad_config_load (ctx, exarg_get_string ("config"));
	else
		result = valvulad_config_load (ctx, "/etc/valvula/valvula.conf");

	/* check here database link if requested */
	if (exarg_is_defined ("check-db")) {
		if (valvulad_db_init (ctx)) {
			printf ("INFO: Database connection working OK\n");
			exit (0);
		}

		printf ("ERROR: Database connection isn't working. Please check database server and/or credentials\n");
		exit (-1);
	} /* end if */

	if (exarg_is_defined ("is-local-destination")) {
		if (! valvulad_db_init (ctx)) {
			printf ("ERROR: Database connection NOT wokring\n");
			exit (-1);
		} /* end if */

		/* check local-domains configuration */
		if (! valvulad_run_check_local_domains_config (ctx)) {
			error ("Unable to startup local domains configuration");
			return axl_false;
		} /* end if */

		if (valvulad_run_is_local_domain (ctx, exarg_get_string ("is-local-destination"))) {
			printf ("INFO: %s is a local domain\n", exarg_get_string ("is-local-destination"));
			exit (0);
		} /* end if */

		if (valvulad_run_is_local_address (ctx, exarg_get_string ("is-local-destination"))) {
			printf ("INFO: %s is a local address\n", exarg_get_string ("is-local-destination"));
			exit (0);
		} /* end if */
		
		printf ("ERROR: %s is not a local domain nor a local address\n", exarg_get_string ("is-local-destination"));
		exit (-1);
	} /* end if */

	/* now check result from valvulad_config_log */
	if (! result) {
		printf ("ERROR: Failed to load configuration file (run with %s -o -d to get more details)..\n", argv[0]);
		exit (-1);
	}

	/* install signal handling */
	signal (SIGINT,  valvulad_signal); 		
	signal (SIGSEGV, valvulad_signal);
	signal (SIGABRT, valvulad_signal);
	signal (SIGTERM, valvulad_signal); 
	signal (SIGTERM, valvulad_signal); 
	signal (SIGUSR1, valvulad_signal); 

#if defined(AXL_OS_UNIX)
/*	signal (SIGKILL, signal_handler); */
	signal (SIGQUIT, valvulad_signal);

	/* check for sighup */
	signal (SIGHUP,  valvulad_signal);
#endif

	/* write pid file and status file */
	valvulad_place_pidfile (ctx);

	if (! valvulad_run_config (ctx)) {
		/* remove pid file */
		if (valvula_support_file_test (pid_file_path, FILE_IS_REGULAR | FILE_EXISTS))
			unlink (pid_file_path);

		printf ("ERROR: Failed to start configuration, unable to start the server\n");
		exit (-1);
	} /* end if */

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
