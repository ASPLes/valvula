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
#include <valvulad_run.h>

#include <dirent.h>

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
				valvula_ctx_register_request_handler (ctx->ctx, module->def->process_request, prio, port, NULL);
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
	stored_day   = GET_CELL_AS_LONG (res, 0);
	stored_month = GET_CELL_AS_LONG (res, 1);

	if (stored_month != valvula_get_month ()) {
		/* ok month changed */
		msg ("Valvula month change detected");
	}

	if (stored_day != valvula_get_day ()) {
		/* ok, day changed */
		msg ("Valvula engine day change detected, notifying..");
		valvulad_notify_day_change (ctx, valvula_get_day ());

		/* save new day */
		
	} /* end if */

	return axl_false; /* do not remove the event, please fire it
			   * again in the future */
}


/** 
 * @brief Starts valvulad engine using the current configuration.
 */
axl_bool valvulad_run_config (ValvuladCtx * ctx)
{
	axlNode           * node;
	ValvulaConnection * listener;

	if (ctx == NULL || ctx->ctx == NULL)
		return axl_false;

	/* get listen nodes and startup server */
	node = axl_doc_get (ctx->config, "/valvula/general/listen");
	if (node == NULL) {
		error ("Failed to start Valvula, found no listen por defined");
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

		/* get listen node */
		node = axl_node_get_next_called (node, "listen");
	} /* end while */

	/* load modules */
	valvulad_run_load_modules (ctx, ctx->config);

	/* now configure priorities, etc */
	msg ("Calling to install/register module handler..");
	valvulad_run_register_handlers (ctx);

	/* install some internal databases */
	__valvulad_run_install_internal_dbs (ctx);

	/* install event to track day and month change */
	valvula_thread_pool_new_event (ctx->ctx, 2000, __valvulad_run_time_tracking, ctx, NULL);
	return axl_true; 
}


