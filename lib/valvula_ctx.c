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

/* global include */
#include <valvula.h>

/* private include */
#include <valvula_private.h>

/** 
 * \defgroup valvula_ctx ValvulaCtx: libValvula context object, the object used to store/represent a single libValvula instance.
 */

/** 
 * \addtogroup valvula_ctx
 * @{
 */

ValvulaCtx * valvula_ctx_new (void)
{
	ValvulaCtx * ctx;

	/* create a new context */
	ctx           = axl_new (ValvulaCtx, 1);
	VALVULA_CHECK_REF (ctx, NULL);
	valvula_log (VALVULA_LEVEL_DEBUG, "created ValvulaCtx reference %p", ctx);

	/* create the hash to store data */
	ctx->data     = valvula_hash_new (axl_hash_string, axl_hash_equal_string);
	VALVULA_CHECK_REF2 (ctx->data, NULL, ctx, axl_free);

	/**** valvula_thread_pool.c: init ****/
	ctx->thread_pool_exclusive = axl_true;

	/* init reference counting */
	valvula_mutex_create (&ctx->ref_mutex);
	ctx->ref_count = 1;

	/* default state */
	ctx->default_state = VALVULA_STATE_DUNNO;

	/* set default line limit request */
	ctx->request_line_limit = 40;

	/* init stats mutex */
	valvula_mutex_create (&ctx->stats_mutex);

	/* init op mutex */
	valvula_mutex_create (&ctx->op_mutex);
	ctx->request_in_process = axl_list_new (axl_list_always_return_1, axl_free);

	/* return context created */
	return ctx;
}

/** 
 * @brief Allows to change and limit request line limit for every
 * request received. If the provided request limit is exceeded the
 * connection is closed.
 *
 * @param ctx The context where the request line limit is going to be
 * configured.
 *
 * @param line_limit The line limit to be configured.
 *
 * 
 */
void        valvula_ctx_set_request_line_limit    (ValvulaCtx       * ctx,
						   int                line_limit)
{
	if (ctx == NULL)
		return;
	ctx->request_line_limit = line_limit;
	return;
}

void __valvula_ctx_free_registry (axlPointer _ptr) {
	ValvulaRequestRegistry * reg = _ptr;

	valvula_mutex_destroy (&reg->stats_mutex);
	axl_free (reg->identifier);
	axl_free (reg);
	
	return;
}

/** 
 * @brief Allows to register a new process handler with the provided priority under the given port.
 *
 * The process handler is a function that is called every time a
 * request has to be resolved. For that, Valvula's engine uses the
 * port to know if the handler must be called and the priority.
 *
 * The port is an indication about when the handler should be
 * considered for execution. In the case -1 is provided, then the
 * handler is always considered. In the case a number is provided,
 * then that number is compared with the listener's port where the
 * request was received. If it matches, then it is considered for
 * execution (according to its priority). 
 *
 * You can use the same port or -1 for all your handlers and all
 * handlers will be considered, considering their priority.
 *
 * The priority value, that can be a value between 1 and 32768, is an
 * indication to Valvula's engine about what handler must be called
 * first. A priority of 1 is higher than 32768, that is, a handler
 * with priority 1 is called first a handler with priority 32768.
 *
 * @param ctx The context where the operation takes place.
 *
 * @param identifier This is a textual string that identifies this
 * handler and provides information about its source. This info is
 * used by the system to now what module or what does this handler
 * (when you use valvulad -s to show stats).
 *
 * @param process_handler The handler that is going to be registered.
 *
 * @param priority The priority to give to the handler (between 1 and 32768).
 *
 * @param port The port to restrict the handler or -1 to have it
 * executed in all ports.
 *
 * @param user_data User defined pointer to be passed to the process_handler when it is called.
 *
 * @return A registry pointer that represents the registry of this
 * handler in this context. You can use this reference to remove the
 * handler or query its settings. The function returns NULL in case of
 * a failure while registering the handler.
 *
 * 
 */
ValvulaRequestRegistry *   valvula_ctx_register_request_handler (ValvulaCtx             * ctx, 
								 const char             * identifier,
								 ValvulaProcessRequest    process_handler, 
								 int                      priority, 
								 int                      port,
								 axlPointer               user_data)
{
	ValvulaRequestRegistry * registry;

	if (ctx == NULL || process_handler == NULL)
		return NULL;
	if (priority < 1 || priority > 32768)
		return NULL;

	/* allow memory and check */
	registry = axl_new (ValvulaRequestRegistry, 1);
	if (registry == NULL)
		return NULL;

	/* set all parameters */
	registry->ctx             = ctx;
	registry->identifier      = axl_strdup (identifier);
	registry->process_handler = process_handler;
	registry->priority        = priority;
	registry->port            = port;
	registry->user_data       = user_data;

	/* init stat mutex */
	valvula_mutex_create (&registry->stats_mutex);
	
	valvula_mutex_lock (&ctx->ref_mutex);

	/* create request registry hash to store all process handlers */
	if (ctx->process_handler_registry == NULL)
		ctx->process_handler_registry = valvula_hash_new (axl_hash_int, axl_hash_equal_int);
	
	/* register */
	valvula_hash_replace_full (ctx->process_handler_registry, registry, __valvula_ctx_free_registry, registry, NULL);

	valvula_mutex_unlock (&ctx->ref_mutex);

	return registry;
}

/** 
 * @brief Allows to register a handler that will be called with the final.
 *
 * @param ctx The context where the handler will be configured.
 *
 * @param handler The handler to be called everytime a final state is found.
 *
 * @param user_data User defined pointer thatwill be passed to the handler configured at the time it is called.
 */
void        valvula_ctx_set_final_state_handler   (ValvulaCtx              * ctx,
						   ValvulaReportFinalState   handler,
						   axlPointer                user_data)
{
	if (ctx == NULL)
		return;
	ctx->report_final_state = handler;
	ctx->report_final_state_user_data = user_data;

	return;
}

/** 
 * @brief Allows to set default reply state to be used in the case no
 * handler is configured or no handler is found for a given port.
 *
 * @param ctx The context to be configured.
 *
 * @param state The state to be used. By default \ref VALVULA_STATE_DUNNO is used.
 */
void        valvula_ctx_set_default_reply_state   (ValvulaCtx       * ctx,
						   ValvulaState       state)
{
	/* check and configure state */
	if (! ctx)
		return;
	ctx->default_state = state;

	return;
}

/** 
 * @brief Allows to store arbitrary data associated to the provided
 * context, which can later retrieved using a particular key. 
 * 
 * @param ctx The ctx where the data will be stored.
 *
 * @param key The key to index the value stored. The key must be a
 * string.
 *
 * @param value The value to be stored. 
 */
void        valvula_ctx_set_data (ValvulaCtx       * ctx, 
				 const char      * key, 
				 axlPointer        value)
{
	v_return_if_fail (ctx && key);

	/* call to configure using full version */
	valvula_ctx_set_data_full (ctx, key, value, NULL, NULL);
	return;
}


/** 
 * @brief Allows to store arbitrary data associated to the provided
 * context, which can later retrieved using a particular key. It is
 * also possible to configure a destroy handler for the key and the
 * value stored, ensuring the memory used will be deallocated once the
 * context is terminated (\ref valvula_ctx_free) or the value is
 * replaced by a new one.
 * 
 * @param ctx The ctx where the data will be stored.
 * @param key The key to index the value stored. The key must be a string.
 * @param value The value to be stored. If the value to be stored is NULL, the function calls to remove previous content stored on the same key.
 * @param key_destroy Optional key destroy function (use NULL to set no destroy function).
 * @param value_destroy Optional value destroy function (use NULL to set no destroy function).
 */
void        valvula_ctx_set_data_full (ValvulaCtx       * ctx, 
				      const char      * key, 
				      axlPointer        value,
				      axlDestroyFunc    key_destroy,
				      axlDestroyFunc    value_destroy)
{
	v_return_if_fail (ctx && key);

	/* check if the value is not null. It it is null, remove the
	 * value. */
	if (value == NULL) {
		valvula_hash_remove (ctx->data, (axlPointer) key);
		return;
	} /* end if */

	/* store the data */
	valvula_hash_replace_full (ctx->data, 
				  /* key and function */
				  (axlPointer) key, key_destroy,
				  /* value and function */
				  value, value_destroy);
	return;
}


/** 
 * @brief Allows to retreive data stored on the given context (\ref
 * valvula_ctx_set_data) using the provided index key.
 * 
 * @param ctx The context where to lookup the data.
 * @param key The key to use as index for the lookup.
 * 
 * @return A reference to the pointer stored or NULL if it fails.
 */
axlPointer  valvula_ctx_get_data (ValvulaCtx       * ctx,
				 const char      * key)
{
	v_return_val_if_fail (ctx && key, NULL);

	/* lookup */
	return valvula_hash_lookup (ctx->data, (axlPointer) key);
}

/** 
 * @brief Allows to increase reference count to the ValvulaCtx
 * instance.
 *
 * @param ctx The reference to update its reference count.
 */
void        valvula_ctx_ref                       (ValvulaCtx  * ctx)
{
	valvula_ctx_ref2 (ctx, "begin ref");
	return;
}

/** 
 * @brief Allows to increase reference count to the ValvulaCtx
 * instance.
 *
 * @param ctx The reference to update its reference count.
 *
 * @param who An string that identifies this ref. Useful for debuging.
 */
void        valvula_ctx_ref2                       (ValvulaCtx  * ctx, const char * who)
{
	/* do nothing */
	if (ctx == NULL)
		return;

	/* acquire the mutex */
	valvula_mutex_lock (&ctx->ref_mutex);
	ctx->ref_count++;

	valvula_log (VALVULA_LEVEL_DEBUG, "%s: increased references to ValvulaCtx %p (refs: %d)", who, ctx, ctx->ref_count);

	valvula_mutex_unlock (&ctx->ref_mutex);

	return;
}

/** 
 * @brief Allows to get current reference counting state from provided
 * valvula context.
 *
 * @param ctx The valvula context to get reference counting
 *
 * @return Reference counting or -1 if it fails.
 */
int         valvula_ctx_ref_count                 (ValvulaCtx  * ctx)
{
	int result;
	if (ctx == NULL)
		return -1;
	
	/* acquire the mutex */
	valvula_mutex_lock (&ctx->ref_mutex); 
	result = ctx->ref_count;
	valvula_mutex_unlock (&ctx->ref_mutex); 

	return result;
}

/** 
 * @brief Decrease reference count and nullify caller's pointer in the
 * case the count reaches 0.
 *
 * @param ctx The context to decrement reference count. In the case 0
 * is reached the ValvulaCtx instance is deallocated and the callers
 * reference is nullified.
 */
void        valvula_ctx_unref                     (ValvulaCtx ** ctx)
{

	valvula_ctx_unref2 (ctx, "unref");
	return;
}

/** 
 * @brief Decrease reference count and nullify caller's pointer in the
 * case the count reaches 0.
 *
 * @param ctx The context to decrement reference count. In the case 0
 * is reached the ValvulaCtx instance is deallocated and the callers
 * reference is nullified.
 *
 * @param who An string that identifies this ref. Useful for debuging.
 */
void        valvula_ctx_unref2                     (ValvulaCtx ** ctx, const char * who)
{
	ValvulaCtx * _ctx;
	axl_bool   nullify;

	/* do nothing with a null reference */
	if (ctx == NULL || (*ctx) == NULL)
		return;

	/* get local reference */
	_ctx = (*ctx);

	/* check if we have to nullify after unref */
	valvula_mutex_lock (&_ctx->ref_mutex);

	/* do sanity check */
	if (_ctx->ref_count <= 0) {
		valvula_mutex_unlock (&_ctx->ref_mutex);

		_valvula_log (NULL, __AXL_FILE__, __AXL_LINE__, VALVULA_LEVEL_CRITICAL, "attempting to unref ValvulaCtx %p object more times than references supported", _ctx);
		/* nullify */
		(*ctx) = NULL;
		return;
	}

	nullify =  (_ctx->ref_count == 1);
	valvula_mutex_unlock (&_ctx->ref_mutex);

	/* call to unref */
	valvula_ctx_free2 (*ctx, who);
	
	/* check to nullify */
	if (nullify)
		(*ctx) = NULL;
	return;
}

/** 
 * @brief Releases the memory allocated by the provided \ref
 * ValvulaCtx.
 * 
 * @param ctx A reference to the context to deallocate.
 */
void        valvula_ctx_free (ValvulaCtx * ctx)
{
	valvula_ctx_free2 (ctx, "end ref");
	return;
}

/** 
 * @brief Releases the memory allocated by the provided \ref
 * ValvulaCtx.
 * 
 * @param ctx A reference to the context to deallocate.
 *
 * @param who An string that identifies this ref. Useful for debuging.
 */
void        valvula_ctx_free2 (ValvulaCtx * ctx, const char * who)
{
	/* do nothing */
	if (ctx == NULL)
		return;

	/* acquire the mutex */
	valvula_mutex_lock (&ctx->ref_mutex);
	ctx->ref_count--;

	if (ctx->ref_count != 0) {
		valvula_log (VALVULA_LEVEL_DEBUG, "%s: decreased references to ValvulaCtx %p (refs: %d)", who, ctx, ctx->ref_count);

		/* release mutex */
		valvula_mutex_unlock (&ctx->ref_mutex);
		return;
	} /* end if */

	/* clear the hash */
	valvula_hash_destroy (ctx->data);
	ctx->data = NULL;

	/* free hash */
	valvula_hash_destroy (ctx->process_handler_registry);
	ctx->process_handler_registry = NULL;

	valvula_log (VALVULA_LEVEL_DEBUG, "finishing ValvulaCtx %p", ctx);

	/* release and clean mutex */
	valvula_mutex_unlock (&ctx->ref_mutex);
	valvula_mutex_destroy (&ctx->ref_mutex);

	valvula_mutex_destroy (&ctx->stats_mutex);
	valvula_mutex_destroy (&ctx->op_mutex);
	axl_list_free (ctx->request_in_process);

	valvula_log (VALVULA_LEVEL_DEBUG, "about.to.free ValvulaCtx %p", ctx);

	/* free the context */
	axl_free (ctx);
	
	return;
}

/** 
 * @}
 */
