/* 
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2017 Advanced Software Production Line, S.L.
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
#ifndef __VALVULAD_MODULE_H__
#define __VALVULAD_MODULE_H__

#include <valvulad.h>

/** 
 * @brief Type definition for a single module loaded into valvulad.
 */ 
typedef struct _ValvuladModule {
	/* module attributes */
	char             * path;
	void             * handle;
	ValvuladModDef   * def;
	axl_bool           skip_unmap;

	/* context that loaded the module */
	ValvuladCtx      * ctx;

	/* list of profiles provided by this module */
	axlList          * provided_profiles;

} ValvuladModule;

/** 
 * @brief Set of handlers that are supported by modules. This handler
 * descriptors are used by some functions to notify which handlers to
 * call: \ref turbulence_module_notify.
 */
typedef enum {
	/** 
	 * @brief Module reload handler 
	 */
	VLD_RELOAD_HANDLER = 1,
	/** 
	 * @brief Module close handler 
	 */
	VLD_CLOSE_HANDLER  = 2,
	/** 
	 * @brief Module init handler 
	 */
	VLD_INIT_HANDLER   = 3,
	/** 
	 * @brief Module process handler
	 */
	VLD_PROCESS_HANDLER   = 4
} ValvuladModHandler;

void               valvulad_module_init         (ValvuladCtx * ctx);

ValvuladModule   * valvulad_module_open         (ValvuladCtx   * ctx, 
						 const char    * module);

ValvuladModule   * valvulad_module_find_by_name (ValvuladCtx * ctx,
						 const char  * mod_name);

axlPointer         valvulad_module_get_symbol   (ValvuladCtx    * ctx,
						 ValvuladModule * module,
						 const char     * name);

void               valvulad_module_unload      (ValvuladCtx   * ctx,
						const char    * module);

const char       * valvulad_module_name        (ValvuladModule * module);

ModInitFunc        valvulad_module_get_init    (ValvuladModule * module);

ModCloseFunc       valvulad_module_get_close   (ValvuladModule * module);

axl_bool           valvulad_module_exists      (ValvuladModule * module);

axl_bool           valvulad_module_register    (ValvuladModule * module);

void               valvulad_module_unregister  (ValvuladModule * module);

ValvuladModule * valvulad_module_open_and_register (ValvuladCtx * ctx, 
						    const char * location);

void               valvulad_module_skip_unmap  (ValvuladCtx * ctx, 
						const char * mod_name);

void               valvulad_module_free        (ValvuladModule  * module);

axl_bool           valvulad_module_notify      (ValvuladCtx           * ctx, 
						ValvuladModHandler      handler,
						axlPointer              data,
						axlPointer              data2,
						axlPointer              data3);

void               valvulad_module_notify_reload_conf (ValvuladCtx * ctx);

void               valvulad_module_notify_close (ValvuladCtx * ctx);

void               valvulad_module_set_no_unmap_modules (axl_bool status);

void               valvulad_module_cleanup      (ValvuladCtx * ctx);

#endif
