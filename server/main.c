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

/* global context */
ValvuladCtx * ctx = NULL;

#define HELP_HEADER "ValvulaD: a high performance policy daemon\n\
Copyright (C) 2014  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER "\n\
If you have question, bugs to report, patches, you can reach us\n\
at <vortex@lists.aspl.es>."

void valvulad_signal (int _signal)
{
	ValvulaCtx        * _ctx      = ctx->ctx;
	char              * bt_file  = NULL;
	char              * cmd;
	ValvulaAsyncQueue * queue;
	axlNode           * node;

	/* unlock listener */
	if (_signal == SIGINT || _signal == SIGTERM) 
		valvula_listener_unlock (_ctx);
	else if (_signal == SIGSEGV || _signal == SIGABRT) {
		error ("Critical signal received: %d", _signal);

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
		} /* end if */

		error ("Ending valvula process...");
		exit (_signal);
		/* now kill the process */
		/* that is, just do nothing */

	} /* end if */

	return;
}

/**
 * @internal Implementation to detach turbulence from console.
 */
void valvulad_detach_process (void)
{
	pid_t   pid;
	/* fork */
	pid = fork ();
	switch (pid) {
	case -1:
		syslog (LOG_ERR, "unable to detach process, failed to executing child process");
		exit (-1);
	case 0:
		/* running inside the child process */
		syslog (LOG_INFO, "running child created in detached mode");
		return;
	default:
		/* child process created (parent code) */
		break;
	} /* end switch */

	/* terminate current process */
	syslog (LOG_INFO, "finishing parent process (created child: %d, parent: %d)..", pid, getpid ());
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

	/* check detach operation */
	if (exarg_is_defined ("detach")) {
		valvulad_detach_process ();
		/* caller do not follow */
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

	/* now check result from valvulad_config_log */
	if (! result) {
		error ("Failed to load configuration file");
		exit (-1);
	}

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

	if (! valvulad_run_config (ctx)) {
		error ("Failed to start configuration, unable to start the server");
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
