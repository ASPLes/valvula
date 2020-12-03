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
#ifndef __VALVULA_SUPPORT_H__
#define __VALVULA_SUPPORT_H__

/**
 * \addtogroup valvula_support
 * @{
 */

#include <valvula.h>

int      valvula_support_getenv_int                 (const char * env_name);

char *   valvula_support_getenv                     (const char * env_name);

axl_bool valvula_support_setenv                     (const char * env_name, 
						    const char * env_value);

axl_bool valvula_support_unsetenv                   (const char * env_name);

/** 
 * @brief Available tests to be performed while using \ref
 * valvula_support_file_test
 */
typedef enum {
	/** 
	 * @brief Check if the path exist.
	 */
	FILE_EXISTS     = 1 << 0,
	/** 
	 * @brief Check if the path provided is a symlink.
	 */
	FILE_IS_LINK    = 1 << 1,
	/** 
	 * @brief Check if the path provided is a directory.
	 */
	FILE_IS_DIR     = 1 << 2,
	/** 
	 * @brief Check if the path provided is a regular file.
	 */
	FILE_IS_REGULAR = 1 << 3
} ValvulaFileTest;

axl_bool valvula_support_file_test                  (const char * path,   
						    ValvulaFileTest test);

char   * valvula_support_build_filename             (const char  * name, ...);

double   valvula_support_strtod                     (const char  * param,
						     char       ** string_aux);

int      valvula_support_itoa                       (unsigned int   value,
						    char         * buffer,
						    int            buffer_size);

int      valvula_timeval_substract                  (struct timeval * a, 
						     struct timeval * b,
						     struct timeval * result);

char   * valvula_support_inet_ntoa                  (ValvulaCtx          * ctx, 
 						    struct sockaddr_in * sin);

int      valvula_support_pipe                       (ValvulaCtx * ctx, int descf[2]);

const char * valvula_support_state_str              (ValvulaState state);

/* @} */

#endif
