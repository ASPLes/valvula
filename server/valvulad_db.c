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
#include <valvulad_db.h>
#include <stdarg.h>

/* mysql flags */
#include <mysql.h>

MYSQL   * valvulad_db_get_connection  (ValvuladCtx * ctx)
{
	axlNode * node;
	MYSQL   * dbconn;
	int       port = 3306;
	int       reconnect = 1;

	/* get configuration node and check everything is working */
	node = axl_doc_get (ctx->config, "/valvula/database/config");
	if (node == NULL) {
		error ("Unable to setup database connection, configuration <database/config> wasn't found at %s",
		       ctx->config_path);
		return NULL;
	} /* end if */

	/* create a mysql connection */
	dbconn = mysql_init (NULL);

	/* get port */
	if (HAS_ATTR (node, "port") && strlen (ATTR_VALUE (node, "port")) > 0) {
		/* get port configured by the user */
		port = atoi (ATTR_VALUE (node, "port"));
	}
	
	/* create a connection */
	if (mysql_real_connect (dbconn, 
				/* get host */
				ATTR_VALUE (node, "host"), 
				/* get user */
				ATTR_VALUE (node, "user"), 
				/* get password */
				ATTR_VALUE (node, "password"), 
				/* get database */
				ATTR_VALUE (node, "dbname"), 
				port, NULL, 0) == NULL) {
		error ("Mysql connect error: %s, failed to run SQL command", mysql_error (dbconn));
		return NULL;
	} /* end if */

	/* flag here to reconnect in case of lost connection */
	mysql_options (dbconn, MYSQL_OPT_RECONNECT, (const char *) &reconnect);

	/* return connection created */
	return dbconn;
}

/** 
 * @brief Allows to release a MySQL connection acquired through valvulad_db_release_connection.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param conn The connection  to be released.
 */
void valvulad_db_release_connection (ValvuladCtx * ctx, MYSQL * conn)
{
	/* close connection */
	mysql_close (conn);

	return;
}

/** 
 * @brief Allows to initialize database module.
 *
 * @param ctx Where the initialization will take place.
 */
axl_bool        valvulad_db_init (ValvuladCtx * ctx)
{
	MYSQL   * conn;

	/* get configuration node and check everything is working */
	conn = valvulad_db_get_connection (ctx);
	if (conn == NULL) {
		error ("Failed to initialize db module. Database connection is failing. Check settings and/or MySQL server status");
		return axl_false;
	} /* end if */

	/* release connection */
	valvulad_db_release_connection (ctx, conn);

	return axl_true;
}

/** 
 * @brief Finalize database module.
 *
 * @param ctx The context where the operation will take place.
 */
void            valvulad_db_cleanup (ValvuladCtx * ctx)
{
	mysql_library_end ();

	return;
}

/** 
 * @brief Allows to check if the database connection is working.
 *
 * @param ctx The context where the operation will take place.
 * 
 * @return axl_true in the case it is working, otherwise axl_false is
 * returned.
 */
axl_bool valvulad_db_check_conn (ValvuladCtx * ctx)
{
	MYSQL   * conn;

	/* get configuration node and check everything is working */
	conn = valvulad_db_get_connection (ctx);
	if (conn == NULL) {
		error ("Database connection is failing. Check settings and/or MySQL server status");
		return axl_false;
	} /* end if */

	/* release connection */
	valvulad_db_release_connection (ctx, conn);
	
	return axl_true;
}

/** 
 * @brief Allows to run the provided query reporting the result.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param query The query to be run.
 *
 * @param ... Additional parameter for the query to be created.
 *
 * @return A reference to MYSQL_RES or NULL if it fails. The function
 * returns axl_true in the case of a NON query (UPDATE, DELETE, INSERT
 * ...) and there is no error.
 */
MYSQL_RES * valvulad_db_run_query (ValvuladCtx * ctx, const char * query, ...)
{
	MYSQL_RES * result;
	MYSQL     * dbconn;
	char      * complete_query;
	va_list     args;
	axl_bool    non_query;
	
	/* check context or query */
	if (ctx == NULL || query == NULL)
		return NULL;

	/* open std args */
	va_start (args, query);

	/* create complete query */
	complete_query = axl_stream_strdup_printfv (query, args);

	/* close std args */
	va_end (args);

	/* clear query */
	axl_stream_trim (complete_query);
	
	/* get if we have a non query request */
	non_query = ! axl_memcmp ("SELECT", complete_query, 6);

	/* get connection */
	dbconn = valvulad_db_get_connection (ctx);

	/* now run query */
	if (mysql_query (dbconn, complete_query)) {
		axl_free (complete_query);
		error ("Failed to run SQL query, error was %u: %s\n", mysql_errno (dbconn), mysql_error (dbconn));

		/* release the connection */
		valvulad_db_release_connection (ctx, dbconn); 
		return NULL;
	} /* end if */

	/* release the query string */
	axl_free (complete_query);
	
	if (non_query) {
		/* release the connection */
		valvulad_db_release_connection (ctx, dbconn); 

		/* report ok */
		return INT_TO_PTR (axl_true);
	} /* end if */

	/* return result */
	result = mysql_store_result (dbconn);

	/* release the connection */
	valvulad_db_release_connection (ctx, dbconn); 

	return result;
}


/** 
 * @brief Allows to check if the table exists using context configuration.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param table_name The database table that will be checked.
 */
axl_bool valvulad_db_table_exists (ValvuladCtx * ctx, const char * table_name)
{
	MYSQL_RES * result;

	if (! valvulad_db_check_conn (ctx)) {
		error ("Unable to check if table exists, database connection is not working");
		return axl_false;
	}

	/* run query */
	result = valvulad_db_run_query (ctx, "SELECT * FROM %s LIMIT 1", table_name);
	if (result == NULL) {
		/* don't report error, table doesn't exists */
		return axl_false;
	} /* end if */

	/* release the result */
	mysql_free_result (result);
	return axl_true;
}

/** 
 * @brief Remove the provided table name on the valvula server
 * database.
 *
 * @param ctx The context where the operation takes place.
 *
 * @param table_name The table name that is going to be removed.
 *
 * @return axl_true in the case the table is removed, otherwise
 * axl_false is reported.
 *
 */
axl_bool        valvulad_db_table_remove (ValvuladCtx * ctx, 
					  const char * table_name)
{
	MYSQL_RES * result;

	if (! valvulad_db_check_conn (ctx)) {
		error ("Unable to check if table exists, database connection is not working");
		return axl_false;
	}

	/* run query */
	result = valvulad_db_run_query (ctx, "DROP TABLE %s", table_name);
	if (result == NULL) {
		/* don't report error, table doesn't exists */
		return axl_false;
	} /* end if */

	/* release the result */
	return axl_true;
}

/** 
 * @brief Allows to check if the table has the provided attribute
 *
 * @param ctx The context where the operation will take place.
 *
 * @param table_name The database table that will be checked.
 */
axl_bool valvulad_db_attr_exists (ValvuladCtx * ctx, const char * table_name, const char * attr_name)
{
	MYSQL_RES * result;

	if (! valvulad_db_check_conn (ctx)) {
		error ("Unable to check if table exists, database connection is not working");
		return axl_false;
	}

	/* run query */
	result = valvulad_db_run_query (ctx, "SELECT %s FROM %s LIMIT 1", attr_name, table_name);
	if (result == NULL) {
		/* don't report error, table doesn't exists */
		return axl_false;
	} /* end if */

	/* release the result */
	mysql_free_result (result);
	return axl_true;
}

/** 
 * @brief Allows to checks and creates the table provided in the case
 * it doesn't exists. 
 *
 * @param ctx The context where the operation will take place.
 *
 * @param table_name The table name to be checked to exist
 * 
 * @param  attr_name The attribute name to be created.
 *
 * @param attr_type The attribute type to associate. If you want to
 * support typical autoincrement index. use as type: "autoincrement int".
 *
 * The function allows to check and create an SQL table with the name
 * provided and the set of attributes provided. The list must be ended
 * by NULL.
 *
 * @return The function returns axl_true in the case everything
 * finished without any issue otherwise axl_false is returned.
 *
 * 
 */ 
axl_bool        valvulad_db_ensure_table (ValvuladCtx * ctx, 
					  const char * table_name,
					  const char * attr_name, const char * attr_type, 
					  ...)
{
	va_list   args;

	/* check if the mysql table exists */
	if (! valvulad_db_table_exists (ctx, table_name)) {
		/* support for auto increments */
		if (axl_cmp (attr_type, "autoincrement int"))
			attr_type = "INT AUTO_INCREMENT PRIMARY KEY";

		/* create the table with the first column */
		if (! valvulad_db_run_query (ctx, "CREATE TABLE %s (%s %s)", table_name, attr_name, attr_type)) {
			error ("Unable to create table %s, failed to ensure table exists", table_name);
			return axl_false;
		} /* end if */
	} /* end if */

	if (! valvulad_db_attr_exists (ctx, table_name, attr_name)) {
		/* support for auto increments */
		if (axl_cmp (attr_type, "autoincrement int"))
			attr_type = "INT AUTO_INCREMENT PRIMARY KEY";

		/* create the table with the first column */
		if (! valvulad_db_run_query (ctx, "ALTER TABLE %s ADD COLUMN %s %s", table_name, attr_name, attr_type)) {
			error ("Unable to update table %s to add attribute %s : %s, failed to ensure table exists",
			       table_name, attr_name, attr_type);
			return axl_false;
		} /* end if */
	} /* end if */

	va_start (args, attr_type);

	while (axl_true) {
		/* get attr name to create and stop if NULL is defined */
		attr_name  = va_arg (args, const char *);
		if (attr_name == NULL)
			break;
		/* get attr type */
		attr_type  = va_arg (args, const char *);

		/* though not supported, check for NULL values here */
		if (attr_type == NULL)
			break;

		/* support for auto increments */
		if (axl_cmp (attr_type, "autoincrement int"))
			attr_type = "INT AUTO_INCREMENT PRIMARY KEY";

		if (! valvulad_db_attr_exists (ctx, table_name, attr_name)) {
			/* create the table with the first column */
			if (! valvulad_db_run_query (ctx, "ALTER TABLE %s ADD COLUMN %s %s", table_name, attr_name, attr_type)) {
				error ("Unable to update table %s to add attribute %s : %s, failed to ensure table exists",
				       table_name, attr_name, attr_type);
				return axl_false;
			} /* end if */
		} /* end if */
	} /* end while */

	return axl_true;
}

/** 
 * @brief Allows to run the the provided query and reporting boolean
 * state according to the result.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param query The query string to execute.
 *
 * @param ... additional parameters to execute.
 *
 * @return the function returns axl_true in the case the query reports
 * content otherwise, axl_false is returned.
 */
axl_bool        valvulad_db_boolean_query (ValvuladCtx * ctx, 
					   const char * query, 
					   ...)
{
	MYSQL_RES * result;
	MYSQL_ROW   row;
	char      * complete_query;
	va_list     args;

	/* open std args */
	va_start (args, query);

	/* create complete query */
	complete_query = axl_stream_strdup_printfv (query, args);

	/* close std args */
	va_end (args);

	/* clear query */
	axl_stream_trim (complete_query);

	/* run query */
	result = valvulad_db_run_query (ctx, complete_query);
	axl_free (complete_query);

	if (result == NULL)
		return axl_false;

	row = mysql_fetch_row (result);

	/* release the result */
	mysql_free_result (result);

	return row != NULL;
}

/** 
 * @brief Allows to run a non query with the provided data.
 *
 * @param ctx The context where the operation takes place.
 *
 * @param query The query to run
 *
 * @param ... Additional parameters to complete the query.
 *
 * @return axl_true in the case the query reported ok, otherwise
 * axl_false is returned.
 */
axl_bool        valvulad_db_run_non_query (ValvuladCtx * ctx, 
					   const char * query, 
					   ...)
{
	MYSQL_RES * result;
	char      * complete_query;
	va_list     args;

	/* open std args */
	va_start (args, query);

	/* create complete query */
	complete_query = axl_stream_strdup_printfv (query, args);

	/* close std args */
	va_end (args);

	/* clear query */
	axl_stream_trim (complete_query);

	/* run query */
	result = valvulad_db_run_query (ctx, complete_query);
	axl_free (complete_query);
	if (result == NULL)
		return axl_false;

	/* release the result */
	if (PTR_TO_INT (result) != axl_true)
		mysql_free_result (result);

	return axl_true;
}
