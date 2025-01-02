/* 
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2025 Advanced Software Production Line, S.L.
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
#ifndef __VALVULA_HASH_H__
#define __VALVULA_HASH_H__

#include <valvula.h>


ValvulaHash * valvula_hash_new_full (axlHashFunc    hash_func,
				     axlEqualFunc   key_equal_func,
				     axlDestroyFunc key_destroy_func,
				     axlDestroyFunc value_destroy_func);

ValvulaHash * valvula_hash_new      (axlHashFunc    hash_func,
				     axlEqualFunc   key_equal_func);

void         valvula_hash_ref      (ValvulaHash   * hash_table);

void         valvula_hash_unref    (ValvulaHash   * hash_table);

void         valvula_hash_insert   (ValvulaHash *hash_table,
				    axlPointer  key,
				    axlPointer  value);

void         valvula_hash_replace  (ValvulaHash *hash_table,
				    axlPointer  key,
				    axlPointer  value);

void         valvula_hash_replace_full  (ValvulaHash     * hash_table,
					 axlPointer       key,
					 axlDestroyFunc   key_destroy,
					 axlPointer       value,
					 axlDestroyFunc   value_destroy);

int          valvula_hash_size     (ValvulaHash   *hash_table);

axlPointer   valvula_hash_lookup   (ValvulaHash   *hash_table,
				    axlPointer    key);

axl_bool     valvula_hash_exists   (ValvulaHash   *hash_table,
				    axlPointer    key);

axlPointer   valvula_hash_lookup_and_clear   (ValvulaHash   *hash_table,
					      axlPointer    key);

int          valvula_hash_lock_until_changed (ValvulaHash   *hash_table,
					      long          wait_microseconds);

axl_bool     valvula_hash_remove   (ValvulaHash   *hash_table,
				    axlPointer    key);

void         valvula_hash_destroy  (ValvulaHash *hash_table);

axl_bool     valvula_hash_delete   (ValvulaHash   *hash_table,
				    axlPointer    key);

void         valvula_hash_foreach  (ValvulaHash         * hash_table,
				    axlHashForeachFunc   func,
				    axlPointer           user_data);

void         valvula_hash_foreach2  (ValvulaHash         * hash_table,
				     axlHashForeachFunc2  func,
				     axlPointer           user_data,
				     axlPointer           user_data2);

void         valvula_hash_foreach3  (ValvulaHash         * hash_table,
				     axlHashForeachFunc3  func,
				     axlPointer           user_data,
				     axlPointer           user_data2,
				     axlPointer           user_data3);

void         valvula_hash_clear    (ValvulaHash *hash_table);

axlHashCursor * valvula_hash_get_cursor (ValvulaHash * hash_table);

#endif
