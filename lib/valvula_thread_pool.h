/* 
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
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
#ifndef __VALVULA_THREAD_POOL_H__
#define __VALVULA_THREAD_POOL_H__

#include <valvula.h>

BEGIN_C_DECLS

typedef struct _ValvulaThreadPool ValvulaThreadPool;

void valvula_thread_pool_init                (ValvulaCtx * ctx, int  max_threads);

void valvula_thread_pool_add                 (ValvulaCtx * ctx, int threads);

void valvula_thread_pool_setup               (ValvulaCtx * ctx, 
					     int         thread_max_limit, 
					     int         thread_add_step,
					     int         thread_add_period, 
					     axl_bool    auto_remove);

void valvula_thread_pool_setup2              (ValvulaCtx * ctx, 
					     int         thread_max_limit, 
					     int         thread_add_step,
					     int         thread_add_period, 
					     int         thread_remove_step,
					     int         thread_remove_period, 
					     axl_bool    auto_remove,
					     axl_bool    preemtive); 

void valvula_thread_pool_remove              (ValvulaCtx * ctx, int threads);

void valvula_thread_pool_exit                (ValvulaCtx * ctx);

void valvula_thread_pool_being_closed        (ValvulaCtx * ctx);

void valvula_thread_pool_new_task            (ValvulaCtx        * ctx,
					      ValvulaThreadFunc   func, 
					      axlPointer         data);

int  valvula_thread_pool_new_event           (ValvulaCtx              * ctx,
					     long                     microseconds,
					     ValvulaThreadAsyncEvent   event_handler,
					     axlPointer               user_data,
					     axlPointer               user_data2);

axl_bool valvula_thread_pool_remove_event        (ValvulaCtx              * ctx,
						 int                      event_id);

void valvula_thread_pool_stats               (ValvulaCtx        * ctx,
					      int              * running_threads,
					      int              * waiting_threads,
					      int              * pending_tasks);

void valvula_thread_pool_event_stats         (ValvulaCtx        * ctx,
					     int              * events_installed);

int  valvula_thread_pool_get_running_threads (ValvulaCtx        * ctx);

void valvula_thread_pool_set_num             (int  number);

int  valvula_thread_pool_get_num             (void);

void valvula_thread_pool_set_exclusive_pool  (ValvulaCtx        * ctx,
					     axl_bool           value);

void valvula_thread_pool_set_cleanup_func    (ValvulaCtx        * ctx,
					      ValvulaThreadCleanup     func);

/* internal API */
void valvula_thread_pool_add_internal        (ValvulaCtx        * ctx, 
					     int                threads);
void valvula_thread_pool_remove_internal     (ValvulaCtx        * ctx, 
					     int                threads);

void __valvula_thread_pool_automatic_resize  (ValvulaCtx * ctx);

END_C_DECLS

#endif
