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
#include <valvula.h>

/* local include */
#include <valvula_private.h>

#define LOG_DOMAIN "valvula-listener"

/** 
 * \defgroup valvula_listener ValvulaListener: API to create libValvula listeners.
 */

/** 
 * \addtogroup valvula_listener
 * @{
 */

typedef struct _ValvulaListenerData {
	char                     * host;
	int                        port;
	axlPointer                 user_data;
	axl_bool                   threaded;
	axl_bool                   register_conn;
	ValvulaCtx                * ctx;
}ValvulaListenerData;

int  __valvula_listener_get_port (const char  * port)
{
	return strtol (port, NULL, 10);
}

/** 
 * @brief Public function that performs a TCP listener accept.
 *
 * @param server_socket The listener socket where the accept() operation will be called.
 *
 * @return Returns a connected socket descriptor or -1 if it fails.
 */
VALVULA_SOCKET valvula_listener_accept (VALVULA_SOCKET server_socket)
{
	struct sockaddr_in inet_addr;
#if defined(AXL_OS_WIN32)
	int               addrlen;
#else
	socklen_t         addrlen;
#endif
	addrlen       = sizeof(struct sockaddr_in);

	/* accept the connection new connection */
	return accept (server_socket, (struct sockaddr *)&inet_addr, &addrlen);
}

/** 
 * @brief Starts a generic TCP listener on the provided address and
 * port. This function is used internally by the valvula listener
 * module to startup the valvula listener TCP session associated,
 * however the function can be used directly to start TCP listeners.
 *
 * @param ctx The context where the listener is started.
 *
 * @param host Host address to allocate. It can be "127.0.0.1" to only
 * listen for localhost connections or "0.0.0.0" to listen on any
 * address that the server has installed. It cannot be NULL.
 *
 * @param port The port to listen on. It cannot be NULL and it must be
 * a non-zero string.
 *
 * @param error Optional axlError reference where a textual diagnostic
 * will be reported in case of error.
 *
 * @return The function returns the listener socket or -1 if it
 * fails. Optionally the axlError reports the textual especific error
 * found. If the function returns -2 then some parameter provided was
 * found to be NULL.
 */
VALVULA_SOCKET     valvula_listener_sock_listen      (ValvulaCtx   * ctx,
						    const char  * host,
						    const char  * port,
						    axlError   ** error)
{
	struct hostent     * he;
       struct in_addr     * haddr;
       struct sockaddr_in   saddr;
	struct sockaddr_in   sin;
	VALVULA_SOCKET        fd;
#if defined(AXL_OS_WIN32)
/*	BOOL                 unit      = axl_true; */
	int                  sin_size  = sizeof (sin);
#else    	
	int                  unit      = 1; 
	socklen_t            sin_size  = sizeof (sin);
#endif	
	uint16_t             int_port;
	int                  backlog   = 0;
	int                  bind_res;

	v_return_val_if_fail (ctx,  -2);
	v_return_val_if_fail (host, -2);
	v_return_val_if_fail (port || strlen (port) == 0, -2);

	/* resolve hostname */
	he = gethostbyname (host);
        if (he == NULL) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "unable to get hostname by calling gethostbyname");
		return -1;
	} /* end if */

	haddr = ((struct in_addr *) (he->h_addr_list)[0]);
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) <= 2) {
		/* do not allow creating sockets reusing stdin (0),
		   stdout (1), stderr (2) */
		valvula_log (VALVULA_LEVEL_DEBUG, "failed to create listener socket: %d (errno=%d:%s)", fd, errno, strerror (errno));
		return -1;
        } /* end if */

#if defined(AXL_OS_WIN32)
	/* Do not issue a reuse addr which causes on windows to reuse
	 * the same address:port for the same process. Under linux,
	 * reusing the address means that consecutive process can
	 * reuse the address without being blocked by a wait
	 * state.  */
	/* setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char  *)&unit, sizeof(BOOL)); */
#else
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &unit, sizeof (unit));
#endif 

	/* get integer port */
	int_port  = (uint16_t) atoi (port);

	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family          = AF_INET;
	saddr.sin_port            = htons(int_port);
	memcpy(&saddr.sin_addr, haddr, sizeof(struct in_addr));

	/* call to bind */
	bind_res = bind(fd, (struct sockaddr *)&saddr,  sizeof (struct sockaddr_in));
	valvula_log (VALVULA_LEVEL_DEBUG, "bind(2) call returned: %d", bind_res);
	if (bind_res == VALVULA_SOCKET_ERROR) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "unable to bind address (port:%u already in use or insufficient permissions). Closing socket: %d", int_port, fd);
		valvula_close_socket (fd);
		return -1;
	}
	
	if (listen(fd, backlog) == VALVULA_SOCKET_ERROR) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "an error have occur while executing listen");
		return -1;
        } /* end if */

	/* notify listener */
	if (getsockname (fd, (struct sockaddr *) &sin, &sin_size) < 0) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "an error have happen while executing getsockname");
		return -1;
	} /* end if */

	/* report and return fd */
	valvula_log  (VALVULA_LEVEL_DEBUG, "running listener at %s:%d (socket: %d)", inet_ntoa(sin.sin_addr), ntohs (sin.sin_port), fd);
	return fd;
}

axlPointer __valvula_listener_new (ValvulaListenerData * data)
{
	char               * host          = data->host;
	axl_bool             threaded      = data->threaded;
	axl_bool             register_conn = data->register_conn;
	char               * str_port      = axl_strdup_printf ("%d", data->port);
	const char         * message       = NULL;
	ValvulaConnection   * listener      = NULL;
	ValvulaCtx          * ctx           = data->ctx;
	axlError           * error         = NULL;
	VALVULA_SOCKET        fd;

	/* free data */
	axl_free (data);

	/* allocate listener */
	fd = valvula_listener_sock_listen (ctx, host, str_port, &error);
	
	/* listener ok */
	/* seems listener to be created, now create the BEEP
	 * connection around it */
	listener = valvula_connection_new_empty (ctx, fd, ValvulaRoleMasterListener);
	if (listener == NULL) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "Unable to start listener at the provided location %s:%s", host, str_port);
		return NULL;
	} /* end if */

	/* configure listener */
	listener->port = str_port;
	listener->host = host;

	/* handle returned socket or error */
	switch (fd) {
	case -2:
		valvula_log (VALVULA_LEVEL_CRITICAL, "Failed to start listener because valvula_listener_sock_listener reported NULL parameter received");
		break;
	case -1:
		valvula_log (VALVULA_LEVEL_CRITICAL, "Failed to start listener, valvula_listener_sock_listener reported (code: %d): %s",
			     axl_error_get_code (error), axl_error_get (error));
		break;
	default:
		/* register the listener socket at the Valvula Reader process.  */
		if (register_conn)
			valvula_reader_watch_listener (ctx, listener);

		/* the listener reference */
		valvula_log (VALVULA_LEVEL_DEBUG, "returning listener running at %s:%s (non-threaded mode)", 
			     valvula_connection_get_host (listener), valvula_connection_get_port (listener));
		return listener;
	} /* end switch */

	/* according to the invocation */
	if (! threaded) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "unable to start valvula server, error was: %s, unblocking valvula_listener_wait", message);

		/* notify the listener that an error was found
		 * (because the server didn't suply a handler) */
		valvula_mutex_lock (&ctx->listener_unlock);
		valvula_async_queue_push (ctx->listener_wait_lock, INT_TO_PTR (axl_true));
		ctx->listener_wait_lock = NULL;
		valvula_mutex_unlock (&ctx->listener_unlock);
	} /* end if */

	/* unref error */
	axl_error_free (error);

	/* return listener created */
	return listener;
}

/** 
 * @internal Implementation to support listener creation functions valvula_listener_new*
 */
ValvulaConnection * __valvula_listener_new_common  (ValvulaCtx              * ctx,
						    const char              * host,
						    int                       port,
						    axl_bool                  register_conn)
{
	ValvulaListenerData * data;

	/* check context is initialized */
	if (! valvula_init_check (ctx))
		return NULL;
	
	/* init listener module */
	valvula_listener_init (ctx);
	
	/* prepare function data */
	data                = axl_new (ValvulaListenerData, 1);
	data->host          = axl_strdup (host);
	data->port          = port;
	data->ctx           = ctx;
	data->register_conn = register_conn;
	
	/* make request */
	if (data->threaded) {
		valvula_log (VALVULA_LEVEL_DEBUG, "invoking listener_new threaded mode");
		valvula_thread_pool_new_task (ctx, (ValvulaThreadFunc) __valvula_listener_new, data);
		return NULL;
	}

	valvula_log (VALVULA_LEVEL_DEBUG, "invoking listener_new non-threaded mode");
	return __valvula_listener_new (data);	
}


/** 
 * @internal Creates a new Valvula Listener accepting incoming connections
 * on the given <b>host:port</b> configuration.
 *
 * If user provides an \ref ValvulaListenerReady "on_ready" callback,
 * the listener will be notified on it, in a separated thread, once
 * the process has finished. Check \ref ValvulaListenerReady handler
 * documentation which is on_ready handler type.
 * 
 * On that notification will also be passed the host and port actually
 * allocated. Think about using as host 0.0.0.0 and port 0. These
 * values will cause to \ref valvula_listener_new to allocate the
 * system configured hostname and a random free port. See \ref
 * valvula_handlers "this section" for more info about on_ready
 * parameter.
 *
 * Host and port value provided to this function could be unrefered
 * once returning from this function. The function performs a local
 * copy for those values, that are deallocated at the appropriate
 * moment.
 *
 * Keep in mind that you can actually call several times to this
 * function before calling to \ref valvula_listener_wait, to make your
 * process to be able to accept connections from several ports and
 * host names at the same time. 
 *
 * While providing the port information, make sure your process will
 * have enough rights to allocate the port provided. Usually, ports
 * from 1 to 1024 are reserved to listener programms that runs with
 * priviledges.
 *
 * There is an alternative API that perform the same function but
 * receive the TCP port value as integer: \ref valvula_listener_new2
 *
 * In the case the optional handler <b>on_ready</b> is not provided,
 * the function will return a reference to the \ref ValvulaConnection
 * representing the listener created. This reference will have the
 * role \ref ValvulaRoleMasterListener, (using \ref
 * valvula_connection_get_role) to indicate that the connection
 * reference is a listener.
 *
 * In the case the <b>on_ready</b> handler is provided, the function
 * will return NULL.
 *
 * Here is an example to start a valvula listener server:
 *
 * \code
 * // On this example you'll find:
 * //   - 3 handlers: frame_received, start_channel and
 * //                 close_channel (which handle the event
 * //                 they represent).
 * //   - An entry point (main function) which creates a
 * //     a simple listener server, using previous three
 * //     basic handlers.
 *
 * // valvula context
 * ValvulaCtx * ctx = NULL;
 *
 * // a frame handler
 * void frame_received (ValvulaChannel    * channel,
 *                      ValvulaConnection * connection,
 *                      ValvulaFrame      * frame,
 *                      axlPointer         user_data)
 * {
 *        // Received a frame from the remote side.
 *        // process it and reply to it if it is a MSG.
 *        return;
 * }
 *
 * axl_bool  start_channel (int                channel_num, 
 *                          ValvulaConnection * connection, 
 *                          axlPointer         user_data)
 * {
 *        printf ("Received an start message=%d!!\n",
 *                 channel_num);
 *        // if the async notifier returns axl_true, the channel
 *        // is implicitly created, if axl_false is returned the
 *        // channel creation is denied and a reply error
 *        // is sent.
 *        return axl_true;
 * }
 *
 * axl_bool      close_channel (int                channel_num, 
 *                              ValvulaConnection * connection, 
 *                              axlPointer         user_data)
 * {
 *        printf ("Got a close message notification!!\n");
 *
 *        // if axl_true is returned, the channel is
 *        // accepted to be closed. Otherwise the channel
 *        // will not be closed and an error reply will be
 *        // sent to the remote peer.
 *        return axl_true;
 * }
 * int main (int argc, char ** argv) {
 *
 *      ValvulaConnection * listener;
 *
 *      // create a context
 *      ctx = valvula_ctx_new ();
 *
 *      // enable log to see whats going on 
 *      valvula_log_enable (ctx, axl_true);
 *
 *      // init valvula library (and check its result!)
 *      if (! valvula_init_ctx (ctx)) {
 *           printf ("Unable to initialize valvula library\n");
 *           exit (-1);
 *      }
 * 
 *      // register a profile
 *      valvula_profiles_register (ctx, "http://valvula.aspl.es/profiles/example", 
 *                                start_channel, NULL,
 *                                close_channel, NULL,
 *                                frame_received, NULL);
 * 
 *      // now create a valvula server
 *      listener = valvula_listener_new (ctx, "0.0.0.0", "3000", NULL, NULL);
 *      if (! valvula_connection_is_ok (listener, axl_false)) {
 *             printf ("ERROR: failed to start listener, error found (code: %d) %s\n", 
 *                     valvula_connection_get_status (listener),
 *                     valvula_connection_get_message (listener));
 *             reutrn -1;
 *      } 
 *      
 *      // wait for listener to finish (maybe due to valvula_exit call)
 *      valvula_listener_wait (ctx);
 *  
 *      // end valvula internal subsystem (if no one have done it yet!)
 *      valvula_ctx_exit (ctx, axl_true);
 * 
 *      // that's all to start BEEPing!
 *      return 0;     
 * }  
 * \endcode
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param host The host to listen on.
 *
 * @param port The port to listen on.
 *
 * @param on_ready A optional callback to get a notification when
 * valvula listener is ready to accept requests.
 *
 * @param user_data A user defined pointer to be passed in to
 * <i>on_ready</i> handler.
 *
 * @return The listener connection created (represented by a \ref
 * ValvulaConnection reference). You must use \ref
 * valvula_connection_is_ok to check if the server was started.
 * 
 * <b>NOTE:</b> the reference returned is only owned by the valvula
 * engine. This is not the case of \ref valvula_connection_new where
 * the caller acquires automatically a reference to the connection (as
 * well as the valvula engine). 
 * 
 * In this case, if your intention is to keep a reference for later
 * operations, you must call to \ref valvula_connection_ref to avoid
 * losing the reference if the system drops the connection. In the
 * same direction, you can't call to \ref valvula_connection_close if
 * you don't own the reference returned by this function.
 * 
 * To close immediately a listener you can use \ref
 * valvula_connection_shutdown. In the case the listener was not
 * started (because \ref valvula_connection_is_ok returned axl_false),
 * you must use \ref valvula_connection_close to terminate the
 * reference (NOT valvula_connection_shutdown).
 *
 * <b>Note about old connecting clients, previous to 1.1.3</b>
 *
 * Until Valvula Library 1.1.3, listener accepting incoming connections
 * were sending the BEEP greetings reply just after the remote peer
 * connects. However, this was changed to allow listener side
 * developers to react, modify or deny greetings at such phase (\ref CONNECTION_STAGE_PROCESS_GREETINGS_FEATURES),
 * hooking into that event, waiting first for the client BEEP greetings
 * (especially to check which features it is providing).
 *
 * This behaviour causes problems with clients from previous releases
 * which are waiting for the listener to issue its BEEP peer greetings
 * (where the listener is also waiting) to issue theirs. That is, old
 * client never connects because he never receives greetings from
 * BEEP listener, and BEEP listener never sends its greetings because
 * the client never sent its greetings (circular problem).
 *
 * To solve this issue you can either update the connecting client
 * software or configure your listener to don't wait for client
 * greetings to send its greetings:
 *
 * - \ref valvula_listener_send_greetings_on_connect
 *
 * This problem do not affect to new clients connecting to old servers.
 */
ValvulaConnection * valvula_listener_new (ValvulaCtx          * ctx,
					  const char          * host, 
					  const char          * port)
{
	/* call to int port API */
	return __valvula_listener_new_common (ctx, host, __valvula_listener_get_port (port), axl_true);
}

/** 
 * @internal Creates a new listener, allowing to get the connection that
 * represents the listener created with the optional handler.
 *
 * This function provides the same functionality than \ref
 * valvula_listener_new and \ref valvula_listener_new2 but allowing to
 * get the connection (\ref ValvulaConnection) representing the
 * listener, by configuring the optional handler on_ready_full (\ref
 * ValvulaListenerReadyFull).
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param host The host to listen on.
 *
 * @param port The port to listen on.
 *
 * @param on_ready_full A optional callback to get a notification when
 * valvula listener is ready to accept requests.
 *
 * @param user_data A user defined pointer to be passed in to
 * <i>on_ready</i> handler.
 *
 * @return The listener connection created, or NULL if the optional
 * handler is provided (on_ready).
 *
 * <b>NOTE:</b> the reference returned is only owned by the valvula
 * engine. This is not the case of \ref valvula_connection_new where
 * the caller acquires automatically a reference to the connection (as
 * well as the valvula engine). 
 * 
 * In this case, if your intention is to own a reference to the
 * listener for later operations, you must call to \ref
 * valvula_connection_ref to avoid losing the reference if the system
 * drops the connection. In the same direction, you can't call to \ref
 * valvula_connection_close if you don't own the reference returned by
 * this function.
 * 
 * To close immediately a listener you can use \ref
 * valvula_connection_shutdown.
 */
ValvulaConnection * valvula_listener_new_full  (ValvulaCtx  * ctx,
						const char  * host,
						const char  * port)
{
	/* call to int port API */
	return __valvula_listener_new_common (ctx, host, __valvula_listener_get_port (port), axl_true);
}

/** 
 * @internal Allows to create a BEEP listener optionally not registering
 * it on valvula reader. See \ref valvula_listener_new_full for more
 * details.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param host The host to listen on.
 *
 * @param port The port to listen on.
 *
 * @param register_conn axl_true makes the function to work like \ref
 * valvula_listener_new_full. Otherwise, axl_false makes the listener
 * created to be not registered on valvula reader process.
 *
 * @param on_ready_full A optional callback to get a notification when
 * valvula listener is ready to accept requests.
 *
 * @param user_data A user defined pointer to be passed in to
 * <i>on_ready</i> handler.
 *
 * @return The listener connection created, or NULL if the optional
 * handler is provided (on_ready).
 *
 * <b>IMPORTANT NOTE:</b>
 *
 * All valvula_listener_new* functions have a common behavior which is
 * reference returned is owned by the valvula engine. In this case, if
 * the caller passes register_conn = axl_false makes the reference
 * returned or notified to be owned by the caller.
 */
ValvulaConnection * valvula_listener_new_full2       (ValvulaCtx             * ctx,
						    const char               * host,
						    const char               * port,
						    axl_bool                   register_conn)
{
	/* call to int port API */
	return __valvula_listener_new_common (ctx, host, __valvula_listener_get_port (port), register_conn);
}

/** 
 * @internal Creates a new Valvula Listener accepting incoming connections
 * on the given <b>host:port</b> configuration, receiving the port
 * configuration as an integer value.
 *
 * See \ref valvula_listener_new for more information. 
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param host The host to listen to.
 *
 * @param port The port to listen to. Value defined for the port must be between 0 up to 65536.
 *
 * @param on_ready A optional notify callback to get when valvula
 * listener is ready to perform replies.
 *
 * @param user_data A user defined pointer to be passed in to
 * <i>on_ready</i> handler.
 *
 * @return The listener connection created, or NULL if the optional
 * handler is provided (on_ready).
 *
 * <b>NOTE:</b> the reference returned is only owned by the valvula
 * engine. This is not the case of \ref valvula_connection_new where
 * the caller acquires automatically a reference to the connection (as
 * well as the valvula engine).
 * 
 * In this case, if your intention is to keep a reference for later
 * operations, you must call to \ref valvula_connection_ref to avoid
 * losing the reference if the system drops the connection. In the
 * same direction, you can't call to \ref valvula_connection_close if
 * you don't own the reference returned by this function.
 * 
 * To close immediately a listener you can use \ref valvula_connection_shutdown.
 */
ValvulaConnection * valvula_listener_new2    (ValvulaCtx   * ctx,
					      const char  * host,
					      int           port)
{

	/* call to common API */
	return __valvula_listener_new_common (ctx, host, port, axl_true);
}



/** 
 * @internal Blocks a listener (or listeners) launched until valvula finish.
 * 
 * This function should be called after creating a listener (o
 * listeners) calling to \ref valvula_listener_new to block current
 * thread.
 * 
 * This function can be avoided if the program structure can ensure
 * that the programm will not exist after calling \ref
 * valvula_listener_new. This happens when the program is linked to (or
 * implements) and internal event loop.
 *
 * This function will be unblocked when the valvula listener created
 * ends or a failure have occur while creating the listener. To force
 * an unlocking, a call to \ref valvula_listener_unlock must be done.
 * 
 * @param ctx The context where the operation will be performed.
 */
void valvula_listener_wait (ValvulaCtx * ctx)
{
	ValvulaAsyncQueue * temp;

	/* check reference received */
	if (ctx == NULL)
		return;

	/* check and init listener_wait_lock if it wasn't: init
	   lock */
	valvula_mutex_lock (&ctx->listener_mutex);

	if (PTR_TO_INT (valvula_ctx_get_data (ctx, "vo:listener:skip:wait"))) {
		/* seems someone called to unlock before we get
		 * here */
		/* unlock */
		valvula_mutex_unlock (&ctx->listener_mutex);
		return;
	} /* end if */
	
	/* create listener locker */
	if (ctx->listener_wait_lock == NULL) 
		ctx->listener_wait_lock = valvula_async_queue_new ();

	/* unlock */
	valvula_mutex_unlock (&ctx->listener_mutex);

	/* double locking to ensure waiting */
	valvula_log (VALVULA_LEVEL_DEBUG, "Locking listener");
	if (ctx->listener_wait_lock != NULL) {
		/* get a local reference to the queue and work with it */
		temp = ctx->listener_wait_lock;

		/* get blocked until the waiting lock is released */
		valvula_async_queue_pop   (temp);
		
		/* unref the queue */
		valvula_async_queue_unref (temp);
	}
	valvula_log (VALVULA_LEVEL_DEBUG, "(un)Locked listener");

	return;
}

/** 
 * @internal Unlock the thread blocked at the \ref valvula_listener_wait.
 * 
 * @param ctx The context where the operation will be performed.
 **/
void valvula_listener_unlock (ValvulaCtx * ctx)
{
	/* check reference received */
	if (ctx == NULL || valvula_ctx_ref_count (ctx) < 1)
		return;

	/* unlock listener */
	valvula_mutex_lock (&ctx->listener_unlock);
	if (ctx->listener_wait_lock != NULL) {

		/* push to signal listener unblocking */
		valvula_log (VALVULA_LEVEL_DEBUG, "(un)Locking listener..");

		/* notify waiters */
		if (valvula_async_queue_waiters (ctx->listener_wait_lock) > 0) {
			valvula_async_queue_push (ctx->listener_wait_lock, INT_TO_PTR (axl_true));
		} else {
			/* unref */
			valvula_async_queue_unref (ctx->listener_wait_lock);
		} /* end if */

		/* nullify */
		ctx->listener_wait_lock = NULL;

		valvula_mutex_unlock (&ctx->listener_unlock);
		return;
	} else {
		/* flag this context to unlock valvula_listener_wait
		 * caller because he still didn't reached */
		valvula_log (VALVULA_LEVEL_DEBUG, "valvula_listener_wait was not called, signalling to do fast unlock");
		valvula_ctx_set_data (ctx, "vo:listener:skip:wait", INT_TO_PTR (axl_true));
	}

	valvula_log (VALVULA_LEVEL_DEBUG, "(un)Locking listener: already unlocked..");
	valvula_mutex_unlock (&ctx->listener_unlock);
	return;
}

/** 
 * @internal
 * 
 * Internal valvula function. This function allows
 * __valvula_listener_new_common to initialize valvula listener module
 * only if a listener is installed.
 **/
void valvula_listener_init (ValvulaCtx * ctx)
{
	return;
}

/** 
 * @internal Allows to cleanup the valvula listener state.
 * 
 * @param ctx The valvula context to cleanup.
 */
void valvula_listener_cleanup (ValvulaCtx * ctx)
{
	ValvulaAsyncQueue * queue;
	v_return_if_fail (ctx);

	/* acquire queue and nullify */
	queue = ctx->listener_wait_lock;
	ctx->listener_wait_lock = NULL;
	valvula_log (VALVULA_LEVEL_DEBUG, "listener wait queue ref: %p", queue);
	if (queue) {
		/* remove pending items from the queue */
		while (valvula_async_queue_items (queue) > 0)
			valvula_async_queue_pop (queue);
		/* unref the queue */
		valvula_async_queue_unref (queue);
	}


	return;
}


/* @} */
