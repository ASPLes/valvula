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
#include <valvula_private.h>
#define LOG_DOMAIN "valvula-hash"

/** 
 * \defgroup valvula_hash ValvulaHash: Thread Safe Hash table used inside Valvula Library.
 */

/** 
 * \addtogroup valvula_hash
 * @{
 */

/*
 * @internal function used to notify changes detected on the hash.
 */
void __valvula_hash_notify_change (ValvulaHash * hash_table)
{
	int waiters;

	/* check hash table reference and changed queued */
	if (hash_table == NULL || hash_table->changed_queue == NULL)
		return;

	/* get number of waiters */
	waiters = valvula_async_queue_waiters (hash_table->changed_queue);
	while (waiters > 0) {

		/* push queue one data for each waiter */
		valvula_async_queue_push (hash_table->changed_queue, INT_TO_PTR (1));

		/* decrease waiters */
		waiters--;

	} /* end while */

	return;
}

/** 
 * @brief Creates a new ValvulaHash setting all functions.
 * 
 * Creates a new Valvula Hash Table. All valvula library is programed
 * making heavy use of hash tables so things can go pretty much
 * faster.
 *
 * But this makes race condition to appear anywhere so, this type
 * allow valvula library to create critical section to all operation
 * that are applied to a hash table.
 *
 * @param hash_func
 * @param key_equal_func
 * @param key_destroy_func
 * @param value_destroy_func
 *
 * @return a new ValvulaHash table or NULL if fail
 **/
ValvulaHash * valvula_hash_new_full (axlHashFunc    hash_func,
				   axlEqualFunc   key_equal_func,
				   axlDestroyFunc key_destroy_func,
				   axlDestroyFunc value_destroy_func)
{
	ValvulaHash * result;

	result                 = axl_new (ValvulaHash, 1);
	result->table          = axl_hash_new (hash_func, key_equal_func);
	result->ref_count      = 1;

	/* configuration functions */
	result->hash_func      = hash_func;
	result->key_equal_func = key_equal_func;

	/* destroy functions */
	result->key_destroy    = key_destroy_func;
	result->value_destroy  = value_destroy_func;

	/* the mutex */
	valvula_mutex_create (&result->mutex);
	
	return result;
	
}

/** 
 * @brief Creates a new ValvulaHash without providing destroy function.
 * 
 * A valvula_hash_new_full version passing in as NULL key_destroy_func and 
 * value_destroy_func. 
 *
 * @param hash_func
 * @param key_equal_func
 * 
 * @return a newly created ValvulaHash or NULL if fails.
 **/
ValvulaHash * valvula_hash_new      (axlHashFunc    hash_func,
				   axlEqualFunc   key_equal_func)
{
	return valvula_hash_new_full (hash_func, key_equal_func, NULL, NULL);
}

/** 
 * @brief Allows to increase in one unit the reference counting on the
 * hash table received. A call to \ref valvula_hash_unref will be
 * required to reduce the reference counting. Reaching 0 will cause
 * \ref valvula_hash_destroy to be called automatically.
 *
 * @param hash_table Increase reference counting by one.
 */
void         valvula_hash_ref      (ValvulaHash   * hash_table)
{
	v_return_if_fail (hash_table);
	valvula_mutex_lock (&hash_table->mutex);
	hash_table->ref_count++;
	valvula_mutex_unlock (&hash_table->mutex);
	return;
}

/**
 * @brief Decrease reference counting and, if reached 0 reference a
 * call to \ref valvula_hash_destroy is done.
 *
 * @param hash_table
 */
void         valvula_hash_unref    (ValvulaHash   * hash_table)
{
	/* call to destroy implementation to unify behaviour:
	 * valvula_hash_destroy already implements reference
	 * counting  */
	valvula_hash_destroy (hash_table);
} 

/**
 * @brief Inserts a pair key/value inside the given ValvulaHash
 *
 * 
 * Insert a key/value pair into hash_table.
 *
 * @param hash_table: the hash table
 * @param key: the key to insert
 * @param value: the value to insert
 **/
void         valvula_hash_insert   (ValvulaHash *hash_table,
				   axlPointer key,
				   axlPointer value)
{
	/* check hash table reference */
	if (hash_table == NULL)
		return;
	valvula_mutex_lock   (&hash_table->mutex);

	axl_hash_insert_full (hash_table->table, 
			      key, hash_table->key_destroy, 
			      value, hash_table->value_destroy);

	valvula_mutex_unlock (&hash_table->mutex);

	/* notify change */
	__valvula_hash_notify_change (hash_table);

	return;
}

/**
 * @brief Replace using the given pair key/value into the given hash.
 * 
 * Replace the key/value pair into hash_table. If previous value key/value
 * is not found then the pair is simply added.
 *
 * @param hash_table the hash table to operate on
 * @param key the key value
 * @param value the value to insert
 **/
void         valvula_hash_replace  (ValvulaHash *hash_table,
				   axlPointer key,
				   axlPointer value)
{
	/* check hash table reference */
	if (hash_table == NULL)
		return;
	valvula_mutex_lock   (&hash_table->mutex);
	
	axl_hash_insert_full (hash_table->table, 
			      key, hash_table->key_destroy, 
			      value, hash_table->value_destroy);

	valvula_mutex_unlock (&hash_table->mutex);

	/* notify change */
	__valvula_hash_notify_change (hash_table);

	return;
}

/** 
 * @brief Replace using the given pair key/value into the given hash,
 * providing the particular key and value destroy function, overrding
 * default ones.
 * 
 * Replace the key/value pair into hash_table. If previous value key/value
 * is not found then the pair is simply added.
 *
 * @param hash_table the hash table to operate on
 * @param key the key value
 * @param key_destroy Destroy function to be called for the key.
 * @param value the value to insert
 * @param value_destroy Destroy value function to be called for the data.
 */
void         valvula_hash_replace_full  (ValvulaHash     * hash_table,
					axlPointer       key,
					axlDestroyFunc   key_destroy,
					axlPointer       value,
					axlDestroyFunc   value_destroy)
{
	/* check hash table reference */
	if (hash_table == NULL)
		return;
	valvula_mutex_lock   (&hash_table->mutex);
	
	axl_hash_insert_full (hash_table->table, 
			      key, key_destroy,
			      value, value_destroy);

	valvula_mutex_unlock (&hash_table->mutex);

	/* notify change */
	__valvula_hash_notify_change (hash_table);

	return;
}

/**
 * @brief Returns number of items insisde the hash.
 *
 * @param hash_table the hash table to operate on.
 *
 * @return Number of items inside the hash -1 if fails.
 **/
int      valvula_hash_size     (ValvulaHash *hash_table)
{
	int result;

	/* check hash table reference */
	if (hash_table == NULL)
		return -1;
	valvula_mutex_lock     (&hash_table->mutex);

	result = axl_hash_items (hash_table->table);

	valvula_mutex_unlock   (&hash_table->mutex);	
	
	return result;
}

/** 
 * @brief Perform a lookup using the given key inside the given hash.
 *
 * 
 * Return the value, if found, associated with the key.
 * 
 * @param hash_table the hash table
 * @param key the key value
 *
 * @return the value found or NULL if fails
 **/
axlPointer   valvula_hash_lookup   (ValvulaHash *hash_table,
				   axlPointer  key)
{
	axlPointer data;

	/* check hash table reference */
	if (hash_table == NULL)
		return NULL;
	valvula_mutex_lock   (&hash_table->mutex);
	
	data = axl_hash_get (hash_table->table, key);

	valvula_mutex_unlock (&hash_table->mutex);	

	return data;
}

/** 
 * @brief Allows to check if a key exists (without depending on its
 * actual value).
 *
 * @param hash_table the hash table
 * @param key the key value
 *
 * @return axl_true if the key is found, otherwise axl_false is
 * returned. Note the function returns axl_false in the case a NULL
 * hash table is received.
 *
 **/
axl_bool   valvula_hash_exists   (ValvulaHash *hash_table,
				 axlPointer  key)
{
	axl_bool result;

	/* check hash table reference */
	if (hash_table == NULL)
		return axl_false;

	valvula_mutex_lock   (&hash_table->mutex);
	
	result = axl_hash_exists (hash_table->table, key);

	valvula_mutex_unlock (&hash_table->mutex);	

	return result;
}

/** 
 * @brief Allows to get the data pointed by the provided key and
 * removing it from the table in one step.
 * 
 * Return the value, if found, associated with the key.
 * 
 * @param hash_table the hash table
 * @param key the key value
 *
 * @return the value found or NULL if fails
 */
axlPointer   valvula_hash_lookup_and_clear   (ValvulaHash   *hash_table,
					     axlPointer    key)
{
	
	axlPointer data;
	axl_bool   was_removed;

	/* check hash table reference */
	if (hash_table == NULL)
		return NULL;
	valvula_mutex_lock   (&hash_table->mutex);
	
	/* get the data */
	data = axl_hash_get (hash_table->table, key);

	/* remove the data */
	was_removed = axl_hash_remove (hash_table->table, key);

	/* unlock and return */
	valvula_mutex_unlock (&hash_table->mutex);	

	if (was_removed) {
		/* notify change */
		__valvula_hash_notify_change (hash_table);
	}

	return data;
}

/**
 * @brief Allows the callers to get locked until a change is detected
 * on the hash table (insert, update or remove operation) found or the
 * wait period is reached (wait_microseconds).
 *
 * During the lock operation the hash table remains usable to other
 * callers (including threads).
 *
 * @param hash_table The hash table to wait for changes.
 *
 * @param wait_microseconds The amount of time to wait. If 0 is used,
 * it will wait without limit until next change is produced. 
 *
 * @return The function returns -2 in the case wrong parameters are
 * received (NULL hash table reference or negative value for
 * wait_microseconds). The function returns 0 in the case the
 * wait_microseconds period is reached without any change. 1 is
 * returned in the case a change is detected during the
 * wait_microseconds. Once the function returns, the change has
 * already taken place.
 */
int          valvula_hash_lock_until_changed (ValvulaHash   *hash_table,
					     long          wait_microseconds)
{
	int result;

	v_return_val_if_fail (hash_table && wait_microseconds >= 0, -2);
	
	/* lock the hash */
	valvula_mutex_lock (&hash_table->mutex);

	/* update reference counting */
	hash_table->ref_count++;

	/* check to create the async queue in the case it is not
	 * created */
	if (hash_table->changed_queue == NULL)
		hash_table->changed_queue = valvula_async_queue_new ();

	/* lock the hash */
	valvula_mutex_unlock (&hash_table->mutex);

	/* wait until change is detected */
	if (wait_microseconds > 0)
		result = PTR_TO_INT (valvula_async_queue_timedpop (hash_table->changed_queue, wait_microseconds));
	else 
		result = PTR_TO_INT (valvula_async_queue_pop (hash_table->changed_queue));

	/* reduce reference counting */
	valvula_hash_unref (hash_table);

	/* return result */
	return result;
}

/**
 * @brief Removes the value index by the given key inside the given hash.
 *
 * 
 * Remove a key/pair value from the hash
 * 
 * @param hash_table the hash table
 * @param key the key value to lookup and destroy
 *
 * @return axl_true if found and removed and axl_false if not removed
 **/
axl_bool     valvula_hash_remove   (ValvulaHash *hash_table,
				   axlPointer key)
{
	axl_bool   was_removed;

	/* check hash table reference */
	if (hash_table == NULL)
		return axl_false;

	valvula_mutex_lock   (&hash_table->mutex);

 	was_removed = axl_hash_remove (hash_table->table, key);
 	
	valvula_mutex_unlock (&hash_table->mutex);

 	if (was_removed) {
 		/* notify change */
 		__valvula_hash_notify_change (hash_table);
 	}
	return axl_true;
}

/** 
 * @brief Destroy the given hash freeing all resources.
 * 
 * Destroy the hash table.
 *
 * @param hash_table the hash table to operate on.
 **/
void         valvula_hash_destroy  (ValvulaHash *hash_table)
{

	/* check hash table reference */
	if (hash_table == NULL)
		return;

	/* lock the mutex */
	valvula_mutex_lock (&hash_table->mutex);

 	/* reduce reference counting */
 	hash_table->ref_count--;
 	if (hash_table->ref_count != 0) {
 		/* unlock the mutex and returns: more callers have a
 		 * referece to this hash */
 		valvula_mutex_unlock (&hash_table->mutex);
 		return;
 	} /* end if */
	
	/* get a reference to the table and nullify it */
	axl_hash_free (hash_table->table);
	hash_table->table = NULL;

 	/* unref waiting queue */
 	if (hash_table->changed_queue)
 		valvula_async_queue_unref (hash_table->changed_queue);

	/* unlock and free */
	valvula_mutex_unlock (&hash_table->mutex);
	valvula_mutex_destroy (&hash_table->mutex);

	/* release the node holding all information */
	axl_free       (hash_table);

	return;
}

/**  
 * @brief Allows to remove the provided key and its associated data on
 * the provided hash without calling to the optionally associated
 * destroy functions.
 * 
 * @param hash_table The hash where the unlink operation will take
 * place.
 *
 * @param key The key for the data to be removed.
 * 
 * @return axl_true if the item was removed (current implementation always
 * return axl_true).
 */
axl_bool          valvula_hash_delete   (ValvulaHash   *hash_table,
					axlPointer    key)
{
	axl_bool   was_removed;

	v_return_val_if_fail (hash_table, axl_false);

	valvula_mutex_lock    (&hash_table->mutex);

	was_removed = axl_hash_delete (hash_table->table, key);
	
	valvula_mutex_unlock  (&hash_table->mutex);

	if (was_removed) {
		/* notify change */
		__valvula_hash_notify_change (hash_table);
	}

	return axl_true;
}

/** 
 * @brief Perform a foreach over all elements inside the ValvulaHash.
 * 
 * @param hash_table The hash table where the foreach operation will
 * be implemented.
 *
 * @param func User defined handler called for each item found, along
 * with the user defined data provided.
 *
 * @param user_data User defined data to be provided to the handler.
 **/
void         valvula_hash_foreach  (ValvulaHash         *hash_table,
				   axlHashForeachFunc  func,
				   axlPointer         user_data)
{
	/* check references */
	v_return_if_fail (hash_table);
	v_return_if_fail (func);
	v_return_if_fail (hash_table->table);

	valvula_mutex_lock   (&hash_table->mutex);
	axl_hash_foreach    (hash_table->table, func, user_data);
	valvula_mutex_unlock (&hash_table->mutex);

	return;
}

/** 
 * @brief Perform a foreach over all elements inside the ValvulaHash,
 * allowing to provide two user defined reference at the handler.
 * 
 * @param hash_table The hash table where the foreach operation will
 * be implemented.
 *
 * @param func User defined handler called for each item found, along
 * with the user defined data provided (two references).
 *
 * @param user_data User defined data to be provided to the handler.
 *
 * @param user_data2 Second user defined data to be provided to the
 * handler.
 **/
void         valvula_hash_foreach2  (ValvulaHash           *hash_table,
				    axlHashForeachFunc2   func,
				    axlPointer            user_data,
				    axlPointer            user_data2)
{
	if (hash_table == NULL || func == NULL || hash_table->table == NULL)
		return;

	valvula_mutex_lock   (&hash_table->mutex);
	axl_hash_foreach2   (hash_table->table, func, user_data, user_data2);
	valvula_mutex_unlock (&hash_table->mutex);

	return;
}

/** 
 * @brief Perform a foreach over all elements inside the ValvulaHash,
 * allowing to provide three user defined reference at the handler.
 * 
 * @param hash_table The hash table where the foreach operation will
 * be implemented.
 *
 * @param func User defined handler called for each item found, along
 * with the user defined data provided (two references).
 *
 * @param user_data User defined data to be provided to the handler.
 *
 * @param user_data2 Second user defined data to be provided to the
 * handler.
 *
 * @param user_data3 Third user defined data to be provided to the
 * handler.
 **/
void         valvula_hash_foreach3  (ValvulaHash         * hash_table,
				    axlHashForeachFunc3  func,
				    axlPointer           user_data,
				    axlPointer           user_data2,
				    axlPointer           user_data3)
{
	if (hash_table == NULL || func == NULL || hash_table->table == NULL)
		return;

	valvula_mutex_lock   (&hash_table->mutex);
	axl_hash_foreach3   (hash_table->table, func, user_data, user_data2, user_data3);
	valvula_mutex_unlock (&hash_table->mutex);
}

/** 
 * @internal
 * @brief Support function for \ref valvula_hash_clear function.
 * 
 * It just returns axl_true.
 */
axl_bool      valvula_hash_clear_allways_true (axlPointer key, axlPointer value, axlPointer user_data) 
{
	return axl_true;
}


/** 
 * @brief Allows to clear a hash table.
 * 
 * @param hash_table The hash table to clear
 */
void         valvula_hash_clear    (ValvulaHash *hash_table)
{
 	int items_found;

	if (hash_table == NULL)
		return;

	valvula_mutex_lock (&hash_table->mutex);

 	/* record number of items deleted to notify change in the case
 	 * something was stored */
 	items_found = axl_hash_items (hash_table->table);

	axl_hash_free (hash_table->table);
	hash_table->table = axl_hash_new (hash_table->hash_func, 
					  hash_table->key_equal_func);

	valvula_mutex_unlock (&hash_table->mutex);

 	/* notify change */
 	if (items_found > 0)
 		__valvula_hash_notify_change (hash_table);

	return;
}

/* @} */
