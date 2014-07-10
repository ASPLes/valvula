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
#include <valvula_reader.h>

/* local/private includes */
#include <valvula_private.h>

#define LOG_DOMAIN "valvula-reader"

/**
 * \defgroup valvula_reader Valvula Reader: The module that reads you frames. 
 */

/**
 * \addtogroup valvula_reader
 * @{
 */

typedef enum {CONNECTION, 
	      LISTENER, 
	      TERMINATE, 
	      IO_WAIT_CHANGED,
	      IO_WAIT_READY
} WatchType;

typedef struct _ValvulaReaderData {
	WatchType             type;
	ValvulaConnection   * connection;
	axlPointer            user_data;
	/* queue used to notify that the foreach operation was
	 * finished: currently only used for type == FOREACH */
	ValvulaAsyncQueue   * notify;
}ValvulaReaderData;

/**
 * @brief Read the next line, byte by byte until it gets a \n or
 * maxlen is reached. Some code errors are used to manage exceptions
 * (see return values)
 * 
 * @param connection The connection where the read operation will be done.
 *
 * @param buffer A buffer to store content read from the network.
 *
 * @param maxlen max content to read from the network.
 * 
 * @return  values returned by this function follows:
 *  0 - remote peer have closed the connection
 * -1 - an error have happened while reading
 * -2 - could read because this connection is on non-blocking mode and there is no data.
 *  n - some data was read.
 * 
 **/
int          valvula_readline (ValvulaConnection * connection, char  * buffer, int  maxlen)
{
	int         n, rc;
	int         desp;
	char        c, *ptr;
#if defined(ENABLE_VALVULA_LOG)
	char      * error_msg;
#endif
#if defined(ENABLE_VALVULA_LOG) && ! defined(SHOW_FORMAT_BUGS)
	ValvulaCtx * ctx = valvula_connection_get_ctx (connection);
#endif

	/* avoid calling to read when no good socket is defined */
	if (connection->session == -1)
		return -1;

	/* clear the buffer received */
	/* memset (buffer, 0, maxlen * sizeof (char ));  */

	/* check for pending line read */
	desp         = 0;
	if (connection->pending_line) {
		/* get size and check exceeded values */
		desp = strlen (connection->pending_line);
		if (desp >= maxlen) {
			valvula_connection_close (connection);
			valvula_log (VALVULA_LEVEL_CRITICAL, "found fragmented frame line header but allowed size was exceeded (desp:%d >= maxlen:%d)",
				     desp, maxlen);
			return -1;
		} /* end if */

		/* now store content into the buffer */
		memcpy (buffer, connection->pending_line, desp);

		/* clear from the connection the line */
		axl_free (connection->pending_line);
		connection->pending_line = NULL;
	}


	/* read current next line */
	ptr = (buffer + desp);
	for (n = 1; n < (maxlen - desp); n++) {
	__valvula_frame_readline_again:
		if (( rc = recv (connection->session, &c, 1, 0)) == 1) {
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
						/* valvula_log (VALVULA_LEVEL_WARNING, "storing partially line read: '%s' n:%d, desp:%d", buffer, n, desp);*/
						connection->pending_line = axl_strdup (buffer);
					} /* end if */
				} /* end if */
				return (-2);
			}
			
#if defined(ENABLE_VALVULA_LOG)
			/* if the connection is closed, just return
			 * without logging a message */
			if (valvula_connection_is_ok (connection) && ! connection->process_launched) {
				error_msg = strerror (errno);
				valvula_log (VALVULA_LEVEL_CRITICAL, "unable to read a line from conn-id (socket %d, rc %d), error was: %s",
					     valvula_connection_get_socket (connection), rc,
					     error_msg ? error_msg : "");
			} /* end if */
#endif
			return (-1);
		} /* end if */
	} /* end for */

	*ptr = 0;
	return (n + desp);

}

axl_bool __valvula_reader_find_next_registry (axlPointer key, axlPointer data, axlPointer user_data, axlPointer user_data2, axlPointer user_data3)
{
	ValvulaRequestRegistry  * current  = user_data;
	ValvulaRequestRegistry ** next     = user_data2;
	int                       port     = PTR_TO_INT (user_data3);

	if (port > 0 && current->port != port)
		return axl_false; /* port doesn't match so we cannot use this for this policy */

	if (current == (*next)) 
		return axl_false; /* skip same value */

	/* check if the current handler has a lower priority than
	   current selected (next). NOTE: (*next) always must point to the latest policy selected */
	if (current->priority < (*next)->priority)
		(*next) = current;

	return axl_false; /* dont stop iterating */
}

void __valvula_reader_send (ValvulaConnection * connection, const char * _status, const char * _message)
{
	ValvulaCtx * ctx = connection->ctx;
	int          bytes_written;
	char       * message;

	/* build message */
	if (_message)
		message = axl_strdup_printf ("action=%s %s\n\n", _status, _message);
	else if (connection->request->message_reply)
		message = axl_strdup_printf ("action=%s %s\n\n", _status, connection->request->message_reply);
	else
		message = axl_strdup_printf ("action=%s\n\n", _status);

	/* send content and catch bytes written */
	bytes_written = send (connection->session, message, strlen (message), 0);

	/* check result */
	if (bytes_written != strlen (message)) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "Failed to send message %s, reported bytes written %d but expected %d",
			     message, bytes_written, strlen (message));
	} else {
		valvula_log (VALVULA_LEVEL_DEBUG, "Sending reply '%s' over session=%d (%p)",
			     message, connection->session, connection);
	} /* end if */

	/* release message */
	axl_free (message);

	return;
}

/** 
 * @internal Function that handles sending a reply back to the gateway
 * using the state provided and optionally, if required, the message.
 */
void __valvula_reader_send_reply (ValvulaCtx        * ctx, 
				  ValvulaConnection * connection, 
				  ValvulaRequest    * request, 
				  ValvulaState        state, 
				  const char        * message)
{
	
	/* check if we have a handler for final notification */
	if (ctx->report_final_state)
		ctx->report_final_state (ctx, connection, request, state, message, ctx->report_final_state_user_data);

	switch (state) {
	case VALVULA_STATE_OK:
		__valvula_reader_send (connection, "ok", message);
		break;
	case VALVULA_STATE_DUNNO:
		__valvula_reader_send (connection, "dunno", message);
		break;
	case VALVULA_STATE_REJECT:
		__valvula_reader_send (connection, "reject", message);
		break;
	case VALVULA_STATE_DEFER_IF_PERMIT:
		__valvula_reader_send (connection, "defer_if_permit", message);
		break;
	case VALVULA_STATE_DEFER_IF_REJECT:
		__valvula_reader_send (connection, "defer_is_reject", message);
		break;
	case VALVULA_STATE_DEFER:
		__valvula_reader_send (connection, "defer", message);
		break;
	case VALVULA_STATE_BCC:
		break;
	case VALVULA_STATE_DISCARD:
		__valvula_reader_send (connection, "discard", message);
		break;
	case VALVULA_STATE_REDIRECT:
		break;
	case VALVULA_STATE_LOG:
		break;
	default:
		break;
	} /* end if */

	/* flag the connection as process finished */
	/* DO NOT UNCOMMENT THE FOLLOWING: code is simplier to handle
	   procesing a request for each connection it is showed to
	   work better */
	/* connection->process_launched = axl_false; */ 

	return;
}

axl_bool __valvula_reader_find_next_handler_in_list (axlPointer _element, axlPointer data)
{
        return _element == data;
}

ValvulaRequestRegistry * __valvula_reader_find_next_handler (ValvulaCtx * ctx, axlHashCursor * cursor, int listener_port, axlList * selected )
{

	ValvulaRequestRegistry * next;
	ValvulaRequestRegistry * registry = NULL;

	/* reset cursor */
	axl_hash_cursor_first (cursor);
	while (axl_hash_cursor_has_item (cursor)) {
		/* get value */
		next = axl_hash_cursor_get_value (cursor);
		if (next == NULL)
			break;

		/* ensure next is not already selected */
		if (axl_list_lookup (selected, __valvula_reader_find_next_handler_in_list, next)) {
		        /* go to the next position */
		        axl_hash_cursor_next (cursor);

		        continue;
		} /* end if */

		/* check port to match current request */
		if (next->port == listener_port) {
			if (registry == NULL || next->priority < registry->priority)
				registry = next;
		} /* end if */
		
		/* go to the next position */
		axl_hash_cursor_next (cursor);
	} /* end while */

	if (registry)
	        axl_list_append (selected, registry);

	return registry;
}

axlPointer __valvula_reader_record_handle_start (ValvulaCtx * ctx, const char * handler_name, ValvulaRequest * request)
{
	ValvulaReaderProcess * process;
	struct timeval tv;

	/* create process */
	process = axl_new (ValvulaReaderProcess, 1);
	if (process == NULL)
		return NULL;

	/* record content */
	process->request      = request;
	process->handler_name = handler_name;

	/* get current stamp */
	gettimeofday (&tv, NULL);
	process->stamp        = tv.tv_sec;

	/* lock registry */
	valvula_mutex_lock (&ctx->op_mutex);

	/* request in process */
	axl_list_append (ctx->request_in_process, process);

	/* release registry */
	valvula_mutex_unlock (&ctx->op_mutex);

	return process;
}

void __valvula_reader_record_handle_stop (ValvulaCtx * ctx, axlPointer _process)
{
	/* lock registry */
	valvula_mutex_lock (&ctx->op_mutex);

	/* request in process */
	axl_list_remove_ptr (ctx->request_in_process, _process);

	/* release registry */
	valvula_mutex_unlock (&ctx->op_mutex);

	return;
}


axlPointer valvula_reader_process_request (axlPointer _connection)
{
	/* get variables */
	ValvulaConnection       * connection    = _connection;
	/* get port where this request was received */
	int                       listener_port = valvula_support_strtod (connection->listener->port, NULL);
	ValvulaCtx              * ctx = connection->ctx;
	char                    * message = NULL;
	ValvulaState              state;

	/* list of selected handlers this time */
	axlList                 * selected;

	/* handler reference */
	ValvulaProcessRequest     handler;
	const char              * handler_name;
	axlPointer                record_id;

	axlPointer                user_data;
	ValvulaRequestRegistry  * registry = NULL;
	axlHashCursor           * cursor;

	/* global operation */
	struct timeval            start;
	struct timeval            stop;
	struct timeval            diff;

	/* module operation */
	struct timeval            start_m;
	struct timeval            stop_m;
	long                      total_microsecs;

	/* update port reported */
	connection->request->listener_port = listener_port;

	if (ctx->process_handler_registry && valvula_hash_size (ctx->process_handler_registry) > 0) {
		/* get first element from the registry */
	        selected = axl_list_new (axl_list_always_return_1, NULL);
		cursor   = valvula_hash_get_cursor (ctx->process_handler_registry);

		/* iterate over all items to find the lowest on this port */
		axl_hash_cursor_first (cursor);
		registry = __valvula_reader_find_next_handler (ctx, cursor, listener_port, selected);

		if (registry == NULL) {
			/* free cursor */
			axl_hash_cursor_free (cursor);
			axl_list_free (selected);

			/* no handlers defined so no policy can be delegated, replying default */
			__valvula_reader_send_reply (ctx, connection, connection->request, ctx->default_state, NULL);
			return NULL;
		} /* end if */

		/* start tracking */
		gettimeofday (&start, NULL);
		do {
			/* get handler and user data */
			handler      = registry->process_handler;
			handler_name = registry->identifier;
			user_data    = registry->user_data;

			valvula_log (VALVULA_LEVEL_DEBUG, "Checking registry handler: %p (%s)", registry, handler_name);

			/* start tracking */
			gettimeofday (&start_m, NULL);

			/* record we are about to enter in a handler with a particular name */
			record_id = __valvula_reader_record_handle_start (ctx, handler_name, connection->request);

			/* call to notify request and get a response */
			message = NULL;
			state   = handler (ctx, connection, connection->request, user_data, &message);

			/* call to record that we finished */
			__valvula_reader_record_handle_stop (ctx, record_id);

			valvula_log (VALVULA_LEVEL_DEBUG, "Handler %p reported state (%d) %s", registry, state, valvula_support_state_str (state));

			/* start tracking */
			gettimeofday (&stop_m, NULL);

			valvula_timeval_substract (&stop_m, &start_m, &diff);
			total_microsecs = (diff.tv_sec * 1000000) + diff.tv_usec;

			/* lock */
			valvula_mutex_lock (&registry->stats_mutex);

			/* update processing stats */
			if (total_microsecs > registry->max_processing) 
				registry->max_processing = total_microsecs;
			if (total_microsecs < registry->min_processing || registry->min_processing == 0)
				registry->min_processing = total_microsecs;
			if (registry->avg_processing == 0)
				registry->avg_processing = total_microsecs;
			else 
				registry->avg_processing = (registry->avg_processing + total_microsecs) / 2;

			/* unlock */
			valvula_mutex_unlock (&registry->stats_mutex);
			

			/* check if the error code is disntict from DUNNO */
			if (state != VALVULA_STATE_DUNNO)
				break;

			axl_free (message);

			/* get next registry */
			registry = __valvula_reader_find_next_handler (ctx, cursor, listener_port, selected);

		} while (registry);

		valvula_log (VALVULA_LEVEL_DEBUG, "Reply to request %p was state=%d (%s), message=%s",
			     connection->request, state, valvula_support_state_str (state), message ? message : "");
		axl_list_free (selected);

		/* finish time tracking */
		gettimeofday (&stop, NULL);

		valvula_timeval_substract (&stop, &start, &diff);
		total_microsecs = (diff.tv_sec * 1000000) + diff.tv_usec;

		/* lock */
		valvula_mutex_lock (&ctx->stats_mutex);

		/* update processing stats */
		if (total_microsecs > ctx->max_processing) 
			ctx->max_processing = total_microsecs;
		if (total_microsecs < ctx->min_processing || ctx->min_processing == 0)
			ctx->min_processing = total_microsecs;
		if (ctx->avg_processing == 0)
			ctx->avg_processing = total_microsecs;
		else 
			ctx->avg_processing = (ctx->avg_processing + total_microsecs) / 2;

		/* record requests handled */
		ctx->requests_handled ++;

		/* unlock */
		valvula_mutex_unlock (&ctx->stats_mutex);
		
		/* send reply */
		__valvula_reader_send_reply (ctx, connection, connection->request, state, message);
		axl_free (message);

		/* free cursor */
		axl_hash_cursor_free (cursor);

		return NULL;
	} /* end if */

	/* lock */
	valvula_mutex_lock (&ctx->stats_mutex);

	/* record requests handled  */
	ctx->requests_handled ++;

	/* unlock */
	valvula_mutex_unlock (&ctx->stats_mutex);

	/* no handlers defined so no policy can be delegated, replying
	   default */
	__valvula_reader_send_reply (ctx, connection, connection->request, 
				     ctx->default_state, NULL);

	return NULL;
}

#define valvula_reader_set_value(var,value_to_configure) do { \
	if (var)                                             \
		axl_free (var);                              \
	var = axl_strdup(value_to_configure);		     \
	} while (0)

/** 
 * @internal
 * 
 * The main purpose of this function is to dispatch received frames
 * into the appropriate channel. It also makes all checks to ensure the
 * frame receive have all indicators (seqno, channel, message number,
 * payload size correctness,..) to ensure the channel receive correct
 * frames and filter those ones which have something wrong.
 *
 * This function also manage frame fragment joining. There are two
 * levels of frame fragment managed by the valvula reader.
 * 
 * We call the first level of fragment, the one described at RFC3080,
 * as the complete frame which belongs to a group of frames which
 * conform a message which was splitted due to channel window size
 * restrictions.
 *
 * The second level of fragment happens when the valvula reader receive
 * a frame header which describes a frame size payload to receive but
 * not all payload was actually received. This can happen because
 * valvula uses non-blocking socket configuration so it can avoid DOS
 * attack. But this behavior introduce the asynchronous problem of
 * reading at the moment where the whole frame was not received.  We
 * call to this internal frame fragmentation. It is also supported
 * without blocking to valvula reader.
 *
 * While reading this function, you have to think about it as a
 * function which is executed for only one frame, received inside only
 * one channel for the given connection.
 *
 * @param connection the connection which have something to be read
 * 
 **/
void __valvula_reader_process_socket (ValvulaCtx        * ctx, 
				      ValvulaConnection * connection)
{

	char    buffer[2048];
	int     bytes_read;
	char ** items;

	/* limit buffer */
	memset (buffer, 0, 2048);
	bytes_read = valvula_readline (connection, buffer, 2048);
	if (bytes_read == -1) 
		return;
	if (bytes_read == -2) 
		return; /* not ready yet */

	valvula_log (VALVULA_LEVEL_DEBUG, "Found content line: %s (%d bytes)", buffer, bytes_read);
	if (axl_memcmp (buffer, "checkserver", 11)) {
		valvula_log (VALVULA_LEVEL_DEBUG, "Received request to check server, reporting ok and closing connection: socket=%d",
			     connection->session);
		send (connection->session, "I'm running right", 17, 0);
		connection->process_launched = axl_true;
		valvula_connection_close (connection); 
		return;
	} /* end if */

	/* increase number of lines found until now to limit them */
	connection->lines_found += 1;
	if (connection->lines_found > ctx->request_line_limit) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "Exceeded  line limit (%d) with %d while reading request, closing..",
			     ctx->request_line_limit, connection->lines_found);
		valvula_connection_close (connection);
		return;
	} /* ned if */

	/* prepare request type to hold all info */
	if (! connection->request)
		connection->request = axl_new (ValvulaRequest, 1);

	/* check for empty line so we can process the request */
	axl_stream_trim (buffer);

	if (strlen (buffer) == 0) {
		/* check if the process was launched */
	        if (connection->process_launched) {
			valvula_log (VALVULA_LEVEL_DEBUG, "Connection close while processing on session=%d (%p), closing connection");
			valvula_connection_close (connection);
			return;
		} /* end if */

		/* flag we are about to launch the process */
		connection->process_launched = axl_true; 

		/* drop a log */
		valvula_log (VALVULA_LEVEL_DEBUG, "Launching process request over connection session=%d (%p)", 
			     connection->session, connection);

		/* process request */
		valvula_thread_pool_new_task (ctx, valvula_reader_process_request, connection);
		return;
	} /* end if */

	/* parse line and attach to the connection request */	
	items = axl_split (buffer, 1, "=");
	if (items == NULL || items[0] == NULL || items[1] == NULL) {
		axl_freev (items);
		valvula_log (VALVULA_LEVEL_CRITICAL, "Failed to process line received, empty content found or malformed, closing connection");
		/* close connection */
		valvula_connection_close (connection);
		return;
	} /* end if */

	if (axl_cmp (items[0], "request"))
		valvula_reader_set_value (connection->request->request, items[1]);
	else if (axl_cmp (items[0], "protocol_state"))
		valvula_reader_set_value (connection->request->protocol_state, items[1]);
	else if (axl_cmp (items[0], "protocol_name"))
		valvula_reader_set_value (connection->request->protocol_name, items[1]);

	else if (axl_cmp (items[0], "queue_id"))
		valvula_reader_set_value (connection->request->queue_id, items[1]);
	else if (axl_cmp (items[0], "size"))
		connection->request->size = (int) valvula_support_strtod (items[1], NULL);

	else if (axl_cmp (items[0], "sender"))
		valvula_reader_set_value (connection->request->sender, items[1]);
	else if (axl_cmp (items[0], "recipient"))
		valvula_reader_set_value (connection->request->recipient, items[1]);
	else if (axl_cmp (items[0], "recipient_count"))
		connection->request->recipient_count = valvula_support_strtod (items[1], NULL);

	else if (axl_cmp (items[0], "helo_name"))
		valvula_reader_set_value (connection->request->helo_name, items[1]);
	else if (axl_cmp (items[0], "client_address"))
		valvula_reader_set_value (connection->request->client_address, items[1]);
	else if (axl_cmp (items[0], "client_name"))
		valvula_reader_set_value (connection->request->client_name, items[1]);
	else if (axl_cmp (items[0], "reverse_client"))
		valvula_reader_set_value (connection->request->reverse_client, items[1]);
	else if (axl_cmp (items[0], "instance"))
		valvula_reader_set_value (connection->request->instance, items[1]);

	else if (axl_cmp (items[0], "sasl_method"))
		valvula_reader_set_value (connection->request->sasl_method, items[1]);
	else if (axl_cmp (items[0], "sasl_username"))
		valvula_reader_set_value (connection->request->sasl_username, items[1]);
	else if (axl_cmp (items[0], "sasl_sender"))
		valvula_reader_set_value (connection->request->sasl_sender, items[1]);

	else if (axl_cmp (items[0], "ccert_subject"))
		valvula_reader_set_value (connection->request->ccert_subject, items[1]);
	else if (axl_cmp (items[0], "ccert_issuer"))
		valvula_reader_set_value (connection->request->ccert_issuer, items[1]);
	else if (axl_cmp (items[0], "ccert_fingerprint"))
		valvula_reader_set_value (connection->request->ccert_fingerprint, items[1]);
	else if (axl_cmp (items[0], "ccert_pubkey_fingerprint"))
		valvula_reader_set_value (connection->request->ccert_pubkey_fingerprint, items[1]);

	else if (axl_cmp (items[0], "encryption_protocol"))
		valvula_reader_set_value (connection->request->encryption_protocol, items[1]);
	else if (axl_cmp (items[0], "encryption_cipher"))
		valvula_reader_set_value (connection->request->encryption_cipher, items[1]);
	else if (axl_cmp (items[0], "encryption_keysize"))
		valvula_reader_set_value (connection->request->encryption_keysize, items[1]);

	else if (axl_cmp (items[0], "etrn_domain"))
		valvula_reader_set_value (connection->request->etrn_domain, items[1]);
	else if (axl_cmp (items[0], "stress"))
		valvula_reader_set_value (connection->request->stress, items[1]);

	/* release memory not needed */
	axl_freev (items);

	/* that's all I can do */
	return;
}

/** 
 * @internal 
 *
 * @brief Classify valvula reader items to be managed, that is,
 * connections or listeners.
 * 
 * @param data The internal valvula reader data to be managed.
 * 
 * @return axl_true if the item to be managed was clearly read or axl_false if
 * an error on registering the item was produced.
 */
axl_bool   valvula_reader_register_watch (ValvulaReaderData * data, axlList * con_list, axlList * srv_list)
{
	ValvulaConnection * connection;

	/* get a reference to the connection (no matter if it is not
	 * defined) */
	connection = data->connection;

	switch (data->type) {
	case CONNECTION:
		/* check the connection */
		if (!valvula_connection_is_ok (connection)) {
			/* check if we can free this connection */
			valvula_connection_unref (connection, "valvula reader (watch)");

			/* release data */
			axl_free (data);
			return axl_false;
		}
			
		/* now we have a first connection, we can start to wait */
		axl_list_append (con_list, connection);

		break;
	case LISTENER:
		axl_list_append (srv_list, connection);
		break;
	case TERMINATE:
	case IO_WAIT_CHANGED:
	case IO_WAIT_READY:
		/* just unref valvula reader data */
		break;
	} /* end switch */
	
	axl_free (data);
	return axl_true;
}

/** 
 * @internal Valvula function to implement valvula reader I/O change.
 */
ValvulaReaderData * __valvula_reader_change_io_mech (ValvulaCtx        * ctx,
						   axlPointer       * on_reading, 
						   axlList          * con_list, 
						   axlList          * srv_list, 
						   ValvulaReaderData * data)
{
	/* get current context */
	ValvulaReaderData * result;

	valvula_log (VALVULA_LEVEL_DEBUG, "found I/O notification change");
	
	/* unref IO waiting object */
	valvula_io_waiting_invoke_destroy_fd_group (ctx, *on_reading); 
	*on_reading = NULL;
	
	/* notify preparation done and lock until new
	 * I/O is installed */
	valvula_log (VALVULA_LEVEL_DEBUG, "notify valvula reader preparation done");
	valvula_async_queue_push (ctx->reader_stopped, INT_TO_PTR(1));
	
	/* free data use the function that includes that knoledge */
	valvula_reader_register_watch (data, con_list, srv_list);
	
	/* lock */
	valvula_log (VALVULA_LEVEL_DEBUG, "lock until new API is installed");
	result = valvula_async_queue_pop (ctx->reader_queue);

	/* initialize the read set */
	valvula_log (VALVULA_LEVEL_DEBUG, "unlocked, creating new I/O mechanism used current API");
	*on_reading = valvula_io_waiting_invoke_create_fd_group (ctx, READ_OPERATIONS);

	return result;
}


/** 
 * @internal
 * @brief Read the next item on the valvula reader to be processed
 * 
 * Once an item is read, it is check if something went wrong, in such
 * case the loop keeps on going.
 * 
 * The function also checks for terminating valvula reader loop by
 * looking for TERMINATE value into the data->type. In such case axl_false
 * is returned meaning that no further loop should be done by the
 * valvula reader.
 *
 * @return axl_true to keep valvula reader working, axl_false if valvula reader
 * should stop.
 */
axl_bool      valvula_reader_read_queue (ValvulaCtx  * ctx,
					axlList    * con_list, 
					axlList    * srv_list, 
					axlPointer * on_reading)
{
	/* get current context */
	ValvulaReaderData * data;
	int                should_continue;

	do {
		data            = valvula_async_queue_pop (ctx->reader_queue);

		/* check if we have to continue working */
		should_continue = (data->type != TERMINATE);

		/* check if the io/wait mech have changed */
		if (data->type == IO_WAIT_CHANGED) {
			/* change io mechanism */
			data = __valvula_reader_change_io_mech (ctx,
							       on_reading, 
							       con_list, 
							       srv_list, 
							       data);
		} /* end if */

	}while (!valvula_reader_register_watch (data, con_list, srv_list));

	return should_continue;
}

/** 
 * @internal Function used by the valvula reader main loop to check for
 * more connections to watch, to check if it has to terminate or to
 * check at run time the I/O waiting mechanism used.
 * 
 * @param con_list The set of connections already watched.
 *
 * @param srv_list The set of listener connections already watched.
 *
 * @param on_reading A reference to the I/O waiting object, in the
 * case the I/O waiting mechanism is changed.
 * 
 * @return axl_true to flag the process to continue working to to stop.
 */
axl_bool      valvula_reader_read_pending (ValvulaCtx  * ctx,
					  axlList    * con_list, 
					  axlList    * srv_list, 
					  axlPointer * on_reading)
{
	/* get current context */
	ValvulaReaderData * data;
	int                length;
	axl_bool           should_continue = axl_true;

	length = valvula_async_queue_length (ctx->reader_queue);
	while (length > 0) {
		length--;
		data            = valvula_async_queue_pop (ctx->reader_queue);

		valvula_log (VALVULA_LEVEL_DEBUG, "read pending type=%d",
			    data->type);

		/* check if we have to continue working */
		should_continue = (data->type != TERMINATE);

		/* check if the io/wait mech have changed */
		if (data->type == IO_WAIT_CHANGED) {
			/* change io mechanism */
			data = __valvula_reader_change_io_mech (ctx, on_reading, con_list, srv_list, data);

		} /* end if */

		/* watch the request received, maybe a connection or a
		 * valvula reader command to process  */
		valvula_reader_register_watch (data, con_list, srv_list);
		
	} /* end while */

	return should_continue;
}

void valvula_reader_accept_connections (ValvulaCtx * ctx, int fds, ValvulaConnection * listener)
{
	VALVULA_SOCKET      new_socket = valvula_listener_accept (fds);
	ValvulaConnection * conn;

	if (new_socket < 0) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "Failed to accept incoming socket from %d (errno=%d)",
			     fds, errno);
		return;
	} /* end if */

	/* create connection */
	conn = valvula_connection_new_empty (ctx, new_socket, ValvulaRoleListener);
	if (conn == NULL) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "Failed to create connection reference (ValvulaConnection)");
		return;
	}

	/* configure listener */
	conn->listener = listener;

	/* watch listener */
	axl_list_append (ctx->conn_list, conn);
	
	return;
}

/** 
 * @internal Auxiliar function that populates the reading set of file
 * descriptors (on_reading), returning the max fds.
 */
VALVULA_SOCKET __valvula_reader_build_set_to_watch_aux (ValvulaCtx     * ctx,
						      axlPointer      on_reading, 
						      axlListCursor * cursor, 
						      VALVULA_SOCKET   current_max)
{
	VALVULA_SOCKET      max_fds     = current_max;
	VALVULA_SOCKET      fds         = 0;
	ValvulaConnection * connection;

	axl_list_cursor_first (cursor);
	while (axl_list_cursor_has_item (cursor)) {

		/* get current connection */
		connection = axl_list_cursor_get (cursor);

		/* check ok status */
		if (! valvula_connection_is_ok (connection)) {

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (cursor);

			/* connection isn't ok, unref it */
			valvula_connection_unref (connection, "valvula reader (build set)");

			continue;
		} /* end if */

		/* get the socket to ge added and get its maximum
		 * value */
		fds        = valvula_connection_get_socket (connection);
		max_fds    = fds > max_fds ? fds: max_fds;

		/* add the socket descriptor into the given on reading
		 * group */
		if (! valvula_io_waiting_invoke_add_to_fd_group (ctx, fds, connection, on_reading)) {
			
			valvula_log (VALVULA_LEVEL_WARNING, 
				    "unable to add the connection to the valvula reader watching set. This could mean you did reach the I/O waiting mechanism limit.");

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (cursor);

			/* set it as not connected */
			if (valvula_connection_is_ok (connection))
				valvula_connection_close (connection);
			valvula_connection_unref (connection, "valvula reader (add fail)");

			continue;
		} /* end if */

		/* get the next */
		axl_list_cursor_next (cursor);

	} /* end while */

	/* return maximum number for file descriptors */
	return max_fds;
	
} /* end __valvula_reader_build_set_to_watch_aux */

VALVULA_SOCKET   __valvula_reader_build_set_to_watch (ValvulaCtx     * ctx,
						    axlPointer      on_reading, 
						    axlListCursor * conn_cursor, 
						    axlListCursor * srv_cursor)
{

	VALVULA_SOCKET       max_fds     = 0;

	/* read server connections */
	max_fds = __valvula_reader_build_set_to_watch_aux (ctx, on_reading, srv_cursor, max_fds);

	/* read client connection list */
	max_fds = __valvula_reader_build_set_to_watch_aux (ctx, on_reading, conn_cursor, max_fds);

	/* return maximum number for file descriptors */
	return max_fds;
	
}

void __valvula_reader_check_connection_list (ValvulaCtx     * ctx,
					    axlPointer      on_reading, 
					    axlListCursor * conn_cursor, 
					    int             changed)
{

	VALVULA_SOCKET       fds        = 0;
	ValvulaConnection  * connection = NULL;
	int                 checked    = 0;

	/* check all connections */
	axl_list_cursor_first (conn_cursor);
	while (axl_list_cursor_has_item (conn_cursor)) {

		/* check changed */
		if (changed == checked)
			return;

		/* check if we have to keep on listening on this
		 * connection */
		connection = axl_list_cursor_get (conn_cursor);
		if (!valvula_connection_is_ok (connection)) {
			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (conn_cursor);

			/* connection isn't ok, unref it */
			valvula_connection_unref (connection, "valvula reader (check list)");
			continue;
		}
		
		/* get the connection and socket. */
	        fds = valvula_connection_get_socket (connection);
		
		/* ask if this socket have changed */
		if (valvula_io_waiting_invoke_is_set_fd_group (ctx, fds, on_reading, ctx)) {

			/* call to process incoming data, activating
			 * all invocation code (first and second level
			 * handler) */
			__valvula_reader_process_socket (ctx, connection);

			/* update number of sockets checked */
			checked++;
		}

		/* get the next */
		axl_list_cursor_next (conn_cursor);

	} /* end for */

	return;
}

int  __valvula_reader_check_listener_list (ValvulaCtx     * ctx, 
					  axlPointer      on_reading, 
					  axlListCursor * srv_cursor, 
					  int             changed)
{

	int                fds      = 0;
	int                checked  = 0;
	ValvulaConnection * connection;

	valvula_log (VALVULA_LEVEL_DEBUG, "Checking activity at listeners..");

	/* check all listeners */
	axl_list_cursor_first (srv_cursor);
	while (axl_list_cursor_has_item (srv_cursor)) {

		/* get the connection */
		connection = axl_list_cursor_get (srv_cursor);

		if (!valvula_connection_is_ok (connection)) {
			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (srv_cursor);

			/* connection isn't ok, unref it */
			valvula_connection_unref (connection, "valvula reader (process), listener closed");

			/* update checked connections */
			checked++;

			continue;
		} /* end if */
		
		/* get the connection and socket. */
		fds  = valvula_connection_get_socket (connection);

		/* check if the socket is activated */
		if (valvula_io_waiting_invoke_is_set_fd_group (ctx, fds, on_reading, ctx)) {
			/* init the listener incoming connection phase */
			valvula_log (VALVULA_LEVEL_DEBUG, "listener (%d) have requests, processing..", fds);
			valvula_reader_accept_connections (ctx, fds, connection);

			/* update checked connections */
			checked++;
		} /* end if */

		/* check to stop listener */
		if (checked == changed)
			return 0;

		/* get the next */
		axl_list_cursor_next (srv_cursor);
	}
	
	/* return remaining sockets active */
	return changed - checked;
}

/** 
 * @internal
 *
 * @brief Internal function called to stop valvula reader and cleanup
 * memory used.
 * 
 */
void __valvula_reader_stop_process (ValvulaCtx     * ctx,
				   axlPointer      on_reading, 
				   axlListCursor * conn_cursor, 
				   axlListCursor * srv_cursor)

{
	/* stop valvula reader process unreferring already managed
	 * connections */

	valvula_async_queue_unref (ctx->reader_queue);

	/* unref listener connections */
	valvula_log (VALVULA_LEVEL_DEBUG, "cleaning pending %d listener connections..", axl_list_length (ctx->srv_list));
	ctx->srv_list = NULL;
	axl_list_free (axl_list_cursor_list (srv_cursor));
	axl_list_cursor_free (srv_cursor);

	/* unref initiators connections */
	valvula_log (VALVULA_LEVEL_DEBUG, "cleaning pending %d peer connections..", axl_list_length (ctx->conn_list));
	ctx->conn_list = NULL;
	axl_list_free (axl_list_cursor_list (conn_cursor));
	axl_list_cursor_free (conn_cursor);

	/* unref IO waiting object */
	valvula_io_waiting_invoke_destroy_fd_group (ctx, on_reading); 

	/* signal that the valvula reader process is stopped */
	valvula_async_queue_push (ctx->reader_stopped, INT_TO_PTR (1));

	return;
}

void __valvula_reader_close_connection (axlPointer pointer)
{
	ValvulaConnection * conn = pointer;

	/* unref the connection */
	valvula_connection_close (conn);
	valvula_connection_unref (conn, "valvula reader");

	return;
}

/** 
 * @internal Dispatch function used to process all sockets that have
 * changed.
 * 
 * @param fds The socket that have changed.
 * @param wait_to The purpose that was configured for the file set.
 * @param connection The connection that is notified for changes.
 */
void __valvula_reader_dispatch_connection (int                  fds,
					  ValvulaIoWaitingFor   wait_to,
					  ValvulaConnection   * connection,
					  axlPointer           user_data)
{
	/* cast the reference */
	ValvulaCtx * ctx = user_data;

	switch (valvula_connection_get_role (connection)) {
	case ValvulaRoleMasterListener:
		/* listener connections */
		valvula_reader_accept_connections (ctx, fds, connection);
		break;
	default:
		/* call to process incoming data, activating all
		 * invocation code (first and second level handler) */
		__valvula_reader_process_socket (ctx, connection);
		break;
	} /* end if */
	return;
}

axl_bool __valvula_reader_detect_and_cleanup_connection (axlListCursor * cursor) 
{
	ValvulaConnection * conn;
	char               bytes[3];
	int                result;
	int                fds;
#if defined(ENABLE_VALVULA_LOG)
	ValvulaCtx        * ctx;
#endif
	
	/* get connection from cursor */
	conn = axl_list_cursor_get (cursor);
	if (valvula_connection_is_ok (conn)) {

		/* get the connection and socket. */
		fds    = valvula_connection_get_socket (conn);
#if defined(AXL_OS_UNIX)
		errno  = 0;
#endif
		result = recv (fds, bytes, 1, MSG_PEEK);
		if (result == -1 && errno == EBADF) {
			  
			/* get context */
#if defined(ENABLE_VALVULA_LOG)
			ctx = conn->ctx;
#endif
			valvula_log (VALVULA_LEVEL_CRITICAL, "Found with session=%d not working (errno=%d), shutting down",
				     fds, errno);
			/* close connection, but remove the socket reference to avoid closing some's socket */
			conn->session = -1;
			valvula_connection_close (conn);
			
			/* connection isn't ok, unref it */
			valvula_connection_unref (conn, "valvula reader (process), wrong socket");
			axl_list_cursor_unlink (cursor);
			return axl_false;
		} /* end if */
	} /* end if */
	
	return axl_true;
}

void __valvula_reader_detect_and_cleanup_connections (ValvulaCtx * ctx)
{
	/* check all listeners */
	axl_list_cursor_first (ctx->conn_cursor);
	while (axl_list_cursor_has_item (ctx->conn_cursor)) {

		/* get the connection */
		if (! __valvula_reader_detect_and_cleanup_connection (ctx->conn_cursor))
			continue;

		/* get the next */
		axl_list_cursor_next (ctx->conn_cursor);
	} /* end while */

	/* check all listeners */
	axl_list_cursor_first (ctx->srv_cursor);
	while (axl_list_cursor_has_item (ctx->srv_cursor)) {

	  /* get the connection */
	  if (! __valvula_reader_detect_and_cleanup_connection (ctx->srv_cursor))
		   continue; 

	    /* get the next */
	    axl_list_cursor_next (ctx->srv_cursor); 
	} /* end while */

	/* clear errno after cleaning descriptors */
#if defined(AXL_OS_UNIX)
	errno = 0;
#endif

	return; 
}

axlPointer __valvula_reader_run (ValvulaCtx * ctx)
{
	VALVULA_SOCKET      max_fds     = 0;
	VALVULA_SOCKET      result;
	int                error_tries = 0;

	/* initialize the read set */
	ctx->on_reading  = valvula_io_waiting_invoke_create_fd_group (ctx, READ_OPERATIONS);

	/* create lists */
	ctx->conn_list = axl_list_new (axl_list_always_return_1, __valvula_reader_close_connection);
	ctx->srv_list = axl_list_new (axl_list_always_return_1, __valvula_reader_close_connection);

	/* create cursors */
	ctx->conn_cursor = axl_list_cursor_new (ctx->conn_list);
	ctx->srv_cursor = axl_list_cursor_new (ctx->srv_list);

	

	/* first step. Waiting blocked for our first connection to
	 * listen */
 __valvula_reader_run_first_connection:
	if (!valvula_reader_read_queue (ctx, ctx->conn_list, ctx->srv_list, &(ctx->on_reading))) {
		/* seems that the valvula reader main loop should
		 * stop */
		__valvula_reader_stop_process (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
		return NULL;
	}

	while (axl_true) {

		/* reset descriptor set */
		valvula_io_waiting_invoke_clear_fd_group (ctx, ctx->on_reading);

		if ((axl_list_length (ctx->conn_list) == 0) && (axl_list_length (ctx->srv_list) == 0)) {
			valvula_log (VALVULA_LEVEL_DEBUG, "no more connection to watch for, putting thread to sleep");
			goto __valvula_reader_run_first_connection;
		}

		/* build socket descriptor to be read */
		max_fds = __valvula_reader_build_set_to_watch (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
		if (errno == EBADF) {
			valvula_log (VALVULA_LEVEL_CRITICAL, "Found wrong file descriptor error...(max_fds=%d, errno=%d), cleaning", max_fds, errno);
			/* detect and cleanup wrong connections */
			__valvula_reader_detect_and_cleanup_connections (ctx);
			continue;
		} /* end if */
		
		/* perform IO blocking wait for read operation */
		result = valvula_io_waiting_invoke_wait (ctx, ctx->on_reading, max_fds, READ_OPERATIONS);

		/* do automatic thread pool resize here */
		__valvula_thread_pool_automatic_resize (ctx);  

		/* check for timeout error */
		if (result == -1 || result == -2)
			goto process_pending;

		/* check errors */
		if ((result < 0) && (errno != 0)) {

			error_tries++;
			if (error_tries == 2) {
				valvula_log (VALVULA_LEVEL_CRITICAL, 
					    "tries have been reached on reader, error was=(errno=%d): %s exiting..",
					     errno, strerror (errno));
				return NULL;
			} /* end if */
			continue;
		} /* end if */

		/* check for fatal error */
		if (result == -3) {
			valvula_log (VALVULA_LEVEL_CRITICAL, "fatal error received from io-wait function, exiting from valvula reader process..");
			__valvula_reader_stop_process (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
			return NULL;
		}

		/* check for each listener */
		if (result > 0) {
			/* check if the mechanism have automatic
			 * dispatch */
			if (valvula_io_waiting_invoke_have_dispatch (ctx, ctx->on_reading)) {
				/* perform automatic dispatch,
				 * providing the dispatch function and
				 * the number of sockets changed */
				valvula_io_waiting_invoke_dispatch (ctx, ctx->on_reading, __valvula_reader_dispatch_connection, result, ctx);

			} else {
				/* call to check listener connections */
				result = __valvula_reader_check_listener_list (ctx, ctx->on_reading, ctx->srv_cursor, result);
			
				/* check for each connection to be watch is it have check */
				__valvula_reader_check_connection_list (ctx, ctx->on_reading, ctx->conn_cursor, result);
			} /* end if */
		}

		/* we have finished the connection dispatching, so
		 * read the pending queue elements to be watched */
		
		/* reset error tries */
	process_pending:
		error_tries = 0;

		/* read new connections to be managed */
		if (!valvula_reader_read_pending (ctx, ctx->conn_list, ctx->srv_list, &(ctx->on_reading))) {
			__valvula_reader_stop_process (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
			return NULL;
		}
	}
	return NULL;
}

/** 
 * @brief Function that returns the number of connections that are
 * currently watched by the reader.
 * @param ctx The context where the reader loop is located.
 * @return Number of connections watched. 
 */
int  valvula_reader_connections_watched         (ValvulaCtx        * ctx)
{
	if (ctx == NULL || ctx->conn_list == NULL || ctx->srv_list == NULL)
		return 0;
	
	/* return list */
	return axl_list_length (ctx->conn_list) + axl_list_length (ctx->srv_list);
}

/** 
 * @internal
 * 
 * Adds a new connection to be watched on valvula reader process. This
 * function is for internal valvula library use.
 **/
void valvula_reader_watch_connection (ValvulaCtx        * ctx,
				     ValvulaConnection * connection)
{
	/* get current context */
	ValvulaReaderData * data;

	v_return_if_fail (valvula_connection_is_ok (connection));
	v_return_if_fail (ctx->reader_queue);

	if (!valvula_connection_set_nonblocking_socket (connection)) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "unable to set non-blocking I/O operation, at connection registration, closing session");
 		return;
	}

	/* increase reference counting */
	if (! valvula_connection_ref (connection, "valvula reader (watch)")) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "unable to increase connection reference count, dropping connection");
		return;
	}

	/* prepare data to be queued */
	data             = axl_new (ValvulaReaderData, 1);
	data->type       = CONNECTION;
	data->connection = connection;

	/* push data */
	valvula_async_queue_push (ctx->reader_queue, data);

	return;
}

/** 
 * @internal
 *
 * Install a new listener to watch for new incoming connections.
 **/
void valvula_reader_watch_listener   (ValvulaCtx        * ctx,
				     ValvulaConnection * listener)
{
	/* get current context */
	ValvulaReaderData * data;
	v_return_if_fail (listener > 0);
	
	/* prepare data to be queued */
	data             = axl_new (ValvulaReaderData, 1);
	data->type       = LISTENER;
	data->connection = listener;

	/* push data */
	valvula_async_queue_push (ctx->reader_queue, data);

	return;
}

/** 
 * @internal
 * 
 * Creates the reader thread process. It will be waiting for any
 * connection that have changed to read its connect and send it
 * appropriate channel reader.
 * 
 * @return The function returns axl_true if the valvula reader was started
 * properly, otherwise axl_false is returned.
 **/
axl_bool  valvula_reader_run (ValvulaCtx * ctx) 
{
	v_return_val_if_fail (ctx, axl_false);

	/* reader_queue */
	ctx->reader_queue   = valvula_async_queue_new ();

	/* reader stopped */
	ctx->reader_stopped = valvula_async_queue_new ();

	/* create the valvula reader main thread */
	if (! valvula_thread_create (&ctx->reader_thread, 
				    (ValvulaThreadFunc) __valvula_reader_run,
				    ctx,
				    VALVULA_THREAD_CONF_END)) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "unable to start valvula reader loop");
		return axl_false;
	} /* end if */
	
	return axl_true;
}

/** 
 * @internal
 * @brief Cleanup valvula reader process.
 */
void valvula_reader_stop (ValvulaCtx * ctx)
{
	/* get current context */
	ValvulaReaderData * data;

	/* skip reader stop as indicated */
	if (ctx->skip_reader_stop)
		return;

	valvula_log (VALVULA_LEVEL_DEBUG, "stopping valvula reader ..");

	/* create a bacon to signal valvula reader that it should stop
	 * and unref resources */
	data       = axl_new (ValvulaReaderData, 1);
	data->type = TERMINATE;

	/* push data */
	valvula_log (VALVULA_LEVEL_DEBUG, "pushing data stop signal..");
	valvula_async_queue_push (ctx->reader_queue, data);
	valvula_log (VALVULA_LEVEL_DEBUG, "signal sent reader ..");

	/* waiting until the reader is stoped */
	valvula_log (VALVULA_LEVEL_DEBUG, "waiting valvula reader 10 seconds to stop");
	if (PTR_TO_INT (valvula_async_queue_timedpop (ctx->reader_stopped, 10000000))) {
		valvula_log (VALVULA_LEVEL_DEBUG, "valvula reader properly stopped, cleaning thread..");
		/* terminate thread */
		valvula_thread_destroy (&ctx->reader_thread, axl_false);

		/* clear queue */
		valvula_async_queue_unref (ctx->reader_stopped);
	} else {
		valvula_log (VALVULA_LEVEL_WARNING, "timeout while waiting valvula reader thread to stop..");
	}

	return;
}

/** 
 * @internal Allows to check notify valvula reader to stop its
 * processing and to change its I/O processing model. 
 * 
 * @return The function returns axl_true to notfy that the reader was
 * notified and axl_false if not. In the later case it means that the
 * reader is not running.
 */
axl_bool  valvula_reader_notify_change_io_api               (ValvulaCtx * ctx)
{
	ValvulaReaderData * data;

	/* check if the valvula reader is running */
	if (ctx == NULL || ctx->reader_queue == NULL)
		return axl_false;

	valvula_log (VALVULA_LEVEL_DEBUG, "stopping valvula reader due to a request for a I/O notify change...");

	/* create a bacon to signal valvula reader that it should stop
	 * and unref resources */
	data       = axl_new (ValvulaReaderData, 1);
	data->type = IO_WAIT_CHANGED;

	/* push data */
	valvula_log (VALVULA_LEVEL_DEBUG, "pushing signal to notify I/O change..");
	valvula_async_queue_push (ctx->reader_queue, data);

	/* waiting until the reader is stoped */
	valvula_async_queue_pop (ctx->reader_stopped);

	valvula_log (VALVULA_LEVEL_DEBUG, "done, now valvula reader will wait until the new API is installed..");

	return axl_true;
}

/** 
 * @internal Allows to notify valvula reader to continue with its
 * normal processing because the new I/O api have been installed.
 */
void valvula_reader_notify_change_done_io_api   (ValvulaCtx * ctx)
{
	ValvulaReaderData * data;

	/* create a bacon to signal valvula reader that it should stop
	 * and unref resources */
	data       = axl_new (ValvulaReaderData, 1);
	data->type = IO_WAIT_READY;

	/* push data */
	valvula_log (VALVULA_LEVEL_DEBUG, "pushing signal to notify I/O is ready..");
	valvula_async_queue_push (ctx->reader_queue, data);

	valvula_log (VALVULA_LEVEL_DEBUG, "notification done..");

	return;
}


/* @} */
