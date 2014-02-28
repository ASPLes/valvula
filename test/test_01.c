/*
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2014 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; version 2.1 of the
 *  License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
 *
 *  For comercial support about integrating valvula or any other ASPL
 *  software production please contact as at:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº10, Edificio Alius A, Despacho 102
 *         Alcala de Henares, 28802 (MADRID)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/valvula
 */

#include <valvulad.h>

axl_bool test_common_enable_debug = axl_false;

int          test_readline (ValvulaCtx * ctx, VALVULA_SOCKET session, char  * buffer, int  maxlen)
{
	int         n, rc;
	int         desp;
	char        c, *ptr;

	/* avoid calling to read when no good socket is defined */
	if (session == -1)
		return -1;

	/* clear the buffer received */
	/* memset (buffer, 0, maxlen * sizeof (char ));  */

	/* check for pending line read */
	desp         = 0;

	/* read current next line */
	ptr = (buffer + desp);
	for (n = 1; n < (maxlen - desp); n++) {
	__valvula_frame_readline_again:
		if (( rc = recv (session, &c, 1, 0)) == 1) {
			*ptr++ = c;
			if (c == '\x0A')
				break;
		}else if (rc == 0) {
			if (n == 1)
				return 0;
			else
				break;
		} else {
			if (errno == VALVULA_EINTR) 
				goto __valvula_frame_readline_again;
			if ((errno == VALVULA_EWOULDBLOCK) || (errno == VALVULA_EAGAIN) || (rc == -2)) {
				if (n > 0) {
					/* store content read until now */
					if ((n + desp - 1) > 0) {
						buffer[n+desp - 1] = 0;
					} /* end if */
				} /* end if */
				return (-2);
			}
			
			return (-1);
		} /* end if */
	} /* end for */

	*ptr = 0;
	return (n + desp);

}

ValvuladCtx *  test_valvula_load_config (const char * label, const char * path, axl_bool run_config)
{
	ValvuladCtx * result;

	/* show a message */
	printf ("%s: loading configuration file at: %s\n", label, path);

	if (! valvulad_init (&result)) {
		printf ("ERROR: failed to initialize Valvulad context..\n");
		printf ("   Maybe mysql database is failing? Please create user valvula, password valvula\n");
		return NULL;
	}

	/* enable debug */
	if (test_common_enable_debug) {
		valvulad_log_enable (result, axl_true);
		valvulad_log2_enable (result, axl_true);
		valvulad_log3_enable (result, axl_true);
		valvulad_color_log_enable (result, axl_true);
	}

	/* load config provided */
	if (! valvulad_config_load (result, path)) {
		printf ("ERROR: failed to load configuration at: %s\n", path);
		return NULL;
	} /* end if */

	/* try run configuration */
	if (run_config && ! valvulad_run_config (result)) {
		printf ("ERROR: failed to run configuration found at: %s\n", path);
		return NULL;
	} /* end if */

	return result;
}

void common_finish (ValvuladCtx * ctx)
{
	valvula_exit_ctx (ctx->ctx, axl_true);
	valvulad_exit (ctx);

	return;
}


axl_bool  test_01 (void)
{
	ValvuladCtx * ctx;
	const char  * path;

	/* load basic configuration */
	path = "../server/valvula.example.conf";
	ctx  = test_valvula_load_config ("Test 01: ", path, axl_true);
	if (! ctx) {
		printf ("ERROR: unable to load configuration file at %s\n", path);
		return axl_false;
	} /* end if */

	/* wait a bit to let the system to startup */
	printf ("Test 01: waiting a bit..\n");
	sleep (1);

	/* free valvula server context */
	printf ("Test 01: finishing configuration..\n");
	common_finish (ctx);
	
	return axl_true;
}

void send_content (VALVULA_SOCKET socket, const char * header, const char * content)
{
	int    bytes_written;
	int    content_length;
	char * payload;

	if (header == NULL || content == NULL)
		return;

	/* build header */
	payload        = axl_strdup_printf ("%s=%s\n", header, content);
	content_length = strlen (payload);

	/* send content */
	bytes_written = send (socket, payload, content_length, 0);
	if (bytes_written != content_length) {
		printf ("ERROR: content sent %d differs from expected %d\n", bytes_written, content_length);
		exit (-1);
	} /* end if */

	axl_free (payload);
	return;
}

ValvulaState test_translate_action (const char * buffer, int buffer_len)
{
	int iterator = 0;

	while (buffer[iterator] != '=' && buffer[iterator] != 0)
		iterator++;
	if (axl_memcmp (buffer + iterator + 1, "dunno", 5))
		return VALVULA_STATE_DUNNO;
	if (axl_memcmp (buffer + iterator + 1, "reject", 6))
		return VALVULA_STATE_REJECT;
	if (axl_memcmp (buffer + iterator + 1, "ok", 2))
		return VALVULA_STATE_OK;
	if (axl_memcmp (buffer + iterator + 1, "discard", 7))
		return VALVULA_STATE_DISCARD;

	
	printf ("ERROR: unable to translate (%s) into an state, reporting generic error..\n", buffer);
	return VALVULA_STATE_GENERIC_ERROR;
}

ValvulaState test_valvula_request (const char * policy_server, const char * port,
				   const char * request, const char * protocol_state, const char * protocol_name,
				   const char * sender, const char * recipient, const char * recipient_count,
				   const char * queue_id, const char * message_size,
				   const char * sasl_method, const char * sasl_username, const char * sasl_sender)
{
	axlError        * error = NULL;
	ValvulaCtx      * ctx   = valvula_ctx_new ();
	VALVULA_SOCKET    session;
	char              buffer[1024];
	int               bytes;
	
	/* create sock connection */
	session = valvula_connection_sock_connect (ctx, policy_server, port, NULL, &error);
	if (session < 1) {
		printf ("ERROR: failed to connect to %s:%s, error was: %s, errno=%d\n", 
			policy_server, port, axl_error_get (error), errno);
		axl_error_free (error);
		return VALVULA_STATE_GENERIC_ERROR;
	} /* end if */

	/* ok, now send policy request */
	send_content (session, "request", request);
	send_content (session, "protocol_state", protocol_state);
	send_content (session, "protocol_name", protocol_name);

	/* sender */
	send_content (session, "sender", sender);
	send_content (session, "recipient", recipient);
	send_content (session, "recipient_count", recipient_count);

	/* message description */
	send_content (session, "queue_id", queue_id);
	send_content (session, "message_size", message_size);

	/* sasl options */
	send_content (session, "sasl_method", sasl_method);
	send_content (session, "sasl_username", sasl_username);
	send_content (session, "sasl_sender", sasl_sender);

	/* send finalization */
	send (session, "\n", 1, 0);

	/* receive content */
	memset (buffer, 0, 1024);
	while (axl_true) {
		bytes = test_readline (ctx, session, buffer, 1024);
		if (bytes == -1) {
			printf ("ERROR: received error code = %d from test_readline\n", bytes);
			return VALVULA_STATE_GENERIC_ERROR;
		}
		if (bytes == -2) {
			sleep (1);
			continue;
		} /* end if */

		printf ("Test --: content received as reply (bytes=%d): %s", bytes, buffer); 
		break;
	}

	if (strstr (buffer, "action=") == NULL) {
		printf ("ERROR: no action found in reply..\n");
		return VALVULA_STATE_GENERIC_ERROR;
	} /* end if */

	valvula_close_socket (session);
	valvula_ctx_unref (&ctx);
	
	return test_translate_action (buffer, 1024);
}


axl_bool  test_01a (void)
{
	ValvuladCtx   * ctx;
	const char    * path;
	ValvulaState    state;

	/* load basic configuration */
	path = "../server/valvula.example.conf";
	ctx  = test_valvula_load_config ("Test 01: ", path, axl_true);
	if (! ctx) {
		printf ("ERROR: unable to load configuration file at %s\n", path);
		return axl_false;
	} /* end if */

	/* do a request */
	state = test_valvula_request (/* policy server location */
				      "127.0.0.1", "3579", 
				      /* state */
				      "smtpd_access_policy", "RCPT", "SMTP",
				      /* sender, recipient, recipient count */
				      "francis@aspl.es", "francis@aspl.es", "1",
				      /* queue-id, size */
				      "935jfe534", "235",
				      /* sasl method, sasl username, sasl sender */
				      "plain", "francis@aspl.es", NULL);
	if (state != VALVULA_STATE_DUNNO) {
		printf ("ERROR: expected valvula state %d but found %d\n", VALVULA_STATE_DUNNO, state);
		return axl_false;
	}


				      
				      

	/* free valvula server context */
	printf ("Test 01: finishing configuration..\n");
	common_finish (ctx);
	
	return axl_true;
}

axl_bool  test_02 (void)
{
	ValvuladCtx   * ctx;
	const char    * path;

	/* load basic configuration */
	path = "../server/valvula.example.conf";
	ctx  = test_valvula_load_config ("Test 01: ", path, axl_true);
	if (! ctx) {
		printf ("ERROR: unable to load configuration file at %s\n", path);
		return axl_false;
	} /* end if */

	/* do a database creation */
	printf ("Test 02: check if test_02_table exists..\n");
	if (valvulad_db_table_exists (ctx, "test_02_table")) {
		printf ("ERROR: expected table test_02_table to be not present but it is..\n");
		return axl_false;
	} /* end if */

	/* create the table */
	printf ("Test 02: ensure test_02_table exists..\n");
	if (! valvulad_db_ensure_table (ctx, "test_02_table", "id", "int", NULL)) {
		printf ("ERROR: failed to create table test_02_table, valvulad_db_ensure_table failed..\n");
		return axl_false;
	} /* end if */

	/* now check the table exists */
	if (! valvulad_db_table_exists (ctx, "test_02_table")) {
		printf ("ERROR: expected table test_02_table to be present but it is NOT..\n");
		return axl_false;
	} /* end if */

	/* now remove the table */
	printf ("Test 02: remove test_02_table..\n");
	if (! valvulad_db_table_remove (ctx, "test_02_table")) {
		printf ("ERROR: expected table test_02_table to be removed by valvulad_db_table_remove () failed\n");
		return axl_false;
	}

	/* now check the table exists */
	if (valvulad_db_table_exists (ctx, "test_02_table")) {
		printf ("ERROR: expected table test_02_table to be NOT present but it is after removal..\n");
		return axl_false;
	} /* end if */

	/* create the table */
	printf ("Test 02: ensure test_02_table exists..\n");
	if (! valvulad_db_ensure_table (ctx, "test_02_table", "id", "int", NULL)) {
		printf ("ERROR: failed to create table test_02_table, valvulad_db_ensure_table failed..\n");
		return axl_false;
	} /* end if */

	/* now check attributes exists */
	if (valvulad_db_attr_exists (ctx, "test_02_table", "new")) {
		printf ("ERROR: attribute new SHOULDN'T exist but it does (valvulad_db_attr_exists() failed..)\n");
		return axl_false;
	}

	/* now check attributes exists */
	if (valvulad_db_attr_exists (ctx, "test_02_table", "value2")) {
		printf ("ERROR: attribute 'value2' SHOULDN'T exist but it does (valvulad_db_attr_exists() failed..)\n");
		return axl_false;
	}

	/* create the table */
	printf ("Test 02: ensure test_02_table exists..\n");
	if (! valvulad_db_ensure_table (ctx, "test_02_table", "id", "int", "new", "text", "value2", "text", NULL)) {
		printf ("ERROR: failed to create table test_02_table, valvulad_db_ensure_table failed (2)..\n");
		return axl_false;
	} /* end if */

	/* now check attributes exists */
	if (! valvulad_db_attr_exists (ctx, "test_02_table", "new")) {
		printf ("ERROR: attribute new should exist but it doesn't (valvulad_db_attr_exists() failed..)\n");
		return axl_false;
	}

	/* now check attributes exists */
	if (! valvulad_db_attr_exists (ctx, "test_02_table", "value2")) {
		printf ("ERROR: attribute 'value2' should exist but it doesn't (valvulad_db_attr_exists() failed..)\n");
		return axl_false;
	}

	/* now remove the table */
	printf ("Test 02: remove test_02_table..\n");
	if (! valvulad_db_table_remove (ctx, "test_02_table")) {
		printf ("ERROR: expected table test_02_table to be removed by valvulad_db_table_remove () failed\n");
		return axl_false;
	}

	printf ("Test 02: finishing..\n");

	/* free valvula server context */
	common_finish (ctx);
	
	return axl_true;
}

#define CHECK_TEST(name) if (run_test_name == NULL || axl_cmp (run_test_name, name))

typedef axl_bool (* ValvulaTestHandler) (void);

/** 
 * @brief Helper handler that allows to execute the function provided
 * with the message associated.
 * @param function The handler to be called (test)
 * @param message The message value.
 */
int run_test (ValvulaTestHandler function, const char * message) {

	if (function ()) {
		printf ("%s [   OK   ]\n", message);
	} else {
		printf ("%s [ FAILED ]\n", message);
		exit (-1);
	}
	return 0;
}

/** 
 * @brief General regression test to check all features inside
 * valvula.
 */
int main (int argc, char ** argv)
{
	char * run_test_name = NULL;

	printf ("** test_01: Valvula: a high performance policy daemon\n");
	printf ("** Copyright (C) 2014 Advanced Software Production Line, S.L.\n**\n");
	printf ("** Regression tests: valvula: %s \n",
		VERSION);
	printf ("** To gather information about time performance you can use:\n**\n");
	printf ("**     time ./test_01 [--help] [--debug] [--run-test=NAME]\n**\n");
	printf ("** To gather information about memory consumed (and leaks) use:\n**\n");
	printf ("**     >> libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes --error-limit=no ./test_01 [--debug]\n**\n");
	printf ("** Providing --run-test=NAME will run only the provided regression test.\n");
	printf ("** Available tests: test_01\n");
	printf ("**\n");
	printf ("** Report bugs to:\n**\n");
	printf ("**     <valvula@lists.aspl.es> Valvula Mailing list\n**\n");

	/* uncomment the following four lines to get debug */
	while (argc > 0) {
		if (axl_cmp (argv[argc], "--help")) 
			exit (0);
		if (axl_cmp (argv[argc], "--debug")) 
			test_common_enable_debug = axl_true;

		if (argv[argc] && axl_memcmp (argv[argc], "--run-test", 10)) {
			run_test_name = argv[argc] + 11;
			printf ("INFO: running test: %s\n", run_test_name);
		}
		argc--;
	} /* end if */

	/* run tests */
	CHECK_TEST("test_01")
	run_test (test_01, "Test 01: basic server startup (using default configuration)");

	/* run tests */
	CHECK_TEST("test_01a")
	run_test (test_01a, "Test 01-a: basic mail policy request");

	/* run tests */
	CHECK_TEST("test_02")
	run_test (test_02, "Test 02: database functions");

	printf ("All tests passed OK!\n");

	/* terminate */
	return 0;
	
} /* end main */
