/* 
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2025 Advanced Software Production Line, S.L.
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

#ifndef __VALVULA_IO_H__
#define __VALVULA_IO_H__

#include <valvula.h>

/* api to configure current I/O system */
axl_bool             valvula_io_waiting_use                     (ValvulaCtx           * ctx,
								 ValvulaIoWaitingType   type);

axl_bool             valvula_io_waiting_is_available            (ValvulaIoWaitingType type);

ValvulaIoWaitingType  valvula_io_waiting_get_current             (ValvulaCtx           * ctx);

void                 valvula_io_waiting_set_create_fd_group     (ValvulaCtx           * ctx,
								 ValvulaIoCreateFdGroup create);

void                 valvula_io_waiting_set_destroy_fd_group    (ValvulaCtx           * ctx,
								 ValvulaIoDestroyFdGroup destroy);

void                 valvula_io_waiting_set_clear_fd_group      (ValvulaCtx           * ctx,
								 ValvulaIoClearFdGroup clear);

void                 valvula_io_waiting_set_add_to_fd_group     (ValvulaCtx           * ctx,
								 ValvulaIoAddToFdGroup add_to);

void                 valvula_io_waiting_set_is_set_fd_group     (ValvulaCtx           * ctx,
								 ValvulaIoIsSetFdGroup is_set);

void                 valvula_io_waiting_set_have_dispatch       (ValvulaCtx           * ctx,
								 ValvulaIoHaveDispatch  have_dispatch);

void                 valvula_io_waiting_set_dispatch            (ValvulaCtx           * ctx,
								 ValvulaIoDispatch      dispatch);

void                 valvula_io_waiting_set_wait_on_fd_group    (ValvulaCtx           * ctx,
								 ValvulaIoWaitOnFdGroup wait_on);

/* api to perform invocations to the current I/O system configured */
axlPointer           valvula_io_waiting_invoke_create_fd_group  (ValvulaCtx           * ctx,
								 ValvulaIoWaitingFor    wait_to);

void                 valvula_io_waiting_invoke_destroy_fd_group (ValvulaCtx           * ctx,
								 axlPointer            fd_group);

void                 valvula_io_waiting_invoke_clear_fd_group   (ValvulaCtx           * ctx,
								 axlPointer            fd_group);

axl_bool             valvula_io_waiting_invoke_add_to_fd_group  (ValvulaCtx           * ctx,
								 VALVULA_SOCKET         fds, 
								 ValvulaConnection    * connection, 
								 axlPointer            fd_group);

axl_bool             valvula_io_waiting_invoke_is_set_fd_group  (ValvulaCtx           * ctx,
								 VALVULA_SOCKET         fds, 
								 axlPointer fd_group,
								 axlPointer user_data);

axl_bool             valvula_io_waiting_invoke_have_dispatch    (ValvulaCtx           * ctx,
								 axlPointer            fd_group);

void                 valvula_io_waiting_invoke_dispatch         (ValvulaCtx           * ctx,
								 axlPointer            fd_group, 
								 ValvulaIoDispatchFunc  func,
								 int                   changed,
								 axlPointer            user_data);

int                  valvula_io_waiting_invoke_wait             (ValvulaCtx           * ctx,
								 axlPointer            fd_group, 
								 int                   max_fds,
								 ValvulaIoWaitingFor    wait_to);

void                 valvula_io_init (ValvulaCtx * ctx);

/* internal API */
axlPointer __valvula_io_waiting_default_create  (ValvulaCtx * ctx, ValvulaIoWaitingFor wait_to);
void       __valvula_io_waiting_default_destroy (axlPointer fd_group);
void       __valvula_io_waiting_default_clear   (axlPointer __fd_group);
int        __valvula_io_waiting_default_wait_on (axlPointer __fd_group, 
						 int        max_fds, 
						 ValvulaIoWaitingFor wait_to);
axl_bool   __valvula_io_waiting_default_add_to  (int                fds, 
						 ValvulaConnection * connection,
						 axlPointer         __fd_set);
axl_bool   __valvula_io_waiting_default_is_set  (int        fds, 
						 axlPointer __fd_set, 
						 axlPointer user_data);

#endif
