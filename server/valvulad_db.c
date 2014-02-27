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

MYSQL   * valvulad_get_connection  (ValvuladCtx * ctx)
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
	conn = valvulad_get_connection (ctx);
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
 * @brief Allows to checks and creates the table provided in the case
 * it doesn't exists. 
 *
 * @param ctx The context where the operation will take place.
 *
 * @param table_name The table name to be checked to exist
 * 
 * @param  attr_name The attribute name to be created.
 *
 * @param attr_type The attribute type to associate.
 *
 * The function allows to check and create an SQL table with the name
 * provided and the set of attributes provided. The list must be ended
 * by NULL.
 *
 * @return The function returns axl_true in the case everything
 * finished without any issue otherwise axl_false is returned.
 */ 
axl_bool        valvulad_db_ensure_table (ValvuladCtx * ctx, 
					  const char * table_name,
					  const char * attr_name, const char * attr_type, 
					  ...)
{
	va_list   args;
	va_start (args, attr_type);
	
	

	return axl_true;
}


