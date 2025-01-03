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
#include <valvulad_run.h>
#include <mysql.h>

#include <dirent.h>

/** 
 * @internal Structure used to hold data about a function resolver and
 * a user defined pointer used by the resolver.
 */
struct _ValvuladObjectResolverData {
	ValvuladObjectResolver resolver;
	axlPointer data;
};
typedef struct _ValvuladObjectResolverData ValvuladObjectResolverData;

/** 
 * \defgroup valvulad_run Valvulad Run: run-time functions provided by valvulad server.
 */

/** 
 * \addtogroup valvulad_run
 * @{
 */


void valvulad_run_load_modules_from_path (ValvuladCtx * ctx, const char * path, DIR * dirHandle)
{
	struct dirent    * entry;
	char             * fullpath = NULL;
	const char       * location;
	axlDoc           * doc;
	axlError         * error;
				      
	/* get the first entry */
	entry = readdir (dirHandle);
	while (entry != NULL) {
		/* nullify full path and doc */
		fullpath = NULL;
		doc      = NULL;
		error    = NULL;

		/* check for non valid directories */
		if (axl_cmp (entry->d_name, ".") ||
		    axl_cmp (entry->d_name, "..")) {
			goto next;
		} /* end if */
		
		fullpath = valvula_support_build_filename (path, entry->d_name, NULL);
		if (! valvula_support_file_test (fullpath, FILE_IS_REGULAR))
			goto next;

		/* check if the fullpath is ended with .xml */
		if (strlen (fullpath) < 5 || ! axl_cmp (fullpath + (strlen (fullpath) - 4), ".xml")) {
			wrn ("skiping file %s which do not end with .xml", fullpath);
			goto next;
		} /* end if */

		/* notify file found */
		msg ("possible module pointer found: %s", fullpath);		
		
		/* check its xml format */
		doc = axl_doc_parse_from_file (fullpath, &error);
		if (doc == NULL) {
			wrn ("file %s does not appear to be a valid xml file (%s), skipping..", fullpath, axl_error_get (error));
			goto next;
		}
		
		msg ("loading mod valvulad pointer: %s", fullpath);

		/* check module basename to be not loaded */
		location = ATTR_VALUE (axl_doc_get_root (doc), "location");

		/* load the module man!!! */
		valvulad_module_open_and_register (ctx, location);

	next:
		/* free the document */
		axl_doc_free (doc);
		
		/* free the error */
		axl_error_free (error);

		/* free full path */
		axl_free (fullpath);

		/* get the next entry */
		entry = readdir (dirHandle);
	} /* end while */

	return;
}


void valvulad_run_load_modules (ValvuladCtx * ctx, axlDoc * doc)
{
	axlNode     * directory;
	const char  * path;
	DIR         * dirHandle;

	directory = axl_doc_get (doc, "/valvula/modules/directory");
	if (directory == NULL) {
		msg ("no module directories were configured, nothing loaded");
		return;
	} /* end if */

	/* check every module */
	while (directory != NULL) {
		/* get the directory */
		path = ATTR_VALUE (directory, "src");
		
		/* try to open the directory */
		if (valvula_support_file_test (path, FILE_IS_DIR | FILE_EXISTS)) {
			dirHandle = opendir (path);
			if (dirHandle == NULL) {
				wrn ("unable to open mod directory '%s' (%s), skiping to the next", strerror (errno), path);
				goto next;
			} /* end if */
		} else {
			wrn ("skiping mod directory: %s (not a directory or do not exists)", path);
			goto next;
		}

		/* directory found, now search for modules activated */
		msg ("found mod directory: %s", path);
		valvulad_run_load_modules_from_path (ctx, path, dirHandle);
		
		/* close the directory handle */
		closedir (dirHandle);
	next:
		/* get the next directory */
		directory = axl_node_get_next_called (directory, "directory");
		
	} /* end while */

	return;
}

void valvulad_run_register_handlers (ValvuladCtx * ctx)
{
	axlNode        * node;
	axlNode        * node2;
	ValvuladModule * module;
	int              port;
	int              prio;

	/* get first node */
	node = axl_doc_get (ctx->config, "/valvula/general/listen");
	while (node) {
		msg ("  - processing node: %p", node);
		/* get default port to be used on this listener */
		prio     = 1;
		port     = -1;
		if (HAS_ATTR (node, "port")) 
			port = atoi (ATTR_VALUE (node, "port"));

		/* get first module */
		node2 = axl_node_get_child_called (node, "run");
		while (node2) {
			/* module */
			module = valvulad_module_find_by_name (ctx, ATTR_VALUE (node2, "module"));
			if (module && module->def->process_request) {
				/* call to register the function on the provided port */
				msg ("Registering module: %s, on port %d (prio: %d)", ATTR_VALUE (node2, "module"), port, prio);
				valvula_ctx_register_request_handler (ctx->ctx, ATTR_VALUE (node2, "module"), module->def->process_request, prio, port, NULL);
			} /* end if */

			/* next node */
			prio ++;
			node2 = axl_node_get_next_called (node2, "run");
		} /* end while */

		/* next node */
		node = axl_node_get_next_called (node, "listen");
	} /* end while */

	return;
}

void __valvulad_run_ensure_day_month_in_place (ValvuladCtx * ctx)
{
	/* run query to check if time tracking values are in place */
	if (! valvulad_db_boolean_query (ctx, "SELECT * FROM time_tracking")) {
		/* store time tracking */
		if (! valvulad_db_run_non_query (ctx, "INSERT INTO time_tracking (day, month) VALUES ('%d', '%d')", valvula_get_day (), valvula_get_month ())) {
			error ("Unable to install current day and month into the database");
		} /* end if */
	} /* end if */

	return;
}

/** 
 * @internal Install internal valvula server databases.
 */
void __valvulad_run_install_internal_dbs (ValvuladCtx * ctx)
{

	/* create databases to be used by the module */
	msg ("Calling valvulad_db_ensure_table() ..");
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "time_tracking", 
				  /* how many tickets does the domain have */
				  "day", "int",
				  /* day limit that can be consumed */
				  "month", "int",
				  NULL);

	/* check if there is something stored and if not, store content */
	__valvulad_run_ensure_day_month_in_place (ctx);

	return;
}

/** 
 * @internal Handler that tracks and triggers that day or month have
 * changed.
 *
 */
axl_bool __valvulad_run_time_tracking        (ValvulaCtx  * _ctx, 
					      axlPointer    user_data,
					      axlPointer    user_data2)
{
	ValvuladCtx        * ctx = user_data;
	ValvuladRes          res;
	ValvuladRow          row;
	long                 stored_day;
	long                 stored_month;

	/* check if there is something stored and if not, store content */
	__valvulad_run_ensure_day_month_in_place (ctx);
	
	/* get current day and month values */
	res = valvulad_db_run_query (ctx, "SELECT day, month FROM time_tracking");
	if (res == NULL) {
		error ("Failed to get time tracking content");
		return axl_false;
	} /* end if */
	
	/* get stored values */
	row          = GET_ROW (res);
	stored_day   = GET_CELL_AS_LONG (row, 0);
	stored_month = GET_CELL_AS_LONG (row, 1);

	if (stored_month != valvula_get_month ()) {
		/* ok, day changed, save it */
		valvulad_db_run_non_query (ctx, "UPDATE time_tracking SET month = '%d'", valvula_get_month ());

		/* notify */
		msg ("Valvula month change detected");
		valvulad_notify_date_change (ctx, valvula_get_month (), VALVULAD_DATE_ITEM_MONTH);

	} /* end if */

	if (stored_day != valvula_get_day ()) {
		/* ok, day changed, save it */
		valvulad_db_run_non_query (ctx, "UPDATE time_tracking SET day = '%d'", valvula_get_day ());

		/* notify */
		msg ("Valvula engine day change detected, notifying..");
		valvulad_notify_date_change (ctx, valvula_get_day (), VALVULAD_DATE_ITEM_DAY);
		
	} /* end if */

	/* release result */
	valvulad_db_release_result (res);

	return axl_false; /* do not remove the event, please fire it
			   * again in the future */
}

char * __valvulad_read_line (FILE * _file)
{
	char buffer[2048];
	int  iterator = 0;
	char value;

	memset (buffer, 0, 2048);
	
	value = fgetc (_file);
	if (value == EOF) 
		return NULL;

	while (value != EOF && value != '\n' && value != '\0' && iterator < (2048 - 1)) {
		/* store value */
		buffer[iterator] = value;

		/* next position */
		iterator++;

		/* get next value */
		value = fgetc (_file);
	} /* end while */

	/* protect operation done */
	buffer[iterator] = 0;

	if (iterator == 0) {
		if (value == EOF)
			return NULL;
		return axl_strdup ("");
	}

	return axl_strdup (buffer);
}

/* get key and decl */
void __valvulad_run_get_key_decl (char * line, char ** key, char ** decl)
{
	int iterator = 0;

	line = axl_strdup (line);

	/* set default values */
	(*key)  = NULL;
	(*decl) = NULL;

	while (line[iterator] && line[iterator] != '=')
		iterator++;
	
	/* value not found inside string */
	if (line[iterator] != '=') {
		axl_free (line);
		return;
	} /* end if */

	line[iterator] = 0;
	axl_stream_trim (line);
	axl_stream_trim (line + iterator + 1);

	(*key)  = axl_strdup (line);
	(*decl) = axl_strdup (line + iterator + 1);

	/* release */
	axl_free (line);
	
	return;
}

char * valvulad_run_release_and_copy (char * ref, char * decl, char * key, const char * key_to_check)
{
	/* check if we have to copy content in case key and reference
	 * key to check matches */
	if (! axl_cmp (key, key_to_check))
		return ref;

	/* if value hold previously matches, release because it is
	 * going to be replaced... */
	if (ref)
		axl_free (ref);

	/* copy content to update caller reference */
	return axl_strdup (decl);
}
				      
axl_bool valvulad_run_check_local_domains_config_detect_postfix_decl (ValvuladCtx * ctx, const char * postfix_decl, const char * section)
{

	char      ** items;
	char      ** items2;
	char       * key;
	char       * decl;
	const char * path;
	FILE       * _file;
	char       * line;
	axl_bool     result = axl_true;
	
	char       * select_field          = NULL;
	char       * table                 = NULL;
	char       * where_field           = NULL;
	char       * additional_conditions = NULL;
	char       * query;

	/** 
	 * mysql_config = 1 : domain  : ld_query
	 * mysql_config = 2 : alias   : ls_query
	 * mysql_config = 3 : account : la_query
	 */
	int          mysql_config = 0;
	
	
	/* split content */
	items = axl_split (postfix_decl, 1, "=");

	if (items == NULL) {
		error ("Empty declaration or imcomplete: %s", postfix_decl);
		return axl_false;
	} /* end if */

	if (items[1] == NULL) {
		error ("Declaration found but it has no content: %s", postfix_decl);
		axl_freev (items);
		return axl_false;
	}

	/* clean values */
	axl_stream_trim (items[1]);
	msg ("Working with postfix declaration: %s (from %s)", items[1], ctx->postfix_file);
	
	/* mysql support */
	path = strstr (items[1], "mysql:");
	if (path == NULL) {
		wrn ("Unable to find mysql: declaration inside postfix declaration: %s (skipping mysql detection to next)", postfix_decl);
		return axl_false;
	} /* end if */
	
	if (path)
		path = path + 6;

	/* ensure we have a clean path to open */
	items2 = axl_split (path, 1, ",");
	if (items2 == NULL || items2[0] == NULL) {
		error ("Split operation for %s failed and reported NULL, unable to continue", path);
		axl_freev (items);
		return axl_false;
	} /* end if */

	/* clean and assign path */
	axl_stream_trim (items2[0]);
	path   = items2[0];
	
	/* attempting to open the file */
	msg ("Found postfix mysql configuration, opening: %s", path);
	_file = fopen (path, "r");
	if (! _file) {
		error ("Unable to open file %s, errno=%d", path, errno);
		axl_freev (items);
		axl_freev (items2);
		return axl_false;
	} /* end if */

	/* get first line */
	line = __valvulad_read_line (_file);
	while (line) {
		/* check to find the right declaration */
		axl_stream_trim (line);
				
		if (line[0] != '#') {
			/* get key and decl */
			__valvulad_run_get_key_decl (line, &key, &decl);

			if (key && decl) {
				msg ("Declaration found: (%s) [%s] -> [%s]", 
				     section, key, axl_cmp (key, "password") ? "xxxxx" : decl);

				/* support for old interface */
				select_field           = valvulad_run_release_and_copy (select_field, decl, key, "select_field");
				table                  = valvulad_run_release_and_copy (table, decl, key, "table");
				where_field            = valvulad_run_release_and_copy (where_field, decl, key, "where_field");
				additional_conditions  = valvulad_run_release_and_copy (additional_conditions, decl, key, "additional_conditions");

				/* now check rest of fields */
				if (axl_cmp (section, "virtual_mailbox_domains")) {

					/* get values */
					ctx->ld_user   = valvulad_run_release_and_copy (ctx->ld_user, decl, key, "user");
					ctx->ld_pass   = valvulad_run_release_and_copy (ctx->ld_pass, decl, key, "password");
					ctx->ld_host   = valvulad_run_release_and_copy (ctx->ld_host, decl, key, "hosts");
					ctx->ld_dbname = valvulad_run_release_and_copy (ctx->ld_dbname, decl, key, "dbname");
					ctx->ld_query  = valvulad_run_release_and_copy (ctx->ld_query, decl, key, "query");
					
					mysql_config = 1; /* domain indication */
					
				} else if (axl_cmp (section, "virtual_alias_maps")) {
					
					/* get values */
					ctx->ls_user   = valvulad_run_release_and_copy (ctx->ls_user, decl, key, "user");
					ctx->ls_pass   = valvulad_run_release_and_copy (ctx->ls_pass, decl, key, "password");
					ctx->ls_host   = valvulad_run_release_and_copy (ctx->ls_host, decl, key, "hosts");
					ctx->ls_dbname = valvulad_run_release_and_copy (ctx->ls_dbname, decl, key, "dbname");
					ctx->ls_query  = valvulad_run_release_and_copy (ctx->ls_query, decl, key, "query");

					mysql_config = 2; /* alias indication */
					
				} else if (axl_cmp (section, "virtual_mailbox_maps") || axl_cmp (section, "local_recipient_maps")) {

					/* get values */
					ctx->la_user   = valvulad_run_release_and_copy (ctx->la_user, decl, key, "user");
					ctx->la_pass   = valvulad_run_release_and_copy (ctx->la_pass, decl, key, "password");
					ctx->la_host   = valvulad_run_release_and_copy (ctx->la_host, decl, key, "hosts");
					ctx->la_dbname = valvulad_run_release_and_copy (ctx->la_dbname, decl, key, "dbname");
					ctx->la_query  = valvulad_run_release_and_copy (ctx->la_query, decl, key, "query");
					
					mysql_config = 3; /* account indication */
				}
			} /* end if */

			/* key and decl */
			axl_free (key);
			axl_free (decl);
			
		} /* end if (line[0] != '#') */

		/* get next line */
		free (line);
		line = __valvulad_read_line (_file);
	} /* end while */

	/* close opened file */
	fclose (_file);

	if (select_field && table && where_field) {
		/* create query */
		query = axl_strdup_printf ("SELECT %s FROM %s WHERE %s = '%%s' %s", select_field, table, where_field, additional_conditions ? additional_conditions : "");
		msg ("Built virtual query because [old interface] -> [query] : %s", query);
		switch (mysql_config) {
		case 1:
			if (ctx->ld_query)
				axl_free (ctx->ld_query);
			ctx->ld_query = query;
			break;
		case 2:
			if (ctx->ls_query)
				axl_free (ctx->ls_query);
			ctx->ls_query = query;
			break;
		case 3:
			if (ctx->la_query)
				axl_free (ctx->la_query);
			ctx->la_query = query;
			break;
		} /* end if */
	}

	axl_free (select_field);
	axl_free (table);
	axl_free (where_field);
	axl_free (additional_conditions);
	
	/* release items */
	axl_freev (items);
	axl_freev (items2);
	return result;
}

/* update variables */
char * __valvulad_process_update_line_with_variables (ValvuladCtx * ctx, axlHash * variables, char * line)
{
	axlHashCursor * cursor;
	const char    * key;
	const char    * value;
	char          * temp;
	
	/* no variables, no processing */
	if (axl_hash_items (variables) == 0)
		return line; /* return same reference */

	if (strstr (line, "$") == NULL)
		return line; /* no variable */

	/* create cursor to iterate and apply all variables found so
	 * far */
	cursor = axl_hash_cursor_new (variables);
	while (axl_hash_cursor_has_item (cursor)) {
		/* get key (variable name) and value (variable content) */
		key   = axl_hash_cursor_get_key (cursor);
		value = axl_hash_cursor_get_value (cursor);

		/* only update in case it is needed */
		if (strstr (line, key)) {

			/* update variable */
			temp = axl_strdup_printf ("${%s}", key);
			axl_replace (line, temp, value);
			axl_free (temp);
			
			temp = axl_strdup_printf ("$%s", key);
			axl_replace (line, temp, value);
			axl_free (temp);
			msg ("Updated %s -> %s : %s", key, value, line);
			
		} /* end if */
		
		axl_hash_cursor_next (cursor);
	} /* end while */
	
	axl_hash_cursor_free (cursor);
	return line;
}

char * __valvulad_process_variable_line (ValvuladCtx * ctx, axlHash * variables, char * line)
{
	char * key = NULL;
	char * value = NULL;

	if (line[0] == '#')
		return line;

	if (strstr (line, "=") == NULL)
		return line;

	/* update variables */
	line = __valvulad_process_update_line_with_variables (ctx, variables, line);

	/* call to get value */
	__valvulad_run_get_key_decl (line, &key, &value);
	if (! key) {
		error ("Unable to parse variable line [%s]", line); 
		axl_free (key);
		axl_free (value);
		return line;
	} /* end if */

	/* store variable */
	axl_hash_insert_full (variables, key, axl_free, value, axl_free);
	
	return line;
}

axl_bool valvulad_run_check_local_domains_config_autodetect (ValvuladCtx * ctx)
{
	FILE       * _file;
	char       * line;
	axlHash    * variables;

	/* open postfix configuration */
	_file = fopen (ctx->postfix_file, "r");
	if (_file == NULL) {
		error ("Unable to open postfix file %s, fopen() system call failed, errno=%d", ctx->postfix_file, errno);
		return axl_false;
	} /* end if */

	/* get first line */
	variables = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	line      = __valvulad_read_line (_file);
	while (line) {
		/* check to find the right declaration */
		axl_stream_trim (line);

		if (strlen (line) == 0)
			goto next_line;

		if (line[0] == '#')
			goto next_line;

		line = __valvulad_process_variable_line (ctx, variables, line);

		/* update variables */
		line = __valvulad_process_update_line_with_variables (ctx, variables, line);
		
		if (line[0] != '#' && strstr (line, "virtual_mailbox_domains") != NULL) {
			/* found virtual mailbox declaration */
			valvulad_run_check_local_domains_config_detect_postfix_decl (ctx, line, "virtual_mailbox_domains");
		} else if (line[0] != '#' && strstr (line, "virtual_alias_maps") != NULL) {
			/* found virtual mailbox declaration */
			valvulad_run_check_local_domains_config_detect_postfix_decl (ctx, line, "virtual_alias_maps");
		} else if (line[0] != '#' && strstr (line, "virtual_mailbox_maps") != NULL) {
			/* found virtual mailbox declaration */
			valvulad_run_check_local_domains_config_detect_postfix_decl (ctx, line, "virtual_mailbox_maps");
		} else if (line[0] != '#' && strstr (line, "local_recipient_maps") != NULL) {
			/* found local mailbox declaration */
			valvulad_run_check_local_domains_config_detect_postfix_decl (ctx, line, "local_recipient_maps");
		} /* end if */

		/* get next line */
	next_line:
		free (line);
		line = __valvulad_read_line (_file);
	} /* end while */

	fclose (_file);
	axl_hash_free (variables);

	return axl_true;
}

void valvulad_run_load_static_names_record_from_file (ValvuladCtx * ctx, axlHash * hash, const char * file)
{
	char   buffer[514];
	FILE * _file = fopen (file, "r");
	int    bytes = 0;

	/* init buffer */
	memset (buffer, 0, 514);

	if (_file) {
		/* read content from buffer */
		bytes = fread (buffer, 512, 1, _file);
		if (bytes > 0) {
			msg ("Registering %s..", buffer);
			axl_hash_insert_full (hash, axl_strdup (buffer), axl_free, INT_TO_PTR (axl_true), NULL);
		} /* end if */

		/* only close if defined */
		fclose (_file);
	} /* end if */


	return;
}

void valvulad_run_load_static_names_record (ValvuladCtx * ctx, axlHash * hash, const char * line)
{
	char ** items;
	char ** values;
	int     iterator;

	items = axl_split (line, 1, "=");
	if (items == NULL || items[0] == NULL || items[1] == NULL) {
		axl_freev (items);
		return;
	} /* end if */

	values   = axl_split (items[1], 1, ",");
	iterator = 0;
	while (values[iterator]) {
		/* clean representation */
		axl_stream_trim (values[iterator]);

		/* register the name if it is defined and it is not a reference */
		if (strlen (values[iterator]) > 0 && strstr (values[iterator], "$") == NULL) {
			
			if (strstr (values[iterator], "/")) {
				/* found file reference, open it */
				valvulad_run_load_static_names_record_from_file (ctx, hash, values[iterator]);

			} else if (! axl_hash_get (hash, values[iterator])) {
				msg ("Registering %s..", values[iterator]);
				/* key not found, store it into the hash */
				axl_hash_insert_full (hash, axl_strdup (values[iterator]), axl_free, INT_TO_PTR (axl_true), NULL);
			} /* end if */
		} /* end if */

		/* next position */
		iterator++;
	} /* end if */

	/* release values */
	axl_freev (items);
	axl_freev (values);
		
	return;
}

axl_bool valvulad_run_load_static_names (ValvuladCtx * ctx)
{
	/* create empty hash */
	axlHash * hash    = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	axlHash * oldHash = NULL;
	FILE    * _file;
	char    * line;

	/* open postfix configuration */
	_file = fopen (ctx->postfix_file, "r");
	if (_file == NULL) {
		error ("Unable to open postfix file %s, fopen() system call failed, errno=%d", ctx->postfix_file, errno);
		return axl_false;
	} /* end if */

	/* get first line */
	line = __valvulad_read_line (_file);
	while (line) {
		/* check to find the right declaration */
		axl_stream_trim (line);
		
		if (line[0] != '#') { 
			if (axl_memcmp (line, "myhostname", 10)) {
				valvulad_run_load_static_names_record (ctx, hash, line);
			} else if (axl_memcmp (line, "myorigin", 8)) {
				valvulad_run_load_static_names_record (ctx, hash, line);
			} else if (axl_memcmp (line, "mydestination", 13)) {
				valvulad_run_load_static_names_record (ctx, hash, line);
			}

		} /* end if */

		/* get next line */
		free (line);
		line = __valvulad_read_line (_file);
	} /* end while */

	fclose (_file);

	/* copy old hash */
	oldHash = ctx->ld_hash;

	/* update reference */
	ctx->ld_hash = hash;

	/* release old hash */
	axl_hash_free (oldHash);

	return axl_true;
}

axl_bool valvulad_run_check_local_domains_config (ValvuladCtx * ctx) {
	axlNode    * node;
	const char * config;

	msg ("Loading local domains configuration..");

	node = axl_doc_get (ctx->config, "/valvula/enviroment/local-domains");
	if (node == NULL) 
		return axl_true;
	
	/* get configuration value */
	config = ATTR_VALUE (node, "config");
	if (config == NULL) {
		error ("Failed to access 'config' attribute inside <valvula/enviroment/local-domains>, unable to load local domains configuration");
		return axl_false;
	} /* end if */

	if (axl_cmp (config, "autodetect")) {
		/* try to detect postfix configuration */
		if (! valvulad_run_check_local_domains_config_autodetect (ctx))
			return axl_false;
	}

	/* now read common host names declared at postfix
	 * configuration */
	if (! valvulad_run_load_static_names (ctx))
		return axl_false;

	return axl_true;
}

/** 
 * @brief Starts valvulad engine using the current configuration.
 */
axl_bool valvulad_run_config (ValvuladCtx * ctx)
{
	axlNode           * node;
	ValvulaConnection * listener;
	int                 gid, pid;
	int                 request_line_limit;

	if (ctx == NULL || ctx->ctx == NULL)
		return axl_false;

	/* init log reporting */
	valvulad_log_init (ctx);

	/* get listen nodes and startup server */
	node = axl_doc_get (ctx->config, "/valvula/general/listen");
	if (node == NULL) {
		error ("Failed to start Valvula, found no listen defined");
		return axl_false;
	} /* end if */

	while (node) {
		if (! HAS_ATTR (node, "host") || ! HAS_ATTR (node, "port")) {
			error ("Failed to start Valvula, found <listen> node without host or port attribute");
			return axl_false;
		}
		
		/* start listener */
		listener = valvula_listener_new (ctx->ctx, ATTR_VALUE (node, "host"), ATTR_VALUE (node, "port"));
		if (! valvula_connection_is_ok (listener)) {
			error ("Failed to start listener at %s:%s, found error", ATTR_VALUE (node, "host"), ATTR_VALUE (node, "port"));
			return axl_false;
		}
		msg ("Started listener at %s:%s", ATTR_VALUE (node, "host"), ATTR_VALUE (node, "port"));
		axl_list_append (ctx->listeners, listener);

		/* get listen node */
		node = axl_node_get_next_called (node, "listen");
	} /* end while */

	/* load what users are going to be used before loading modules
	   :: BUT WITHOUT changing anything yet */
	node = axl_doc_get (ctx->config, "/valvula/global-settings/running");
	if (node && ATTR_VALUE (node, "user") && ATTR_VALUE (node, "group") && HAS_ATTR_VALUE (node, "enabled", "yes")) {
		/* change group first */
		gid = valvulad_get_system_id (ctx, ATTR_VALUE (node, "group"), axl_false);
		if (gid == -1) {
			error ("Group defined is not present: %s", ATTR_VALUE (node, "group"));
			return axl_false;
		}
		ctx->running_gid = gid;

		/* change user last */
		pid = valvulad_get_system_id (ctx, ATTR_VALUE (node, "user"), axl_true);
		if (pid == -1) {
			error ("User defined is not present: %s", ATTR_VALUE (node, "user"));
			return axl_false;
		}
		ctx->running_uid = pid;

	} /* end if */

	/* load modules */
	valvulad_run_load_modules (ctx, ctx->config);

	/* now configure priorities, etc */
	msg ("Calling to install/register module handler..");
	valvulad_run_register_handlers (ctx);

	/* install some internal databases */
	__valvulad_run_install_internal_dbs (ctx);

	/* install event to track day and month change */
	valvula_thread_pool_new_event (ctx->ctx, 2000000, __valvulad_run_time_tracking, ctx, NULL);

	/* check local-domains configuration */
	if (! valvulad_run_check_local_domains_config (ctx)) {
		error ("Unable to startup local domains configuration");
		return axl_false;
	} /* end if */

	/*** NOTE: before this line valvula will change its user/group
	 * so any root operation won't be possible */

	/* check for running user */
	if (ctx->running_uid > 0 && ctx->running_gid > 0) {
		/* get node where user and group is defined */
		node = axl_doc_get (ctx->config, "/valvula/global-settings/running");
		msg ("Changing process to gid %d (%s)", ctx->running_gid, ATTR_VALUE (node, "group"));
		/* change group */
		if (setgid (ctx->running_gid) == -1) {
			error ("Failed to change process group. setgid() call failed with errno=%d", errno);
			return axl_false;
		}

		/* change user */
		msg ("Changing process to pid %d (%s)", ctx->running_uid, ATTR_VALUE (node, "user"));
		if (setuid (ctx->running_uid) == -1) {
			error ("Failed to change user group. setuid() call failed with errno=%d", errno);
			return axl_false;
		}
	} /* end if */

	/* set default request limit if defined */
	node = axl_doc_get (ctx->config, "/valvula/global-settings/request-line");
	if (node && HAS_ATTR (node, "limit")) {
		request_line_limit = valvula_support_strtod (ATTR_VALUE (node, "limit"), NULL);
		if (request_line_limit != 40) {
			msg ("Configuring request line limit to %d", request_line_limit);
			valvula_ctx_set_request_line_limit (ctx->ctx, request_line_limit);
		} /* end if */
	} /* end if */


	return axl_true; 
}

/* check for resolvers here */
axl_bool __valvulad_run_request_common_object_resolve_via_resolver (ValvuladCtx * ctx, const char * item_name, ValvuladObjectRequest request_type)
{
	axl_bool                     result = axl_false;
	int                          iterator;
	ValvuladObjectResolverData * ref;
	
	/* check if no resolver is defined */
	if (axl_list_length (ctx->object_resolvers) <= 0)
		return axl_false;

	valvula_mutex_lock (&ctx->object_resolvers_mutex);

	/* check if this revolver was already added */
	iterator = 0;
	while (iterator < axl_list_length (ctx->object_resolvers)) {
		/* get resolver */
		ref = axl_list_get_nth (ctx->object_resolvers, iterator);
		if (ref && ref->resolver (ctx, item_name, request_type, ref->data)) {
			/* flag that resolver was found and break */
			result = axl_true;
			break;
		} /* end if */
			
		/* next position */
		iterator++;
	} /* end while */

	/* release lock */
	valvula_mutex_unlock (&ctx->object_resolvers_mutex);

	/* return resolver result */
	return result;

}

axl_bool __valvulad_run_request_common_object (ValvuladCtx * ctx, const char * item_name, ValvuladObjectRequest request_type)
{
	MYSQL      * dbconn;
	int          port = 3306;
	MYSQL_RES  * result;
	MYSQL_ROW    row;
	axl_bool     f_result = axl_false;

	char       * query  = NULL;
	const char * user   = NULL;
	const char * pass   = NULL;
	const char * host   = NULL;
	const char * dbname = NULL;
	const char * label  = NULL;

	if (ctx == NULL || item_name == NULL)
		return axl_false;

	/* check unallowed characters */
	if (! valvulad_db_check_unallowed_chars (ctx, item_name)) {
		error ("Detected unallowed characters in [%s], reporting error", item_name);
		return axl_false;
	}

	/* check for resolvers here */
	if (__valvulad_run_request_common_object_resolve_via_resolver (ctx, item_name, request_type))
		return axl_true;

	/* check item_name first it static hash table */
	switch (request_type) {
	case VALVULAD_OBJECT_DOMAIN:
		if (axl_hash_get (ctx->ld_hash, (axlPointer) item_name))
			return axl_true;
		/* get query reference */
		query   = ctx->ld_query;
		user    = ctx->ld_user;
		pass    = ctx->ld_pass;
		host    = ctx->ld_host;
		dbname  = ctx->ld_dbname;
		label   = "DOMAIN  -- local domain detection will not work -- rules depending on this will not work";
		break;
	case VALVULAD_OBJECT_ACCOUNT:
		if (axl_hash_get (ctx->la_hash, (axlPointer) item_name))
			return axl_true;
		/* get query reference */
		query   = ctx->la_query;
		user    = ctx->la_user;
		pass    = ctx->la_pass;
		host    = ctx->la_host;
		dbname  = ctx->la_dbname;
		label   = "ACCOUNT -- local account detection will not work -- rules depending on this will not work";
		break;
	case VALVULAD_OBJECT_ALIAS:
		if (axl_hash_get (ctx->ls_hash, (axlPointer) item_name))
			return axl_true;
		/* get query reference */
		query   = ctx->ls_query;
		user    = ctx->ls_user;
		pass    = ctx->ls_pass;
		host    = ctx->ls_host;
		dbname  = ctx->ls_dbname;
		label   = "ALIAS -- local alias detection will not work -- rules depending on this will not work";
		break;
	} /* end if */

	if (! query) {
		if (ctx->debug_queries) 
			msg ("%s: no SQL query for (%s), returning false", __AXL_PRETTY_FUNCTION__, label);

		/* if no query, no check is possible, return axl_false */
	        return f_result;
	}

	/* mysql mode */
	query = axl_strdup (query);
	axl_replace (query, "%s", item_name);
	axl_replace (query, "%d", valvula_get_domain (item_name));

	if (ctx->debug_queries)
		msg ("%s: running query (non-query=%d): %s", __AXL_PRETTY_FUNCTION__, axl_true, query);

	/* create a mysql connection */
	dbconn = mysql_init (NULL);

	/* create a connection */
	if (mysql_real_connect (dbconn, 
				/* get host */
				host,
				/* get user */
				user,
				/* get password */
				pass,
				/* get database */
				dbname,
				port, NULL, 0) == NULL) {
		error ("Mysql connect error: mysql_error(dbconn)=[%s], failed to run SQL command, mysql_real_connect() failed", mysql_error (dbconn));
		return axl_false;
	} /* end if */

	/* now run query */
	if (mysql_query (dbconn, query)) {
		axl_free (query);
		error ("Failed to run SQL query, error was %u: %s\n", mysql_errno (dbconn), mysql_error (dbconn));
			
		/* release the connection */
		mysql_close (dbconn);
		return axl_false;
	} /* end if */

	/* return result */
	result = mysql_store_result (dbconn);
	if (result == NULL) {
		axl_free (query);
		error ("Failed to run SQL query, error was %u: %s\n", mysql_errno (dbconn), mysql_error (dbconn));
			
		/* release the connection */
		mysql_close (dbconn);
		return axl_false;
	} /* end if */

	row = mysql_fetch_row (result);
	if (row == NULL || row[0] == NULL) {
		/* release result */
		mysql_free_result (result);

		/* release query */
		axl_free (query);
		
		/* close connection */
		mysql_close (dbconn);
			
		return axl_false;
	} /* end if */

	/* compare result */
	f_result = (row[0]) && strlen (row[0]) > 0;

	/* release result */
	mysql_free_result (result);

	/* release query */
	axl_free (query);

	/* close connection */
	mysql_close (dbconn);

	return f_result;
}

/** 
 * @brief Allows to check if the provided domain is considered local,
 * that is, current server is handling this domain so a delivery to
 * this domain won't be relayed.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param domain The domain to be checked.
 *
 * @return axl_true if the domain is considered local, otherwise
 * axl_false is returned. The function also returns axl_false in the case domain or ctx references are NULL.
 *
 * This function will report proper results if current valvulad server
 * is properly configured. Please, check valvula configuration at
 * valvula.conf, inside xml path /valvula/enviroment/local-domains.
 */
axl_bool valvulad_run_is_local_domain (ValvuladCtx * ctx, const char * domain)
{
	/* Call to request common object */
	return __valvulad_run_request_common_object (ctx, domain, VALVULAD_OBJECT_DOMAIN);
}

/** 
 * @brief Allows to check if the provided address is considered local,
 * that is, current server is handling this account or alias so a
 * delivery to this account/alias won't be relayed.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param address The address/alias to be checked.
 *
 * @return axl_true if the address/alias is considered local,
 * otherwise axl_false is returned. The function also returns
 * axl_false in the case address/alias or ctx references are NULL.
 *
 * This function will report proper results if current valvulad server
 * is properly configured. Please, check valvula configuration at
 * valvula.conf, inside xml path /valvula/enviroment/local-domains.
 */
axl_bool valvulad_run_is_local_address (ValvuladCtx * ctx, const char * address)
{
	/* Call to request common object */
	return __valvulad_run_request_common_object (ctx, address, VALVULAD_OBJECT_ACCOUNT) || __valvulad_run_request_common_object (ctx, address, VALVULAD_OBJECT_ALIAS);
}


/** 
 * @brief Allows to check if the provided request represents a local
 * delivery (to domains handled by this server).
 *
 * @param ctx The context where the operation will be checked.
 *
 * @param request The request to be checked.
 *
 * @return axl_true in the case the request is considered local,
 * otherwise axl_false is returned, which in that case, represents a
 * relay operation. The function also returns axl_false in the case
 * request or ctx is NULL.
 */
axl_bool valvulad_run_is_local_delivery (ValvuladCtx * ctx, ValvulaRequest * request)
{
	if (ctx == NULL || request == NULL)
		return axl_false;

	/* check if the recipient domain represents a local delivery */
	return valvulad_run_is_local_domain (ctx, valvula_get_recipient_domain (request));
}

/** 
 * @brief Allows to add a particular domain into the local domains
 * hash. This will make this domain to be considered as local by
 * valvula. This way, valvula will change the way it works for
 * operations involving this domain.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param domain The domain that is being added.
 */
void     valvulad_run_add_local_domain (ValvuladCtx * ctx, const char * domain)
{
	if (ctx == NULL || domain == NULL)
		return;

	/* init hash */
	if (ctx->ld_hash == NULL)
		ctx->ld_hash = axl_hash_new (axl_hash_string, axl_hash_equal_string);

	if (axl_hash_get (ctx->ld_hash, (axlPointer) domain))
		return;

	/* insert domain */
	axl_hash_insert_full (ctx->ld_hash, axl_strdup (domain), axl_free, INT_TO_PTR (axl_true), NULL);

	return;
}

/** 
 * @brief Allows to add an object resolver handler, a function that
 * provides support to valvula engine to resolve and identify if a
 * domain, account or alias is local to current server.
 *
 * @param ctx The context where the operation happens.
 *
 * @param resolver A handler called every time it is required to
 * resolver a object (check if it is a domain, alias or account).
 *
 * @param data Reference to user defined data.
 *
 * Function does nothing if ctx or resolver references are NULL.
 */
void     valvulad_run_add_object_resolver (ValvuladCtx * ctx, ValvuladObjectResolver resolver, axlPointer data)
{
	ValvuladObjectResolverData * ref;
	
	if (ctx == NULL || resolver == NULL)
		return;

	/* remove previous configuration */
	valvulad_run_remove_object_resolver (ctx, resolver, data);

	/* lock */
	valvula_mutex_lock (&ctx->object_resolvers_mutex);

	/* add reference */
	ref = axl_new (ValvuladObjectResolverData, 1);
	if (ref) {
		/* only add resolver if allocation was ok */
		ref->resolver = resolver;
		ref->data     = data;
		/* add resolver */
		axl_list_append (ctx->object_resolvers, ref);
	} /* end if */
	
	/* release lock */
	valvula_mutex_unlock (&ctx->object_resolvers_mutex);
	
	return;
}

/** 
 * @brief Allows to remove an object resolver handler previous
 * installed using \ref valvulad_run_add_object_resolver.
 *
 * @param ctx The context where the operation happens.
 *
 * @param resolver A handler to remove.
 *
 * @param data Reference to user defined data.
 *
 * Function does nothing if ctx or resolver references are NULL.
 */
void     valvulad_run_remove_object_resolver (ValvuladCtx * ctx, ValvuladObjectResolver resolver, axlPointer data)
{
	ValvuladObjectResolverData * ref;
	int                          iterator;
	
	if (ctx == NULL || resolver == NULL)
		return;

	valvula_mutex_lock (&ctx->object_resolvers_mutex);

	/* check if this revolver was already added */
	iterator = 0;
	while (iterator < axl_list_length (ctx->object_resolvers)) {
		/* get resolver */
		ref = axl_list_get_nth (ctx->object_resolvers, iterator);
		if (ref && ref->resolver == resolver && ref->data == data) {
			/* remove reference from the list  */
			axl_list_remove (ctx->object_resolvers, ref);
			break;
		} /* end if */
			
		
		/* next position */
		iterator++;
	} /* end while */

	/* release lock */
	valvula_mutex_unlock (&ctx->object_resolvers_mutex);
	
	return;
}

/** 
 * @}
 */
