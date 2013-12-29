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

#include <valvula.h>

/* local include */
#include <valvula_private.h>

#define LOG_DOMAIN "valvula-support"


/** 
 * @brief Allows to get the integer value stored in the provided
 * environment varible.
 *
 * The function tries to get the content from the environment
 * variable, and return the integer content that it is
 * representing. The function asumes the environment variable provides
 * has a numeric value. 
 * 
 * @return The variable numeric value. If the variable is not defined,
 * then 0 will be returned.
 */
int      valvula_support_getenv_int                 (const char * env_name)
{
#if defined (AXL_OS_UNIX)
	/* get the variable value */
	char * variable = getenv (env_name);

	if (variable == NULL)
		return 0;
	
	/* just return the content translated */
	return (strtol (variable, NULL, 10));
#elif defined(AXL_OS_WIN32)
	char  variable[1024];
	int   size_returned = 0;
	int   value         = 0;

	/* get the content of the variable */
	memset (variable, 0, sizeof (char) * 1024);
	size_returned = GetEnvironmentVariableA (env_name, variable, 1023);

	if (size_returned > 1023) {
		return 0;
	}
	
	/* return the content translated */
	value = (strtol (variable, NULL, 10));

	return value;
#endif	
}

/** 
 * @brief Allows to get the string variable found at the provided
 * env_name.
 *
 * The function tries to get the content from the environment
 * variable, and return the string content that it is
 * representing. 
 * 
 * @return The variable value or NULL if it fails. The caller must
 * dealloc the string returned when no longer needed by calling to
 * axl_free.
 */
char *   valvula_support_getenv                 (const char * env_name)
{
#if defined (AXL_OS_UNIX)
	/* get the variable value */
	char * variable = getenv (env_name);

	if (variable == NULL)
		return NULL;
	
	/* just return the content translated */
	return axl_strdup (variable);

#elif defined(AXL_OS_WIN32)

	char  variable[1024];
	int   size_returned = 0;

	/* get the content of the variable */
	memset (variable, 0, sizeof (char) * 1024);
	size_returned = GetEnvironmentVariableA (env_name, variable, 1023);

	if (size_returned > 1023) {
		return 0;
	}
	
	/* return the content translated */
	return axl_strdup (variable);
#endif	
}

/**
 * @brief Allows to configure the environment value identified by
 * env_name, with the value provided env_value.
 *
 * @param env_name The environment variable to configure.
 *
 * @param env_value The environment value to configure. The value
 * provide must be not NULL. To unset an environment variable use \ref valvula_support_unsetenv
 *
 * @return axl_true if the operation was successfully completed, otherwise
 * axl_false is returned.
 */
axl_bool     valvula_support_setenv                     (const char * env_name, 
							const char * env_value)
{
	/* check values received */
	if (env_name == NULL || env_value == NULL)
		return axl_false;
	
#if defined (AXL_OS_WIN32)
	/* use windows implementation */
	return SetEnvironmentVariableA (env_name, env_value);
#elif defined(AXL_OS_UNIX)
	/* use the unix implementation */
	return setenv (env_name, env_value, 1) == 0;
#endif
}

/**
 * @brief Allows to unset the provided environment variable.
 *
 * @param env_name The environment variable to unset its value.
 *
 * @return axl_true if the operation was successfully completed, otherwise
 * axl_false is returned.
 */
axl_bool      valvula_support_unsetenv                   (const char * env_name)
{
	/* check values received */
	if (env_name == NULL)
		return axl_false;

#if defined (AXL_OS_WIN32)
	/* use windows implementation */
	return SetEnvironmentVariableA (env_name, NULL);
#elif defined(AXL_OS_UNIX)
	/* use the unix implementation */
	setenv (env_name, "", 1);
		
	/* always axl_true */
	return axl_true;
#endif
}

/** 
 * @brief Allows to create a path to a filename, by providing its
 * tokens, ending them with NULL.
 * 
 * @param name The first token to be provided, followed by the rest to
 * tokens that conforms the path, ended by a NULL terminator.
 * 
 * @return A newly allocated string that must be deallocated using
 * axl_free.
 */
char   * valvula_support_build_filename      (const char * name, ...)
{
	va_list   args;
	char    * result;
	char    * aux    = NULL;
	char    * token;

	/* do not produce a result if a null is received */
	if (name == NULL)
		return NULL;

	/* initialize the args value */
	va_start (args, name);

	/* get the token */
	result = axl_strdup (name);
	token  = va_arg (args, char *);
	
	while (token != NULL) {
		aux    = result;
		result = axl_strdup_printf ("%s%s%s", result, VALVULA_FILE_SEPARATOR, token);
		axl_free (aux);

		/* get next token */
		token = va_arg (args, char *);
	} /* end while */

	/* end args values */
	va_end (args);

	return result;
}

/** 
 * @brief Tries to translate the double provided on the string
 * received, doing the best effort (meaning that the locale will be
 * skiped).
 * 
 * @param param The string that is considered to contain a double
 * value.
 *
 * @param string_aux A reference to a pointer that signal the place
 * were an error was found. Optional argument.
 * 
 * @return The double value representing the string received or 0.0 if
 * it fails.
 */
double   valvula_support_strtod                     (const char * param, char ** string_aux)
{
	double     double_value;
	axl_bool   second_try = axl_false;
	char     * alt_string = NULL;

	/* provide a local reference */
	if (string_aux == NULL)
		string_aux = &alt_string;

	/* try to get the double value */
 try_again:
	double_value = strtod (param, string_aux);
	if (string_aux != NULL && strlen (*string_aux) == 0) {
		return double_value;
	}

	if ((! second_try) && (string_aux != NULL && strlen (*string_aux) > 0)) {

		/* check if the discord value that makes POSIX
		 * designers mind to be not possible to translate the
		 * double value */
		if ((*string_aux) [0] == '.') {
			(*string_aux) [0] = ',';
			second_try     = axl_true;
		} else if ((*string_aux) [0] == ',') {
			(*string_aux) [0] = '.';
			second_try     = axl_true;
		}

		/* check if we can try again */
		if (second_try)
			goto try_again;
	}

	/* unable to find the double value, maybe it is not a double
	 * value */
	return 0.0;
}

/** 
 * @brief Allows to convert the provided integer value into its string
 * representation leaving the result on the provided buffer.
 *
 * @param value The value to convert.
 * @param buffer Pointer to the buffer that will hold the result.
 * @param buffer_size The size of the buffer that will hold the result.
 *
 * Note the function does not place a NUL char at the end of the number
 * written.
 * 
 * @return The function returns bytes written into the buffer or -1 if
 * the buffer can't hold the content.
 */ 
int      valvula_support_itoa                       (unsigned int    value,
						     char          * buffer,
						     int             buffer_size)
{
	static char digits[] = "0123456789";
	char        inverse[10];
	int         iterator  = 0;
	int         iterator2 = 0;

	if (buffer_size <= 0)
		return -1;

	/* do the conversion */
	while (iterator < 10) {
		/* copy content */
		inverse[iterator] = digits[value % 10];

		/* reduce the value */
		value = value / 10;

		if (value == 0)
			break;
		iterator++;
	} /* end while */

	/* now reserve the content */
	while (iterator2 < buffer_size) {
		buffer[iterator2] = inverse[iterator];
		iterator2++;
		iterator--;

		if (iterator == -1)
			break;
			
	} /* end while */
    
	/* check result */
	if (iterator != -1) 
		return -1;

	/* return size created */
	return iterator2;
}

/** 
 * @brief Performs a timeval substract leaving the result in
 * (result). Subtract the `struct timeval' values a and b, storing the
 * result in result.  
 *
 * @param a First parameter to substract
 *
 * @param b Second parameter to substract
 *
 * @param result Result variable. Do no used a or b to place the
 * result.
 *
 * @return 1 if the difference is negative, otherwise 0 (operations
 * implemented is a - b).
 */ 
int     valvula_timeval_substract                  (struct timeval * a, 
						   struct timeval * b,
						   struct timeval * result)
{
	/* Perform the carry for the later subtraction by updating
	 * y. */
	if (a->tv_usec < b->tv_usec) {
		int nsec = (b->tv_usec - a->tv_usec) / 1000000 + 1;
		b->tv_usec -= 1000000 * nsec;
		b->tv_sec += nsec;
	}

	if (a->tv_usec - b->tv_usec > 1000000) {
		int nsec = (a->tv_usec - b->tv_usec) / 1000000;
		b->tv_usec += 1000000 * nsec;
		b->tv_sec -= nsec;
	}
	
	/* get the result */
	result->tv_sec = a->tv_sec - b->tv_sec;
	result->tv_usec = a->tv_usec - b->tv_usec;
     
       /* return 1 if result is negative. */
       return a->tv_sec < b->tv_sec;	
}

/**
 * @brief Thread safe implementation for inet_ntoa.
 *
 * @param ctx The valvula context where the operation will be
 * performed.  @param sin Socket information where to get inet_ntoa
 * result.
 *
 * @return A newly allocated string that must be deallocated with
 * axl_free that contains the host information. The function return
 * NULL if it fails.
 */
char   * valvula_support_inet_ntoa                  (ValvulaCtx           * ctx, 
						    struct sockaddr_in  * sin)
{
	char * result;

	v_return_val_if_fail (ctx && sin, NULL);

	/* lock during operation */
	valvula_mutex_lock (&ctx->inet_ntoa_mutex);

	/* allocate the string */
	result = axl_strdup (inet_ntoa (sin->sin_addr));

	/* unlock */
	valvula_mutex_unlock (&ctx->inet_ntoa_mutex);

	/* return the string */
	return result;
}

/** 
 * @brief Creates a portable pipe (by creating a socket connected
 * pair).
 *
 * @param descf The pointer to the buffer where descriptors will be
 * stored.
 *
 * @param ctx The context where the operation will take place.
 *
 * @return The function returns -1 in the case of failure or 0 if pipe
 * was properly created.
 */
int      valvula_support_pipe                       (ValvulaCtx * ctx, int descf[2])
{
	struct sockaddr_in      saddr;
	struct sockaddr_in      sin;

	VALVULA_SOCKET           listener_fd;
#if defined(AXL_OS_WIN32)
/*	BOOL                    unit      = axl_true; */
	int                     sin_size  = sizeof (sin);
#else    	
	int                     unit      = 1; 
	socklen_t               sin_size  = sizeof (sin);
#endif	  
	int                     bind_res;
	int                     result;

	/* create listener socket */
	if ((listener_fd = socket(AF_INET, SOCK_STREAM, 0)) <= 2) {
		/* do not allow creating sockets reusing stdin (0),
		   stdout (1), stderr (2) */
		valvula_log (VALVULA_LEVEL_CRITICAL, "failed to create listener socket: %d (errno=%d:%s)", listener_fd, errno, strerror (errno));
		return -1;
        } /* end if */

#if defined(AXL_OS_WIN32)
	/* Do not issue a reuse addr which causes on windows to reuse
	 * the same address:port for the same process. Under linux,
	 * reusing the address means that consecutive process can
	 * reuse the address without being blocked by a wait
	 * state.  */
	/* setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char  *)&unit, sizeof(BOOL)); */
#else
	setsockopt (listener_fd, SOL_SOCKET, SO_REUSEADDR, &unit, sizeof (unit));
#endif 

	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family          = AF_INET;
	saddr.sin_port            = 0;
	saddr.sin_addr.s_addr     = htonl (INADDR_LOOPBACK);

	/* call to bind */
	bind_res = bind (listener_fd, (struct sockaddr *)&saddr,  sizeof (struct sockaddr_in));
	if (bind_res == VALVULA_SOCKET_ERROR) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "unable to bind address (port already in use or insufficient permissions). Closing socket: %d", listener_fd);
		valvula_close_socket (listener_fd);
		return -1;
	}
	
	if (listen (listener_fd, 1) == VALVULA_SOCKET_ERROR) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "an error have occur while executing listen");
		valvula_close_socket (listener_fd);
		return -1;
        } /* end if */

	/* notify listener */
	if (getsockname (listener_fd, (struct sockaddr *) &sin, &sin_size) < -1) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "an error have happen while executing getsockname");
		valvula_close_socket (listener_fd);
		return -1;
	} /* end if */

	valvula_log  (VALVULA_LEVEL_DEBUG, "created listener running listener at %s:%d (socket: %d)", inet_ntoa(sin.sin_addr), ntohs (sin.sin_port), listener_fd);

	/* on now connect: read side */
	descf[0]      = socket (AF_INET, SOCK_STREAM, 0);
	if (descf[0] == VALVULA_INVALID_SOCKET) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "Unable to create socket required for pipe");
		valvula_close_socket (listener_fd);
		return -1;
	} /* end if */

	/* disable nagle */
	valvula_connection_set_sock_tcp_nodelay (descf[0], axl_true);

	/* set non blocking connection */
	valvula_connection_set_sock_block (descf[0], axl_false);  

        memset(&saddr, 0, sizeof(saddr));
	saddr.sin_addr.s_addr     = htonl(INADDR_LOOPBACK);
        saddr.sin_family          = AF_INET;
        saddr.sin_port            = sin.sin_port;

	/* connect in non blocking manner */
	result = connect (descf[0], (struct sockaddr *)&saddr, sizeof (saddr));
	if (result < 0 && errno != VALVULA_EINPROGRESS) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "connect () returned %d, errno=%d:%s", 
			     result, errno, strerror (errno));
		valvula_close_socket (listener_fd);
		return -1;
	}

	/* accept connection */
	valvula_log  (VALVULA_LEVEL_DEBUG, "calling to accept () socket");
	descf[1] = valvula_listener_accept (listener_fd);

	if (descf[1] <= 0) {
		valvula_log (VALVULA_LEVEL_CRITICAL, "Unable to accept connection, failed to create pipe");
		valvula_close_socket (listener_fd);
		return -1;
	}
	/* set pipe read end from result returned by thread */
	valvula_log (VALVULA_LEVEL_DEBUG, "Created pipe [%d, %d]", descf[0], descf[1]);

	/* disable nagle */
	valvula_connection_set_sock_tcp_nodelay (descf[1], axl_true);

	/* close listener */
	valvula_close_socket (listener_fd);

	/* report and return fd */
	return 0;
}

/** 
 * @brief Allows to perform a set of test for the provided path.
 * 
 * @param path The path that will be checked.
 *
 * @param test The set of test to be performed. Separate each test
 * with "|" to perform several test at the same time.
 * 
 * @return axl_true if all test returns axl_true. Otherwise axl_false is returned.
 */
axl_bool    valvula_support_file_test (const char * path, ValvulaFileTest test)
{
	axl_bool    result = axl_false;
	struct stat file_info;

	/* perform common checks */
	axl_return_val_if_fail (path, axl_false);

	/* call to get status */
	result = (stat (path, &file_info) == 0);
	if (! result) {
		/* check that it is requesting for not file exists */
		if (errno == ENOENT && (test & FILE_EXISTS) == FILE_EXISTS)
			return axl_false;
		return axl_false;
	} /* end if */

	/* check for file exists */
	if ((test & FILE_EXISTS) == FILE_EXISTS) {
		/* check result */
		if (result == axl_false)
			return axl_false;
		
		/* reached this point the file exists */
		result = axl_true;
	}

	/* check if the file is a link */
	if ((test & FILE_IS_LINK) == FILE_IS_LINK) {
		if (! S_ISLNK (file_info.st_mode))
			return axl_false;

		/* reached this point the file is link */
		result = axl_true;
	}

	/* check if the file is a regular */
	if ((test & FILE_IS_REGULAR) == FILE_IS_REGULAR) {
		if (! S_ISREG (file_info.st_mode))
			return axl_false;

		/* reached this point the file is link */
		result = axl_true;
	}

	/* check if the file is a directory */
	if ((test & FILE_IS_DIR) == FILE_IS_DIR) {
		if (! S_ISDIR (file_info.st_mode)) {
			return axl_false;
		}

		/* reached this point the file is link */
		result = axl_true;
	}

	/* return current result */
	return result;
}

/* @} */
