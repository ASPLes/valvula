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
#include <mod-mquota.h>

/* private import: do not include it unless you know what you are
 * doing */
#include <valvula_private.h>

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

ValvuladCtx *  test_valvula_load_config_aux (const char * label, const char * path, axl_bool run_config, 
					     ValvuladCtx * result, const char * postfix_file) {

	/* enable debug */
	if (test_common_enable_debug) {
		valvulad_log_enable (result, axl_true);
		valvulad_log2_enable (result, axl_true);
		valvulad_log3_enable (result, axl_true);
		valvulad_color_log_enable (result, axl_true);
		result->debug_queries = axl_true;
	}

	/* configure postfix file */
	result->postfix_file = postfix_file;

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

	return test_valvula_load_config_aux (label, path, run_config, result, "/etc/postfix/main.cf");
}

void common_finish (ValvuladCtx * ctx)
{
	valvula_exit_ctx (ctx->ctx, axl_true);
	valvulad_exit (ctx);

	return;
}

axl_bool  test_00 (void) {

	ValvulaCtx      * ctx   = valvula_ctx_new ();

	/*** check valvula_get_domain ***/

	/* check valvula_get_domain */
	if (! axl_cmp (valvula_get_domain ("francis@aspl.es"), "aspl.es")) {
		printf ("ERROR 0.1: expected different value: %s..\n", valvula_get_domain ("francis@aspl.es"));
		return axl_false;
	} /* end if */

	/* check valvula_get_domain */
	if (! axl_cmp (valvula_get_domain ("aspl.es"), "aspl.es")) {
		printf ("ERROR 0.2: expected different value: %s..\n", valvula_get_domain ("aspl.es"));
		return axl_false;
	} /* end if */

	/* check valvula_get_domain */
	if (axl_cmp (valvula_get_domain (NULL), "aspl.es")) {
		printf ("ERROR 0.3: expected different value..\n");
		return axl_false;
	} /* end if */

	/*** check valvula_address_rule_match ***/

	/* check address rule match */
	if (! valvula_address_rule_match (ctx, NULL, "francis@aspl.es")) {
		printf ("ERROR 0.4: expected positive..\n");
		return axl_false;
	} /* end if */

	/* check address rule match */
	if (! valvula_address_rule_match (ctx, "", "francis@aspl.es")) {
		printf ("ERROR 0.5: expected positive..\n");
		return axl_false;
	} /* end if */

	/* check address rule match */
	if (! valvula_address_rule_match (ctx, "test.com", "test@test.com")) {
		printf ("ERROR 0.6: expected positive..\n");
		return axl_false;
	} /* end if */

	/* check address rule match */
	if (valvula_address_rule_match (ctx, "test.com", "francis@aspl.es")) {
		printf ("ERROR 0.7: expected negative..\n");
		return axl_false;
	} /* end if */

	/* check address rule match */
	if (! valvula_address_rule_match (ctx, "test.com", "test.com")) {
		printf ("ERROR 0.8: expected positive..\n");
		return axl_false;
	} /* end if */

	/* check address rule match */
	if (valvula_address_rule_match (ctx, "test2@test.com", "test@test.com")) {
		printf ("ERROR 0.9: expected negative..\n");
		return axl_false;
	} /* end if */

	/* check address rule match */
	if (valvula_address_rule_match (ctx, "francis@aspl.es", "francis2@aspl.es")) {
		printf ("ERROR 0.10: expected negative..\n");
		return axl_false;
	} /* end if */

	valvula_ctx_unref (&ctx);

	return axl_true;
}


axl_bool  test_01 (void)
{
	ValvuladCtx * ctx;
	const char  * path;

	/* load basic configuration */
	path = "test_01.conf";
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

const char * test_valvula_content = NULL;

ValvulaState test_translate_action_step2 (ValvulaState state, const char * content) {
	/* if nothing to check, just report state received */
	if (! test_valvula_content)
		return state;

	if (! axl_memcmp (test_valvula_content, content, strlen (test_valvula_content))) {
		printf ("ERROR: expected to find '%s' but found '%s'\n", test_valvula_content, content);
		exit (-1);
	} /* end if */

	/* test ok, reset it and report value reported */
	test_valvula_content = NULL;
	return state;
}


ValvulaState test_translate_action (const char * buffer, int buffer_len)
{
	int iterator = 0;

	while (buffer[iterator] != '=' && buffer[iterator] != 0)
		iterator++;

	if (axl_memcmp (buffer + iterator + 1, "dunno", 5))
		return test_translate_action_step2 (VALVULA_STATE_DUNNO, buffer + iterator + 7);
	if (axl_memcmp (buffer + iterator + 1, "reject", 6)) 
		return test_translate_action_step2 (VALVULA_STATE_REJECT, buffer + iterator + 8);
	if (axl_memcmp (buffer + iterator + 1, "ok", 2))
		return test_translate_action_step2 (VALVULA_STATE_OK, buffer + iterator + 4);
	if (axl_memcmp (buffer + iterator + 1, "discard", 7))
		return test_translate_action_step2 (VALVULA_STATE_DISCARD, buffer + iterator + 9);
	if (axl_memcmp (buffer + iterator + 1, "filter", 6)) {
		return test_translate_action_step2 (VALVULA_STATE_FILTER, buffer + iterator + 8);
	}

	
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

	/* client address */
	send_content (session, "client_address", policy_server);

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
	path = "test_01.conf";
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
		printf ("ERROR (01-a.1): expected valvula state %d but found %d\n", VALVULA_STATE_DUNNO, state);
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
	long            result;

	/* load basic configuration */
	path = "test_01.conf";
	ctx  = test_valvula_load_config ("Test 01: ", path, axl_true);
	if (! ctx) {
		printf ("ERROR: unable to load configuration file at %s\n", path);
		return axl_false;
	} /* end if */

	/* now remove the table */
	printf ("Test 02: remove test_02_table (if it is present)..\n");
	valvulad_db_table_remove (ctx, "test_02_table");

	/* do a database creation */
	printf ("Test 02: check if test_02_table exists..\n");
	if (valvulad_db_table_exists (ctx, "test_02_table")) {
		printf ("ERROR: expected table test_02_table to be not present but it is..\n");
		return axl_false;
	} /* end if */

	/* create the table */
	printf ("Test 02: ensure test_02_table exists..\n");
	if (! valvulad_db_ensure_table (ctx, "test_02_table", "id", "autoincrement int", NULL)) {
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
	if (! valvulad_db_ensure_table (ctx, "test_02_table", "id", "autoincrement int", NULL)) {
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
	if (! valvulad_db_ensure_table (ctx, "test_02_table", "id", "autoincrement int", "new", "text", "value2", "text", NULL)) {
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

	/* check boolean check queries */
	if (valvulad_db_boolean_query (ctx, "SELECT * FROM test_02_table")) {
		printf ("ERROR: expected to find valvulad_db_boolean_query to report false but it is reporting ok status..\n");
		return axl_false;
	}

	/* insert content */
	if (! valvulad_db_run_non_query (ctx, "INSERT INTO test_02_table (new) VALUES ('%s')", "test")) {
		printf ("ERROR: expected to insert value with valvulad_db_run_non_query but found a failure..\n");
		return axl_false;
	} /* end if */

	/* get the id installed */
	result = valvulad_db_run_query_as_long (ctx, "SELECT id FROM test_02_table");
	printf ("Test 02: id from test_02_table: %ld\n", result);
	if (result < 1) {
		printf ("ERROR: expected to receive a proper value defined from valvulad_db_run_query_as_long but received %ld\n", result);
		return axl_false;
	} /* end if */

	/* get the id installed */
	result = valvulad_db_run_query_as_long (ctx, "SELECT id FROM test_02_table WHERE new like '#te#'");
	printf ("Test 02: id from test_02_table (like version): %ld\n", result);
	if (result < 1) {
		printf ("ERROR: expected to receive a proper value defined from valvulad_db_run_query_as_long but received %ld\n", result);
		return axl_false;
	} /* end if */

	/* check boolean check queries */
	if (! valvulad_db_boolean_query (ctx, "SELECT * FROM test_02_table")) {
		printf ("ERROR: expected to find valvulad_db_boolean_query to report true but it is reporting false status..\n");
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

axl_bool  test_02a (void)
{
	ValvulaRequest * request = axl_new (ValvulaRequest, 1);

	/* check requests */
	request->sender = "francis@aspl.es";
	if (! axl_cmp ("aspl.es", valvula_get_sender_domain (request))) {
		printf ("ERROR: expected to find sender domain aspl.es domain but it failed..\n");
		return axl_false;
	} /* end if */

	request->sender = "MAILER-DAEMON";
	if (! axl_cmp ("MAILER-DAEMON", valvula_get_sender_domain (request))) {
		printf ("ERROR: expected to find sender domain MAILER-DAEMON domain but it failed..\n");
		return axl_false;
	} /* end if */

	/* release request */
	axl_free (request);

	return axl_true;
}

axl_bool  test_02b (void)
{
	ValvuladCtx    * ctx = axl_new (ValvuladCtx, 1);
	ValvulaRequest * request;

	/* init the library */
	if (! valvulad_init_aux (ctx)) {
		printf ("ERROR: failed to initialize Valvulad context..\n");
		return axl_false;
	} /* end if */

	/* load configuration */
	test_valvula_load_config_aux ("Test 02-b", "test_02b.conf", axl_true, ctx, "test_02b.postfix.cf");

	/* create some tables */
	if (! valvulad_db_ensure_table (ctx, "domain", "domain", "text", "active", "int", NULL)) {
		printf ("ERROR: unable to create local domain table..\n");
		return axl_false;
	} /* end if */

	/* insert some values */
	valvulad_db_run_non_query (ctx, "DELETE FROM domain");
	valvulad_db_run_non_query (ctx, "INSERT INTO domain (domain, active) VALUES ('aspl.es', 1)");
	valvulad_db_run_non_query (ctx, "INSERT INTO domain (domain, active) VALUES ('asplhosting.com', 1)");
	valvulad_db_run_non_query (ctx, "INSERT INTO domain (domain, active) VALUES ('microsoft.com', 1)");
	valvulad_db_run_non_query (ctx, "INSERT INTO domain (domain, active) VALUES ('core-admin.com', 1)");

	/* now check if these values are detected as local domains */
	if (! valvulad_run_is_local_domain (ctx, "aspl.es")) {
		printf ("ERROR: expected aspl.es to be reported as local domain...\n");
		return axl_false;
	} /* end if */

	/* now check if these values are detected as local domains */
	if (! valvulad_run_is_local_domain (ctx, "asplhosting.com")) {
		printf ("ERROR: expected aspl.es to be reported as local domain...\n");
		return axl_false;
	} /* end if */

	/* now check if these values are detected as local domains */
	if (! valvulad_run_is_local_domain (ctx, "microsoft.com")) {
		printf ("ERROR: expected aspl.es to be reported as local domain...\n");
		return axl_false;
	} /* end if */

	/* now check if these values are detected as local domains */
	if (! valvulad_run_is_local_domain (ctx, "core-admin.com")) {
		printf ("ERROR: expected aspl.es to be reported as local domain...\n");
		return axl_false;
	} /* end if */

	/* now check if these values are detected as local domains */
	if (valvulad_run_is_local_domain (ctx, "aspl2.es")) {
		printf ("ERROR: expected aspl2.es NOT to be reported as local domain...\n");
		return axl_false;
	} /* end if */

	/* now check if these values are detected as local domains */
	if (valvulad_run_is_local_domain (ctx, "asplhosting2.es")) {
		printf ("ERROR: expected aspl2.es NOT to be reported as local domain...\n");
		return axl_false;
	} /* end if */

	/* now check if these values are detected as local domains */
	if (valvulad_run_is_local_domain (ctx, "'; SELECT 1 --")) {
		printf ("ERROR: expected aspl2.es NOT to be reported as local domain...\n");
		return axl_false;
	} /* end if */

	/* now check if these values are detected as local domains */
	if (valvulad_run_is_local_domain (ctx, "''''")) {
		printf ("ERROR: expected aspl2.es NOT to be reported as local domain...\n");
		return axl_false;
	} /* end if */

	printf ("Test --: checking local domain detection..\n");
	
	/* now check domains that are declared at the configuration
	 * file but not found in the database. */
	if (! valvulad_run_is_local_domain (ctx, "mysql.aspl.es")) {
		printf ("ERROR: expected mysql.aspl.es NOT to be reported as local domain...\n");
		return axl_false;
	} /* end if */

	if (! valvulad_run_is_local_domain (ctx, "aspl-test.com")) {
		printf ("ERROR: expected mysql.aspl.es NOT to be reported as local domain...\n");
		return axl_false;
	} /* end if */

	if (! valvulad_run_is_local_domain (ctx, "aspl-1234.com")) {
		printf ("ERROR: expected mysql.aspl.es NOT to be reported as local domain...\n");
		return axl_false;
	} /* end if */

	if (valvulad_run_is_local_domain (ctx, "aspl-test-12345.com")) {
		printf ("ERROR: expected aspl-test-12345.com NOT to be reported as local domain...\n");
		return axl_false;
	} /* end if */

	printf ("Test --: checking local delivery detection..\n");

	/* create request */
	request = axl_new (ValvulaRequest, 1);
	request->recipient = "francis@aspl.es";
	
	if (! valvulad_run_is_local_delivery (ctx, request)) {
		printf ("ERROR: expected to find local delivery indication, but found it wasn't!\n");
		return axl_false;
	}

	request->recipient = "francis@google.com";
	if (valvulad_run_is_local_delivery (ctx, request)) {
		printf ("ERROR: expected to NOT find local delivery indication, but found it wasn't!\n");
		return axl_false;
	}

	/* release memory */
	axl_free (request);

	/* finish library */
	common_finish (ctx);


	return axl_true;
}

axl_bool  test_02c (void)
{
	ValvuladCtx    * ctx = axl_new (ValvuladCtx, 1);

	/* init the library */
	if (! valvulad_init_aux (ctx)) {
		printf ("ERROR: failed to initialize Valvulad context..\n");
		return axl_false;
	} /* end if */

	/* load configuration */
	test_valvula_load_config_aux ("Test 02-b", "test_02b.conf", axl_true, ctx, "test_02b.postfix.cf");

	/* create some tables */
	if (! valvulad_db_ensure_table (ctx, "mailbox", "username", "text", "active", "int", "domain", "text", NULL)) {
		printf ("ERROR: unable to create local domain table..\n");
		return axl_false;
	} /* end if */

	/* create some tables */
	if (! valvulad_db_ensure_table (ctx, "alias", "goto", "text", "active", "int", "domain", "text", "address", "text", NULL)) {
		printf ("ERROR: unable to create local domain table..\n");
		return axl_false;
	} /* end if */

	/* insert some values */
	valvulad_db_run_non_query (ctx, "DELETE FROM mailbox");
	valvulad_db_run_non_query (ctx, "INSERT INTO mailbox (username, active, domain) VALUES ('francis@aspl.es', '1', 'aspl.es')");
	valvulad_db_run_non_query (ctx, "INSERT INTO mailbox (username, active, domain) VALUES ('test@limited.com', '1', 'limited.com')");

	/* now check if these values are detected as local domains */
	if (! valvulad_run_is_local_address (ctx, "francis@aspl.es")) {
		printf ("ERROR: expected francis@aspl.es to be reported as local address...\n");
		return axl_false;
	} /* end if */

	/* now check if these values are detected as local domains */
	if (! valvulad_run_is_local_address (ctx, "test@limited.com")) {
		printf ("ERROR: expected test@limited.com to be reported as local address...\n");
		return axl_false;
	} /* end if */

	/* now check if these values are detected as local domains */
	if (valvulad_run_is_local_address (ctx, "test@asplhosting2.es")) {
		printf ("ERROR: expected test@asplhosting2.es NOT to be reported as local address...\n");
		return axl_false;
	} /* end if */

	/* now check if these values are detected as local domains */
	if (valvulad_run_is_local_domain (ctx, "'; SELECT 1 --")) {
		printf ("ERROR: expected aspl2.es NOT to be reported as local domain...\n");
		return axl_false;
	} /* end if */

	printf ("Test --: now test alias support\n");
	valvulad_db_run_non_query (ctx, "DELETE FROM alias");
	valvulad_db_run_non_query (ctx, "INSERT INTO alias (address, goto, active, domain) VALUES ('test@limited.com', 'aspl@asplhosting.com', '1', 'limited.com')");

	/* now check if these values are detected as local domains */
	if (valvulad_run_is_local_address (ctx, "aspl@asplhosting.com")) {
		printf ("ERROR: expected aspl@asplhosting.com to be reported as local address...\n");
		return axl_false;
	} /* end if */

	/* finish library */
	common_finish (ctx);


	return axl_true;
}

axl_bool  test_02d (void)
{
	ValvuladCtx    * ctx = axl_new (ValvuladCtx, 1);

	/* init the library */
	if (! valvulad_init_aux (ctx)) {
		printf ("ERROR: failed to initialize Valvulad context..\n");
		return axl_false;
	} /* end if */

	/* load configuration */
	test_valvula_load_config_aux ("Test 02-d", "test_02b.conf", axl_true, ctx, "test_02d.postfix.cf");

	/* create some tables */
	if (! valvulad_db_ensure_table (ctx, "mailbox", "username", "text", "active", "int", "domain", "text", NULL)) {
		printf ("ERROR (1.1): unable to create local domain table..\n");
		return axl_false;
	} /* end if */

	/* create some tables */
	if (! valvulad_db_ensure_table (ctx, "alias", "goto", "text", "active", "int", "domain", "text", "address", "text", NULL)) {
		printf ("ERROR (1.2): unable to create local domain table..\n");
		return axl_false;
	} /* end if */

	/* insert some values */
	valvulad_db_run_non_query (ctx, "DELETE FROM mailbox");
	valvulad_db_run_non_query (ctx, "INSERT INTO mailbox (username, active, domain) VALUES ('francis@aspl.es', '1', 'aspl.es')");
	valvulad_db_run_non_query (ctx, "INSERT INTO mailbox (username, active, domain) VALUES ('test@limited.com', '1', 'limited.com')");

	/* now check if these values are detected as local domains */
	if (! valvulad_run_is_local_address (ctx, "francis@aspl.es")) {
		printf ("ERROR (1.3): expected francis@aspl.es to be reported as local address...\n");
		return axl_false;
	} /* end if */

	/* now check if these values are detected as local domains */
	if (! valvulad_run_is_local_address (ctx, "test@limited.com")) {
		printf ("ERROR (1.4): expected test@limited.com to be reported as local address...\n");
		return axl_false;
	} /* end if */

	/* now check if these values are detected as local domains */
	if (valvulad_run_is_local_address (ctx, "test@asplhosting2.es")) {
		printf ("ERROR (1.5): expected test@asplhosting2.es NOT to be reported as local address...\n");
		return axl_false;
	} /* end if */

	/* now check if these values are detected as local domains */
	if (valvulad_run_is_local_domain (ctx, "'; SELECT 1 --")) {
		printf ("ERROR (1.6): expected aspl2.es NOT to be reported as local domain...\n");
		return axl_false;
	} /* end if */

	printf ("Test --: now test alias support\n");
	valvulad_db_run_non_query (ctx, "DELETE FROM alias");
	valvulad_db_run_non_query (ctx, "INSERT INTO alias (address, goto, active, domain) VALUES ('test@limited.com', 'aspl@asplhosting.com', '1', 'limited.com')");
	valvulad_db_run_non_query (ctx, "INSERT INTO alias (address, goto, active, domain) VALUES ('test2@limited.com', 'acinom', '1', 'limited.com')");

	/* now check if these values are detected as local domains */
	if (valvulad_run_is_local_address (ctx, "aspl@asplhosting.com")) {
		printf ("ERROR (1.7): expected aspl@asplhosting.com to be reported as local address...\n");
		return axl_false;
	} /* end if */

	/* now check if these values are detected as local domains */
	if (! valvulad_run_is_local_address (ctx, "test2@limited.com")) {
		printf ("ERROR (1.7): expected aspl@asplhosting.com to be reported as local address...\n");
		return axl_false;
	} /* end if */

	/* finish library */
	common_finish (ctx);


	return axl_true;
}

#define test_02e_account "prvs=6199d1e9f=kd5fet.gfj3t6@dkg3t5.com"

void __test_02e_final_state (ValvulaCtx        * ctx,
			     ValvulaConnection * connection, 
			     ValvulaRequest    * request, 
			     ValvulaState        state, 
			     const char        * message,
			     axlPointer          user_data)
{
	
	printf ("Test 02-e: sender=%s\n", request->sender);
	printf ("Test 02-e: recipient=%s\n", request->recipient);
	
	if (! axl_cmp (request->sender, test_02e_account)) {
		printf ("ERROR: expected to receive parsed account %s but found %s\n", test_02e_account, request->sender);
		exit (-1);
	}

	if (! axl_cmp (request->recipient, "francis@aspl.es")) {
		printf ("ERROR: received unexpected value (..1..)\n");
		exit (-1);
	}

	if (! axl_cmp (request->queue_id, "935jfe534")) {
		printf ("ERROR: received unexpected value (..2..), value received: %s\n", request->queue_id);
		exit (-1);
	}

	if (request->size != 235) {
		printf ("ERROR: received unexpected value (..3..): %d\n", request->size);
		exit (-1);
	}

	if (! axl_cmp (request->protocol_state, "RCPT")) {
		printf ("ERROR: received unexpected value (..4..)..\n");
		exit (-1);
	}

	if (! axl_cmp (request->protocol_name, "SMTP")) {
		printf ("ERROR: received unexpected value (..5..)..\n");
		exit (-1);
	}

	if (! axl_cmp (request->sasl_method, "plain")) {
		printf ("ERROR: received unexpected value (..6..)..\n");
		exit (-1);
	}

	if (! axl_cmp (request->sasl_username, "francis@aspl.es")) {
		printf ("ERROR: received unexpected value (..7..)..\n");
		exit (-1);
	}

	if (! axl_cmp (request->client_address, "127.0.0.1")) {
		printf ("ERROR: received unexpected value (..8..)..\n");
		exit (-1);
	}



	return;
}

axl_bool  test_02e (void)
{
	ValvuladCtx   * ctx;
	const char    * path;
	ValvulaState    state;

	/* load basic configuration */
	path = "test_01.conf";
	ctx  = test_valvula_load_config ("Test 02-e: ", path, axl_true);
	if (! ctx) {
		printf ("ERROR: unable to load configuration file at %s\n", path);
		return axl_false;
	} /* end if */

	/* configure final report function */
	ctx->ctx->report_final_state           = __test_02e_final_state;

	/* do a request */
	state = test_valvula_request (/* policy server location */
				      "127.0.0.1", "3579", 
				      /* state */
				      "smtpd_access_policy", "RCPT", "SMTP",
				      /* sender, recipient, recipient count */
				      "prvs=6199d1e9f=kd5fet.gfj3t6@dkg3t5.com", "francis@aspl.es", "1",
				      /* queue-id, size */
				      "935jfe534", "235",
				      /* sasl method, sasl username, sasl sender */
				      "plain", "francis@aspl.es", NULL);
	if (state != VALVULA_STATE_DUNNO) {
		printf ("ERROR (01-a.1): expected valvula state %d but found %d\n", VALVULA_STATE_DUNNO, state);
		return axl_false;
	}

	/* free valvula server context */
	printf ("Test 02-e: finishing configuration..\n");
	common_finish (ctx);

	return axl_true;
}


axl_bool test_sending_limit_and_final_reject (const char * auth_user, int allowed_sending_item, axl_bool check_final_error) {
	int            iterator;
	ValvulaState   state;

	/* testing day limited */
	iterator = 0;
	while (iterator < allowed_sending_item) {
		/* SHOULD WORK: now try to run some requests. The
		 * following should work  */
		state = test_valvula_request (/* policy server location */
			"127.0.0.1", "3579", 
			/* state */
			"smtpd_access_policy", "RCPT", "SMTP",
			/* sender, recipient, recipient count */
			"francis@aspl.es", "francis@aspl.es", "1",
			/* queue-id, size */
			"935jfe534", "235",
			/* sasl method, sasl username, sasl sender */
			"plain", auth_user, NULL);

		if (state != VALVULA_STATE_DUNNO) {
			printf ("ERROR (03.1): expected valvula state %d but found %d\n", VALVULA_STATE_DUNNO, state);
			return axl_false;
		} /* end if */

		/* next iterator */
		iterator++;
	}

	if (! check_final_error)
		return axl_true;

	printf ("Test --: now test if account %s is limited to %d..\n", auth_user, allowed_sending_item);
	/* SHOULD WORK: now try to run some requests. The following
	 * should work  */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"francis@aspl.es", "francis@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", auth_user, NULL);
	
	if (state != VALVULA_STATE_REJECT) {
		/* check state to better configure the state */
		if (state == VALVULA_STATE_DUNNO) 
			printf ("ERROR (03.2): expected valvula state %d but found %d (should have received REJECT but received DUNNO)\n", VALVULA_STATE_REJECT, state);
		else
			printf ("ERROR (03.2): expected valvula state %d but found %d\n", VALVULA_STATE_REJECT, state);
		return axl_false;
	} /* end if */

	return axl_true;
}


/* test mod ticket */
axl_bool test_03 (void) {
	ValvuladCtx   * ctx;
	const char    * path;
	const char    * query;
	ValvulaState    state;
	long            record_id;
	

	/* load basic configuration */
	path = "test_03.conf";
	ctx  = test_valvula_load_config ("Test 03: ", path, axl_true);
	if (! ctx) {
		printf ("ERROR (1): unable to load configuration file at %s\n", path);
		return axl_false;
	} /* end if */

	printf ("Test 03: phase 1\n");
	printf ("Test --:\n");

	/* check if the module was loaded */
	if (axl_list_length (ctx->registered_modules) == 0) {
		printf ("ERROR (2): expected to find 1 module loaded but found only %d..\n", axl_list_length (ctx->registered_modules));
		return axl_false;
	} /* end if */

	/* prepare all information at the mod ticket module */
	if (! valvulad_db_run_non_query (ctx, "DELETE FROM ticket_plan")) {
		printf ("ERROR (3): failed to remove old plans..\n");
		return axl_false;
	} /* end if */

	/* delete domain tickets */
	if (! valvulad_db_run_non_query (ctx, "DELETE FROM domain_ticket")) {
		printf ("ERROR (4): failed to remove old plans..\n");
		return axl_false;
	} /* end if */

	/* notify day change before continue to avoid confusing it */
	valvulad_notify_date_change (ctx, valvula_get_day (), VALVULAD_DATE_ITEM_DAY);

	/* now add some ticke plans */
	query = "INSERT INTO ticket_plan (name, description, total_limit, day_limit, month_limit) VALUES ('test 03', 'description', 20, 4, 10)";
	if (! valvulad_db_run_non_query (ctx, query)) {
		printf ("ERROR (5): failed to remove old plans..\n");
		return axl_false;
	} /* end if */

	printf ("Test 03: test limited sasl user ..\n");
	/* add the user that is going to be limited */
	query = "INSERT INTO domain_ticket (sasl_user, valid_until, ticket_plan_id) VALUES ('test@limited.com', %d, (SELECT max(id) FROM ticket_plan))";
	if (! valvulad_db_run_non_query (ctx, query, valvula_now () + 1000)) {
		printf ("ERROR (6): failed to remove old plans..\n");
		return axl_false;
	} /* end if */

	/* SHOULD WORK: now try to run some requests. The following
	 * should work by allowing unlimited users to pass through the
	 * module */
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
		printf ("ERROR (7): expected valvula state %d but found %d\n", VALVULA_STATE_DUNNO, state);
		return axl_false;
	} /* end if */

	printf ("Test 03: phase 2\n");
	printf ("Test --:\n");


	printf ("Test 03: testing allowed tickets (4)..\n");
	if (! test_sending_limit_and_final_reject ("test@limited.com", 4, axl_true))
		return axl_false;

	printf ("Test 03: perfect, now notify day change and see we can keep on sending (1)...\n");
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_DAY);

	printf ("Test 03: testing again allowed tickets (4)..\n");
	if (! test_sending_limit_and_final_reject ("test@limited.com", 4, axl_true))
		return axl_false;

	printf ("Test 03: perfect, now notify day change to test last 2 rounds for month limit (2)...\n");
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_DAY);

	printf ("Test 03: now try to reach month limit (3)...");
	if (! test_sending_limit_and_final_reject ("test@limited.com", 2, axl_true))
		return axl_false;

	printf ("Test 03: phase 3\n");
	printf ("Test --:\n");

	/* now check database here */
	printf ("Test 03: perfect, now notify month change to test rest of rounds (3)...\n");
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_MONTH);
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_DAY);

	printf ("Test 03: test again (4)..\n");
	if (! test_sending_limit_and_final_reject ("test@limited.com", 4, axl_true))
		return axl_false;

	printf ("Test 03: phase 4\n");
	printf ("Test --:\n");

	/* try to reach total limit */
	printf ("Test 03: sending 4 more messages (change day)..\n");
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_DAY);
	if (! test_sending_limit_and_final_reject ("test@limited.com", 4, axl_true))
		return axl_false;

	printf ("Test 03: sending 2 more messages (change day)..\n");
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_DAY);
	if (! test_sending_limit_and_final_reject ("test@limited.com", 2, axl_true))
		return axl_false;

	printf ("Test 03: now testing if total limits are honoured even after updating days or months (4)\n");
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_DAY);
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_MONTH);

	printf ("Test 03: phase 5\n");
	printf ("Test --:\n");

	/* SHOULD NOT WORK:  */
	printf ("Test --: Sending last test request..\n");
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"francis@aspl.es", "francis@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "test@limited.com", NULL);

	if (state != VALVULA_STATE_REJECT) {
		printf ("ERROR (8): expected valvula state %d but found %d\n", VALVULA_STATE_REJECT, state);
		return axl_false;
	} /* end if */

	printf ("Test 03: TEST MULTI USER on tickets..\n");

	printf ("Test 03: test limited sasl user (with multiple users in the same ticket) ..\n");
	/* add the user that is going to be limited */
	query = "INSERT INTO domain_ticket (sasl_user, valid_until, ticket_plan_id) VALUES ('test@limited2.com, test@limited3.com, test@limited4.com', %d, (SELECT max(id) FROM ticket_plan))";
	if (! valvulad_db_run_non_query (ctx, query, valvula_now () + 1000)) {
		printf ("ERROR (9): failed to remove old plans..\n");
		return axl_false;
	} /* end if */

	/* build query string */
	record_id = valvulad_db_run_query_as_long (ctx, "select id from domain_ticket where sasl_user like '#%s#'", "limited2.com");

	printf ("Test 03: phase 6 (record %ld)\n", record_id);
	printf ("Test --:\n");

	if (record_id <= 0) {
		printf ("ERROR (10): failed to get domain ticket created for multi user, expected a value > 0 but found %ld\n", record_id);
		return axl_false;
	} /* end if */

	printf ("Test 03: now testing test@limited2.com..\n");
	if (! test_sending_limit_and_final_reject ("test@limited2.com", 4, axl_true))
		return axl_false;

	printf ("Test 03: perfect, now notify day change and see we can keep on sending (1)...\n");
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_DAY);

	printf ("Test 03: now testing test@limited3.com..\n");
	if (! test_sending_limit_and_final_reject ("test@limited3.com", 4, axl_true))
		return axl_false;

	printf ("Test 03: phase 7\n");
	printf ("Test --:\n");

	printf ("Test 03: perfect, now notify day change to test last 2 rounds for month limit (2)...\n");
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_DAY);

	printf ("Test 03: now testing test@limited4.com..\n");
	if (! test_sending_limit_and_final_reject ("test@limited4.com", 2, axl_true))
		return axl_false;

	printf ("Test 03: now testing if total limits are honoured even after updating days or months (4)\n");
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_DAY);
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_MONTH);

	printf ("Test 03: phase 8\n");
	printf ("Test --:\n");

	printf ("Test 03: now testing test@limited2.com..\n");
	if (! test_sending_limit_and_final_reject ("test@limited2.com", 4, axl_true))
		return axl_false;

	printf ("Test 03: perfect, now notify day change and see we can keep on sending (1)...\n");
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_DAY);

	printf ("Test 03: now testing test@limited3.com..\n");
	if (! test_sending_limit_and_final_reject ("test@limited3.com", 4, axl_true))
		return axl_false;

	printf ("Test 03: phase 7\n");
	printf ("Test --:\n");

	printf ("Test 03: perfect, now notify day change to test last 2 rounds for month limit (2)...\n");
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_DAY);

	printf ("Test 03: now testing test@limited4.com..\n");
	if (! test_sending_limit_and_final_reject ("test@limited4.com", 2, axl_true))
		return axl_false;

	printf ("Test 03: now testing if total limits are honoured even after updating days or months (4)\n");
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_DAY);
	valvulad_notify_date_change (ctx, valvula_get_day () + 1, VALVULAD_DATE_ITEM_MONTH);

	printf ("Test 03: nice, valvula seems doing as expected..\n");

	/* SHOULD NOT WORK:  */
	printf ("Test --: Sending last test request..\n");
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"francis@aspl.es", "francis@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "test@limited3.com", NULL);

	if (state != VALVULA_STATE_REJECT) {
		printf ("ERROR (11): expected valvula state %d but found %d\n", VALVULA_STATE_REJECT, state);
		return axl_false;
	} /* end if */

	/* SHOULD NOT WORK:  */
	printf ("Test --: Testing (block_ticket) flag for test@limited.com..\n");
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"francis@aspl.es", "francis@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "test@limited.com", NULL);

	if (state != VALVULA_STATE_REJECT) {
		printf ("ERROR (2.17.27): expected valvula state %d but found %d\n", VALVULA_STATE_REJECT, state);
		return axl_false;
	} /* end if */

	/* add credits */
	if (! valvulad_db_run_non_query (ctx, "UPDATE domain_ticket SET total_used = 0 WHERE sasl_user = 'test@limited.com'")) {
		printf ("ERROR (12): expected to insert value with valvulad_db_run_non_query but found a failure..\n");
		return axl_false;
	} /* end if */

	/* SHOULD WORK */
	printf ("Test --: testing (block_ticket), ensure account works..\n");
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"francis@aspl.es", "francis@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "test@limited.com", NULL);

	if (state != VALVULA_STATE_DUNNO) {
		printf ("ERROR (2.17.27.13): expected valvula state %d but found %d\n", VALVULA_STATE_DUNNO, state);
		return axl_false;
	} /* end if */

	/* block account */
	if (! valvulad_db_run_non_query (ctx, "UPDATE domain_ticket SET block_ticket = '1' WHERE sasl_user = 'test@limited.com'")) {
		printf ("ERROR (14): expected to insert value with valvulad_db_run_non_query but found a failure..\n");
		return axl_false;
	} /* end if */

	/* SHOULD NOT WORK:  */
	printf ("Test --: Testing (block_ticket) flag for test@limited.com..\n");
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"francis@aspl.es", "francis@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "test@limited.com", NULL);

	if (state != VALVULA_STATE_REJECT) {
		printf ("ERROR (2.17.27.15): expected valvula state %d but found %d\n", VALVULA_STATE_REJECT, state);
		return axl_false;
	} /* end if */

	/* finish test */
	common_finish (ctx);

	return axl_true;
}

axl_bool test_03a (void) {

	ValvuladCtx   * ctx;
	const char    * path;
	ValvulaState    state;
	

	/* load basic configuration */
	path = "test_03.conf";
	ctx  = test_valvula_load_config ("Test 03a: ", path, axl_true);
	if (! ctx) {
		printf ("ERROR (1): unable to load configuration file at %s\n", path);
		return axl_false;
	} /* end if */

	printf ("Test 03a: phase 1\n");
	printf ("Test ---:\n");


	printf ("Test 03a: Check filter support (outgoing support for especific transports)..\n");
	valvulad_db_run_non_query (ctx, "DELETE FROM outgoing_ip");
	valvulad_db_run_non_query (ctx, "INSERT INTO outgoing_ip (id, is_active, outgoing_ip, transport, label) VALUES ('1', '1', '129.23.3.23', 'transport11', 'label for transport')");
	/* block account */
	valvulad_db_run_non_query (ctx, "UPDATE domain_ticket SET block_ticket = '0', total_used = '0', current_day_usage = '0', current_month_usage = '0', has_outgoing_ip = '1', outgoing_ip_id = '1', valid_until = '%d' WHERE sasl_user = 'test@limited.com'", valvula_now () + 1000);
	

	test_valvula_content = "transport11";
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"francis@aspl.es", "francis@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "test@limited.com", NULL);

	if (state != VALVULA_STATE_FILTER) {
		printf ("ERROR (3a.17.27.16): expected valvula state %d but found %d\n", VALVULA_STATE_FILTER, state);
		return axl_false;
	} /* end if */


	/* check that filter report is the following value */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"francis@aspl.es", "francis@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "test2@limited.com", NULL);

	if (state == VALVULA_STATE_FILTER) {
		printf ("ERROR (3a.17.27.17): expected valvula state %d but found %d\n", VALVULA_STATE_FILTER, state);
		return axl_false;
	} /* end if */

	/* finish test */
	common_finish (ctx);
	
	return axl_true;
}

/* test mod ticket */
axl_bool test_04 (void) {

	int result;

	printf ("Test 04: running valvulad-mgr.py regression tests..\n");
	result = system ("./test_01.py");
	if (result ) {
		printf ("ERROR: regression test failed\n");
		return axl_false;
	}

	return axl_true;
}

/* test mod bwl */
axl_bool test_05 (void) {

	ValvuladCtx   * ctx;
	const char    * path;
	ValvulaState    state;
	

	/* load basic configuration */
	path = "test_05.conf";
	ctx  = test_valvula_load_config ("Test 05: ", path, axl_true);
	if (! ctx) {
		printf ("ERROR (1): unable to load configuration file at %s\n", path);
		return axl_false;
	} /* end if */

	printf ("Test 05: phase 1\n");
	printf ("Test --:\n");

	/** delete current rules **/
	if (! valvulad_db_run_non_query (ctx, "DELETE FROM bwl_global")) {
		printf ("ERROR: unable to remove all global rules..\n");
		return axl_false;
	} /* end if */

	/** delete current rules **/
	if (! valvulad_db_run_non_query (ctx, "DELETE FROM bwl_domain")) {
		printf ("ERROR: unable to remove all global rules..\n");
		return axl_false;
	} /* end if */

	/** delete current rules **/
	if (! valvulad_db_run_non_query (ctx, "DELETE FROM bwl_global_sasl")) {
		printf ("ERROR: unable to remove all global rules..\n");
		return axl_false;
	} /* end if */

	/* SHOULD WORK: now try to run some requests. The following
	 * should work by allowing unlimited users to pass through the
	 * module */
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
		printf ("ERROR (4.1): expected valvula state %d but found %d\n", VALVULA_STATE_DUNNO, state);
		return axl_false;
	}

	printf ("Test 05: testing * -> francis@aspl.es (reject)\n");
	/** 
	 * Test * -> francis@aspl.es : rejected 
	 */
	/* insert content */
	if (! valvulad_db_run_non_query (ctx, "INSERT INTO bwl_global (is_active, destination, status) VALUES ('1', 'francis@aspl.es', 'reject')")) {
		printf ("ERROR: expected to insert value with valvulad_db_run_non_query but found a failure..\n");
		return axl_false;
	} /* end if */

	/* SHOULD WORK: now try to run some requests. The following
	 * should work by allowing unlimited users to pass through the
	 * module */
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

	if (state == VALVULA_STATE_DUNNO) {
		printf ("ERROR (4.2): expected valvula state %d but found %d\n", VALVULA_STATE_DUNNO, state);
		return axl_false;
	}

	printf ("Test 05: testing test@test.com -> francis@aspl.es (ok)\n");

	/** 
	 * Test test@test.com -> francis@aspl.es : ok 
	 */
	/* now insert a rule to allow especific deliveries even when
	 * we have a more generic rule (the one we inserted before) */
	if (! valvulad_db_run_non_query (ctx, "INSERT INTO bwl_global (is_active, source, destination, status) VALUES ('1', 'test@test.com', 'francis@aspl.es', 'ok')")) {
		printf ("ERROR: expected to insert value with valvulad_db_run_non_query but found a failure..\n");
		return axl_false;
	} /* end if */
	
	/* SHOULD NOT WORK because aspl.es is not local: now try to
	 * run some requests. The following should work by allowing
	 * unlimited users to pass through the module */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"test@test.com", "francis@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "francis@aspl.es", NULL);

	if (state != VALVULA_STATE_REJECT) {
		printf ("ERROR (4.3): expected valvula state %d but found %d\n", VALVULA_STATE_REJECT, state);
		return axl_false;
	}

	/* register aspl.es as local */
	valvulad_run_add_local_domain (ctx, "aspl.es");

	printf ("Test --: added aspl.es as local domain, checking again..\n");

	/* SHOULD WORK because aspl.es IS local: now try to run some
	 * requests. The following should work by allowing unlimited
	 * users to pass through the module */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"test@test.com", "francis@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "francis@aspl.es", NULL);

	if (state != VALVULA_STATE_OK) {
		printf ("ERROR (4.3.1): expected valvula state %d but found %d\n", VALVULA_STATE_OK, state);
		return axl_false;
	}

	/** 
	 * Test test.com -> * : reject
	 */
	printf ("Test 05: testing test.com -> * (reject)\n");

	/* now insert a rule to allow especific deliveries even when
	 * we have a more generic rule (the one we inserted before) */
	if (! valvulad_db_run_non_query (ctx, "INSERT INTO bwl_global (is_active, source, status) VALUES ('1', 'test.com', 'reject')")) {
		printf ("ERROR: expected to insert value with valvulad_db_run_non_query but found a failure..\n");
		return axl_false;
	} /* end if */
	
	/* SHOULD WORK: now try to run some requests. The following
	 * should work by allowing unlimited users to pass through the
	 * module */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"test@test.com", "francis2@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "francis@aspl.es", NULL);

	if (state != VALVULA_STATE_REJECT) {
		printf ("ERROR (4.4): expected valvula state %d but found %d\n", VALVULA_STATE_REJECT, state);
		return axl_false;
	}

	printf ("Test 05: testing test.com -> * (reject) do not limit test@test.com -> francis@aspl.es (previous accepted)\n");

	/* SHOULD WORK: now try to run some requests. The following
	 * should work by allowing unlimited users to pass through the
	 * module */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"test@test.com", "francis@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "francis@aspl.es", NULL);

	if (state != VALVULA_STATE_OK) {
		printf ("ERROR (4.5): expected valvula state %d but found %d\n", VALVULA_STATE_OK, state);
		return axl_false;
	}

	printf ("Test 05: test domain level rules\n");

	/* now insert a rule to allow especific deliveries even when
	 * we have a more generic rule (the one we inserted before) */
	if (! valvulad_db_run_non_query (ctx, "INSERT INTO bwl_domain (is_active, rules_for, source, status) VALUES ('1', 'aspl.es', 'test2.com', 'reject')")) {
		printf ("ERROR: expected to insert value with valvulad_db_run_non_query but found a failure..\n");
		return axl_false;
	} /* end if */

	printf ("Test --: test2.com -> aspl.es (reject)\n");
	
	/* SHOULD NOT WORK: now try to run some requests. The
	 * following should work by allowing unlimited users to pass
	 * through the module */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"test@test2.com", "alise@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		NULL, NULL, NULL);

	if (state != VALVULA_STATE_REJECT) {
		printf ("ERROR (4.6): expected valvula state %d but found %d\n", VALVULA_STATE_REJECT, state);
		return axl_false;
	} /* end if */

	printf ("Test 05: test accoutn level rules\n");

	printf ("Test --: test@test3.com -> rmandro@aspl.es (reject)\n");

	/* now insert a rule to allow especific deliveries even when
	 * we have a more generic rule (the one we inserted before) */
	if (! valvulad_db_run_non_query (ctx, "INSERT INTO bwl_account (is_active, rules_for, source, status) VALUES ('1', 'rmandro@aspl.es', 'test@test3.com', 'reject')")) {
		printf ("ERROR: expected to insert value with valvulad_db_run_non_query but found a failure..\n");
		return axl_false;
	} /* end if */
	
	/* SHOULD NOT WORK: now try to run some requests. The
	 * following should work by allowing unlimited users to pass
	 * through the module */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"test@test3.com", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		NULL, NULL, NULL);

	if (state != VALVULA_STATE_REJECT) {
		printf ("ERROR (4.7): expected valvula state %d but found %d\n", VALVULA_STATE_REJECT, state);
		return axl_false;
	} /* end if */

	printf ("Test --: checking test4.com -> rmandro@aspl.es (reject) (account level)\n");

	/* now insert a rule to allow especific deliveries even when
	 * we have a more generic rule (the one we inserted before) */
	if (! valvulad_db_run_non_query (ctx, "INSERT INTO bwl_account (is_active, rules_for, source, status) VALUES ('1', 'rmandro@aspl.es', 'test4.com', 'reject')")) {
		printf ("ERROR: expected to insert value with valvulad_db_run_non_query but found a failure..\n");
		return axl_false;
	} /* end if */
	
	/* SHOULD NOT WORK: now try to run some requests. The
	 * following should work by allowing unlimited users to pass
	 * through the module */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"anything@test4.com", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		NULL, NULL, NULL);

	if (state != VALVULA_STATE_REJECT) {
		printf ("ERROR (4.8): expected valvula state %d but found %d\n", VALVULA_STATE_REJECT, state);
		return axl_false;
	} /* end if */

	printf ("Test --: checking bwl sasl restrictions..\n");

	/* SHOULD WORK: now try to run some requests. The following
	 * should work by allowing unlimited users to pass through the
	 * module */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"test@test.com", "francis@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "francis@aspl.es", NULL);

	if (state != VALVULA_STATE_OK) {
		printf ("ERROR (4.9): expected valvula state %d but found %d\n", VALVULA_STATE_OK, state);
		return axl_false;
	}

	/* now insert restriction */
	valvulad_db_run_non_query (ctx, "INSERT INTO bwl_global_sasl (is_active, sasl_user) VALUES ('1', 'francis@aspl.es')");

	/* SHOULD WORK: now try to run some requests. The following
	 * should work by allowing unlimited users to pass through the
	 * module */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"test@test.com", "francis@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "francis@aspl.es", NULL);

	if (state != VALVULA_STATE_REJECT) {
		/* check state to better configure the state */
		if (state == VALVULA_STATE_DUNNO) 
			printf ("ERROR (4.11): expected valvula state %d but found %d (should have received REJECT but received DUNNO)\n", VALVULA_STATE_REJECT, state);
		else
			printf ("ERROR (4.11): expected valvula state %d but found %d\n", VALVULA_STATE_REJECT, state);
		return axl_false;
	} /* end if */

	valvulad_db_run_non_query (ctx, "DELETE FROM bwl_global_sasl");

	/* finish test */
	common_finish (ctx);

	return axl_true;
}

/* test mod slm */
axl_bool test_06 (void) {

	ValvuladCtx   * ctx = axl_new (ValvuladCtx, 1);
	ValvulaState    state;

	printf ("Test 06: checking mod-slm=full\n");

	/* init the library */
	if (! valvulad_init_aux (ctx)) {
		printf ("ERROR: failed to initialize Valvulad context..\n");
		return axl_false;
	} /* end if */

	/* load configuration */
	test_valvula_load_config_aux ("Test 06", "test_06.conf", axl_true, ctx, "test_02b.postfix.cf");

	/* ctx  = test_valvula_load_config ("Test 06: ", path, axl_true);  */
	if (! ctx) {
		printf ("ERROR (1): unable to load configuration file at test06.conf\n");
		return axl_false;
	} /* end if */

	/* SHOULD NOT WORK: now try to run some requests. The
	 * following should work by allowing unlimited users to pass
	 * through the module */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"anything@test4.com", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "francis@aspl.es", NULL);

	if (state != VALVULA_STATE_REJECT) {
		printf ("ERROR (1.1): expected failure for authenticated operation that do not match..\n");
		return axl_false;
	}

	/* SHOULD WORK: now try to run some requests. The following
	 * should work by allowing unlimited users to pass through the
	 * module */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"anything@test4.com", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "anything@test4.com", NULL);

	if (state != VALVULA_STATE_DUNNO) {
		printf ("ERROR (1.1): expected OK operation for authenticated operation that do match..\n");
		return axl_false;
	}

	/* SHOULD WORK: now try to run some requests. The following
	 * should work by allowing unlimited users to pass through the
	 * module */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"ANYTHING@TEST4.COM", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "anything@test4.com", NULL);

	if (state != VALVULA_STATE_DUNNO) {
		printf ("ERROR (1.1.1): expected OK operation for authenticated operation that do match (capitalized)..\n");
		return axl_false;
	}

	/* finish test */
	common_finish (ctx);

	printf ("Test 06: checking mod-slm=same-domain\n");

	/* init the library */
	ctx = axl_new (ValvuladCtx, 1);
	if (! valvulad_init_aux (ctx)) {
		printf ("ERROR: failed to initialize Valvulad context..\n");
		return axl_false;
	} /* end if */

	/* load basic configuration */
	/* load configuration */
	test_valvula_load_config_aux ("Test 06", "test_06.same-domain.conf", axl_true, ctx, "test_02b.postfix.cf");
	if (! ctx) {
		printf ("ERROR (1): unable to load configuration file at test_06.same-domain.conf\n");
		return axl_false;
	} /* end if */

	/* record account */
	valvulad_db_run_non_query (ctx, "DELETE FROM mailbox");
	valvulad_db_run_non_query (ctx, "INSERT INTO mailbox (username, active, domain) VALUES ('anything@test4.com', '1', 'test4.com')");

	/* SHOULD WORK */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"anything@test4.com", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "account@test4.com", NULL);

	if (state != VALVULA_STATE_DUNNO) {
		printf ("ERROR (1.3): expected OK operation for authenticated operation that do match (same-domain)..\n");
		return axl_false;
	}

	/* SHOULD WORK */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"anything2@test4.com", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "account2@test4.com", NULL);

	if (state != VALVULA_STATE_REJECT) {
		printf ("ERROR (1.4): expected REJECT operation for authenticated operation that do match (same-domain)..\n");
		return axl_false;
	}

	/* SHOULD WORK */
	printf ("Test 06: mod-slm testing accepting <> at the mail from (same-domain)..\n");
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "account2@test4.com", NULL);

	if (state != VALVULA_STATE_DUNNO) {
		printf ("ERROR (1.4.1): expected DUNNO operation for authenticated operation that do NOT match but exception is installed..\n");
		return axl_false;
	}

	/* finish test */
	common_finish (ctx);

	/* init the library */
	ctx = axl_new (ValvuladCtx, 1);
	if (! valvulad_init_aux (ctx)) {
		printf ("ERROR: failed to initialize Valvulad context..\n");
		return axl_false;
	} /* end if */

	/* load basic configuration */
	/* load configuration */
	printf ("Test 06: testing to block DSNs (mail from: <>)\n");
	test_valvula_load_config_aux ("Test 06", "test_06.same-domain.02.conf", axl_true, ctx, "test_02b.postfix.cf");
	if (! ctx) {
		printf ("ERROR (1): unable to load configuration file at test_06.same-domain.conf\n");
		return axl_false;
	} /* end if */

	/* SHOULD WORK */
	printf ("Test 06: mod-slm testing rejection of <> at the mail from (same-domain)..\n");
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "account2@test4.com", NULL);

	if (state != VALVULA_STATE_REJECT) {
		printf ("ERROR (1.4.2): expected REJECT operation but received: %d\n", state);
		return axl_false;
	}

	/* finish test */
	common_finish (ctx);

	printf ("Test 06: checking valid-from support\n");

	/* init the library */
	ctx = axl_new (ValvuladCtx, 1);
	if (! valvulad_init_aux (ctx)) {
		printf ("ERROR: failed to initialize Valvulad context..\n");
		return axl_false;
	} /* end if */

	/* load basic configuration */
	/* load configuration */
	test_valvula_load_config_aux ("Test 06", "test_06.valid-from.conf", axl_true, ctx, "test_02b.postfix.cf");
	if (! ctx) {
		printf ("ERROR (1): unable to load configuration file at test_06.valid-from.conf\n");
		return axl_false;
	} /* end if */

	/* SHOULD WORK */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"anything@test4.com", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "account@test4.com", NULL);

	if (state != VALVULA_STATE_DUNNO) {
		printf ("ERROR (1.3): expected OK operation for authenticated operation that do match (valid-from)..\n");
		return axl_false;
	}

	/* SHOULD WORK */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"anything2@test4.com", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "account2@test4.com", NULL);

	if (state != VALVULA_STATE_REJECT) {
		printf ("ERROR (1.4): expected REJECT operation for authenticated operation that do match (valid-from)..\n");
		return axl_false;
	}

	/* test exception */
	printf ("Test 06: checking exceptions..\n");
	valvulad_db_run_non_query (ctx, "DELETE FROM slm_exception");
	valvulad_db_run_non_query (ctx, "INSERT INTO slm_exception (mail_from, sasl_username, is_active) VALUES ('different13@test4.com', 'hyper3@test4.com', '1')");

	/* SHOULD WORK */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"different13@test4.com", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "hyper3@test4.com", NULL);

	if (state != VALVULA_STATE_DUNNO) {
		printf ("ERROR (1.5): expected DUNNO operation for authenticated operation that do NOT match but exception is installed..\n");
		return axl_false;
	}

	/* exception a sasl user */
	valvulad_db_run_non_query (ctx, "DELETE FROM slm_exception");
	valvulad_db_run_non_query (ctx, "INSERT INTO slm_exception (sasl_username, is_active) VALUES ('kb', '1')");

	/* SHOULD WORK */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"different13@test4.com", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "kb", NULL);

	if (state != VALVULA_STATE_DUNNO) {
		printf ("ERROR (1.6): expected DUNNO operation for authenticated operation that do NOT match but exception is installed..\n");
		return axl_false;
	}

	/* SHOULD WORK */
	printf ("Test 06: mod-slm testing accepting <> at the mail from..\n");
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"<>", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		NULL, NULL, NULL);

	if (state != VALVULA_STATE_DUNNO) {
		printf ("ERROR (1.7): expected DUNNO operation for authenticated operation that do NOT match but exception is installed..\n");
		return axl_false;
	}

	/* finish test */
	common_finish (ctx);

	return axl_true;	
}

typedef axl_bool (*Test07MinuteHandler)   (ValvulaCtx  * _ctx, 
					   axlPointer   user_data,
					   axlPointer   user_data2);

/* test mod mquota */
axl_bool test_07 (void) {

	Test07MinuteHandler    minute_handler;
	ValvuladCtx          * ctx;
	ValvulaState           state; 
	ValvuladModule       * module;
	int                    iterator;
	ModMquotaLimit       * limit;

	/* get reference */
	ctx = axl_new (ValvuladCtx, 1);

	printf ("Test 07: checking mod-mquota\n");

	/* init the library */
	if (! valvulad_init_aux (ctx)) {
		printf ("ERROR: failed to initialize Valvulad context..\n");
		return axl_false;
	} /* end if */

	/* load configuration */
	test_valvula_load_config_aux ("Test 07", "test_07.conf", axl_true, ctx, "test_02b.postfix.cf");

	/* ctx  = test_valvula_load_config ("Test 06: ", path, axl_true);  */
	if (! ctx) {
		printf ("ERROR (1): unable to load configuration file at test07.conf\n");
		return axl_false;
	} /* end if */

	/***** 08:00 -> night quota *****/
	printf ("Test 07: detect period right at 08:00 -> night quota..\n");
	limit = mod_mquota_get_current_period (0, 8);
	if (limit == NULL) {
		printf ("ERROR (1.1): unable to get current limits...NULL pointer was received..\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp (limit->label, "night quota")) {
		printf ("ERROR (1.2): expected to find night quota label but found: %s\n", limit->label);
		return axl_false;
	} /* end if */

	/****** 10:00 -> day quota *******/
	printf ("Test 07: detect period right at 10:00 -> day quota..\n");
	limit = mod_mquota_get_current_period (0, 10);
	if (limit == NULL) {
		printf ("ERROR (1.3): unable to get current limits...NULL pointer was received..\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp (limit->label, "day quota")) {
		printf ("ERROR (1.4): expected to find day quota label but found: %s\n", limit->label);
		return axl_false;
	} /* end if */

	/****** 20:49 -> day quota *******/
	printf ("Test 07: detect period right at 20:49 -> day quota..\n");
	limit = mod_mquota_get_current_period (49, 20);
	if (limit == NULL) {
		printf ("ERROR (1.3): unable to get current limits...NULL pointer was received..\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp (limit->label, "day quota")) {
		printf ("ERROR (1.4): expected to find day quota label but found: %s\n", limit->label);
		return axl_false;
	} /* end if */

	/****** 22:00 -> day quota *******/
	printf ("Test 07: detect period right at 22:00 -> night quota..\n");
	limit = mod_mquota_get_current_period (0, 22);
	if (limit == NULL) {
		printf ("ERROR (1.3): unable to get current limits...NULL pointer was received..\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp (limit->label, "night quota")) {
		printf ("ERROR (1.4): expected to find day quota label but found: %s\n", limit->label);
		return axl_false;
	} /* end if */

	/****** 23:59 -> day quota *******/
	printf ("Test 07: detect period right at 23:59 -> night quota..\n");
	limit = mod_mquota_get_current_period (59, 23);
	if (limit == NULL) {
		printf ("ERROR (1.3): unable to get current limits...NULL pointer was received..\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp (limit->label, "night quota")) {
		printf ("ERROR (1.4): expected to find day quota label but found: %s\n", limit->label);
		return axl_false;
	} /* end if */

	/****** 00:00 -> night quota *******/
	printf ("Test 07: detect period right at 00:00 -> night quota..\n");
	limit = mod_mquota_get_current_period (0, 0);
	if (limit == NULL) {
		printf ("ERROR (1.3): unable to get current limits...NULL pointer was received..\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp (limit->label, "night quota")) {
		printf ("ERROR (1.4): expected to find day quota label but found: %s\n", limit->label);
		return axl_false;
	} /* end if */

	/****** 00:01 -> night quota *******/
	printf ("Test 07: detect period right at 00:01 -> night quota..\n");
	limit = mod_mquota_get_current_period (1, 0);
	if (limit == NULL) {
		printf ("ERROR (1.3): unable to get current limits...NULL pointer was received..\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp (limit->label, "night quota")) {
		printf ("ERROR (1.4): expected to find day quota label but found: %s\n", limit->label);
		return axl_false;
	} /* end if */

	/****** 01:00 -> night quota *******/
	printf ("Test 07: detect period right at 01:00 -> night quota..\n");
	limit = mod_mquota_get_current_period (0, 1);
	if (limit == NULL) {
		printf ("ERROR (1.3): unable to get current limits...NULL pointer was received..\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp (limit->label, "night quota")) {
		printf ("ERROR (1.4): expected to find day quota label but found: %s\n", limit->label);
		return axl_false;
	} /* end if */

	printf ("Test 07: test basic minute limites..\n");

	/* call to send content */
	/* SHOULD WORK */
	state = test_valvula_request (/* policy server location */
		"127.0.0.1", "3579", 
		/* state */
		"smtpd_access_policy", "RCPT", "SMTP",
		/* sender, recipient, recipient count */
		"hyper3@test4.com", "rmandro@aspl.es", "1",
		/* queue-id, size */
		"935jfe534", "235",
		/* sasl method, sasl username, sasl sender */
		"plain", "test@limited.com", NULL);

	if (state != VALVULA_STATE_DUNNO) {
		printf ("ERROR (7.1): expected DUNNO operation for simple quota operation..\n");
		return axl_false;
	} /* end if */

	printf ("Test 07: now test to reach current limit..\n");
	if (! test_sending_limit_and_final_reject ("test@limited.com", 4, axl_true))
		return axl_false;

	/* perfect, limit reached, now "manually" change time */
	module         = valvulad_module_find_by_name (ctx, "mod-mquota");
	if (module == NULL) {
		printf ("Test 07: expected to find module (mod-mquota) reference to call internal functions..\n");
		return axl_false;
	}
	
	/* get minute handler */
	minute_handler = valvulad_module_get_symbol (ctx, module, "__mod_mquota_minute_handler");
	if (minute_handler == NULL) {
		printf ("Test 07: unable to find minute handler to call it to force minute change.. NULL reference returned..\n");
		return axl_false;
	} /* end if */

	printf ("Test 07: simulating minute handler change..\n");
	minute_handler (ctx->ctx, NULL, NULL);

	printf ("Test 07: doing again another send operation..\n");
	if (! test_sending_limit_and_final_reject ("test@limited.com", 5, axl_true))
		return axl_false;

	printf ("Test 07: simulating minute handler change (again)..\n");
	minute_handler (ctx->ctx, NULL, NULL);

	printf ("Test 07: try to hit hour limit...\n");
	if (! test_sending_limit_and_final_reject ("test@limited.com", 0, axl_true))
		return axl_false;

	/* try to change hour to reach global limit  */
	printf ("Test 07: changing hour..\n");
	iterator = 0;
	while (iterator < 60) {
		/* change minutes */
		minute_handler (ctx->ctx, NULL, NULL);

		/* next iterator */
		iterator++;
	}

	printf ("Test 07: reach again minute limit..\n");
	if (! test_sending_limit_and_final_reject ("test@limited.com", 5, axl_true))
		return axl_false;

	/* change minutes */
	minute_handler (ctx->ctx, NULL, NULL);


	printf ("Test 07: change hour...\n");	
	iterator = 0;
	while (iterator < 60) {
		/* change minutes */
		minute_handler (ctx->ctx, NULL, NULL);

		/* next iterator */
		iterator++;
	}

	printf ("Test 07: reach again minute limit (2)..\n");
	if (! test_sending_limit_and_final_reject ("test@limited.com", 5, axl_true))
		return axl_false;

	/** DOMAIN TEST ***/
	valvulad_db_run_non_query (ctx, "DELETE FROM mquota_exception");

	/*** TEST exceptions ***/
	valvulad_db_run_non_query (ctx, "DELETE FROM mquota_exception");

	printf ("Test 07: test limits BY DOMAIN (limited2.com)..\n");
	if (! test_sending_limit_and_final_reject ("test@limited2.com", 3, axl_false))
		return axl_false;

	printf ("Test 07: (1) sending another 3 (test1@limited2.com).....\n");
	if (! test_sending_limit_and_final_reject ("test1@limited2.com", 3, axl_false))
		return axl_false;

	printf ("Test 07: (2) sending another 2 (test2@limited2.com).....\n");
	if (! test_sending_limit_and_final_reject ("test2@limited2.com", 2, axl_false))
		return axl_false;

	printf ("Test 07: (3) sending another 2 (test4@limited2.com).....\n");
	if (! test_sending_limit_and_final_reject ("test4@limited2.com", 2, axl_true))
		return axl_false;

	/*** TEST exceptions ***/
	valvulad_db_run_non_query (ctx, "INSERT INTO mquota_exception (is_active, sasl_user) VALUES ('1', 'test4@limited2.com')");

	printf ("Test 07: check again sending with test4@limited2.com with exception..\n");
	/* check again */
	if (! test_sending_limit_and_final_reject ("test4@limited2.com", 1, axl_false))
		return axl_false;

	/* but it should fail for another domain */
	if (! test_sending_limit_and_final_reject ("test2@limited2.com", 0, axl_true))
		return axl_false;

	/* check again */
	if (! test_sending_limit_and_final_reject ("test4@limited2.com", 1, axl_false))
		return axl_false;
	
	/* finish test */
	common_finish (ctx);

	return axl_true;	
}

/* test mod mquota */
axl_bool test_08 (void) {

	ValvuladCtx          * ctx;
	extern axl_bool        __valvulad_simulate_connection_error;

	/* get reference */
	ctx = axl_new (ValvuladCtx, 1);

	printf ("Test 08: checking SQL failure..\n");

	/* init the library */
	if (! valvulad_init_aux (ctx)) {
		printf ("ERROR: failed to initialize Valvulad context..\n");
		return axl_false;
	} /* end if */

	/* load configuration */
	test_valvula_load_config_aux ("Test 07", "test_07.conf", axl_true, ctx, "test_02b.postfix.cf");

	/* ctx  = test_valvula_load_config ("Test 06: ", path, axl_true);  */
	if (! ctx) {
		printf ("ERROR (1): unable to load configuration file at test07.conf\n");
		return axl_false;
	} /* end if */

	/* simulate sql error */
	__valvulad_simulate_connection_error = axl_true;

	/** DOMAIN TEST ***/
	valvulad_db_run_non_query (ctx, "DELETE FROM mquota_exception");

	/* finish test */
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
	printf ("**     time ./test_01 [--help] [--debug] [--no-unmap] [--run-test=NAME]\n**\n");
	printf ("** To gather information about memory consumed (and leaks) use:\n**\n");
	printf ("**     >> libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes --error-limit=no ./test_01 [--debug]\n**\n");
	printf ("** Providing --run-test=NAME will run only the provided regression test.\n");
	printf ("** Available tests: test_00, test_01, test_02, test_02a, test_02b, test_02c, test_02d, test_02e, test_03, test_03a, test_04, test_05,\n");
	printf ("**                  test_06, test_07, test_08\n");
	printf ("**\n");
	printf ("** Report bugs to:\n**\n");
	printf ("**     <valvula@lists.aspl.es> Valvula Mailing list\n**\n");

	/* uncomment the following four lines to get debug */
	while (argc > 0) {
		if (axl_cmp (argv[argc], "--help")) 
			exit (0);
		if (axl_cmp (argv[argc], "--debug")) 
			test_common_enable_debug = axl_true;
		if (axl_cmp (argv[argc], "--no-unmap"))
			valvulad_module_set_no_unmap_modules (axl_true);

		if (argv[argc] && axl_memcmp (argv[argc], "--run-test", 10)) {
			run_test_name = argv[argc] + 11;
			printf ("INFO: running test: %s\n", run_test_name);
		}
		argc--;
	} /* end if */

	/* run tests */
	CHECK_TEST("test_00")
	run_test (test_00, "Test 00: generic API function checks");

	/* run tests */
	CHECK_TEST("test_01")
	run_test (test_01, "Test 01: basic server startup (using default configuration)");

	/* run tests */
	CHECK_TEST("test_01a")
	run_test (test_01a, "Test 01-a: basic mail policy request");

	/* run tests */
	CHECK_TEST("test_02")
	run_test (test_02, "Test 02: database functions");

	/* various api functions to get information from requests */
	CHECK_TEST("test_02a")
	run_test (test_02a, "Test 02a: API functions to get data from requests");

	/* run tests */
	CHECK_TEST("test_02b")
	run_test (test_02b, "Test 02-b: test local domains detection support");

	/* run tests */
	CHECK_TEST("test_02c")
	run_test (test_02c, "Test 02-c: test local accounts detection support");

	/* run tests */
	CHECK_TEST("test_02d")
	run_test (test_02d, "Test 02-d: more test local accounts detection support");

	CHECK_TEST("test_02e")
	run_test (test_02e, "Test 02-e: detect accounts and notifications with (=) in the midle");

	/* run tests */
	CHECK_TEST("test_03")
	run_test (test_03, "Test 03: checking mod-ticket");

	/* run tests */
	CHECK_TEST("test_03a")
	run_test (test_03a, "Test 03a: checking mod-ticket transport change support");

	/* run tests */
	CHECK_TEST("test_04")
	run_test (test_04, "Test 04: test valvulad-mgr.py");

	/* run tests */
	CHECK_TEST("test_05")
	run_test (test_05, "Test 05: test mod-bwl");

	/* run tests */
	CHECK_TEST("test_06")
	run_test (test_06, "Test 06: test mod-slm");

	/* run tests */
	CHECK_TEST("test_07")
	run_test (test_07, "Test 07: test mod-mquota");

	/* run tests */
	CHECK_TEST("test_08")
	run_test (test_08, "Test 08: test mod-mquota");

	printf ("All tests passed OK!\n");

	/* terminate */
	return 0;
	
} /* end main */
