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
#include <valvula.h>

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
	      IO_WAIT_READY,
	      FOREACH
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
#if defined(ENABLE_VALVULA_LOG)
	ValvulaCtx        * ctx;
#endif

	/* get a reference to the connection (no matter if it is not
	 * defined) */
	connection = data->connection;
#if defined(ENABLE_VALVULA_LOG)
	ctx        = connection->ctx;
#endif

	switch (data->type) {
	case CONNECTION:
		/* check the connection */
		if (!valvula_connection_is_ok (connection, axl_false)) {
			/* check if we can free this connection */
			valvula_connection_unref (connection, "valvula reader (watch)");
			valvula_log (VALVULA_LEVEL_DEBUG, "received a non-valid connection, ignoring it");

			/* release data */
			axl_free (data);
			return axl_false;
		}
			
		/* now we have a first connection, we can start to wait */
		valvula_log (VALVULA_LEVEL_DEBUG, "new connection (conn-id=%d) to be watched (%d)", 
			    valvula_connection_get_id (connection), valvula_connection_get_socket (connection));
		axl_list_append (con_list, connection);

		break;
	case LISTENER:
		valvula_log (VALVULA_LEVEL_DEBUG, "new listener connection to be watched (socket: %d --> %s:%s, conn-id: %d)",
			    valvula_connection_get_socket (connection), 
			    valvula_connection_get_host (connection), 
			    valvula_connection_get_port (connection),
			    valvula_connection_get_id (connection));
		axl_list_append (srv_list, connection);
		break;
	case TERMINATE:
	case IO_WAIT_CHANGED:
	case IO_WAIT_READY:
	case FOREACH:
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


/* do a foreach operation */
void valvula_reader_foreach_impl (ValvulaCtx        * ctx, 
				  axlList           * con_list, 
				  axlList           * srv_list, 
				  ValvulaReaderData * data)
{
	axlListCursor * cursor;

	valvula_log (VALVULA_LEVEL_DEBUG, "doing valvula reader foreach notification..");

	/* check for null function */
	if (data->func == NULL) 
		goto foreach_impl_notify;

	/* foreach the connection list */
	cursor = axl_list_cursor_new (con_list);
	while (axl_list_cursor_has_item (cursor)) {

		/* notify, if the connection is ok */
		if (valvula_connection_is_ok (axl_list_cursor_get (cursor), axl_false)) {
			data->func (axl_list_cursor_get (cursor), data->user_data);
		} /* end if */

		/* next cursor */
		axl_list_cursor_next (cursor);
	} /* end while */
	
	/* free cursor */
	axl_list_cursor_free (cursor);

	/* foreach the connection list */
	cursor = axl_list_cursor_new (srv_list);
	while (axl_list_cursor_has_item (cursor)) {
		/* notify, if the connection is ok */
		if (valvula_connection_is_ok (axl_list_cursor_get (cursor), axl_false)) {
			data->func (axl_list_cursor_get (cursor), data->user_data);
		} /* end if */

		/* next cursor */
		axl_list_cursor_next (cursor);
	} /* end while */

	/* free cursor */
	axl_list_cursor_free (cursor);

	/* notify that the foreach operation was completed */
 foreach_impl_notify:
	valvula_async_queue_push (data->notify, INT_TO_PTR (1));

	return;
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
		} else if (data->type == FOREACH) {
			/* do a foreach operation */
			valvula_reader_foreach_impl (ctx, con_list, srv_list, data);

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

		} else if (data->type == FOREACH) {
			/* do a foreach operation */
			valvula_reader_foreach_impl (ctx, con_list, srv_list, data);

		} /* end if */

		/* watch the request received, maybe a connection or a
		 * valvula reader command to process  */
		valvula_reader_register_watch (data, con_list, srv_list);
		
	} /* end while */

	return should_continue;
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
	long               time_stamp  = 0;

	/* get current time stamp if idle handler is defined */
	if (ctx->global_idle_handler)
		time_stamp = (long) time (NULL);
	
	axl_list_cursor_first (cursor);
	while (axl_list_cursor_has_item (cursor)) {

		/* get current connection */
		connection = axl_list_cursor_get (cursor);

		/* check for idle status */
		if (ctx->global_idle_handler)
			valvula_connection_check_idle_status (connection, ctx, time_stamp);

		/* check ok status */
		if (! valvula_connection_is_ok (connection, axl_false)) {

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (cursor);

			/* connection isn't ok, unref it */
			valvula_connection_unref (connection, "valvula reader (build set)");

			continue;
		} /* end if */

		/* check if the connection must be unwatched */
		if (connection->reader_unwatch) {
			/* remove the unwatch flag from the connection */
			connection->reader_unwatch = axl_false;

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (cursor);

			/* connection isn't ok, unref it */
			valvula_connection_unref (connection, "valvula reader (process: unwatch)");

			continue;
		} /* end if */

		/* check if the connection is blocked (no I/O read to
		 * perform on it) */
		if (valvula_connection_is_blocked (connection)) {
			/* valvula_log (VALVULA_LEVEL_DEBUG, "connection id=%d has I/O read blocked (valvula_connection_block)", 
			   valvula_connection_get_id (connection)); */
			/* get the next */
			axl_list_cursor_next (cursor);
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
			if (valvula_connection_is_ok (connection, axl_false))
				__valvula_connection_shutdown_and_record_error (connection, ValvulaError, "valvula reader (add fail)");
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
		if (!valvula_connection_is_ok (connection, axl_false)) {
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

	/* check all listeners */
	axl_list_cursor_first (srv_cursor);
	while (axl_list_cursor_has_item (srv_cursor)) {

		/* get the connection */
		connection = axl_list_cursor_get (srv_cursor);

		if (!valvula_connection_is_ok (connection, axl_false)) {
			valvula_log (VALVULA_LEVEL_DEBUG, "valvula reader found listener id=%d not operational, unreference",
				    valvula_connection_get_id (connection));

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
			valvula_listener_accept_connections (ctx, fds, connection);

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
	QUEUE_PUSH (ctx->reader_stopped, INT_TO_PTR (1));

	return;
}

void __valvula_reader_close_connection (axlPointer pointer)
{
	ValvulaConnection * conn = pointer;

	/* unref the connection */
	valvula_connection_shutdown (conn);
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
		/* check if there are pre read handler to be executed on this 
		   connection. */
		if (valvula_connection_is_defined_preread_handler (connection)) {
			/* if defined preread handler invoke it and return. */
			valvula_connection_invoke_preread_handler (connection);
			return;
		} /* end if */

		/* listener connections */
		valvula_listener_accept_connections (ctx, fds, connection);
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
	if (valvula_connection_is_ok (conn, axl_false)) {

		/* get the connection and socket. */
		fds    = valvula_connection_get_socket (conn);
#if defined(AXL_OS_UNIX)
		errno  = 0;
#endif
		result = recv (fds, bytes, 1, MSG_PEEK);
		if (result == -1 && errno == EBADF) {
			  
			/* get context */
#if defined(ENABLE_VALVULA_LOG)
			ctx = CONN_CTX (conn);
#endif
			valvula_log (VALVULA_LEVEL_CRITICAL, "Found connection-id=%d, with session=%d not working (errno=%d), shutting down",
				    valvula_connection_get_id (conn), fds, errno);
			/* close connection, but remove the socket reference to avoid closing some's socket */
			conn->session = -1;
			valvula_connection_shutdown (conn);
			
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
	if (ctx->on_reading != NULL)
		valvula_io_waiting_invoke_destroy_fd_group (ctx, ctx->on_reading);
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
			/* check if we have to terminate the process
			 * in the case no more connections are
			 * available: useful when the current instance
			 * is running in the context of turbulence */
			valvula_ctx_check_on_finish (ctx);

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
					    errno, valvula_errno_get_last_error ());
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
 * @internal Function used to unwatch the provided connection from the
 * valvula reader loop.
 *
 * @param ctx The context where the unread operation will take place.
 * @param connection The connection where to be unwatched from the reader...
 */
void valvula_reader_unwatch_connection          (ValvulaCtx        * ctx,
						ValvulaConnection * connection)
{
	v_return_if_fail (ctx && connection);
	/* flag connection valvula reader unwatch */
	connection->reader_unwatch = axl_true;
	return;
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

	v_return_if_fail (valvula_connection_is_ok (connection, axl_false));
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

	valvula_log (VALVULA_LEVEL_DEBUG, "Accepting conn-id=%d into reader queue %p, library status: %d", 
		    valvula_connection_get_id (connection),
		    ctx->reader_queue,
		    valvula_is_exiting (ctx));

	/* prepare data to be queued */
	data             = axl_new (ValvulaReaderData, 1);
	data->type       = CONNECTION;
	data->connection = connection;

	/* push data */
	QUEUE_PUSH (ctx->reader_queue, data);

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
	QUEUE_PUSH (ctx->reader_queue, data);

	return;
}

/** 
 * @internal Function used to configure the connection so the next
 * call to terminate the list of connections will not close the
 * connection socket.
 */
axl_bool __valvula_reader_configure_conn (axlPointer ptr, axlPointer data)
{
	/* set the connection socket to be not closed */
	valvula_connection_set_close_socket ((ValvulaConnection *) ptr, axl_false);
	return axl_false; /* not found so all items are iterated */
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

	/* check connection list to be previously created to terminate
	   it without closing sockets associated to each connection */
	if (ctx->conn_list != NULL) {
		valvula_log (VALVULA_LEVEL_DEBUG, "releasing previous client connections, installed: %d",
			    axl_list_length (ctx->conn_list));
		ctx->reader_cleanup = axl_true;
		axl_list_lookup (ctx->conn_list, __valvula_reader_configure_conn, NULL);
		axl_list_cursor_free (ctx->conn_cursor);
		axl_list_free (ctx->conn_list);
		ctx->conn_list   = NULL;
		ctx->conn_cursor = NULL;
	} /* end if */
	if (ctx->srv_list != NULL) {
		valvula_log (VALVULA_LEVEL_DEBUG, "releasing previous listener connections, installed: %d",
			    axl_list_length (ctx->srv_list));
		ctx->reader_cleanup = axl_true;
		axl_list_lookup (ctx->srv_list, __valvula_reader_configure_conn, NULL);
		axl_list_cursor_free (ctx->srv_cursor);
		axl_list_free (ctx->srv_list);
		ctx->srv_list   = NULL;
		ctx->srv_cursor = NULL;
	} /* end if */

	/* clear reader cleanup flag */
	ctx->reader_cleanup = axl_false;

	/* reader_queue */
	if (ctx->reader_queue != NULL)
		valvula_async_queue_release (ctx->reader_queue);
	ctx->reader_queue   = valvula_async_queue_new ();

	/* reader stopped */
	if (ctx->reader_stopped != NULL) 
		valvula_async_queue_release (ctx->reader_stopped);
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

	valvula_log (VALVULA_LEVEL_DEBUG, "stopping valvula reader ..");

	/* create a bacon to signal valvula reader that it should stop
	 * and unref resources */
	data       = axl_new (ValvulaReaderData, 1);
	data->type = TERMINATE;

	/* push data */
	valvula_log (VALVULA_LEVEL_DEBUG, "pushing data stop signal..");
	QUEUE_PUSH (ctx->reader_queue, data);
	valvula_log (VALVULA_LEVEL_DEBUG, "signal sent reader ..");

	/* waiting until the reader is stoped */
	valvula_log (VALVULA_LEVEL_DEBUG, "waiting valvula reader 60 seconds to stop");
	if (PTR_TO_INT (valvula_async_queue_timedpop (ctx->reader_stopped, 60000000))) {
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
	QUEUE_PUSH (ctx->reader_queue, data);

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
	QUEUE_PUSH (ctx->reader_queue, data);

	valvula_log (VALVULA_LEVEL_DEBUG, "notification done..");

	return;
}


/* @} */
