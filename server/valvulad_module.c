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

#include <valvulad.h>


#if defined(AXL_OS_UNIX)
#include <dlfcn.h>
#endif

axl_bool __valvulad_module_no_unmap = axl_false;

/** 
 * @internal Starts the valvulad module initializing all internal
 * variables.
 */
void               valvulad_module_init      (ValvuladCtx * ctx)
{
	/* a list of all modules loaded */
	ctx->registered_modules = axl_list_new (axl_list_always_return_1, 
						(axlDestroyFunc) valvulad_module_free);
	/* init mutex */
	valvula_mutex_create (&ctx->registered_modules_mutex);
	return;
}

/** 
 * @brief Allows to get the module name.
 *
 * @param module The module to get the name from.
 *
 * @return module name or NULL if it fails.
 */
const char       * valvulad_module_name        (ValvuladModule * module)
{
	axl_return_val_if_fail (module, NULL);
	axl_return_val_if_fail (module->def, NULL);

	/* return stored name */
	return module->def->mod_name;
}

/** 
 * @brief Loads a valvulad module, from the provided path.
 * 
 * @param ctx The context where the module will be opened.
 * @param module The module path to load.
 * 
 * @return A reference to the module loaded or NULL if it fails.
 */
ValvuladModule * valvulad_module_open (ValvuladCtx * ctx, const char * module)
{
	ValvuladModule * result;

	axl_return_val_if_fail (module, NULL);

	/* allocate memory for the result */
	result         = axl_new (ValvuladModule, 1);
	result->path   = axl_strdup (module);
	result->ctx    = ctx;
#if defined(AXL_OS_UNIX)
	result->handle = dlopen (module, RTLD_LAZY | RTLD_GLOBAL);
#elif defined(AXL_OS_WIN32)
	result->handle = LoadLibraryEx (module, NULL, 0);
	if (result->handle == NULL)
		result->handle = LoadLibraryEx (module, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
#endif

	/* check loaded module */
	if (result->handle == NULL) {
		/* unable to load the module */
#if defined(AXL_OS_UNIX)
		error ("unable to load module (%s): %s", module, dlerror ());
#elif defined(AXL_OS_WIN32)
		error ("unable to load module (%s), error code: %d", module, GetLastError ());
#endif

		/* free the result and return */
		valvulad_module_free (result);
		return NULL;
	} /* end if */

	/* find the module */
#if defined(AXL_OS_UNIX)
	result->def = (ValvuladModDef *) dlsym (result->handle, "module_def");
#elif defined(AXL_OS_WIN32)
	result->def = (ValvuladModDef *) GetProcAddress (result->handle, "module_def");
#endif
	if (result->def == NULL) {
		/* unable to find module def */
#if defined(AXL_OS_UNIX)
		error ("unable to find 'module_def' symbol, it seems it isn't a valvulad module: %s", dlerror ());
#elif defined(AXL_OS_WIN32)
		error ("unable to find 'module_def' symbol, it seems it isn't a valvulad module: error code %d", GetLastError ());
#endif
		
		/* free the result and return */
		valvulad_module_free (result);
		return NULL;
	} /* end if */
	
	msg ("module found: [%s]", result->def->mod_name);
	
	return result;
}

/** 
 * @brief Allows to get a particular symbol (by name) from the provided module.
 *
 * @param ctx The context where the operation will happen.
 *
 * @param module The module where the look up operation will take place.
 *
 * @param name The symbol name that is being looked. The caller must
 * properly cast reference returned to work. This function is not safe.
 *
 * @return The function returns the reference found or NULL if it
 * fails or ctx, module or name references are NULL.
 */
axlPointer         valvulad_module_get_symbol   (ValvuladCtx    * ctx,
						 ValvuladModule * module,
						 const char     * name)
{
	if (ctx == NULL || module == NULL || name == NULL)
		return NULL;

	/* find the module */
#if defined(AXL_OS_UNIX)
	return dlsym (module->handle, name);
#elif defined(AXL_OS_WIN32)
	return GetProcAddress (module->handle, name);
#endif
}

/** 
 * @brief Allows to find a the module by the provided name.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param mod_name The module name to find by name.
 *
 * @return The module reference or NULL if it fails.
 */
ValvuladModule   * valvulad_module_find_by_name (ValvuladCtx * ctx,
						 const char  * mod_name)
{
	int              iterator;
	ValvuladModule * module;

	/* check values received */
	if (ctx == NULL || mod_name == NULL)
		return NULL;

	/* register the module */
	valvula_mutex_lock (&ctx->registered_modules_mutex);

	/* check first that the module is not already added */
	iterator = 0;
	while (iterator < axl_list_length (ctx->registered_modules)) {

		/* get module in position */
		module = axl_list_get_nth (ctx->registered_modules, iterator);
	
		/* check mod name */
		if (axl_cmp (module->def->mod_name, mod_name)) {
			/* terminate it */
			valvula_mutex_unlock (&ctx->registered_modules_mutex);

			return module;
		} /* end if */

		/* next position */
		iterator++;
	} /* end while */

	/* release lock */
	valvula_mutex_unlock (&ctx->registered_modules_mutex);

	/* module not found */
	return NULL;
}

/** 
 * @brief Allows to close the provided module, unloading from memory
 * and removing all registration references. The function do not
 * perform any module space notification. The function makes use of
 * the unload function implemented by modules (if defined).
 *
 * @param ctx The context where the module must be removed.
 * @param module The module name to be unloaded.
 */
void               valvulad_module_unload       (ValvuladCtx * ctx,
						   const char    * module)
{
	int                iterator;
	ValvuladModule * mod_added;

	/* check values received */
	if (ctx == NULL || module == NULL)
		return;

	/* register the module */
	valvula_mutex_lock (&ctx->registered_modules_mutex);

	/* check first that the module is not already added */
	iterator = 0;
	while (iterator < axl_list_length (ctx->registered_modules)) {
		/* get module in position */
		mod_added = axl_list_get_nth (ctx->registered_modules, iterator);

		/* check mod name */
		if (axl_cmp (mod_added->def->mod_name, module)) {
			/* call to unload module */
			if (mod_added->def->unload != NULL)
				mod_added->def->unload (ctx);

			/* remove from registered modules */
			axl_list_remove_at (ctx->registered_modules, iterator);

			/* terminate it */
			valvula_mutex_unlock (&ctx->registered_modules_mutex);
			return;
		} /* end if */

		/* next position */
		iterator++;
	} /* end while */
	valvula_mutex_unlock (&ctx->registered_modules_mutex);

	/* no module with the same name was found */
	return;
}

/** 
 * @brief Allows to get the init function for the module reference
 * provided.
 * 
 * @param module The module that is being requested to return the init
 * function.
 * 
 * @return A reference to the init function or NULL if the function
 * doesn't have a init function defined (which is possible and allowed).
 */
ModInitFunc        valvulad_module_get_init  (ValvuladModule * module)
{
	/* check the reference received */
	if (module == NULL)
		return NULL;

	/* return the reference */
	return module->def->init;
}

/** 
 * @brief Allows to get the close function for the module reference
 * provided.
 * 
 * @param module The module that is being requested to return the
 * close function.
 * 
 * @return A refernce to the close function or NULL if the function
 * doesn't have a close function defined (which is possible and
 * allowed).
 */
ModCloseFunc       valvulad_module_get_close (ValvuladModule * module)
{
	/* check the reference received */
	if (module == NULL)
		return NULL;

	/* return the reference */
	return module->def->close;
}

/** 
 * @brief Allows to check if there is another module already
 * registered with the same name.
 */
axl_bool           valvulad_module_exists      (ValvuladModule * module)
{
	ValvuladCtx    * ctx;
	int                iterator;
	ValvuladModule * mod_added;

	/* check values received */
	v_return_val_if_fail (module, axl_false);

	/* get context reference */
	ctx = module->ctx;

	/* register the module */
	valvula_mutex_lock (&ctx->registered_modules_mutex);

	/* check first that the module is not already added */
	iterator = 0;
	while (iterator < axl_list_length (ctx->registered_modules)) {
		/* get module in position */
		mod_added = axl_list_get_nth (ctx->registered_modules, iterator);

		/* check mod name */
		if (axl_cmp (mod_added->def->mod_name, module->def->mod_name)) {
			valvula_mutex_unlock (&ctx->registered_modules_mutex);
			return axl_true;
		} /* end if */

		/* next position */
		iterator++;
	} /* end while */
	valvula_mutex_unlock (&ctx->registered_modules_mutex);

	/* no module with the same name was found */
	return axl_false;
}

/** 
 * @brief Allows to register the module loaded to allow future access
 * to it.
 * 
 * @param module The module being registered.
 *
 * @return axl_false in the case module was not registered, otherwise
 * axl_true when the module is registered.
 */
axl_bool             valvulad_module_register  (ValvuladModule * module)
{
	ValvuladCtx    * ctx;
	int                iterator;
	ValvuladModule * mod_added;

	/* check values received */
	v_return_val_if_fail (module, axl_false);

	/* get context reference */
	ctx = module->ctx;

	/* register the module */
	valvula_mutex_lock (&ctx->registered_modules_mutex);

	/* check first that the module is not already added */
	iterator = 0;
	while (iterator < axl_list_length (ctx->registered_modules)) {
		/* get module in position */
		mod_added = axl_list_get_nth (ctx->registered_modules, iterator);

		/* check mod name */
		if (axl_cmp (mod_added->def->mod_name, module->def->mod_name)) {
			wrn ("skipping module found: %s, already found a module registered with the same name, at path: %s",
			     module->def->mod_name, mod_added->path);
			valvula_mutex_unlock (&ctx->registered_modules_mutex);
			return axl_false;
		} /* end if */

		/* next position */
		iterator++;
	} /* end while */

	axl_list_add (ctx->registered_modules, module);
	msg ("Registered modules (%d, %p)", axl_list_length (ctx->registered_modules), ctx->registered_modules);

	valvula_mutex_unlock (&ctx->registered_modules_mutex);

	return axl_true;
}

/** 
 * @brief Unregister the module provided from the list of modules
 * loaded. The function do not close the module (\ref
 * valvulad_module_free). This is required to be done by the
 * caller.
 * 
 * @param module The module to unregister.
 */
void               valvulad_module_unregister  (ValvuladModule * module)
{
	ValvuladCtx    * ctx;

	/* check values received */
	v_return_if_fail (module);

	/* get a reference to the context */
	ctx = module->ctx;

	/* register the module */
	valvula_mutex_lock (&ctx->registered_modules_mutex);
	axl_list_unlink (ctx->registered_modules, module);
	valvula_mutex_unlock (&ctx->registered_modules_mutex);

	return;
}

/** 
 * @brief High level function that opens the module located at the
 * provided location, checking if the module was registered, calling
 * to init it, and then registering it if the init did not fail.
 *
 * @param ctx The context where the load operation will take place.
 * @param location The location where the module is found.
 *
 * @return A reference to the module loaded (\ref ValvuladModule) or NULL if it fails.
 */
ValvuladModule           * valvulad_module_open_and_register (ValvuladCtx * ctx, const char * location)
{
	ValvuladModule * module;
	ModInitFunc        init;

	if (ctx == NULL || location == NULL)
		return NULL;

	/* load the module man!!! */
	module = valvulad_module_open (ctx, location);
	if (module == NULL) {
		wrn ("unable to open module: %s", location);
		return NULL;
	} /* end if */

	/* check if module exists */
	if (valvulad_module_exists (module)) {
		wrn ("unable to load module: %s, another module is already loaded with the same name",
		     valvulad_module_name (module));

		/* close the module */
		valvulad_module_free (module);
		return NULL;
	}

	/* init the module */
	init = valvulad_module_get_init (module);
		
	/* check init */
	if (! init (ctx)) {
		wrn ("init module: %s have failed, skiping", location);
		
		/* close the module */
		valvulad_module_free (module);
		
		return NULL;
		
	} else {
		msg ("init ok, registering module: %s", location);
	}

	/* register the module to be loaded */
	valvulad_module_register (module);

	return module;
}

/** 
 * @brief Allows to mark a module to not be unmapped when module is
 * closed.
 * @param ctx The context that will be configured.
 * @param mod_name The module name to skip unmapping.
 */
void               valvulad_module_skip_unmap  (ValvuladCtx * ctx, 
						  const char * mod_name)
{
	int                iterator;
	ValvuladModule * mod_added;

	/* check values received */
	v_return_if_fail (ctx);
	v_return_if_fail (mod_name);

	/* register the module */
	valvula_mutex_lock (&ctx->registered_modules_mutex);

	/* check first that the module is not already added */
	iterator = 0;
	while (iterator < axl_list_length (ctx->registered_modules)) {
		/* get module in position */
		mod_added = axl_list_get_nth (ctx->registered_modules, iterator);

		/* check mod name */
		if (axl_cmp (mod_added->def->mod_name, mod_name)) {
			/* flag skip */
			mod_added->skip_unmap = axl_true;
			valvula_mutex_unlock (&ctx->registered_modules_mutex);
			return;
		} /* end if */

		/* next position */
		iterator++;
	} /* end while */

	valvula_mutex_unlock (&ctx->registered_modules_mutex);
	return;
}

/** 
 * @brief Closes and free the module reference.
 * 
 * @param module The module reference to free and close.
 */
void               valvulad_module_free (ValvuladModule * module)
{
	ValvuladCtx * ctx;
	/* check values received */
	v_return_if_fail (module);

	ctx = module->ctx;

	msg ("Unmapping module (%d)?: %s (%s)", ! __valvulad_module_no_unmap && ! module->skip_unmap, module->def ? module->def->mod_name : "undef", module->path);

	axl_free (module->path);
	/* call to unload the module */
	if (module->handle && ! __valvulad_module_no_unmap && ! module->skip_unmap) {
#if defined(AXL_OS_UNIX)
		dlclose (module->handle);
#elif defined(AXL_OS_WIN32)
		FreeLibrary (module->handle);
#endif
	}
	axl_free (module);
	
	
	return;
}

/** 
 * @brief Allows to do a handler notification on all registered
 * modules.
 * @param ctx The context where the notification will take place.
 * @param handler The handler to be called.
 *
 * @param data Optional data to be passed to the handler. This pointer
 * is handler especific.
 *
 * @param data2 Second optional data to be passed to the handler. This
 * pointer is handler especific.
 *
 * @param data3 Third optional data to be passed to the handler. This
 * pointer is handler especific.
 *
 * @return Returns axl_true if all handlers executed also returned
 * axl_true. Those handler that have no return value will cause the
 * function to always return axl_true. 
 */
axl_bool           valvulad_module_notify      (ValvuladCtx         * ctx, 
						ValvuladModHandler    handler,
						axlPointer              data,
						axlPointer              data2,
						axlPointer              data3)
{
	/* get valvulad context */
	ValvuladModule   * module;
	int                  iterator = 0;

	valvula_mutex_lock (&ctx->registered_modules_mutex);

	/* load search paths here in case of VLD_PPATH_SELECTED_HANDLER */
	while (iterator < axl_list_length (ctx->registered_modules)) {
		/* get the module */
		module = axl_list_get_nth (ctx->registered_modules, iterator);

		/* check reference */
		if (module == NULL || module->def == NULL) {
			iterator++;
			continue;
		} /* end if */
			
		switch (handler) {
		case VLD_CLOSE_HANDLER:
			/* notify if defined reconf function */
			if (module->def->close != NULL) {
				msg ("closing module: %s (%s)", module->def->mod_name, module->path);
				valvula_mutex_unlock (&ctx->registered_modules_mutex);
				module->def->close (ctx);
				valvula_mutex_lock (&ctx->registered_modules_mutex);
			}
			break;
		case VLD_RELOAD_HANDLER:
			/* notify if defined reconf function */
			if (module->def->reconf != NULL) {
				msg ("reloading module: %s (%s)", module->def->mod_name, module->path);
				valvula_mutex_unlock (&ctx->registered_modules_mutex);
				module->def->reconf (ctx);
				valvula_mutex_lock (&ctx->registered_modules_mutex);
			}
			break;
		case VLD_INIT_HANDLER:
			/* notify if defined reconf function */
			if (module->def->init != NULL) {
				msg ("initializing module: %s (%s)", module->def->mod_name, module->path);
				valvula_mutex_unlock (&ctx->registered_modules_mutex);
				if (! module->def->init (ctx)) {
					/* init failed */
					wrn ("failed to initialized module: %s, it returned initialization failure", module->def->mod_name);
					return axl_false;
				}
				valvula_mutex_lock (&ctx->registered_modules_mutex);
					
			}
			break;
		case VLD_PROCESS_HANDLER:
			/* not defined */
			break;
		}

		/* next iterator */
		iterator++;

	} /* end if */
	valvula_mutex_unlock (&ctx->registered_modules_mutex);

	/* reached this point always return TRUE dude!! */
	return axl_true;
}


/** 
 * @brief Allows to notify all modules loaded, that implements the
 * \ref ModReconfFunc, to reload its configuration data.
 */
void               valvulad_module_notify_reload_conf (ValvuladCtx * ctx)
{
	valvulad_module_notify (ctx, VLD_RELOAD_HANDLER, NULL, NULL, NULL);

	return;
}

/** 
 * @brief Send a module close notification to all modules registered
 * without unloading module code.
 */
void               valvulad_module_notify_close (ValvuladCtx * ctx)
{
	valvulad_module_notify (ctx, VLD_CLOSE_HANDLER, NULL, NULL, NULL);

	return;
}

void               valvulad_module_set_no_unmap_modules (axl_bool status)
{
	__valvulad_module_no_unmap = status;
}

/** 
 * @brief Cleans the module, releasing all resources and unloading all
 * modules.
 */
void               valvulad_module_cleanup   (ValvuladCtx * ctx)
{
	/* do not operate if a null value is received */
	if (ctx == NULL)
		return;

	/* release the list and all modules */
	msg ("Cleaning up valvulad %d modules (ctx: %p)..", axl_list_length (ctx->registered_modules), ctx);
	axl_list_free (ctx->registered_modules);
	ctx->registered_modules = NULL;
	valvula_mutex_destroy (&ctx->registered_modules_mutex);

	return;
}




