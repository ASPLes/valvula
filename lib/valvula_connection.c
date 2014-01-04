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

/* global include */
#include <valvula.h>

/* private include */
#include <valvula_private.h>

#if defined(AXL_OS_UNIX)
# include <netinet/tcp.h>
#endif

#define LOG_DOMAIN "valvula-connection"
#define VALVULA_CONNECTION_BUFFER_SIZE 32768

/** 
 * \defgroup valvula_connection_opts Valvula Connection Options: connection create options
 */


/** 
 * @internal Support function for connection identificators.
 *
 * This is used to generate and return the next connection identifier.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @return Next connection identifier available.
 */
int  __valvula_connection_get_next_id (ValvulaCtx * ctx)
{
	/* get current context */
	int         result;

	/* lock */
	valvula_mutex_lock (&ctx->connection_id_mutex);
	
	/* increase */
	result = ctx->connection_id;
	ctx->connection_id++;

	/* unlock */
	valvula_mutex_unlock (&ctx->connection_id_mutex);

	return result;
}


/** 
 * \brief Allows to change connection semantic to blocking.
 *
 * This function should not be useful for Valvula Library consumers
 * because the internal Valvula Implementation requires connections to
 * be non-blocking semantic.
 * 
 * @param connection the connection to set as blocking
 * 
 * @return axl_true if blocking state was set or axl_false if not.
 */
axl_bool      valvula_connection_set_blocking_socket (ValvulaConnection    * connection)
{
#if defined(ENABLE_VALVULA_LOG)
	ValvulaCtx * ctx;
#endif
#if defined(AXL_OS_UNIX)
	int  flags;
#endif
	/* check the connection reference */
	if (connection == NULL)
		return axl_false;

#if defined(ENABLE_VALVULA_LOG)
	/* get a reference to the ctx */
	ctx = connection->ctx;
#endif
	
#if defined(AXL_OS_WIN32)
	if (!valvula_win32_blocking_enable (connection->session)) {
		__valvula_connection_shutdown_and_record_error (
			connection, ValvulaError, "unable to set blocking I/O");
		return axl_false;
	}
#else
	if ((flags = fcntl (connection->session, F_GETFL, 0)) < 0) {
		__valvula_connection_shutdown_and_record_error (
			connection, ValvulaError,
			"unable to get socket flags to set non-blocking I/O");
		return axl_false;
	}
	valvula_log (VALVULA_LEVEL_DEBUG, "actual flags state before setting blocking: %d", flags);
	flags &= ~O_NONBLOCK;
	if (fcntl (connection->session, F_SETFL, flags) < 0) {
		__valvula_connection_shutdown_and_record_error (
			connection, ValvulaError, "unable to set non-blocking I/O");
		return axl_false;
	}
	valvula_log (VALVULA_LEVEL_DEBUG, "actual flags state after setting blocking: %d", flags);
#endif
	valvula_log (VALVULA_LEVEL_DEBUG, "setting connection as blocking");
	return axl_true;
}

/** 
 * \brief Allows to change connection semantic to nonblocking.
 *
 * Sets a connection to be non-blocking while sending and receiving
 * data. This function should not be useful for Valvula Library
 * consumers.
 * 
 * @param connection the connection to set as nonblocking.
 * 
 * @return axl_true if nonblocking state was set or axl_false if not.
 */
axl_bool      valvula_connection_set_nonblocking_socket (ValvulaConnection * connection)
{
#if defined(ENABLE_VALVULA_LOG)
	ValvulaCtx * ctx;
#endif

#if defined(AXL_OS_UNIX)
	int  flags;
#endif
	/* check the reference */
	if (connection == NULL)
		return axl_false;

#if defined(ENABLE_VALVULA_LOG)
	/* get a reference to context */
	ctx = connection->ctx;
#endif
	
#if defined(AXL_OS_WIN32)
	if (!valvula_win32_nonblocking_enable (connection->session)) {
		__valvula_connection_shutdown_and_record_error (
			connection, ValvulaError, "unable to set non-blocking I/O");
		return axl_false;
	}
#else
	if ((flags = fcntl (connection->session, F_GETFL, 0)) < 0) {
		__valvula_connection_shutdown_and_record_error (
			connection, ValvulaError,
			"unable to get socket flags to set non-blocking I/O");
		return axl_false;
	}

	valvula_log (VALVULA_LEVEL_DEBUG, "actual flags state before setting nonblocking: %d", flags);
	flags |= O_NONBLOCK;
	if (fcntl (connection->session, F_SETFL, flags) < 0) {
		__valvula_connection_shutdown_and_record_error (
			connection, ValvulaError, "unable to set non-blocking I/O");
		return axl_false;
	}
	valvula_log (VALVULA_LEVEL_DEBUG, "actual flags state after setting nonblocking: %d", flags);
#endif
	valvula_log (VALVULA_LEVEL_DEBUG, "setting connection as non-blocking");
	return axl_true;
}

/** 
 * @brief Allows to configure tcp no delay flag (enable/disable Nagle
 * algorithm).
 * 
 * @param socket The socket to be configured.
 *
 * @param enable The value to be configured, axl_true to enable tcp no
 * delay.
 * 
 * @return axl_true if the operation is completed.
 */
axl_bool                 valvula_connection_set_sock_tcp_nodelay   (VALVULA_SOCKET socket,
								   axl_bool      enable)
{
	/* local variables */
	int result;

#if defined(AXL_OS_WIN32)
	BOOL   flag = enable ? TRUE : FALSE;
	result      = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char  *)&flag, sizeof(BOOL));
#else
	int    flag = enable;
	result      = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof (flag));
#endif
	if (result < 0) {
		return axl_false;
	}

	/* properly configured */
	return axl_true;
} /* end */

/** 
 * @brief Allows to enable/disable non-blocking/blocking behavior on
 * the provided socket.
 * 
 * @param socket The socket to be configured.
 *
 * @param enable axl_true to enable blocking I/O, otherwise use
 * axl_false to enable non blocking I/O.
 * 
 * @return axl_true if the operation was properly done, otherwise axl_false is
 * returned.
 */
axl_bool                 valvula_connection_set_sock_block         (VALVULA_SOCKET socket,
								   axl_bool      enable)
{
#if defined(AXL_OS_UNIX)
	int  flags;
#endif

	if (enable) {
		/* enable blocking mode */
#if defined(AXL_OS_WIN32)
		if (!valvula_win32_blocking_enable (socket)) {
			return axl_false;
		}
#else
		if ((flags = fcntl (socket, F_GETFL, 0)) < 0) {
			return axl_false;
		} /* end if */

		flags &= ~O_NONBLOCK;
		if (fcntl (socket, F_SETFL, flags) < 0) {
			return axl_false;
		} /* end if */
#endif
	} else {
		/* enable nonblocking mode */
#if defined(AXL_OS_WIN32)
		/* win32 case */
		if (!valvula_win32_nonblocking_enable (socket)) {
			return axl_false;
		}
#else
		/* unix case */
		if ((flags = fcntl (socket, F_GETFL, 0)) < 0) {
			return axl_false;
		}
		
		flags |= O_NONBLOCK;
		if (fcntl (socket, F_SETFL, flags) < 0) {
			return axl_false;
		}
#endif
	} /* end if */

	return axl_true;
}


/** 
 * @internal wrapper to avoid possible problems caused by the
 * gethostbyname implementation which is not required to be reentrant
 * (thread safe).
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param hostname The host to translate.
 * 
 * @return A reference to the struct hostent or NULL if it fails to
 * resolv the hostname.
 */
struct in_addr * valvula_gethostbyname (ValvulaCtx  * ctx, 
				       const char * hostname)
{
	/* get current context */
	struct in_addr * result;
	struct hostent * _result;

	/* check that context and hostname are valid */
	if (ctx == NULL || hostname == NULL)
		return NULL;
	
	/* lock and resolv */
	valvula_mutex_lock (&ctx->connection_hostname_mutex);

	/* resolv using the hash */
	result = axl_hash_get (ctx->connection_hostname, (axlPointer) hostname);
	if (result == NULL) {
		_result = gethostbyname (hostname);
		if (_result != NULL) {
			/* alloc and get the address */
			result         = axl_new (struct in_addr, 1);
			if (result == NULL) {
				valvula_mutex_unlock (&ctx->connection_hostname_mutex);
				return NULL;
			} /* end if */
			result->s_addr = ((struct in_addr *) (_result->h_addr_list)[0])->s_addr;

			/* now store the result */
			axl_hash_insert_full (ctx->connection_hostname, 
					      /* the hostname */
					      axl_strdup (hostname), axl_free,
					      /* the address */
					      result, axl_free);
		} /* end if */
	} /* end if */

	/* unlock and return the result */
	valvula_mutex_unlock (&ctx->connection_hostname_mutex);

	return result;
	
}

/** 
 * @brief Allows to create a plain socket connection against the host
 * and port provided. 
 *
 * @param ctx The context where the connection happens.
 *
 * @param host The host server to connect to.
 *
 * @param port The port server to connect to.
 *
 * @param timeout Parameter where optionally is returned the timeout
 * defined by the library (\ref valvula_connection_get_connect_timeout)
 * that remains after only doing a socket connected. The value is only
 * returned if the caller provide a reference.
 *
 * @param error Optional axlError reference to report an error code
 * and a textual diagnostic.
 *
 * @return A connected socket or -1 if it fails. The particular error
 * is reported at axlError optional reference.
 */
VALVULA_SOCKET valvula_connection_sock_connect (ValvulaCtx   * ctx,
					      const char  * host,
					      const char  * port,
					      int         * timeout,
					      axlError   ** error)
{
	struct in_addr     * haddr;
	struct sockaddr_in   saddr;
	int		     err          = 0;
	VALVULA_SOCKET        session;

	/*
	 * standard tcp socket voodo connection (I would like to know
	 * who was the great mind designer of this api)
	 */
	haddr = valvula_gethostbyname (ctx, host);
        if (haddr == NULL) {
		valvula_log (VALVULA_LEVEL_WARNING, "unable to get host name by using gethostbyname host=%s",
			    host);
		axl_error_report (error, ValvulaNameResolvFailure, "unable to get host name by using gethostbyname");
		return -1;
	}

	/* create the socket and check if it */
	session      = socket (AF_INET, SOCK_STREAM, 0);
	if (session == VALVULA_INVALID_SOCKET) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "unable to create socket");
		axl_error_report (error, ValvulaNameResolvFailure, "unable to create socket (socket call have failed)");
		return -1;
	} /* end if */

	/* check socket limit */
	if (! valvula_connection_check_socket_limit (ctx, session)) {
		axl_error_report (error, ValvulaSocketSanityError, "Unable to create more connections, socket limit reached");
		return -1;
	}

	/* do a sanity check on socket created */
	if (!valvula_connection_do_sanity_check (ctx, session)) {
		/* close the socket */
		valvula_close_socket (session);

		/* report error */
		axl_error_report (error, ValvulaSocketSanityError, 
				  "created socket descriptor using a reserved socket descriptor (%d), this is likely to cause troubles");
		return -1;
	} /* end if */
	
	/* disable nagle */
	valvula_connection_set_sock_tcp_nodelay (session, axl_true);

	/* prepare socket configuration to operate using TCP/IP
	 * socket */
        memset(&saddr, 0, sizeof(saddr));
        memcpy(&saddr.sin_addr, haddr, sizeof(struct in_addr));
        saddr.sin_family    = AF_INET;
        saddr.sin_port      = htons((uint16_t) strtod (port, NULL));
	
	/* get current valvula connection timeout to check if the
	 * application have requested to configure a particular TCP
	 * connect timeout. */
	if (timeout) {
		(*timeout)  = valvula_connection_get_connect_timeout (ctx); 
		if ((*timeout) > 0) {
			/* translate hold value for timeout into seconds  */
			(*timeout) = (int) (*timeout) / (int) 1000000;
			
			/* set non blocking connection */
			valvula_connection_set_sock_block (session, axl_false);
		} /* end if */
	} /* end if */

	/* do a tcp connect */
        if (connect (session, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		if(timeout == 0 || (errno != VALVULA_EINPROGRESS && errno != VALVULA_EWOULDBLOCK)) { 
			shutdown (session, SHUT_RDWR);
			valvula_close_socket (session);
			valvula_log (VALVULA_LEVEL_WARNING, "unable to connect to remote host errno=%d, timeout reached",
				    errno);
			axl_error_report (error, ValvulaConnectionError, "unable to connect to remote host");
			return -1;
		} /* end if */
	} /* end if */

	/* if a connection timeout is defined, wait until connect */
	if (timeout && ((*timeout) > 0)) {
		/* wait for write operation, signaling that the
		 * connection is available */
		err = __valvula_connection_wait_on (ctx, WRITE_OPERATIONS, session, timeout);

#if defined(AXL_OS_WIN32)
		/* under windows we have to also we to be readable */

		/* NOTE: the following code was commented because
		 * starting from 1.1.3 BEEP listener do send content
		 * inmmediately (greetings) but waits for client
		 * greetings to reply with the proper values. The
		 * following waiting code causes select(2) call to not
		 * report TCP proper connection until some data is
		 * received, which is obviously a windows winsock
		 * bug. */
		if (err > 0) {  
/*			valvula_log (VALVULA_LEVEL_DEBUG, "connect ok, but need to check readable state for socket %d..", session); */
/*			err = __valvula_connection_wait_on (ctx, READ_OPERATIONS, session, timeout); */
		} /* end if */
#endif
		
		if(err <= 0){
			/* timeout reached while waiting for the connection to terminate */
			shutdown (session, SHUT_RDWR);
			valvula_close_socket (session);
			valvula_log (VALVULA_LEVEL_WARNING, "unable to connect to remote host (timeout)");
			axl_error_report (error, ValvulaNameResolvFailure, "unable to connect to remote host (timeout)");
			return -1;
		} /* end if */
	} /* end if */

	/* return socket created */


	return session;
}
			

/**
 * @internal Reference counting update implementation.
 */
axl_bool               valvula_connection_ref_internal                    (ValvulaConnection * connection, 
									  const char       * who,
									  axl_bool           check_ref)
{
#if defined(ENABLE_VALVULA_LOG)
	ValvulaCtx * ctx;
#endif

	v_return_val_if_fail (connection, axl_false);
	if (check_ref)
		v_return_val_if_fail (valvula_connection_is_ok (connection, axl_false), axl_false);

#if defined(ENABLE_VALVULA_LOG)
	/* get a reference to the ctx */
	ctx = connection->ctx;
#endif
	
	/* lock ref/unref operations over this connection */
	valvula_mutex_lock   (&connection->ref_mutex);

	/* increase and log the connection increased */
	connection->ref_count++;

	valvula_log (VALVULA_LEVEL_DEBUG, "%d increased connection id=%d (%p) reference to %d by %s\n",
		    valvula_getpid (),
		    connection->id, connection,
		    connection->ref_count, who ? who : "??" ); 

	/* unlock ref/unref options over this connection */
	valvula_mutex_unlock (&connection->ref_mutex);

	return axl_true;
}

/** 
 * @brief Increase internal valvula connection reference counting.
 * 
 * Because Valvula Library design, several on going threads shares
 * references to the same connection for several purposes. 
 * 
 * Connection reference counting allows to every on going thread to
 * notify the system that connection reference is no longer be used
 * so, if the reference counting reach a zero value, connection
 * resources will be deallocated.
 *
 * While using the Valvula Library is not required to use this function
 * especially for those applications which are built on top of a
 * profile which is layered on Valvula Library. 
 *
 * This is because connection handling is done through functions such
 * \ref valvula_connection_new and \ref valvula_connection_close (which
 * automatically handles connection reference counting for you).
 *
 * However, while implementing new profiles these function becomes a
 * key concept to ensure the profile implementation don't get lost
 * connection references.
 *
 * Keep in mind that using this function implied to use \ref
 * valvula_connection_unref function in all code path implemented. For
 * each call to \ref valvula_connection_ref it should exist a call to
 * \ref valvula_connection_unref. Failing on doing this will cause
 * either memory leak or memory corruption because improper connection
 * deallocations.
 * 
 * The function return axl_true to signal that the connection reference
 * count was increased in one unit. If the function return axl_false, the
 * connection reference count wasn't increased and a call to
 * valvula_connection_unref should not be done. Here is an example:
 * 
 * \code
 * // try to ref the connection
 * if (! valvula_connection_ref (connection, "some known module or file")) {
 *    // unable to ref the connection
 *    return;
 * }
 *
 * // connection referenced, do work 
 *
 * // finally unref the connection
 * valvula_connection_unref (connection, "some known module or file");
 * \endcode
 *
 * @param connection the connection to operate.
 * @param who who have increased the reference.
 *
 * @return axl_true if the connection reference was increased or axl_false if
 * an error was found.
 */
axl_bool               valvula_connection_ref                    (ValvulaConnection * connection, 
								 const char       * who)
{
	/* checked ref */
	return valvula_connection_ref_internal (connection, who, axl_true);
}

/**
 * @brief Allows to perform a ref count operation on the connection
 * provided without checking if the connection is working (no call to
 * \ref valvula_connection_is_ok).
 *
 * @param connection The connection to update.

 * @return axl_true if the reference update operation is completed,
 * otherwise axl_false is returned.
 */
axl_bool               valvula_connection_uncheck_ref           (ValvulaConnection * connection)
{
	/* unchecked ref */
	return valvula_connection_ref_internal (connection, "unchecked", axl_false);
}

/** 
 * @brief Decrease valvula connection reference counting.
 *
 * Allows to decrease connection reference counting. If this reference
 * counting goes under 0 the connection resources will be deallocated. 
 *
 * See also \ref valvula_connection_ref
 * 
 * @param connection The connection to operate.
 * @param who        Who have decreased the reference. This is a string value used to log which entity have decreased the connection counting.
 */
void               valvula_connection_unref                  (ValvulaConnection * connection, 
							     char const       * who)
{

#if defined(ENABLE_VALVULA_LOG)
	ValvulaCtx  * ctx;
#endif
	int          count;

	/* do not operate if no reference is received */
	if (connection == NULL)
		return;

	/* lock the connection being unrefered */
	valvula_mutex_lock     (&(connection->ref_mutex));

#if defined(ENABLE_VALVULA_LOG)
	/* get context */
	ctx = connection->ctx;
#endif

	/* decrease reference counting */
	connection->ref_count--;

	valvula_log (VALVULA_LEVEL_DEBUG, "%d decreased connection id=%d (%p) reference count to %d decreased by %s\n", 
		valvula_getpid (),
		connection->id, connection,
		connection->ref_count, who ? who : "??");  
		
	/* get current count */
	count = connection->ref_count;
	valvula_mutex_unlock (&(connection->ref_mutex));

	/* if counf is 0, free the connection */
	if (count == 0) {
		valvula_connection_free (connection);
	} /* end if */

	return;
}


/** 
 * @brief Allows to get current reference count for the provided connection.
 *
 * See also the following functions:
 *  - \ref valvula_connection_ref
 *  - \ref valvula_connection_unref
 * 
 * @param connection The connection that is requested to return its
 * count.
 *
 * @return The function returns the reference count or -1 if it fails.
 */
int                 valvula_connection_ref_count              (ValvulaConnection * connection)
{
	int result;

	/* check reference received */
	if (connection == NULL)
		return -1;

	/* return the reference count */
	valvula_mutex_lock     (&(connection->ref_mutex));
	result = connection->ref_count;
	valvula_mutex_unlock     (&(connection->ref_mutex));
	return result;
}

/** 
 * @brief Allows to get current connection status
 *
 * This function will allow you to check if your valvula connection is
 * actually connected. You must use this function before calling
 * valvula_connection_new to check what have actually happen.
 *
 * You can also use valvula_connection_get_message to check the message
 * returned by the valvula layer. This may be useful on connection
 * errors.  The free_on_fail parameter can be use to free valvula
 * connection resources if this valvula connection is not
 * connected. This operation will be done by using \ref valvula_connection_close.
 *
 * 
 * @param connection the connection to get current status.
 *
 * @param free_on_fail if axl_true the connection will be closed using
 * valvula_connection_close on not connected status.
 * 
 * @return current connection status for the given connection
 */
axl_bool                valvula_connection_is_ok (ValvulaConnection * connection, 
						 axl_bool           free_on_fail)
{
	axl_bool  result = axl_false;

	/* check connection null referencing. */
	if  (connection == NULL) 
		return axl_false;

	/* check for the socket this connection has */
	valvula_mutex_lock  (&(connection->ref_mutex));
	result = (connection->session < 0) || (! connection->is_connected);
	valvula_mutex_unlock  (&(connection->ref_mutex));

	/* implement free_on_fail flag */
	if (free_on_fail && result) {
		valvula_connection_close (connection);
		return axl_false;
	} /* end if */
	
	/* return current connection status. */
	return ! result;
}

/** 
 * @brief Returns the socket used by this ValvulaConnection object.
 * 
 * @param connection the connection to get the socket.
 * 
 * @return the socket used or -1 if fail
 */
VALVULA_SOCKET    valvula_connection_get_socket           (ValvulaConnection * connection)
{
	/* check reference received */
	if (connection == NULL)
		return -1;

	return connection->session;
}

/** 
 * @brief Returns the actual host this connection is connected to.
 *
 * In the case the connection you have provided have the \ref
 * ValvulaRoleMasterListener role (\ref valvula_connection_get_role),
 * that is, listener connections that are waiting for connections, the
 * function will return the actual host used by the listener.
 *
 * You must not free returned value.  If you do so, you will get
 * unexpected behaviors.
 * 
 * 
 * @param connection the connection to get host value.
 * 
 * @return the host the given connection is connected to or NULL if something fail.
 */
const char         * valvula_connection_get_host             (ValvulaConnection * connection)
{
	if (connection == NULL)
		return NULL;

	return connection->host;
}

/** 
 * @internal Function used to setup manuall values returned by \ref
 * valvula_connection_get_host and \ref valvula_connection_get_port.
 */
void                valvula_connection_set_host_and_port      (ValvulaConnection * connection, 
							      const char       * host,
							      const char       * port,
							      const char       * host_ip)
{
	v_return_if_fail (connection && host && port);
	
	if (connection->host)
		axl_free (connection->host);
	if (connection->port)
		axl_free (connection->port);
	if (connection->host_ip)
		axl_free (connection->host_ip);
	
	/* set host, port and ip value */
	connection->host    = axl_strdup (host);
	connection->port    = axl_strdup (port);
	connection->host_ip = axl_strdup (host_ip);

	return;
}

/** 
 * @brief Allows to get the actual host ip this connection is
 * connected to.
 *
 * This function works like \ref valvula_connection_get_host_ip but
 * returning the actual ip in the case a name was used.
 *
 * @return A reference to the IP or NULL if it fails.
 */
const char        * valvula_connection_get_host_ip            (ValvulaConnection * connection)
{
	struct sockaddr_in     sin;
#if defined(AXL_OS_WIN32)
	/* windows flavors */
	int                    sin_size     = sizeof (sin);
#else
	/* unix flavors */
	socklen_t              sin_size     = sizeof (sin);
#endif
	/* acquire lock to check if host ip was defined previously */
	valvula_mutex_lock (&connection->op_mutex);
	if (connection->host_ip) {
		valvula_mutex_unlock (&connection->op_mutex);
		return connection->host_ip;
	} /* end if */

	/* get actual IP value */
	if (getpeername (connection->session, (struct sockaddr *) &sin, &sin_size) < 0) {
		valvula_mutex_unlock (&connection->op_mutex);
		return NULL;
	} /* end if */

	/* set local addr and local port */
	connection->host_ip = valvula_support_inet_ntoa (connection->ctx, &sin);

	/* unlock and return value created */
	valvula_mutex_unlock (&connection->op_mutex);
	return connection->host_ip;
}

/** 
 * @brief  Returns the connection unique identifier.
 *
 * The connection identifier is a unique integer assigned to all
 * connection created under Valvula Library. This allows Valvula programmer to
 * use this identifier for its own application purposes
 *
 * @param connection The connection to get the the unique integer
 * identifier from.
 * 
 * @return the unique identifier.
 */
int                valvula_connection_get_id               (ValvulaConnection * connection)
{
	if (connection == NULL)
		return -1;

	return connection->id;
}

/** 
 * @brief Allows to get the serverName under which the remote BEEP
 * peer is working. 
 *
 * During the BEEP session, the first channel created under a provided
 * serverName attribute is meaningful for the rest of the
 * session. This means that the connection gets flagged with the
 * serverName under which is acting.
 * 
 * @param connection The connection that is required to return its
 * server name value.
 * 
 * @return The serverName value or NULL if no server name was
 * configured.
 */
const char        * valvula_connection_get_server_name        (ValvulaConnection * connection)
{
	if (connection == NULL)
		return NULL;
	
	/* return current serverName configured */
	return connection->serverName;
}

/** 
 * @internal Function that allows to configure the serverName for the
 * first caller. Rest of the callers will fail to set the name (the
 * function will do nothing) if the serverName is found to be
 * configured.
 * 
 * @param connection The connection to configure its serverName.
 * @param serverName The server name value to configured.
 */
void                valvula_connection_set_server_name         (ValvulaConnection * connection,
							       const char       * serverName)
{
#if defined(ENABLE_VALVULA_LOG)
	ValvulaCtx * ctx = CONN_CTX (connection);
#endif
	int iterator = 0;

	/* check if the connection is null or the serverName is
	 * null */
	if (connection == NULL || serverName == NULL || connection->serverName != NULL)
		return;

	/* configure serverName */
	connection->serverName = axl_strdup (serverName);

	/* remove : values and the rest behind it */
	while (connection->serverName[iterator]) {
		if (connection->serverName[iterator] == ':')
			connection->serverName[iterator] = 0;

		/* next iterator*/
		iterator++;
	}
	valvula_log (VALVULA_LEVEL_DEBUG, "Received serverName %s and configured %s", serverName, connection->serverName);
	
	return;
}

/** 
 * @brief Returns the actual port this connection is connected to.
 *
 * In the case the connection you have provided have the \ref
 * ValvulaRoleMasterListener role (\ref valvula_connection_get_role),
 * that is, a listener that is waiting for connections, the
 * function will return the actual port used by the listener.
 *
 * @param connection the connection to get the port value.
 * 
 * @return the port or NULL if something fails.
 */
const char        * valvula_connection_get_port             (ValvulaConnection * connection)
{
	
	/* check reference received */
	if (connection == NULL)
		return NULL;

	return connection->port;
}

/** 
 * @brief Allows to get local address used by the connection.
 * @param connection The connection to check.
 *
 * @return A reference to the local address used or NULL if it fails.
 */
const char        * valvula_connection_get_local_addr         (ValvulaConnection * connection)
{
	/* check reference received */
	if (connection == NULL)
		return NULL;

	return connection->local_addr;
}

/** 
 * @brief Allows to get the local port used by the connection. 
 * @param connection The connection to check.
 * @return A reference to the local port used or NULL if it fails.
 */
const char        * valvula_connection_get_local_port         (ValvulaConnection * connection)
{
	/* check reference received */
	if (connection == NULL)
		return NULL;

	return connection->local_port;
}

/** 
 * @brief Sets user defined data associated with the given connection.
 *
 * The function allows to store arbitrary data associated to the given
 * connection. Data stored will be indexed by the provided key,
 * allowing to retrieve the information using: \ref
 * valvula_connection_get_data.
 *
 * If the value provided is NULL, this will be considered as a
 * removing request for the given key and its associated data.
 * 
 * See also \ref valvula_connection_set_data_full function. It is an
 * alternative API that allows configuring a set of destroy handlers
 * for key and data stored.
 *
 * @param connection The connection where data will be associated.
 *
 * @param key The string key indexing the data stored associated to
 * the given key.
 *
 * @param value The value to be stored. NULL to remove previous data
 * stored.
 */
void               valvula_connection_set_data               (ValvulaConnection * connection,
							     const char       * key,
							     axlPointer         value)
{
	/* use the full version so all source code is supported in one
	 * function. */
	valvula_connection_set_data_full (connection, (axlPointer) key, value, NULL, NULL);
	return;
}

/** 
 * @brief Allows to remove a key/value pair installed by
 * valvula_connection_set_data and valvula_connection_set_data_full
 * without calling destroy functions associated.
 *
 * @param connection The connection where the key/value entry will be
 * removed without calling destroy function associated (if any) to
 * both (key and value).
 *
 * @param key The key that identifies the entry to be deleted.
 * 
 */
void                valvula_connection_delete_key_data        (ValvulaConnection * connection,
							      const char       * key)
{
	if (connection == NULL || connection->data == NULL)
		return;
	valvula_hash_delete (connection->data, (axlPointer) key);
	return;
}
/** 
 * @brief Allows to store user space data into the connection like
 * \ref valvula_connection_set_data does but configuring functions to
 * be called once required to deallocate data stored.
 *
 * While storing user defined data into the connection it could be
 * necessary to also define destroy functions for the value stored and
 * the key stored. This allows to not worry about to free those data
 * (including the key) once the connection is dropped.
 *
 * This function allows to store data into the given connection
 * defining destroy functions for the key and the value stored, per item.
 * 
 * \code
 * [...]
 * void destroy_data_1 (axlPointer data) 
 * {
 *     // perform a memory deallocation for data1
 * }
 * 
 * void destroy_data_2 (axlPointer data) 
 * {
 *     // perform a memory deallocation for data2
 * }
 * [...]
 * // store data 1 providing a destroy value function
 * valvula_connection_set_data_full (connection, "some:data:1", 
 *                                  data_1, NULL, destroy_data_1);
 *
 * // store data 2 providing a destroy value function
 * valvula_connection_set_data_full (connection, "some:data:2",
 *                                  data_2, NULL, destroy_data_2);
 * [...]
 * \endcode
 * 
 *
 * @param connection    The connection where the data will be stored.
 * @param key           The unique string key value.
 * @param value         The value to be stored associated to the given key.
 * @param key_destroy   An optional key destroy function used to destroy (deallocate) memory used by the key.
 * @param value_destroy An optional value destroy function used to destroy (deallocate) memory used by the value.
 */
void                valvula_connection_set_data_full          (ValvulaConnection * connection,
							      char             * key,
							      axlPointer         value,
							      axlDestroyFunc     key_destroy,
							      axlDestroyFunc     value_destroy)
{

	/* check reference */
	if (connection == NULL || key == NULL)
		return;

	/* check if the value is not null. It it is null, remove the
	 * value. */
	if (value == NULL) {
		valvula_hash_remove (connection->data, key);
		return;
	}

	/* store the data selected replacing previous one */
	valvula_hash_replace_full (connection->data, 
				  key, key_destroy, 
				  value, value_destroy);
	
	/* return from setting the value */
	return;
}

/** 
 * @brief Allows to set a commonly used user land pointer associated
 * to the provided connection.
 *
 * Though you can use \ref valvula_connection_set_data_full or \ref valvula_connection_set_data, this function allows to set a pointer
 * that can be retreived by using \ref valvula_connection_get_hook with a low cpu usage.
 *
 * @param connection The connection where the user land pointer is associated.
 *
 * @param ptr The pointer that will be associated to the connection.
 */
void                valvula_connection_set_hook               (ValvulaConnection * connection,
							      axlPointer         ptr)
{
	if (connection == NULL)
		return;
	connection->hook = ptr;
	return;
}

/** 
 * @brief Allows to get the user land pointer configured by \ref valvula_connection_set_hook.
 *
 * @param connection The connection where the user land pointer is
 * being queried.
 *
 * @return The pointer stored.
 */
axlPointer          valvula_connection_get_hook               (ValvulaConnection * connection)
{
	if (connection == NULL)
		return NULL;
	return connection->hook;
}


/** 
 * @brief Gets stored value indexed by the given key inside the given connection.
 *
 * The function returns stored data using \ref
 * valvula_connection_set_data or \ref valvula_connection_set_data_full.
 * 
 * @param connection the connection where the value will be looked up.
 * @param key the key to look up.
 * 
 * @return the value or NULL if fails.
 */
axlPointer         valvula_connection_get_data               (ValvulaConnection * connection,
							     const char       * key)
{
 	v_return_val_if_fail (connection,       NULL);
 	v_return_val_if_fail (key,              NULL);
 	v_return_val_if_fail (connection->data, NULL);

	return valvula_hash_lookup (connection->data, (axlPointer) key);
}

/** 
 * @brief Allows to get current data hash object used by the provided
 * connection. This way you can use this hash object directly with
 * \ref valvula_hash "valvula hash API".
 *
 * @param connection The connection where the hash has been requested..
 *
 * @return A reference to the hash or NULL if it fails.
 */
ValvulaHash        * valvula_connection_get_data_hash          (ValvulaConnection * connection)
{
	v_return_val_if_fail (connection, NULL);
	return connection->data;
}

/** 
 * @brief Allows to get current connection role.
 * 
 * @param connection The ValvulaConnection to get the current role from.
 * 
 * @return Current role represented by \ref ValvulaPeerRole. If the
 * function receives a NULL reference it will return \ref
 * ValvulaRoleUnknown.
 */
ValvulaPeerRole      valvula_connection_get_role               (ValvulaConnection * connection)
{
	/* if null reference received, return unknown role */
	v_return_val_if_fail (connection, ValvulaRoleUnknown);

	return connection->role;
}

/** 
 * @brief In the case the connection was automatically created at the
 * listener BEEP side, the connection was accepted under an especific
 * listener started with \ref valvula_listener_new (and its associated
 * functions).
 *
 * This function allows to get a reference to the listener that
 * accepted and created the current connection. In the case this
 * function is called over a client connection NULL, will be returned.
 * 
 * @param connection The connection that is requried to return the
 * master connection associated (the master connection will have the
 * role: \ref ValvulaRoleMasterListener).
 * 
 * @return The reference or NULL if it fails. 
 */
ValvulaConnection  * valvula_connection_get_listener           (ValvulaConnection * connection)
{
	/* check reference received */
	if (connection == NULL)
		return NULL;
	
	/* returns current master connection associated */
	return connection->listener;
}

/** 
 * @brief Allows to get the context under which the connection was
 * created. 
 * 
 * @param connection The connection that is requried to return the context under
 * which it was created.
 * 
 * @return A reference to the context associated to the connection or
 * NULL if it fails.
 */
ValvulaCtx         * valvula_connection_get_ctx                (ValvulaConnection * connection)
{
	/* check value received */
	if (connection == NULL)
		return NULL;

	/* reference to the connection */
	return connection->ctx;
}



/* @} */

