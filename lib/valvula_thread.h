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
#ifndef __VALVULA_THREAD_H__
#define __VALVULA_THREAD_H__

#include <valvula.h>

BEGIN_C_DECLS

/**
 * \addtogroup valvula_thread
 * @{
 */

axl_bool           valvula_thread_create   (ValvulaThread      * thread_def,
					   ValvulaThreadFunc    func,
					   axlPointer          user_data,
					   ...);

axl_bool           valvula_thread_destroy  (ValvulaThread      * thread_def, 
					   axl_bool            free_data);

void               valvula_thread_set_create (ValvulaThreadCreateFunc  create_fn);

void               valvula_thread_set_destroy(ValvulaThreadDestroyFunc destroy_fn);

axl_bool           valvula_mutex_create    (ValvulaMutex       * mutex_def);

axl_bool           valvula_mutex_destroy   (ValvulaMutex       * mutex_def);

void               valvula_mutex_lock      (ValvulaMutex       * mutex_def);

void               valvula_mutex_unlock    (ValvulaMutex       * mutex_def);

axl_bool           valvula_cond_create     (ValvulaCond        * cond);

void               valvula_cond_signal     (ValvulaCond        * cond);

void               valvula_cond_broadcast  (ValvulaCond        * cond);

/** 
 * @brief Useful macro that allows to perform a call to
 * valvula_cond_wait registering the place where the call was started
 * and ended.
 * 
 * @param c The cond variable to use.
 * @param mutex The mutex variable to use.
 */
#define VALVULA_COND_WAIT(c, mutex) do{\
valvula_cond_wait (c, mutex);\
}while(0);

axl_bool           valvula_cond_wait       (ValvulaCond        * cond, 
					   ValvulaMutex       * mutex);

/** 
 * @brief Useful macro that allows to perform a call to
 * valvula_cond_timewait registering the place where the call was
 * started and ended. 
 * 
 * @param r Wait result
 * @param c The cond variable to use.
 * @param mutex The mutex variable to use.
 * @param m The amount of microseconds to wait.
 */
#define VALVULA_COND_TIMEDWAIT(r, c, mutex, m) do{\
r = valvula_cond_timedwait (c, mutex, m);\
}while(0)


axl_bool           valvula_cond_timedwait  (ValvulaCond        * cond, 
					   ValvulaMutex       * mutex,
					   long                microseconds);

void               valvula_cond_destroy    (ValvulaCond        * cond);

ValvulaAsyncQueue * valvula_async_queue_new       (void);

axl_bool           valvula_async_queue_push      (ValvulaAsyncQueue * queue,
						 axlPointer         data);

axl_bool           valvula_async_queue_priority_push  (ValvulaAsyncQueue * queue,
						      axlPointer         data);

axl_bool           valvula_async_queue_unlocked_push  (ValvulaAsyncQueue * queue,
						      axlPointer         data);

axlPointer         valvula_async_queue_pop          (ValvulaAsyncQueue * queue);

axlPointer         valvula_async_queue_unlocked_pop (ValvulaAsyncQueue * queue);

axlPointer         valvula_async_queue_timedpop  (ValvulaAsyncQueue * queue,
						 long               microseconds);

int                valvula_async_queue_length    (ValvulaAsyncQueue * queue);

int                valvula_async_queue_waiters   (ValvulaAsyncQueue * queue);

int                valvula_async_queue_items     (ValvulaAsyncQueue * queue);

axl_bool           valvula_async_queue_ref       (ValvulaAsyncQueue * queue);

int                valvula_async_queue_ref_count (ValvulaAsyncQueue * queue);

void               valvula_async_queue_unref      (ValvulaAsyncQueue * queue);

void               valvula_async_queue_release    (ValvulaAsyncQueue * queue);

void               valvula_async_queue_safe_unref (ValvulaAsyncQueue ** queue);

void               valvula_async_queue_foreach   (ValvulaAsyncQueue         * queue,
						 ValvulaAsyncQueueForeach    foreach_func,
						 axlPointer                 user_data);

axlPointer         valvula_async_queue_lookup    (ValvulaAsyncQueue         * queue,
						 axlLookupFunc              lookup_func,
						 axlPointer                 user_data);

void               valvula_async_queue_lock      (ValvulaAsyncQueue * queue);

void               valvula_async_queue_unlock    (ValvulaAsyncQueue * queue);

END_C_DECLS

#endif

/**
 * @}
 */ 
