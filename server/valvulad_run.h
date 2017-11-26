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
#ifndef __VALVULAD_RUN_H__
#define __VALVULAD_RUN_H__

#include <valvulad.h>

/** 
 * @brief Resolver request type. Indicates what kind of request is
 * being requested (domain, account, alias...).
 */
typedef enum {

	/** 
	 * @brief Request to resolv if object is an account.
	 */
	VALVULAD_OBJECT_ACCOUNT = 1,
	
	/** 
	 * @brief Request to resolv if object is a domain.
	 */
	VALVULAD_OBJECT_DOMAIN = 2,
	
	/** 
	 * @brief Request to resolv if object is an alias.
	 */
	VALVULAD_OBJECT_ALIAS = 3
	
} ValvuladObjectRequest;

/** 
 * @brief Optional object resolver handler to help valvula engine to
 * detect accounts, domains and aliases that are local to the current
 * server.
 *
 * By default, valvula tries to determine current postfix
 * configuration to find domains, accounts and aliases that are
 * recognized by postfix to better apply rules defined by user.
 *
 * For example, if a request received into postfix and notified to
 * valvula is detected to be delivered to a certain user, it is then
 * perceived as a local delivery (by valvula).
 *
 * However, without that information, valvula cannot take certain
 * decisions: it will see all addresses as remote (from remote sender
 * to a remote receiver, as if it were a relay operation).
 *
 * In such context, if valvula is not able to detect such
 * configuration, a module can provide an object resolver to report if
 * an account/domain/alias (item_name) is a local object.
 *
 * These handlers are configured using \ref valvulad_run_add_object_resolver
 *
 * @param ctx The context where the operation happens.
 *
 * @param item_name The item name to be checked to be local (domain, account or alias), according to the request_type.
 *
 * @param request_type The request type received: if is an account, domain or alias.
 *
 * @param data User defined pointer (configured at \ref valvulad_run_add_object_resolver).
 */
typedef axl_bool (*ValvuladObjectResolver) (ValvuladCtx * ctx, const char * item_name, ValvuladObjectRequest request_type, axlPointer data);

axl_bool valvulad_run_config (ValvuladCtx * ctx);

axl_bool valvulad_run_is_local_domain (ValvuladCtx * ctx, const char * domain);

axl_bool valvulad_run_is_local_address (ValvuladCtx * ctx, const char * address);

axl_bool valvulad_run_is_local_delivery (ValvuladCtx * ctx, ValvulaRequest * request);

void     valvulad_run_add_local_domain (ValvuladCtx * ctx, const char * domain);

axl_bool valvulad_run_check_local_domains_config (ValvuladCtx * ctx);

axl_bool valvulad_run_check_local_domains_config_detect_postfix_decl (ValvuladCtx * ctx, 
								      const char  * postfix_decl, 
								      const char  * section);
void     valvulad_run_add_object_resolver (ValvuladCtx * ctx, ValvuladObjectResolver resolver, axlPointer data);

/**** private api ****/
void valvulad_run_load_modules (ValvuladCtx * ctx, axlDoc * doc);

#endif
