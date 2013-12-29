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

	/** mutexes **/
	ValvulaMutex exit_mutex;
	ValvulaMutex ref_mutex;
	ValvulaMutex inet_ntoa_mutex;
	
};

/** 
 * @internal Definition of Valvula connection
 */
struct _ValvulaConnection {
	ValvulaCtx * ctx;
};

#endif
