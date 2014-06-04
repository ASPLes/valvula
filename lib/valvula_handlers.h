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
#ifndef __VALVULA_HANDLERS_H__
#define __VALVULA_HANDLERS_H__

/** 
 * \defgroup valvula_handlers ValvulaHandlers: Handler functions used by the librar, server and plugins
 */

/** 
 * \addtogroup valvula_handlers
 * @{
 */

/** 
 * @brief IO handler definition to allow defining the method to be
 * invoked while createing a new fd set.
 *
 * @param ctx The context where the IO set will be created.
 *
 * @param wait_to Allows to configure the file set to be prepared to
 * be used for the set of operations provided. 
 * 
 * @return A newly created fd set pointer, opaque to Valvula, to a
 * structure representing the fd set, that will be used to perform IO
 * waiting operation at the \ref valvula_io "Valvula IO module".
 * 
 */
typedef axlPointer   (* ValvulaIoCreateFdGroup)        (ValvulaCtx * ctx, 
							ValvulaIoWaitingFor wait_to);

/** 
 * @brief IO handler definition to allow defining the method to be
 * invoked while destroying a fd set. 
 *
 * The reference that the handler will receive is the one created by
 * the \ref ValvulaIoCreateFdGroup handler.
 * 
 * @param ValvulaIoDestroyFdGroup The fd_set, opaque to valvula, pointer
 * to a structure representing the fd set to be destroy.
 * 
 */
typedef void     (* ValvulaIoDestroyFdGroup)        (axlPointer             fd_set);

/** 
 * @brief IO handler definition to allow defining the method to be
 * invoked while clearing a fd set.
 * 
 * @param ValvulaIoClearFdGroup The fd_set, opaque to valvula, pointer
 * to a structure representing the fd set to be clear.
 * 
 */
typedef void     (* ValvulaIoClearFdGroup)        (axlPointer             fd_set);



/** 
 * @brief IO handler definition to allow defining the method to be
 * used while performing a IO blocking wait, by default implemented by
 * the IO "select" call.
 *
 * @param ValvulaIoWaitOnFdGroup The handler to set.
 *
 * @param The maximum value for the socket descriptor being watched.
 *
 * @param The requested operation to perform.
 * 
 * @return An error code according to the description found on this
 * function: \ref valvula_io_waiting_set_wait_on_fd_group.
 */
typedef int      (* ValvulaIoWaitOnFdGroup)       (axlPointer             fd_group,
						   int                    max_fds,
						   ValvulaIoWaitingFor     wait_to);

/** 
 * @brief IO handler definition to perform the "add to" the fd set
 * operation.
 * 
 * @param fds The socket descriptor to be added.
 *
 * @param fd_group The socket descriptor group to be used as
 * destination for the socket.
 * 
 * @return returns axl_true if the socket descriptor was added, otherwise,
 * axl_false is returned.
 */
typedef axl_bool      (* ValvulaIoAddToFdGroup)        (int                    fds,
							ValvulaConnection     * connection,
							axlPointer             fd_group);

/** 
 * @brief IO handler definition to perform the "is set" the fd set
 * operation.
 * 
 * @param fds The socket descriptor to be added.
 *
 * @param fd_group The socket descriptor group to be used as
 * destination for the socket.
 *
 * @param user_data User defined pointer provided to the function.
 *
 * @return axl_true if the socket descriptor is active in the given fd
 * group.
 *
 */
typedef axl_bool      (* ValvulaIoIsSetFdGroup)        (int                    fds,
							axlPointer             fd_group,
							axlPointer             user_data);

/** 
 * @brief Handler definition to allow implementing the have dispatch
 * function at the valvula io module.
 *
 * An I/O wait implementation must return axl_true to notify valvula engine
 * it support automatic dispatch (which is a far better mechanism,
 * supporting better large set of descriptors), or axl_false, to notify
 * that the \ref valvula_io_waiting_set_is_set_fd_group mechanism must
 * be used.
 *
 * In the case the automatic dispatch is implemented, it is also
 * required to implement the \ref ValvulaIoDispatch handler.
 * 
 * @param fd_group A reference to the object created by the I/O waiting mechanism.
 * p
 * @return Returns axl_true if the I/O waiting mechanism support automatic
 * dispatch, otherwise axl_false is returned.
 */
typedef axl_bool      (* ValvulaIoHaveDispatch)         (axlPointer             fd_group);

/** 
 * @brief User space handler to implement automatic dispatch for I/O
 * waiting mechanism implemented at valvula io module.
 *
 * This handler definition is used by:
 * - \ref valvula_io_waiting_invoke_dispatch
 *
 * Do not confuse this handler definition with \ref ValvulaIoDispatch,
 * which is the handler definition for the actual implemenation for
 * the I/O mechanism to implement automatic dispatch.
 * 
 * @param fds The socket that is being notified and identified to be dispatched.
 * 
 * @param wait_to The purpose of the created I/O waiting mechanism.
 *
 * @param connection Connection where the dispatch operation takes
 * place.
 * 
 * @param user_data Reference to the user data provided to the dispatch function.
 */
typedef void     (* ValvulaIoDispatchFunc)         (int                    fds,
						    ValvulaIoWaitingFor     wait_to,
						    ValvulaConnection     * connection,
						    axlPointer             user_data);

/** 
 * @brief Handler definition for the automatic dispatch implementation
 * for the particular I/O mechanism selected.
 *
 * This handler is used by:
 *  - \ref valvula_io_waiting_set_dispatch
 *  - \ref valvula_io_waiting_invoke_dispatch (internally)
 *
 * If this handler is implemented, the \ref ValvulaIoHaveDispatch must
 * also be implemented, making it to always return axl_true. If this two
 * handler are implemented, its is not required to implement the "is
 * set?" functionality provided by \ref ValvulaIoIsSetFdGroup (\ref
 * valvula_io_waiting_set_is_set_fd_group).
 * 
 * @param fd_group A reference to the object created by the I/O
 * waiting mechanism.
 * 
 * @param dispatch_func The dispatch user space function to be called.
 *
 * @param changed The number of descriptors that changed, so, once
 * inspected that number, it is not required to continue.
 *
 * @param user_data User defined data provided to the dispatch
 * function once called.
 */
typedef void     (* ValvulaIoDispatch)             (axlPointer             fd_group,
						    ValvulaIoDispatchFunc   dispatch_func,
						    int                    changed,
						    axlPointer             user_data);

/** 
 * @brief Handler used by Valvula library to create a new thread. A custom handler
 * can be specified using \ref valvula_thread_set_create
 *
 * @param thread_def A reference to the thread identifier created by
 * the function. This parameter is not optional.
 *
 * @param func The function to execute.
 *
 * @param user_data User defined data to be passed to the function to
 * be executed by the newly created thread.
 *
 * @return The function returns axl_true if the thread was created
 * properly and the variable thread_def is defined with the particular
 * thread reference created.
 *
 * @see valvula_thread_create
 */
typedef axl_bool (* ValvulaThreadCreateFunc) (ValvulaThread      * thread_def,
                                             ValvulaThreadFunc    func,
                                             axlPointer          user_data,
                                             va_list             args);

/** 
 * @brief Handler used by Valvula Library to release a thread's resources.
 * A custom handler can be specified using \ref valvula_thread_set_destroy
 *
 * @param thread_def A reference to the thread that must be destroyed.
 *
 * @param free_data Boolean that set whether the thread pointer should
 * be released or not.
 *
 * @return axl_true if the destroy operation was ok, otherwise axl_false is
 * returned.
 *
 * @see valvula_thread_destroy
 */
typedef axl_bool (* ValvulaThreadDestroyFunc) (ValvulaThread      * thread_def,
                                              axl_bool            free_data);

/** 
 * @brief Handler definition used by \ref valvula_async_queue_foreach
 * to implement a foreach operation over all items inside the provided
 * queue, blocking its access during its process.
 *
 * @param queue The queue that will receive the foreach operation.
 *
 * @param item_stored The item stored on the provided queue.
 *
 * @param position Item position inside the queue. 0 position is the
 * next item to pop.
 *
 * @param user_data User defined optional data provided to the foreach
 * function.
 */
typedef void (*ValvulaAsyncQueueForeach) (ValvulaAsyncQueue * queue,
					 axlPointer         item_stored,
					 int                position,
					 axlPointer         user_data);

/** 
 * @brief Handler used by async event handlers activated via \ref
 * valvula_thread_pool_new_event, which causes the handler definition
 * to be called at the provided milliseconds period.
 *
 * @param ctx The valvula context where the async event will be fired.
 * @param user_data User defined pointer that was defined at \ref valvula_thread_pool_new_event function.
 * @param user_data2 Second User defined pointer that was defined at \ref valvula_thread_pool_new_event function.
 *
 * @return The function returns axl_true to signal the system to
 * remove the handler. Otherwise, axl_false must be returned to cause
 * the event to be fired again in the future at the provided period.
 */
typedef axl_bool (* ValvulaThreadAsyncEvent)        (ValvulaCtx  * ctx, 
						     axlPointer   user_data,
						     axlPointer   user_data2);

/** 
 * @brief Handler definition for those set of functions that are able
 * to process an incoming request and reports what action should be
 * reported by Valvala to the gateway software (i.e. postfix).
 *
 * This is a key handler and provides one of the basic functions of
 * Valvula. Everything valvula receives a request, it tries to call
 * registered \ref ValvulaProcessRequest handlers to find out what to
 * report to the gateway software (i.e. postfix).
 *
 * @param ctx The context where the operation/request is taking place.
 *
 * @param connection The connection where the operation/request was received.
 *
 * @param request The request that was received and has been asked to be resolved.
 *
 * @param request_data User defined pointer passed to this handler.
 *
 * @param message Optional pointer to a message or particular content
 * that is allowed by the handler itself. The content and how it is
 * formated depends on the value that returns the handler. Check every
 * code available at \ref ValvulaState to know more about the kind of
 * message you can report. In any case, the handler, if wants to report something, must allocate the string using axl_strdup, axl_strdup_printf or axl_new (char, <num of chars). Once the engine is done with the message, memory is released using axl_free.
 *
 * @return An allowed \ref ValvulaState value reporting what to do
 * with the request received.
 */
typedef ValvulaState (* ValvulaProcessRequest) (ValvulaCtx        * ctx, 
						ValvulaConnection * connection, 
						ValvulaRequest    * request,
						axlPointer          request_data,
						char             ** message);

/** 
 * @brief Set of functions defined at \ref
 * valvula_thread_pool_set_cleanup_func that are called every time a
 * thread from the thread pool is stopped.
 *
 * @param ctx The context where the operation takes place.
 */
typedef void         (* ValvulaThreadCleanup) (ValvulaCtx         * ctx);


/** 
 * @brief Set of handlers that valvula library uses to notify final
 * state for a particular request.
 *
 * @param ctx The context where the operation takes place.
 * @param connection The connection where the operation takes place.
 *
 * @param request The request that has a final state from valvula 
 * 
 * @param state The state that is the response to this request.
 *
 * @param message Optional message associated to the state.
 */
typedef void         (* ValvulaReportFinalState) (ValvulaCtx * ctx,
						  ValvulaConnection * connection, 
						  ValvulaRequest    * request, 
						  ValvulaState        state, 
						  const char        * message,
						  axlPointer          user_data);

#endif

/** 
 * @}
 */
