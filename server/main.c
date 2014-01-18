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

/* global context */
ValvuladCtx * ctx = NULL;

#define HELP_HEADER "ValvulaD: a high performance policy daemon\n\
Copyright (C) 2014  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER "\n\
If you have question, bugs to report, patches, you can reach us\n\
at <vortex@lists.aspl.es>."

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
