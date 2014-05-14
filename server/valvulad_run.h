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
#ifndef __VALVULAD_RUN_H__
#define __VALVULAD_RUN_H__

#include <valvulad.h>

axl_bool valvulad_run_config (ValvuladCtx * ctx);

axl_bool valvulad_run_is_local_domain (ValvuladCtx * ctx, const char * domain);

axl_bool valvulad_run_is_local_address (ValvuladCtx * ctx, const char * address);

axl_bool valvulad_run_is_local_delivery (ValvuladCtx * ctx, ValvulaRequest * request);

void     valvulad_run_add_local_domain (ValvuladCtx * ctx, const char * domain);

axl_bool valvulad_run_check_local_domains_config (ValvuladCtx * ctx);

#endif
