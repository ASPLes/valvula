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

/* global include */
#include <valvula.h>

/* private include */
#include <valvula_private.h>

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

	/* return context created */
	return ctx;
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

	valvula_log (VALVULA_LEVEL_DEBUG, "finishing ValvulaCtx %p", ctx);

	/* release and clean mutex */
	valvula_mutex_unlock (&ctx->ref_mutex);
	valvula_mutex_destroy (&ctx->ref_mutex);

	valvula_log (VALVULA_LEVEL_DEBUG, "about.to.free ValvulaCtx %p", ctx);

	/* free the context */
	axl_free (ctx);
	
	return;
}

/** 
 * @}
 */
