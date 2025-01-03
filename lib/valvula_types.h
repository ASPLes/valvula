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
#ifndef __VALVULA_TYPES_H__
#define __VALVULA_TYPES_H__

/** 
 * \defgroup valvula_types ValvulaTypes: Common types used by libValvula API
 */

/** 
 * \addtogroup valvula_types
 * @{
 */

/** 
 * @internal Definitions to accomodate the underlaying thread
 * interface to the Valvula thread API.
 */
#if defined(AXL_OS_WIN32)

#define __OS_THREAD_TYPE__ win32_thread_t
#define __OS_MUTEX_TYPE__  HANDLE
#define __OS_COND_TYPE__   win32_cond_t

typedef struct _win32_thread_t {
	HANDLE    handle;
	void*     data;
	unsigned  id;	
} win32_thread_t;

/** 
 * @internal pthread_cond_t definition, fully based on the work done
 * by Dr. Schmidt. Take a look into his article (it is an excelent article): 
 * 
 *  - http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
 * 
 * Just a wonderful work. 
 *
 * Ok. The following is a custom implementation to solve windows API
 * flaw to support conditional variables for critical sections. The
 * solution provided its known to work under all windows platforms
 * starting from NT 4.0. 
 *
 * In the case you are experimenting problems for your particular
 * windows platform, please contact us through the mailing list.
 */
typedef struct _win32_cond_t {
	/* Number of waiting threads. */
	int waiters_count_;
	
	/* Serialize access to <waiters_count_>. */
	CRITICAL_SECTION waiters_count_lock_;

	/* Semaphore used to queue up threads waiting for the
	 * condition to become signaled. */
	HANDLE sema_;

	/* An auto-reset event used by the broadcast/signal thread to
	 * wait for all the waiting thread(s) to wake up and be
	 * released from the semaphore.  */
	HANDLE waiters_done_;

	/* Keeps track of whether we were broadcasting or signaling.
	 * This allows us to optimize the code if we're just
	 * signaling. */
	size_t was_broadcast_;
	
} win32_cond_t;

#elif defined(AXL_OS_UNIX)

#include <pthread.h>
#define __OS_THREAD_TYPE__ pthread_t
#define __OS_MUTEX_TYPE__  pthread_mutex_t
#define __OS_COND_TYPE__   pthread_cond_t

#endif

/** 
 * @brief Thread definition, which encapsulates the os thread API,
 * allowing to provide a unified type for all threading
 * interface. 
 */
typedef __OS_THREAD_TYPE__ ValvulaThread;

/** 
 * @brief Mutex definition that encapsulates the underlaying mutex
 * API.
 */
typedef __OS_MUTEX_TYPE__  ValvulaMutex;

/** 
 * @brief Conditional variable mutex, encapsulating the underlaying
 * operating system implementation for conditional variables inside
 * critical sections.
 */
typedef __OS_COND_TYPE__   ValvulaCond;

/** 
 * @brief Message queue implementation that allows to communicate
 * several threads in a safe manner. 
 */
typedef struct _ValvulaAsyncQueue ValvulaAsyncQueue;

/** 
 * @brief Handle definition for the family of function that is able to
 * accept the function \ref valvula_thread_create.
 *
 * The function receive a user defined pointer passed to the \ref
 * valvula_thread_create function, and returns an pointer reference
 * that must be used as integer value that could be retrieved if the
 * thread is joined.
 *
 * Keep in mind that there are differences between the windows and the
 * posix thread API, that are supported by this API, about the
 * returning value from the start function. 
 * 
 * While POSIX defines as returning value a pointer (which could be a
 * reference pointing to memory high above 32 bits under 64
 * architectures), the windows API defines an integer value, that
 * could be easily used to return pointers, but only safe on 32bits
 * machines.
 *
 * The moral of the story is that you must use another mechanism to
 * return data from this function to the thread that is expecting data
 * from this function. 
 * 
 * Obviously if you are going to return an status code, there is no
 * problem. This only applies to user defined data that is returned as
 * a reference to allocated data.
 */
typedef axlPointer (* ValvulaThreadFunc) (axlPointer user_data);

/** 
 * @brief Thread configuration its to modify default behaviour
 * provided by the thread creation API.
 */
typedef enum  {
	/** 
	 * @brief Marker used to signal \ref valvula_thread_create that
	 * the configuration list is finished.
	 * 
	 * The following is an example on how to create a new thread
	 * without providing any configuration, using defaults:
	 *
	 * \code
	 * ValvulaThread thread;
	 * if (! valvula_thread_created (&thread, 
	 *                              some_start_function, NULL,
	 *                              VALVULA_THREAD_CONF_END)) {
	 *      // failed to create the thread 
	 * }
	 * // thread created
	 * \endcode
	 */
	VALVULA_THREAD_CONF_END = 0,
	/** 
	 * @brief Allows to configure if the thread create can be
	 * joined and waited by other. 
	 *
	 * Default state for all thread created with \ref
	 * valvula_thread_create is true, that is, the thread created
	 * is joinable.
	 *
	 * If configured this value, you must provide as the following
	 * value either axl_true or axl_false.
	 *
	 * \code
	 * ValvulaThread thread;
	 * if (! valvula_thread_create (&thread, some_start_function, NULL, 
	 *                             VALVULA_THREAD_CONF_JOINABLE, axl_false,
	 *                             VALVULA_THREAD_CONF_END)) {
	 *    // failed to create the thread
	 * }
	 * 
	 * // Nice! thread created
	 * \endcode
	 */
	VALVULA_THREAD_CONF_JOINABLE  = 1,
	/** 
	 * @brief Allows to configure that the thread is in detached
	 * state, so no thread can join and wait for it for its
	 * termination but it will also provide.
	 */
	VALVULA_THREAD_CONF_DETACHED = 2,
}ValvulaThreadConf;


/** 
 * @internal Valvula Operation Status.
 * 
 * This enum is used to represent different Valvula Library status,
 * especially while operating with \ref ValvulaConnection
 * references. Values described by this enumeration are returned by
 * \ref valvula_connection_get_status.
 */
typedef enum {
	/** 
	 * @internal Represents an Error while Valvula Library was operating.
	 *
	 * The operation asked to be done by Valvula Library could be
	 * completed.
	 */
	ValvulaError                  = 1,
	/** 
	 * @internal Represents the operation have been successfully completed.
	 *
	 * The operation asked to be done by Valvula Library have been
	 * completed.
	 */
	ValvulaOk                     = 2,
} ValvulaStatus;


/** 
 * @brief A single valvula context definition. 
 */
typedef struct _ValvulaCtx ValvulaCtx;

/** 
 * @brief A single connection representing in incoming request to be
 * handled or a server that can receive new connections.
 */
typedef struct _ValvulaConnection ValvulaConnection;

/** 
 * @brief Thread safe hash definition.
 */
typedef struct _ValvulaHash ValvulaHash;

typedef enum {
	/** 
	 * @internal Log a message as a debug message.
	 */
	VALVULA_LEVEL_DEBUG    = 1 << 0,
	/** 
	 * @internal Log a warning message.
	 */
	VALVULA_LEVEL_WARNING  = 1 << 1,
	/** 
	 * @internal Log a critical message.
	 */
	VALVULA_LEVEL_CRITICAL = 1 << 2,
} ValvulaDebugLevel;

/** 
 * @internal Allows to specify which type of operation should be
 * implemented while calling to Valvula Library internal IO blocking
 * abstraction.
 */
typedef enum {
	/** 
	 * @internal A read watching operation is requested. If this
	 * value is received, the fd set containins a set of socket
	 * descriptors which should be watched for incoming data to be
	 * received.
	 */
	READ_OPERATIONS  = 1 << 0, 
	/** 
	 * @internal A write watching operation is requested. If this
	 * value is received, the fd set contains a set of socket that
	 * is being requested for its availability to perform a write
	 * operation on them.
	 */
	WRITE_OPERATIONS = 1 << 1
} ValvulaIoWaitingFor;

/** 
 * @internal
 */
typedef enum {
	/**
	 * @internal Allows to configure the select(2) system call based
	 * mechanism. It is known to be available on any platform,
	 * however it has some limitations while handling big set of
	 * sockets, and it is limited to a maximum number of sockets,
	 * which is configured at the compilation process.
	 *
         * Its main disadvantage it that it can't handle
	 * more connections than the number provided at the
	 * compilation process. See <valvula.h> file, variable
	 * FD_SETSIZE and VALVULA_FD_SETSIZE.
	 */
	VALVULA_IO_WAIT_SELECT = 1,
	/**
	 * @internal Allows to configure the poll(2) system call based
	 * mechanism. 
	 * 
	 * It is also a widely available mechanism on POSIX
	 * envirionments, but not on Microsoft Windows. It doesn't
	 * have some limitations found on select(2) call, but it is
	 * known to not scale very well handling big socket sets as
	 * happens with select(2) (\ref VALVULA_IO_WAIT_SELECT).
	 *
	 * This mechanism solves the runtime limitation that provides
	 * select(2), making it possible to handle any number of
	 * connections without providing any previous knowledge during
	 * the compilation process. 
	 * 
	 * Several third party tests shows it performs badly while
	 * handling many connections compared to (\ref VALVULA_IO_WAIT_EPOLL) epoll(2).
	 *
	 * However, reports showing that results, handles over 50.000
	 * connections at the same time (up to 400.000!). In many
	 * cases this is not going your production environment.
	 *
	 * At the same time, many reports (and our test results) shows
	 * that select(2), poll(2) and epoll(2) performs the same
	 * while handling up to 10.000 connections at the same time.
	 */
	VALVULA_IO_WAIT_POLL   = 2,
	/**
	 * @internal Allows to configure the epoll(2) system call based
	 * mechanism.
	 * 
	 * It is a mechanism available on GNU/Linux starting from
	 * kernel 2.6. It is supposed to be a better implementation
	 * than poll(2) and select(2) due the way notifications are
	 * done.
	 *
	 * It is currently selected by default if your kernel support
	 * it. It has the advantage that performs very well with
	 * little set of connections (0-10.000) like
	 * (\ref VALVULA_IO_WAIT_POLL) poll(2) and (\ref VALVULA_IO_WAIT_SELECT)
	 * select(2), but scaling much better when going to up heavy
	 * set of connections (50.000-400.000).
	 *
	 * It has also the advantage to not require defining a maximum
	 * socket number to be handled at the compilation process.
	 */
	VALVULA_IO_WAIT_EPOLL  = 3,
} ValvulaIoWaitingType;

/** 
 * @brief Connection role inside libValvula and ValvulaD server.
 */
typedef enum {
	/** 
	 * @brief This value is used to represent an unknown role state.
	 */
	ValvulaRoleUnknown,
	
	/** 
	 * @brief The connection is acting as an Initiator one.
	 */
	ValvulaRoleInitiator,

	/** 
	 * @brief The connection is acting as a Listener one.
	 */
	ValvulaRoleListener,
	
	/** 
	 * @brief This especial value for the this enumeration allows
	 * to know that the connection is a listener connection
	 * accepting connections. 
	 */
	ValvulaRoleMasterListener
	
} ValvulaPeerRole;

typedef struct _ValvulaRequest {
	/* protocol declaration and state of the request */
	char * request;
	char * protocol_state;
	char * protocol_name;

	/* message description */
	char * queue_id;
	int    size;

	/* sender/recipient */
	char * sender;
	char * recipient;
	int    recipient_count;

	/* network information */
	char * helo_name;
	char * client_address;
	char * client_name;
	char * reverse_client;
	char * instance;

	/* SASL auth content */
	char * sasl_method;
	char * sasl_username;
	char * sasl_sender;
	
	/* certificate info */
	char * ccert_subject;
	char * ccert_issuer;
	char * ccert_fingerprint;
	char * ccert_pubkey_fingerprint;
	char * encryption_protocol;
	char * encryption_cipher;
	char * encryption_keysize;
	
	/* additional info */
	char * etrn_domain;
	char * stress;

	/* message reply to report */
	char * message_reply;

	/* listener port */
	int    listener_port;
} ValvulaRequest;

/** 
 * @brief These are valvula states that can be returned by
 * handlers. More information at:
 *
 * http://www.postfix.org/access.5.html
 */ 
typedef enum {
	/** 
	 * @brief Allows requested action and stops further processing.
	 */
	VALVULA_STATE_OK = 0,
	/** 
	 * @brief Pretend that the request hasn't any key found so
	 * nothing can be said in any direction (negative or possitive).
	 */
	VALVULA_STATE_DUNNO = 1,
	/** 
	 * @brief Rejects the operation requested. This is also Deny.
	 */
	VALVULA_STATE_REJECT = 2,
	/** 
	 * @brief Defer (reject temporally) the operation if rest of
	 * rules allows the operation. This causes the Postfix SMTP
	 * server to reject the request with a 450 temporary error
	 * code and with text "Service temporarily unavailable", if
	 * the Postfix SMTP server finds no reason to reject the
	 * request permanently.
	 */
	VALVULA_STATE_DEFER_IF_PERMIT = 3,
	/** 
	 * @brief Defer (reject temporally) the operation if rest of
	 * rules denies the operation.
	 */
	VALVULA_STATE_DEFER_IF_REJECT = 4,
	/** 
	 * @brief Defer (reject temporally) the operation.
	 */
	VALVULA_STATE_DEFER = 5,
	/** 
	 * @brief Bcc a copy of the message to the especific recipient. Recipient is configured by the returning "message variable".
	 */
	VALVULA_STATE_BCC = 6,
	/** 
	 * @brief Allows to discard the message. Works like reject but
	 * without reporting a failure to the remote peer.
	 */
	VALVULA_STATE_DISCARD = 7,
	/** 
	 * @brief Holds the message until it is resumed manually by an
	 * administrator or a software.
	 */
	VALVULA_STATE_HOLD = 8,
	/** 
	 * @brief Allows to add an additional header to the message. The header, with format "header: content" is specified by "message variable".
	 */
	VALVULA_STATE_PREPEND = 9,
	/** 
	 * @brief Allows to redirect the message to the particular recipient as defined by "message variable".
	 */
	VALVULA_STATE_REDIRECT = 10,
	/** 
	 * @brief Allows to log a message as defined by "message variable".
	 */
	VALVULA_STATE_LOG = 11,
	/** 
	 * @brief An error happened during processing but it doesn't
	 * match any known error.
	 */
	VALVULA_STATE_GENERIC_ERROR = 12,

	/** 
	 * @brief Allows to configure postfix filter option (see access(5))
	 */
	VALVULA_STATE_FILTER = 13
} ValvulaState;

/** 
 * @brief A process handler registry. This type represents a single
 * process/request handler that was registered by \ref valvula_ctx_register_request_handler.
 */
typedef struct _ValvulaRequestRegistry ValvulaRequestRegistry;


#endif

/** 
 * @}
 */
