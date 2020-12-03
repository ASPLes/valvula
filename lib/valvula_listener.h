/* 
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2020 Advanced Software Production Line, S.L.
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
#ifndef __VALVULA_LISTENER_H__
#define __VALVULA_LISTENER_H__

#include <valvula.h>

/**
 * \addtogroup valvula_listener
 * @{
 */

ValvulaConnection * valvula_listener_new             (ValvulaCtx           * ctx,
						      const char           * host, 
						      const char           * port);

ValvulaConnection * valvula_listener_new2            (ValvulaCtx           * ctx,
						      const char           * host,
						      int                    port);

ValvulaConnection * valvula_listener_new_full        (ValvulaCtx                * ctx,
						      const char               * host,
						      const char               * port);

ValvulaConnection * valvula_listener_new_full2       (ValvulaCtx                * ctx,
						      const char               * host,
						      const char               * port,
						      axl_bool                   register_conn);

VALVULA_SOCKET     valvula_listener_sock_listen      (ValvulaCtx   * ctx,
						      const char  * host,
						      const char  * port,
						      axlError   ** error);

VALVULA_SOCKET valvula_listener_accept               (VALVULA_SOCKET server_socket);

void          valvula_listener_wait                 (ValvulaCtx * ctx);

void          valvula_listener_unlock               (ValvulaCtx * ctx);

void          valvula_listener_init                 (ValvulaCtx * ctx);

void          valvula_listener_cleanup              (ValvulaCtx * ctx);


/* @} */

#endif
