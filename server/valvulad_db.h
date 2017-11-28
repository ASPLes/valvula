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
#ifndef __VALVULAD_DB_H__
#define __VALVULAD_DB_H__

#include <valvulad.h>

/** 
 * @brief Type that represents a single row inside a query result \ref ValvuladRes.
 */
typedef axlPointer ValvuladRow;

/** 
 * @brief Type that represents a complete query as reported by \ref valvulad_db_run_query.
 */
typedef axlPointer ValvuladRes;

axl_bool        valvulad_db_init (ValvuladCtx * ctx);

void            valvulad_db_cleanup (ValvuladCtx * ctx);

void            valvulad_db_cleanup_thread (ValvuladCtx * ctx);

axl_bool        valvulad_db_check_conn (ValvuladCtx * ctx);

axl_bool        valvulad_db_attr_exists (ValvuladCtx * ctx, 
					 const char * table_name, 
					 const char * attr_name);

axl_bool        valvulad_db_table_exists (ValvuladCtx * ctx, 
					  const char * table_name);

axl_bool        valvulad_db_table_remove (ValvuladCtx * ctx, 
					  const char * table_name);

axl_bool        valvulad_db_ensure_table (ValvuladCtx * ctx, 
					  const char * table_name,
					  const char * attr_name, const char * attr_type, 
					  ...);

ValvuladRes     valvulad_db_run_query     (ValvuladCtx * ctx, 
					   const char * query, 
					   ...);

ValvuladRes     valvulad_db_run_query_s   (ValvuladCtx * ctx, 
					   const char  * query);

/** SQlite interface **/
ValvuladRes     valvulad_db_sqlite_run_query (ValvuladCtx * ctx,
					      const char  * sqlite_path,
					      const char  * query,
					      ...);

axl_bool        valvulad_db_sqlite_run_sql  (ValvuladCtx * ctx,
					      const char  * sqlite_path,
					      const char  * query,
					      ...);

ValvuladRow     valvulad_db_sqlite_get_row   (ValvuladCtx * ctx, ValvuladRes result);

const char  *   valvulad_db_sqlite_get_cell  (ValvuladCtx * ctx, ValvuladRow row, int position);

void            valvulad_db_sqlite_release_result (ValvuladRes result);

/** 
 * @brief Get the next row from a result object.
 *
 * @param result The value reported by valvulad_db_run_query.
 *
 * @return The row reference or NULL if it fails.
 */
#define GET_ROW(result) valvulad_db_get_row(ctx, result)

ValvuladRow      valvulad_db_get_row       (ValvuladCtx * ctx, ValvuladRes result);

void             valvulad_db_first_row     (ValvuladCtx * ctx, ValvuladRes result);

/** 
 * @brief Get the cell inside a particular a particular row at the given position.
 *
 * @param row The row where the content is being retreived.
 *
 * @param pos The position that is being requested.
 *
 * @return String value or NULL if it fails.
 */
#define GET_CELL(row,pos) valvulad_db_get_cell(ctx,row,pos)

const char  *   valvulad_db_get_cell         (ValvuladCtx * ctx, ValvuladRow row, int position);

/** 
 * @brief Get the cell inside a particular a particular row at the given position.
 *
 * @param row The row where the content is being retreived.
 *
 * @param pos The position that is being requested.
 *
 * @return Long value found or -1 if it fails.
 */
#define GET_CELL_AS_LONG(row,pos) valvulad_db_get_cell_as_long(ctx,row,pos)

long            valvulad_db_get_cell_as_long (ValvuladCtx * ctx, ValvuladRow row, int position);

axl_bool        valvulad_db_boolean_query (ValvuladCtx * ctx, 
					   const char * query, 
					   ...);

axl_bool        valvulad_db_run_non_query (ValvuladCtx * ctx, 
					   const char * query, 
					   ...);

long            valvulad_db_run_query_as_long (ValvuladCtx * ctx, 
					       const char * query, 
					       ...);

void            valvulad_db_release_result        (ValvuladRes result);

axl_bool        valvulad_db_check_unallowed_chars (ValvuladCtx * ctx, const char * string_to_check);

#endif 
