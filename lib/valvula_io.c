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

#include <valvula.h>

/* local include */
#include <valvula_private.h>

#define LOG_DOMAIN "valvula-io"

/**
 * \defgroup valvula_io Valvula IO: Valvula Library IO abstraction layer 
 */

/** 
 * @internal Macro that allows to check the kind of operation that is
 * being required.
 */
#define VALVULA_IO_IS(data,op) ((data & op) == op)

/** 
 * \addtogroup valvula_io
 * @{
 */

typedef struct _ValvulaSelect {
	ValvulaCtx          * ctx;
	fd_set                set;
	int                   length;
	ValvulaIoWaitingFor   wait_to;
}ValvulaSelect;

/** 
 * @internal
 *
 * @brief Internal valvula implementation to create a compatible
 * "select" IO call fd set reference.
 *
 * @return A newly allocated fd_set reference.
 */
axlPointer __valvula_io_waiting_default_create (ValvulaCtx * ctx, ValvulaIoWaitingFor wait_to) 
{
	ValvulaSelect * select = axl_new (ValvulaSelect, 1);

	/* set default behaviour expected for the set */
	select->wait_to       = wait_to;
	select->ctx           = ctx;
	
	/* clear the set */
	FD_ZERO (&(select->set));

	return select;
}

/** 
 * @internal
 *
 * @brief Internal valvula implementation to destroy the "select" IO
 * call created by the default create.
 * 
 * @param fd_group The fd group to be deallocated.
 */
void    __valvula_io_waiting_default_destroy (axlPointer fd_group)
{
	fd_set * __fd_set = fd_group;

	/* release memory allocated */
	axl_free (__fd_set);
	
	/* nothing more to do */
	return;
}

/** 
 * @internal
 *
 * @brief Internal valvula implementation to clear the "select" IO
 * call created by the default create.
 * 
 * @param fd_group The fd group to be deallocated.
 */
void    __valvula_io_waiting_default_clear (axlPointer __fd_group)
{
	ValvulaSelect * select = __fd_group;

	/* clear the fd set */
	select->length = 0;
	FD_ZERO (&(select->set));

	/* nothing more to do */
	return;
}

/** 
 * @internal
 *
 * @brief Default internal implementation for the wait operation to
 * change its status at least one socket description inside the fd set
 * provided.
 * 
 * @param __fd_group The fd set having all sockets to be watched.
 * @param wait_to The operation requested.
 * 
 * @return The error code as described by \ref valvula_io_waiting_set_wait_on_fd_group.
 */
int __valvula_io_waiting_default_wait_on (axlPointer __fd_group, int max_fds, ValvulaIoWaitingFor wait_to)
{
	int                result = -1;
	struct timeval     tv;
	ValvulaSelect     * _select = __fd_group;

	/* perform the select operation according to the
	 * <b>wait_to</b> value. */
	if (VALVULA_IO_IS (wait_to, READ_OPERATIONS)) {
		/* init wait */
		tv.tv_sec    = 0;
		tv.tv_usec   = 500000;
		result       = select (max_fds + 1, &(_select->set), NULL,   NULL, &tv);
	} else if (VALVULA_IO_IS (wait_to, WRITE_OPERATIONS)) {
		tv.tv_sec    = 1;
		tv.tv_usec   = 0;
		result       = select (max_fds + 1, NULL, &(_select->set), NULL, &tv);
	}
	
	/* check result */
	if ((result == VALVULA_SOCKET_ERROR) && (errno == VALVULA_EINTR))
		return -1;
	
	return result;
}

/** 
 * @internal
 *
 * @brief Default Valvula Library implementation for the "add to" on fd
 * set operation.
 * 
 * @param fds The socket descriptor to be added.
 *
 * @param fd_set The fd set where the socket descriptor will be added.
 */
axl_bool  __valvula_io_waiting_default_add_to (int                fds, 
					      ValvulaConnection * connection,
					      axlPointer         __fd_set)
{
	ValvulaSelect * select = (ValvulaSelect *) __fd_set;
#if defined(ENABLE_VALVULA_LOG) && ! defined(SHOW_FORMAT_BUGS)
	ValvulaCtx    * ctx    = valvula_connection_get_ctx (connection);
#endif

#if defined(AXL_OS_UNIX)
	/* disable the following check on windows because it doesn't
	 * reuse socket descriptors, using values that are higher than
	 * VALVULA_FD_SETSIZE (actuall FD_SETSIZE) */
	if (fds >= VALVULA_FD_SETSIZE) {
		valvula_log (VALVULA_LEVEL_CRITICAL, 
			    "reached max amount of descriptors for socket set based on select(2). See VALVULA_FD_SETSIZE=%d, fds=%d, or other I/O mechanism, closing connection.", VALVULA_FD_SETSIZE, fds);
		return axl_false;
	} /* end if */
#endif

	if (fds < 0) {
		valvula_log (VALVULA_LEVEL_CRITICAL,
			    "received a non valid socket (%d), unable to add to the set", fds);
		return axl_false;
	}

	if (select->length == (VALVULA_FD_SETSIZE - 1)) {
		valvula_log (VALVULA_LEVEL_CRITICAL, 
			    "reached max amount of descriptors for socket set based on select(2). See VALVULA_FD_SETSIZE=%d, fds=%d, or other I/O mechanism, closing connection.", VALVULA_FD_SETSIZE, fds);
		return axl_false;
	} /* end if */

	/* set the value */
	FD_SET (fds, &(select->set));

	/* update length */
	select->length++;

	return axl_true;
}

/** 
 * @internal
 *
 * @brief Default Valvula Library implementation for the "is set" on fd
 * set operation.
 * 
 * @param fds The socket descriptor to be checked to be active on the
 * given fd group.
 *
 * @param fd_set The fd set where the socket descriptor will be checked.
 */
axl_bool      __valvula_io_waiting_default_is_set (int        fds, 
						  axlPointer __fd_set, 
						  axlPointer user_data)
{
	ValvulaSelect * select = __fd_set;
	
	return FD_ISSET (fds, &(select->set));
}

/**
 * poll(2) system call implementation.
 */
#if defined(VALVULA_HAVE_POLL)
typedef struct _ValvulaPoll {
	ValvulaCtx           * ctx;
	int                   max;
	int                   length;
	struct pollfd       * set;
	ValvulaConnection   ** connections;
	ValvulaIoWaitingFor    wait_to;
}ValvulaPoll;
/** 
 * @internal
 *
 * @brief Internal valvula implementation to support poll(2) interface
 * to the file set creation interface.
 *
 * @return A newly allocated file set reference, supporting poll(2).
 */
axlPointer __valvula_io_waiting_poll_create (ValvulaCtx * ctx, ValvulaIoWaitingFor wait_to) 
{
	int          max = 4096;
	ValvulaPoll * poll;

	valvula_log (VALVULA_LEVEL_DEBUG, "creating empty poll(2) set");

	/* check if max points to something not useful */
	if (max <= 0)
		max = 4096;

	poll              = axl_new (ValvulaPoll, 1);
	poll->ctx         = ctx;
	poll->max         = max;
	poll->wait_to     = wait_to;
	poll->set         = axl_new (struct pollfd, max);
	poll->connections = axl_new (ValvulaConnection *, max);

	return poll;
}

/** 
 * @internal
 *
 * Internal implementation to destroy a file set support the poll(2)
 * interface.
 * 
 * @param fd_group The file set to be deallocated.
 */
void    __valvula_io_waiting_poll_destroy (axlPointer fd_group)
{
	ValvulaPoll * poll = (ValvulaPoll *) fd_group;

	/* free the poll */
	axl_free (poll->set);
	axl_free (poll->connections);
	axl_free (poll);
	
	/* nothing more to do */
	return;
}

/** 
 * @internal
 *
 * Clears the file set supporting poll(2) interface.
 */
void    __valvula_io_waiting_poll_clear (axlPointer __fd_group)
{
	ValvulaPoll * poll = (ValvulaPoll *) __fd_group;

	/* do not clear nothing, try to reuse */
	poll->length = 0;
	memset (poll->set, 0, sizeof (struct pollfd) * poll->max);

	/* nothing more to do */
	return;
}

/** 
 * @internal
 *
 * Add to file set implementation for poll(2) interface
 * 
 * @param fds The socket descriptor to be added.
 *
 * @param fd_set The fd set where the socket descriptor will be added.
 */
axl_bool  __valvula_io_waiting_poll_add_to (int                fds, 
					   ValvulaConnection * connection,
					   axlPointer         __fd_set)
{
	ValvulaPoll * poll   = (ValvulaPoll *) __fd_set;
	ValvulaCtx  * ctx    = poll->ctx;
	int           max    = 4096;

	/* check if max size reached */
	if (poll->length == poll->max) {
	
		if (poll->max >= max) {
			valvula_log (VALVULA_LEVEL_DEBUG, "unable to accept more sockets, max poll set reached.");
			return axl_false;
		} /* end if */

		valvula_log (VALVULA_LEVEL_DEBUG, "max amount of file descriptors reached, expanding");

		/* limit reached */
		poll->max          = max;
		poll->set          = axl_realloc (poll->set,         sizeof (struct pollfd)      * poll->max);
		poll->connections  = axl_realloc (poll->connections, sizeof (ValvulaConnection *) * poll->max);
	} /* end if */

	/* configure the socket to be watched */
	poll->set[poll->length].fd      = fds;
	poll->connections[poll->length] = connection;

	/* configure events to check */
	poll->set[poll->length].events = 0;
	if (VALVULA_IO_IS(poll->wait_to, READ_OPERATIONS)) {
		poll->set[poll->length].events |= POLLIN;
		poll->set[poll->length].events |= POLLPRI;
	} /* end if */
	if (VALVULA_IO_IS(poll->wait_to, WRITE_OPERATIONS))
		poll->set[poll->length].events |= POLLOUT;

	/* update length */
	poll->length++;

	return axl_true;
}

/** 
 * @internal
 *
 * Perform a wait operation over the object support poll(2) interface.
 */
int __valvula_io_waiting_poll_wait_on (axlPointer __fd_group, int max_fds, ValvulaIoWaitingFor wait_to)
{
	int          result  = -1;
	ValvulaPoll * _poll   = (ValvulaPoll *) __fd_group;

	/* perform the select operation according to the
	 * <b>wait_to</b> value. */
	if (VALVULA_IO_IS (wait_to, READ_OPERATIONS)) {
		/* wait for read operations */
		result       = poll (_poll->set, _poll->length, 500);
	} else 	if (VALVULA_IO_IS (wait_to, WRITE_OPERATIONS)) {
		/* wait for write operations */
		result       = poll (_poll->set, _poll->length, 1000);
	}
	
	/* check result */
	if ((result == VALVULA_SOCKET_ERROR) && (errno == VALVULA_EINTR))
		return -1;
	
	return result;
}


/** 
 * @internal Notify that we have dispatch support.
 */
axl_bool      __valvula_io_waiting_poll_have_dispatch (axlPointer fd_group)
{
	return axl_true;
}

/** 
 * @internal
 *
 * Poll implementation for the automatic dispatch.
 * 
 * @param fds The socket descriptor to be checked to be active on the
 * given fd group.
 *
 * @param fd_set The fd set where the socket descriptor will be checked.
 */
void     __valvula_io_waiting_poll_dispatch (axlPointer           fd_group, 
					    ValvulaIoDispatchFunc dispatch_func,
					    int                  changed,
					    axlPointer           user_data)
{
	ValvulaPoll * poll     = (ValvulaPoll *) fd_group;
	int          iterator = 0;
	int          checked  = 0;

	/* for all sockets polled */
	while (iterator < poll->length && (checked < changed)) {

		/* item found now check the event */
		if (VALVULA_IO_IS(poll->wait_to, READ_OPERATIONS)) {
			
			if ((poll->set[iterator].revents & POLLIN) == POLLIN ||
			    (poll->set[iterator].revents & POLLPRI) == POLLPRI) {
				
				/* found read event, dispatch */
				dispatch_func (
					/* socket found */
					poll->set[iterator].fd,
					/* purpose for the waiting set */
					poll->wait_to, 
					/* connection associated */
					poll->connections[iterator], 
					/* dispatch user data */
					user_data);

				/* update items checked */
				checked++;
			} /* end if */

		} /* end if */

		/* item found now check the event */
		if (VALVULA_IO_IS(poll->wait_to, WRITE_OPERATIONS)) {
			if ((poll->set[iterator].revents & POLLOUT) == POLLOUT) {
				/* found read event, dispatch */
				dispatch_func (
					/* socket found */
					poll->set[iterator].fd,
					/* purpose for the waiting set */
					poll->wait_to, 
					/* connection associated */
					poll->connections[iterator], 
					/* dispatch user data */
					user_data);
				
				/* update items checked */
				checked++;
			} /* end if */
		} /* end if */
		
		/* go to the next */
		iterator++;

	} /* end if */
	
	return;
}
#endif /* VALVULA_HAVE_POLL */


/**
 * linux epoll(2) system call implementation.
 */
#if defined(VALVULA_HAVE_EPOLL)
typedef struct _ValvulaEPoll {
	ValvulaCtx           * ctx;
	int                   max;
	int                   length;
	int                   set;
	struct epoll_event  * events;
	ValvulaIoWaitingFor    wait_to;
}ValvulaEPoll;
/** 
 * @internal
 *
 * @brief Internal valvula implementation to support epoll(2) interface
 * to the file set creation interface.
 *
 * @return A newly allocated file set reference, supporting epoll(2).
 */
axlPointer __valvula_io_waiting_epoll_create (ValvulaCtx * ctx, ValvulaIoWaitingFor wait_to) 
{
	int           set;
	int           max = 4096;
	ValvulaEPoll * epoll;

	set = epoll_create (max);
	if (set == -1) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "failed to create the epoll interface (epoll_create system call have failed): %s",
			     strerror (errno));
		return NULL;
	} /* end if */

	/* set the epoll socket as closable on exec */
	fcntl (set, F_SETFD, fcntl(set, F_GETFD) | FD_CLOEXEC);
		
	epoll              = axl_new (ValvulaEPoll, 1);
	epoll->ctx         = ctx;
	epoll->max         = max;
	epoll->wait_to     = wait_to;
	epoll->set         = set;
	epoll->events      = axl_new (struct epoll_event, max);

	return epoll;
}

/** 
 * @internal
 *
 * Internal implementation to destroy a file set support the epoll(2)
 * interface.
 * 
 * @param fd_group The file set to be deallocated.
 */
void    __valvula_io_waiting_epoll_destroy (axlPointer fd_group)
{
	ValvulaEPoll * epoll = (ValvulaEPoll *) fd_group;

	/* free the poll */
	close    (epoll->set);
	axl_free (epoll->events);
	axl_free (epoll);
	
	/* nothing more to do */
	return;
}

/** 
 * @internal
 *
 * Clears the file set supporting epoll(2) interface.
 */
void    __valvula_io_waiting_epoll_clear (axlPointer __fd_group)
{
	ValvulaEPoll *      epoll  = (ValvulaEPoll *) __fd_group;

	/* close the epoll set, to avoid having to remove all sockets
	 * that were added but not removed by the kernel */
	close (epoll->set);
	epoll->set    = epoll_create (epoll->max);

	/* set the epoll socket as closable on exec */
	fcntl (epoll->set, F_SETFD, fcntl(epoll->set, F_GETFD) | FD_CLOEXEC);

	epoll->length = 0;
	return;
}

/** 
 * @internal
 *
 * Add to file set implementation for epoll(2) interface.
 * 
 * @param fds The socket descriptor to be added.
 *
 * @param fd_set The fd set where the socket descriptor will be added.
 */
axl_bool  __valvula_io_waiting_epoll_add_to (int                fds, 
					    ValvulaConnection * connection,
					    axlPointer         __fd_set)
{
	ValvulaEPoll *        epoll  = (ValvulaEPoll *) __fd_set;
	ValvulaCtx   *        ctx    = epoll->ctx;
	int                   max    = 4096;
	struct epoll_event   ev;

	/* check if max size reached */
	if (epoll->length == epoll->max) {
	
		if (epoll->max >= max) {
			valvula_log (VALVULA_LEVEL_DEBUG, "unable to accept more sockets, max poll set reached (%d).", epoll->max);
			return axl_false;
		} /* end if */

		valvula_log (VALVULA_LEVEL_DEBUG, "max amount of file descriptors reached, expanding");

		/* limit reached */
		epoll->max          = max;
		epoll->events       = axl_realloc (epoll->events, sizeof (struct epoll_event) * epoll->max);
	} /* end if */

	/* clear data */
	memset (&ev, 0, sizeof (struct epoll_event));

	/* configure the kind of polling */
	if (VALVULA_IO_IS(epoll->wait_to, READ_OPERATIONS)) {
		ev.events = EPOLLIN | EPOLLPRI;
	} /* end if */

	if (VALVULA_IO_IS(epoll->wait_to, WRITE_OPERATIONS)) {
		ev.events = EPOLLOUT;
	} /* end if */

	/* configure the file descriptor */
	ev.data.ptr = connection;
	if (epoll_ctl(epoll->set, EPOLL_CTL_ADD, fds, &ev) != 0 && errno != EEXIST) {
		
		valvula_log (VALVULA_LEVEL_CRITICAL, 
			    "failed to add to the epoll fd=%d, epoll_ctl system call have failed: %s",
			     fds, strerror (errno));
		return axl_false;
	} /* end if */

	/* update length */
	epoll->length++;

	return axl_true;
}

/** 
 * @internal
 *
 * Perform a wait operation over the object supporting epoll(2)
 * interface.
 */
int __valvula_io_waiting_epoll_wait_on (axlPointer __fd_group, int max_fds, ValvulaIoWaitingFor wait_to)
{
	int           result  = -1;
	ValvulaEPoll * epoll   = (ValvulaEPoll *) __fd_group;

	/* clear events reported */
	/* memset (epoll->events, 0, sizeof (struct epoll_event) * epoll->max); */

	/* perform the select operation according to the
	 * <b>wait_to</b> value. */
	if (VALVULA_IO_IS (wait_to, READ_OPERATIONS)) {
		result = epoll_wait (epoll->set, epoll->events, epoll->length, 500);
	} else 	if (VALVULA_IO_IS (wait_to, WRITE_OPERATIONS)) {
		result = epoll_wait (epoll->set, epoll->events, epoll->length, 1000);
	} /* end if */

	/* check result */
	if ((result == VALVULA_SOCKET_ERROR) && (errno == VALVULA_EINTR))
		return -1;
	
	return result;
}

/** 
 * @internal Notify that we have dispatch support.
 */
axl_bool      __valvula_io_waiting_epoll_have_dispatch (axlPointer fd_group)
{
	return axl_true;
}

/** 
 * @internal
 *
 * Poll implementation for the automatic dispatch.
 * 
 * @param fds The socket descriptor to be checked to be active on the
 * given fd group.
 *
 * @param fd_set The fd set where the socket descriptor will be checked.
 */
void     __valvula_io_waiting_epoll_dispatch (axlPointer           fd_group, 
					     ValvulaIoDispatchFunc dispatch_func,
					     int                  changed,
					     axlPointer           user_data)
{
	ValvulaEPoll      * epoll    = (ValvulaEPoll *) fd_group;
	ValvulaConnection * connection;
	int                iterator = 0;

	/* for all sockets polled */
	while ((iterator < changed) && (iterator < epoll->length)) {
		
		/* item found now check the event */
		if (VALVULA_IO_IS (epoll->wait_to, READ_OPERATIONS)) {
			
			if ((epoll->events[iterator].events & EPOLLIN) == EPOLLIN ||
			    (epoll->events[iterator].events & EPOLLPRI) == EPOLLPRI) {

				/* get the connection */
				connection = (ValvulaConnection *) epoll->events[iterator].data.ptr;
				
				/* found read event, dispatch */
				dispatch_func (
					/* socket found */
					valvula_connection_get_socket (connection),
					/* purpose for the waiting set */
					epoll->wait_to, 
					/* connection associated */
					connection,
					/* dispatch user data */
					user_data);
			} /* end if */

		} /* end if */

		/* item found now check the event */
		if (VALVULA_IO_IS(epoll->wait_to, WRITE_OPERATIONS)) {
			if ((epoll->events[iterator].events & EPOLLOUT) == EPOLLOUT) {

				/* get the connection */
				connection = (ValvulaConnection *) epoll->events[iterator].data.ptr;
				
				/* found read event, dispatch */
				dispatch_func (
					/* socket found */
					valvula_connection_get_socket (connection),
					/* purpose for the waiting set */
					epoll->wait_to, 
					/* connection associated */
					connection,
					/* dispatch user data */
					user_data);
			} /* end if */
		} /* end if */
		
		/* go to the next */
		iterator++;

	} /* end if */
	
	return;
}
#endif /* VALVULA_HAVE_EPOLL */


/** 
 * @brief Allows to configure the default io waiting mechanism to be
 * used by the library.
 * 
 * To get current mechanism selected you must use \ref
 * valvula_io_waiting_get_current. During the compilation process,
 * valvula selects the default mechanism, choosing the best available. 
 *
 * This function is designed to support changing the I/O API at
 * runtime so, you can call to this function during normal valvula
 * operations.
 *
 * Here is an example of code on how to change the default mechanism
 * to use system call poll(2):
 *
 * \code
 * // check the mechanism is available
 * if (valvula_io_waiting_is_available (VALVULA_IO_WAIT_POLL)) {
 *     // configure the mechanism
 *     valvula_io_waiting_use (ctx, VALVULA_IO_WAIT_POLL);
 * }
 * \endcode
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param type The I/O waiting mechanism to be used by the library
 * (especially at the valvula reader module).
 * 
 * @return axl_true if the mechanism was activated, otherwise axl_false
 * is returned.
 */
axl_bool                  valvula_io_waiting_use (ValvulaCtx * ctx, ValvulaIoWaitingType type)
{
	/* get current context */
	axl_bool       result = axl_false;
	axl_bool       do_notify;
#if defined(ENABLE_VALVULA_LOG)
	const char   * mech = "";
#endif

	valvula_log (VALVULA_LEVEL_DEBUG, "about to change I/O waiting API");

	/* check if the type is available */
	if (! valvula_io_waiting_is_available (type)) {
		valvula_log (VALVULA_LEVEL_DEBUG, "I/O waiting API not available");
		return axl_false;
	}

	/* check if the user is requesting to change to the same IO
	 * API */
	if (type == ctx->waiting_type) {
		valvula_log (VALVULA_LEVEL_DEBUG, "I/O waiting API requested currently installed");
		return axl_true;
	}

	/* notify the reader to stop processing and dealloc resources
	 * with current IO api installed. */
	valvula_log (VALVULA_LEVEL_DEBUG, "requesting valvula reader to change its I/O API");
	do_notify = valvula_reader_notify_change_io_api (ctx);

	valvula_log (VALVULA_LEVEL_DEBUG, "done, now valvula reader is blocked until we finish");

	switch (type) {
	case VALVULA_IO_WAIT_SELECT:
		/* use select mechanism */
		ctx->waiting_create        = __valvula_io_waiting_default_create;
		ctx->waiting_destroy       = __valvula_io_waiting_default_destroy;
		ctx->waiting_clear         = __valvula_io_waiting_default_clear;
		ctx->waiting_wait_on       = __valvula_io_waiting_default_wait_on;
		ctx->waiting_add_to        = __valvula_io_waiting_default_add_to;
		ctx->waiting_is_set        = __valvula_io_waiting_default_is_set;
		ctx->waiting_have_dispatch = NULL;
		ctx->waiting_dispatch      = NULL;
		ctx->waiting_type          = VALVULA_IO_WAIT_SELECT;
#if defined(ENABLE_VALVULA_LOG)
		mech                       = "select(2) system call";
#endif

		/* ok */
		result = axl_true;
		break;
	case VALVULA_IO_WAIT_POLL:
		/* use poll mechanism */
#if defined (VALVULA_HAVE_POLL)
		/* use select mechanism */
		ctx->waiting_create        = __valvula_io_waiting_poll_create;
		ctx->waiting_destroy       = __valvula_io_waiting_poll_destroy;
		ctx->waiting_clear         = __valvula_io_waiting_poll_clear;
		ctx->waiting_wait_on       = __valvula_io_waiting_poll_wait_on;
		ctx->waiting_add_to        = __valvula_io_waiting_poll_add_to;
		/* no is_set support but automatic dispatch */
		ctx->waiting_is_set        = NULL;
		ctx->waiting_have_dispatch = __valvula_io_waiting_poll_have_dispatch;
		ctx->waiting_dispatch      = __valvula_io_waiting_poll_dispatch;
		ctx->waiting_type          = VALVULA_IO_WAIT_POLL;
#if defined(ENABLE_VALVULA_LOG)
		mech                       = "poll(2) system call";
#endif
	       
		/* ok */
		result = axl_true;
#else 
		result = axl_false;
#endif
		/* important, leave the break outside the mech
		 * definition */
		break;
	case VALVULA_IO_WAIT_EPOLL:
		/* use poll mechanism */
#if defined (VALVULA_HAVE_EPOLL)
		/* use select mechanism */
		ctx->waiting_create        = __valvula_io_waiting_epoll_create;
		ctx->waiting_destroy       = __valvula_io_waiting_epoll_destroy;
		ctx->waiting_clear         = __valvula_io_waiting_epoll_clear;
		ctx->waiting_wait_on       = __valvula_io_waiting_epoll_wait_on;
		ctx->waiting_add_to        = __valvula_io_waiting_epoll_add_to;
		/* no is_set support but automatic dispatch */
		ctx->waiting_is_set        = NULL;
		ctx->waiting_have_dispatch = __valvula_io_waiting_epoll_have_dispatch;
		ctx->waiting_dispatch      = __valvula_io_waiting_epoll_dispatch;
		ctx->waiting_type          = VALVULA_IO_WAIT_EPOLL;
#if defined(ENABLE_VALVULA_LOG)
		mech                       = "linux epoll(2) system call";
#endif
	       
		/* ok */
		result = axl_true;
#else 
		result = axl_false;
#endif
		/* important, leave the break outside the mech
		 * definition */
		break;
	} /* end switch */

	valvula_log (VALVULA_LEVEL_DEBUG, "I/O API changed with result=%s%s%s", 
		    result ? "ok" : "fail",
		    (strlen (mech) > 0) ? ", now using: " :"",
		    (strlen (mech) > 0) ? mech            : "");

	/* notify that the API change process have finished */
	if (do_notify) {
		valvula_reader_notify_change_done_io_api (ctx);
	}

	valvula_log (VALVULA_LEVEL_DEBUG, "valvula reader notified");

	/* fail */
	return result;
}

/** 
 * @brief Allows to check if a particular mechanism is available to be
 * installed.
 * 
 * @param type The mechanism to check.
 * 
 * @return axl_true if available, otherwise axl_false is returned.
 */
axl_bool                  valvula_io_waiting_is_available (ValvulaIoWaitingType type)
{
	switch (type) {
	case VALVULA_IO_WAIT_SELECT:
		/* ok */
		return axl_true;
	case VALVULA_IO_WAIT_POLL:
		/* use poll mechanism */
#if defined (VALVULA_HAVE_POLL)
		/* ok */
		return axl_true;
#else
		/* not available */
		return axl_false;
#endif
	case VALVULA_IO_WAIT_EPOLL:
		/* use poll mechanism */
#if defined (VALVULA_HAVE_EPOLL)
		/* ok */
		return axl_true;
#else
		/* not available */
		return axl_false;
#endif
	} /* end switch */

	/* fail */
	return axl_false;
}

/** 
 * @brief Allows to check current I/O waiting mechanism being used at
 * the providec context.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @return The I/O mechanism used by the core.
 */
ValvulaIoWaitingType  valvula_io_waiting_get_current  (ValvulaCtx * ctx)
{
	if (ctx == NULL)
		return 0;

	/* return current implementation */
	return ctx->waiting_type;
}

/** 
 * @brief Allows to set current handler that allows to create a fd
 * group that will be used by Valvula internals to perform IO blocking.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param create The handler to be executed by the Valvula Library to
 * create a new IO fd set. The function will fail to set the default
 * create handler is a NULL handler is provided. No handler will be
 * modified is the create handler is not provided.
 */
void                 valvula_io_waiting_set_create_fd_group (ValvulaCtx * ctx, 
							    ValvulaIoCreateFdGroup create)
{
	/* check that the handler provided is not null */
	if (ctx == NULL || create == NULL)
		return;

	/* set default create handler */
	ctx->waiting_create = create;
	
	return;
}

/** 
 * @internal
 *
 * @brief Perform an invocation to create a fd group using current
 * create handler defined.
 * 
 * This function is for internal Valvula Library purposes and should
 * not be used by API consumers.
 * 
 * @return Returns a reference to a new allocated fd set.
 */
axlPointer           valvula_io_waiting_invoke_create_fd_group (ValvulaCtx           * ctx,
							       ValvulaIoWaitingFor    wait_to)
{
	/* check current create IO handler configuration */
	if (ctx == NULL || ctx->waiting_create == NULL) {
		valvula_log (VALVULA_LEVEL_CRITICAL, 
		       "critical error, found that create fd set handler is not properly set, this is critical malfunctions");
		return NULL;
	}
	
	/* return a newly allocated fd set */
	return ctx->waiting_create (ctx, wait_to);
}


/** 
 * @brief Allows to define the destroy method to be used on the fd_set
 * allocated by calling current configured \ref ValvulaIoCreateFdGroup
 * "create handler".
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param destroy The handler to deallocate. The function will fail if
 * the provided destroy function is NULL. No operation will be
 * performed in such case.
 */
void                 valvula_io_waiting_set_destroy_fd_group    (ValvulaCtx              * ctx, 
								ValvulaIoDestroyFdGroup   destroy)
{
	/* check if the destroy handler is not a NULL reference */
	if (ctx == NULL || destroy == NULL)
		return;

	/* set default destroy handler */
	ctx->waiting_destroy = destroy;

	return;
}

/** 
 * @internal
 *
 * @brief Invokes current destroy handler configured to release memory
 * allocate by the fd group created by calling \ref
 * valvula_io_waiting_invoke_create_fd_group.
 *
 * This function is for internal Valvula Library purposes and should
 * not be used by API consumers.
 * 
 * @param fd_group The fd group to destroy. 
 */
void                 valvula_io_waiting_invoke_destroy_fd_group (ValvulaCtx  * ctx, 
								axlPointer   fd_group)
{
	/* check that the received fd_group is not null */
	if (ctx == NULL || fd_group == NULL)
		return;

	/* check current destroy IO handler configuration */
	if (ctx->waiting_destroy == NULL) {
		valvula_log (VALVULA_LEVEL_CRITICAL, 
		       "critical error, found that destroy fd set handler is not properly set, this is critical malfunctions");
		return;
	}
	
	/* return a newly allocated fd set */
	ctx->waiting_destroy (fd_group);

	return;
}


/** 
 * @brief Allows to configure the clear handler to be executed while
 * it is required to clear a descriptor set.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param clear The handler to executed once Valvula internal process
 * requires to create a socket descriptor set.
 */
void                 valvula_io_waiting_set_clear_fd_group    (ValvulaCtx             * ctx, 
							      ValvulaIoClearFdGroup    clear)
{
	/* check if the clear handler is not a NULL reference */
	if (clear == NULL || ctx == NULL)
		return;

	/* set default clear handler */
	ctx->waiting_clear = clear;

	return;
}

/** 
 * @internal
 *
 * @brief Invokes current clear fd group configure handler on the
 * given reference received.
 *
 * This function is internal to valvula library, and it is used at the
 * right place. This function should be be used by API consumers.
 *
 * @param fd_group The fd_group reference to clear.
 */
void                 valvula_io_waiting_invoke_clear_fd_group (ValvulaCtx * ctx, axlPointer fd_group)
{
	/* check that the received fd_group is not null */
	if (fd_group == NULL || ctx == NULL)
		return;

	/* check current clear IO handler configuration */
	if (ctx->waiting_clear == NULL) {
		valvula_log (VALVULA_LEVEL_CRITICAL, 
		       "critical error, found that clear fd set handler is not properly set, this is critical malfunctions");
		return;
	}
	
	/* return a newly allocated fd set */
	ctx->waiting_clear (fd_group);

	return;
}

/** 
 * @brief Allows to configure the add socket to fd set operation.
 *
 * The function will fail if the provided handler is NULL.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param add_to The handler to be invoked when it is required to add
 * a socket descriptor into the fd set.
 */
void                 valvula_io_waiting_set_add_to_fd_group     (ValvulaCtx            * ctx, 
								ValvulaIoAddToFdGroup   add_to)
{
	/* check for NULL reference handlers */
	if (add_to == NULL || ctx == NULL)
		return;

	/* set the new handler */
	ctx->waiting_add_to = add_to;

	return;
}

/** 
 * @internal
 *
 * @brief Invokes current add to operation for the given socket
 * descriptor into the given fd set.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param fds The socket descriptor to be added. The socket descriptor
 * should be greater than 0.
 *
 * @param on_reading The fd set where the socket descriptor will be
 * added.
 */
axl_bool               valvula_io_waiting_invoke_add_to_fd_group  (ValvulaCtx        * ctx,
								  VALVULA_SOCKET      fds, 
								  ValvulaConnection * connection, 
								  axlPointer         fd_group)
{
	if (ctx != NULL && ctx->waiting_add_to != NULL) {
		
		/* invoke add to operation */
		return ctx->waiting_add_to (fds, connection, fd_group);
	} /* end if */

	/* return axl_false if it fails */
	return axl_false;
}

/** 
 * @brief Allows to configure the is set operation for the socket on the fd set.
 *
 * The function will fail if the provided handler is NULL.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param is_set The handler to be invoked when it is required to
 * check if a socket descriptor is set into a given fd set.
 */
void                 valvula_io_waiting_set_is_set_fd_group     (ValvulaCtx            * ctx, 
								ValvulaIoIsSetFdGroup   is_set)
{
	/* check for NULL reference handlers */
	if (is_set == NULL || ctx == NULL)
		return;

	/* set the new handler */
	ctx->waiting_is_set = is_set;

	return;
}

/** 
 * @brief Allows to configure the "have dispatch" function.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param have_dispatch The handler to be executed by the valvula I/O
 * module to check if the current mechanism support automatic
 * dispatch.
 */
void                 valvula_io_waiting_set_have_dispatch       (ValvulaCtx             * ctx,
								ValvulaIoHaveDispatch    have_dispatch)
{

	/* check for NULL reference handlers */
	if (have_dispatch == NULL || ctx == NULL)
		return;

	/* set the new handler */
	ctx->waiting_have_dispatch = have_dispatch;
	return;
}

/** 
 * @brief Allows to configure the dispatch function to be used for the
 * automatic dispatch function.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param dispatch The dispatch function to be executed on every
 * socket changed.
 */
void                 valvula_io_waiting_set_dispatch            (ValvulaCtx          * ctx,
								ValvulaIoDispatch     dispatch)
{
	/* check for NULL reference handlers */
	if (dispatch == NULL || ctx == NULL)
		return;
	
	/* set the new handler */
	ctx->waiting_dispatch = dispatch;
	return;
}

/** 
 * @internal
 *
 * @brief Invokes current is set to operation for the given socket
 * descriptor into the given fd set.
 * 
 * @param fds The socket descriptor to be checked. The socket
 * descriptor should be greater than 0.
 *
 * @param on_reading The fd set where the socket descriptor will be
 * checked for its activation.
 *
 * @param user_data User defined pointer provided to the is set
 * function.
 */
axl_bool                   valvula_io_waiting_invoke_is_set_fd_group  (ValvulaCtx      * ctx,
								      VALVULA_SOCKET    fds, 
								      axlPointer       fd_group, 
								      axlPointer       user_data)
{
	if (ctx == NULL || fd_group == NULL || fds <= 0)
		return axl_false;
	
	/* check for properly add to handler configuration */
	if (ctx->waiting_is_set == NULL) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "there is no \"is set\" operation defined, this will cause critical malfunctions");
		return axl_false;
	}

	/* invoke add to operation */
	return ctx->waiting_is_set (fds, fd_group, user_data);
}

/** 
 * @brief Allows to check if the current I/O mechanism have automatic
 * dispatch support, that is, the I/O mechanism takes care about
 * invoking the appropiate function if the socket is activated for the
 * registered purpose.
 *
 * If the implementation provided support this feature, the function
 * must return axl_true. By returning axl_true it is implied that \ref
 * valvula_io_waiting_invoke_dispatch is also implemented.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param fd_group The reference to the object created by the I/O
 * implemetation that is being used.
 * 
 * @return axl_true if the current I/O mechanism support automatic
 * dispatch, otherwise axl_false is returned.
 */
axl_bool                  valvula_io_waiting_invoke_have_dispatch    (ValvulaCtx  * ctx, 
								     axlPointer   fd_group)
{
	/* check the references received */
	if (ctx == NULL || fd_group == NULL)
		return axl_false;
	
	/* check for properly add to handler configuration */
	if (ctx->waiting_have_dispatch == NULL) {
		return axl_false;
	} /* end if */

	/* invoke have dispatch operation */
	return ctx->waiting_have_dispatch (fd_group);
}

/** 
 * @brief Performs automatic dispath, over all activated resources,
 * invoking the provided handler.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param fd_group The file set group that is being notified to
 * automatically dispatch.
 *
 * @param func The function to execute on each activated socket
 * descriptor.
 *
 * @param changed Number of file descriptors that have been reported
 * changed state.
 *
 * @param user_data Reference to the user defined data to be passed to
 * the dispatch function.
 */
void                 valvula_io_waiting_invoke_dispatch         (ValvulaCtx            * ctx,
								axlPointer             fd_group, 
								ValvulaIoDispatchFunc   func,
								int                    changed,
								axlPointer             user_data)
{
	/* check parameters */
	if (ctx == NULL || fd_group == NULL || func == NULL)
		return;

	/* check for properly add to handler configuration */
	if (ctx->waiting_dispatch == NULL) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "there is no \"dispatch\" operation defined, this will cause critical malfunctions");
		return;
	} /* end if */

	/* invoke dispatch operation */
	ctx->waiting_dispatch (fd_group, func, changed, user_data);
	return;
}




/** 
 * @brief Allows to configure the wait on fd group handler. 
 * 
 * This handler allows to configure how is implemented a non-consuming
 * resource on a given fd group.
 *
 * The invoked function should return a selected error code according
 * to the following values:
 * 
 * - A timeout operation was received, then <b>-1</b> should be
 * returned.
 *
 * - A error has being received but the call loop should continue
 * without paying attention to it, without checking for the sockets
 * for its status, and to perform as soon as possible the same
 * wait. This is used because current current default implementation,
 * which used the IO "select" call could return because a signal was
 * received. If the IO implemented doesn't require to implement this
 * concept, just ignore it. Returned value should be <b>-2</b>.
 *
 * - A non recoverable error has happened, and valvula reader should
 * exist. In this case <b>-3</b> should be returned. This error code
 * is also used by the internal valvula IO handler invocation to report
 * that a wait handler haven't been defined.
 *
 * - No socket descriptor have change but it is required to perform
 * again the blocking IO wait. Then <b>0</b> should be returned.
 *
 * - For the rest of the cases, the number of sockets could be
 * returned by this function, however, this is not used by the Valvula
 * Library. To report that a socket have changed (or a group of then
 * inside the fd set), and you don't want to return how many sockets
 * have changed, just return a positive value (<b>1</b>).
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param wait_on The handler to be used. The function will fail on
 * setting the handler if it is provided a NULL reference.
 */
void                 valvula_io_waiting_set_wait_on_fd_group    (ValvulaCtx             * ctx, 
								ValvulaIoWaitOnFdGroup   wait_on)
{
	/* check references */
	if (ctx == NULL || wait_on == NULL)
		return;

	/* set new default handler to be used */
	ctx->waiting_wait_on = wait_on;

	return;
}

/** 
 * @internal
 *
 * @brief Invokes current implementation for IO blocking until the
 * requested operation could be performed of any of the given sockets
 * descriptors inside the descriptor group selected.
 *
 * @param fd_group The socket descriptor group where the request wait
 * operation will be performed.
 *
 * @param wait_to The operation requested.
 * 
 * @return Current error code or the number of sockets that have
 * changed.
 * 
 */
int                  valvula_io_waiting_invoke_wait             (ValvulaCtx          * ctx,
								        axlPointer           fd_group, 
								        int                  max_fds,
								        ValvulaIoWaitingFor   wait_to)
{
	/* check reference to the context */
	if (ctx == NULL)
		return -3;

	/* check for NULL reference */
	if (ctx->waiting_wait_on == NULL) {
		valvula_log (VALVULA_LEVEL_CRITICAL, 
		       "wait handler is not defined, this is a critical error that will produce a great malfunctions");
		return -3;
	}

	/* perform IO blocking operation */
	return ctx->waiting_wait_on (fd_group, max_fds, wait_to);
}

/**
 * @internal Private function to init the context with the proper
 * functions.
 */
void                 valvula_io_init (ValvulaCtx * ctx)
{
	/*
	 * @internal
	 * 
	 * Following is the set of handlers to be used while performing IO
	 * waiting operation, as defined by the API exported to modify
	 * function to be used.
	 *  
	 * There is one handler for each operation that could be performed. No
	 * mutex and any other blocking operation should be used while
	 * implementing this set of handler.
	 *
	 * Critical section, resource exclusion and any other race condition
	 * is ensure by the Valvula Library to not happen. 
	 *
	 * See default handles implemented to get an idea about how should be
	 * implemented other IO blocking operations using other APIs rather than
	 * select.
	 */
#if defined(DEFAULT_EPOLL)
	
	ctx->waiting_create        = __valvula_io_waiting_epoll_create;
	ctx->waiting_destroy       = __valvula_io_waiting_epoll_destroy;
	ctx->waiting_clear         = __valvula_io_waiting_epoll_clear;
	ctx->waiting_wait_on       = __valvula_io_waiting_epoll_wait_on;
	ctx->waiting_add_to        = __valvula_io_waiting_epoll_add_to;
	ctx->waiting_is_set        = NULL;
	ctx->waiting_have_dispatch = __valvula_io_waiting_epoll_have_dispatch;
	ctx->waiting_dispatch      = __valvula_io_waiting_epoll_dispatch;

	/* current implementation used */
	ctx->waiting_type          = VALVULA_IO_WAIT_EPOLL;

#elif defined(DEFAULT_POLL) /* poll(2) system call support */

	ctx->waiting_create        = __valvula_io_waiting_poll_create;
	ctx->waiting_destroy       = __valvula_io_waiting_poll_destroy;
	ctx->waiting_clear         = __valvula_io_waiting_poll_clear;
	ctx->waiting_wait_on       = __valvula_io_waiting_poll_wait_on;
	ctx->waiting_add_to        = __valvula_io_waiting_poll_add_to;
	ctx->waiting_is_set        = NULL;
	ctx->waiting_have_dispatch = __valvula_io_waiting_poll_have_dispatch;
	ctx->waiting_dispatch      = __valvula_io_waiting_poll_dispatch;

	/* current implementation used */
	ctx->waiting_type          = VALVULA_IO_WAIT_POLL;

#else /* select(2) system call support */

	ctx->waiting_create        = __valvula_io_waiting_default_create;
	ctx->waiting_destroy       = __valvula_io_waiting_default_destroy;
	ctx->waiting_clear         = __valvula_io_waiting_default_clear;
	ctx->waiting_wait_on       = __valvula_io_waiting_default_wait_on;
	ctx->waiting_add_to        = __valvula_io_waiting_default_add_to;
	ctx->waiting_is_set        = __valvula_io_waiting_default_is_set;
	ctx->waiting_have_dispatch = NULL;
	ctx->waiting_dispatch      = NULL;

	/* current implementation used */
	ctx->waiting_type          = VALVULA_IO_WAIT_SELECT;

#endif
	return;
}

/* @} */


