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
#ifndef __VALVULA_PRIVATE_H__
#define __VALVULA_PRIVATE_H__

/** 
 * @internal Definition of Valvula context. 
 */
struct _ValvulaCtx {
	axl_bool debug_checked;
	axl_bool debug;

	axl_bool debug2_checked;
	axl_bool debug2;

	axl_bool debug_color_checked;
	axl_bool debug_color;

	axl_bool valvula_initialized;
	axl_bool valvula_exit;

	ValvulaState default_state;

	/** mutexes **/
	ValvulaMutex exit_mutex;
	ValvulaMutex ref_mutex;
	int          ref_count;
	ValvulaMutex inet_ntoa_mutex;

	ValvulaHash            * process_handler_registry;
	ValvulaRequestRegistry * first_handler;

	ValvulaMutex         listener_unlock;
	ValvulaAsyncQueue  * listener_wait_lock;
	ValvulaMutex         listener_mutex;

	/*** queues ***/
	ValvulaAsyncQueue * reader_stopped;	
	ValvulaAsyncQueue * reader_queue;	

	ValvulaMutex        connection_hostname_mutex;
	axlHash           * connection_hostname;

	/*** lists ***/
	axlList           * srv_list;
	axlList           * conn_list;
	axlListCursor     * srv_cursor;
	axlListCursor     * conn_cursor;

	axlPointer          on_reading;

	ValvulaThread       reader_thread;

	/**** valvula io waiting module state ****/
	ValvulaIoWaitingType waiting_type;
	ValvulaIoCreateFdGroup  waiting_create;
	ValvulaIoDestroyFdGroup waiting_destroy;
	ValvulaIoClearFdGroup   waiting_clear;
	ValvulaIoWaitOnFdGroup  waiting_wait_on;
	ValvulaIoAddToFdGroup   waiting_add_to;
	ValvulaIoIsSetFdGroup   waiting_is_set;
	ValvulaIoHaveDispatch   waiting_have_dispatch;
	ValvulaIoDispatch       waiting_dispatch;

	/*** thread pool ***/
	ValvulaThreadPool       * thread_pool;
	axl_bool                  thread_pool_being_stopped;
	axl_bool                  skip_thread_pool_wait;
	axl_bool                  thread_pool_exclusive;

	/*** valvula hash ***/
	ValvulaHash             * data;

};

/** 
 * @internal Definition of Valvula connection
 */
struct _ValvulaConnection {
	ValvulaCtx * ctx;

	VALVULA_SOCKET session;

	ValvulaMutex   ref_mutex;
	int            ref_count;
	
	ValvulaMutex    op_mutex;

	char          * host;
	char          * port;
	char          * host_ip;
	char          * local_addr;
	char          * local_port;

	ValvulaPeerRole role;
	
	ValvulaConnection * listener;

	char              * pending_line;

	ValvulaRequest    * request;
	axl_bool            process_launched;
};

struct _ValvulaHash {
	axlHash          * table;
	ValvulaMutex        mutex;
	int                ref_count;

	/* configuration functions */
	axlHashFunc        hash_func;
	axlEqualFunc       key_equal_func;

	/* destroy functions */
	axlDestroyFunc     key_destroy;
	axlDestroyFunc     value_destroy;
	
	/* watchers */
	ValvulaAsyncQueue * changed_queue;
};

struct _ValvulaRequestRegistry {
	ValvulaCtx              * ctx;
	ValvulaProcessRequest     process_handler;
	int                       priority;
	int                       port;
	axlPointer                user_data;
};

#endif
