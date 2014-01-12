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

#ifndef __VALVULA_CONNECTION_H__
#define __VALVULA_CONNECTION_H__

#include <valvula.h>

/** 
 * \addtogroup valvula_connection
 * @{
 */

ValvulaConnection  * valvula_connection_new_empty              (ValvulaCtx      * ctx,
								VALVULA_SOCKET    _socket,
								ValvulaPeerRole   role);

VALVULA_SOCKET       valvula_connection_sock_connect           (ValvulaCtx   * ctx,
								const char  * host,
								const char  * port,
								int         * timeout,
								axlError   ** error);

axl_bool            valvula_connection_ref                    (ValvulaConnection * connection,
							      const char         * who);

axl_bool            valvula_connection_uncheck_ref            (ValvulaConnection * connection);

void                valvula_connection_unref                  (ValvulaConnection * connection,
							      const char       * who);

int                 valvula_connection_ref_count              (ValvulaConnection * connection);

axl_bool            valvula_connection_is_ok                  (ValvulaConnection * connection);

void                valvula_connection_close                  (ValvulaConnection * connection);

void                valvula_connection_free                   (ValvulaConnection * connection);

VALVULA_SOCKET       valvula_connection_get_socket             (ValvulaConnection * connection);

const char        * valvula_connection_get_host               (ValvulaConnection * connection);

const char        * valvula_connection_get_host_ip            (ValvulaConnection * connection);

const char        * valvula_connection_get_port               (ValvulaConnection * connection);

const char        * valvula_connection_get_local_addr         (ValvulaConnection * connection);

const char        * valvula_connection_get_local_port         (ValvulaConnection * connection);

void                valvula_connection_set_host_and_port      (ValvulaConnection * connection, 
							      const char       * host,
							      const char       * port,
							      const char       * host_ip);

axl_bool            valvula_connection_set_blocking_socket    (ValvulaConnection * connection);

axl_bool            valvula_connection_set_nonblocking_socket (ValvulaConnection * connection);

axl_bool            valvula_connection_set_sock_tcp_nodelay   (VALVULA_SOCKET socket,
							      axl_bool      enable);

axl_bool            valvula_connection_set_sock_block         (VALVULA_SOCKET socket,
							      axl_bool      enable);

ValvulaPeerRole      valvula_connection_get_role               (ValvulaConnection * connection);

ValvulaConnection  * valvula_connection_get_listener          (ValvulaConnection * connection);

ValvulaCtx         * valvula_connection_get_ctx               (ValvulaConnection * connection);

/** 
 * @internal
 * Do not use the following functions, internal Valvula Library purposes.
 */

/** private API **/
axl_bool               valvula_connection_ref_internal                    (ValvulaConnection * connection, 
									  const char         * who,
									  axl_bool           check_ref);


#endif

/* @} */
