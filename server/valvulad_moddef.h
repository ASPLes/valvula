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
#ifndef __VALVULAD_MODDEF_H__
#define __VALVULAD_MODDEF_H__

/** 
 * \defgroup valvulad_moddef Valvulad Module Def: Type and handler definitions for valvulad modules
 */

/**
 * \addtogroup valvulad_moddef
 * @{
 */

/** 
 * @brief Public definition for the init function that must implement
 * a valvulad module.
 *
 * The module must return axl_true to signal the modules was properly
 * initialized so valvulad can register it as usable.
 *
 * @param ctx The valvulad context where the init operation is
 * taking place.
 * 
 * @return axl_true if the module is usable or axl_false if
 * not. Returning axl_false caused the module to be not loaded.
 */
typedef axl_bool  (*ModInitFunc)  (ValvuladCtx * ctx);

/** 
 * @brief Public definition for the close function that must implement
 * all operations required to unload and terminate a module.
 * 
 * The function doesn't receive and return any data.
 *
 * @param ctx The valvulad context where the close operation is
 * taking place.
 */
typedef void (*ModCloseFunc) (ValvuladCtx * ctx);

/** 
 * @brief A reference to the unload method that must implement all
 * unload code required in the case valvulad ask the module
 * to stop its function.
 * 
 * Unload function is only called in the context of child process
 * created by valvulad to isolate some requests (modules and
 * profiles) to be handled by a low permissions user.
 *
 * @param ctx The valvulad context where the close operation is
 * taking place.
 */
typedef void (*ModUnloadFunc) (ValvuladCtx * ctx);

/** 
 * @brief Public definition for the reconfiguration function that must
 * be implemented to receive notification if the valvulad server
 * configuration is reloaded.
 *
 * @param ctx The valvulad context where the reconf operation is
 * taking place.
 */
typedef void (*ModReconfFunc) (ValvuladCtx * ctx);

/** 
 * @brief Public definition for the main entry point for all modules
 * developed for valvulad. 
 *
 * This structure contains pointers to all functions that may
 * implement a valvulad module.
 *
 * See <a class="el" href="http://www.aspl.es/valvulad/extending.html">how to create Valvulad modules</a>.
 */
typedef struct _ValvuladModDef {
	/** 
	 * @brief The module name. This name is used by valvulad to
	 * refer to the module.
	 */
	char         * mod_name;

	/** 
	 * @brief The module long description.
	 */
	char         * mod_description;

	/** 
	 * @brief A reference to the init function associated to the
	 * module.
	 */
	ModInitFunc    init;

	/** 
	 * @brief A reference to the close function associated to the
	 * module.
	 */
	ModCloseFunc   close;

	/** 
	 * @brief A reference to the process request handler (if
	 * defined).
	 */
	ValvulaProcessRequest process_request;
	
	/** 
	 * @brief A reference to the reconf function associated to the
	 * module.
	 */
	ModReconfFunc  reconf;

	/** 
	 * @brief A reference to the unload function associated to the
	 * module.
	 */
	ModUnloadFunc  unload;

} ValvuladModDef;

/** 
 * @brief Allows to prepare de module with the valvulad context (and
 * the vortex context associated).
 *
 * This macro must be called inside the module init, before any
 * operation is done. 
 * 
 * @param _ctx The context received by the module at the init functio.
 */
#define VLD_MOD_PREPARE(_ctx) do{ctx = _ctx;}while(0)

#endif

/* @} */
