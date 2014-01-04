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
		valvula_log (VALVULA_LEVEL_CRITICAL, "unable to get socket flags to set non-blocking I/O");
		return axl_false;
	}
	valvula_log (VALVULA_LEVEL_DEBUG, "actual flags state before setting blocking: %d", flags);
	flags &= ~O_NONBLOCK;
	if (fcntl (connection->session, F_SETFL, flags) < 0) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "unable to set non-blocking I/O");
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
		valvula_log (VALVULA_LEVEL_CRITICAL, "unable to get socket flags to set non-blocking I/O");
		return axl_false;
	}

	valvula_log (VALVULA_LEVEL_DEBUG, "actual flags state before setting nonblocking: %d", flags);
	flags |= O_NONBLOCK;
	if (fcntl (connection->session, F_SETFL, flags) < 0) {
		valvula_log (VALVULA_LEVEL_CRITICAL,  "unable to set non-blocking I/O");
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

	/* unlock ref/unref options over this connection */
	valvula_mutex_unlock (&connection->ref_mutex);

	return axl_true;
}


/** 
 * @brief Allows to create a new working connection with the provided
 * socket, role and context.
 */
ValvulaConnection  * valvula_connection_new_empty              (ValvulaCtx      * ctx,
								VALVULA_SOCKET    _socket,
								ValvulaPeerRole   role)
{
	ValvulaConnection * conn;

	if (ctx == NULL || _socket < 0)
		return NULL;

	conn = axl_new (ValvulaConnection, 1);
	if (conn == NULL)
		return NULL;

	valvula_mutex_create (&conn->ref_mutex);
	conn->ref_count = 1;
	conn->ctx       = ctx;
	conn->session   = _socket;

	return conn;
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
	result = (connection->session < 0);
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
 * @brief Allows to close the provided connection.
 *
 * @param connection The connection to be closed. After this, the
 * connection cannot be used anymore.
 * 
 */
void                valvula_connection_close                  (ValvulaConnection * connection)
{
	if (connection == NULL)
		return;

	valvula_close_socket (connection->session);
	connection->session = -1;

	return;
}

void                valvula_connection_free (ValvulaConnection * conn)
{
	if (conn == NULL)
		return;

	valvula_mutex_destroy (&conn->ref_mutex);

	valvula_mutex_destroy (&conn->op_mutex);

	axl_free (conn->host);
	axl_free (conn->host_ip);
	axl_free (conn->port);
	axl_free (conn->local_addr);
	axl_free (conn->local_port);

	axl_free (conn);
	return;
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

