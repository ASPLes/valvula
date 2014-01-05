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
#include <dirent.h>
#include <valvulad_config.h>

/** 
 * @internal Iteration function used to find all <include> nodes.
 */
axl_bool valvulad_config_find_include_nodes (axlNode    * node, 
					     axlNode    * parent, 
					     axlDoc     * doc, 
					     axl_bool   * was_removed, 
					     axlPointer   ptr)
{
	/* check and add */
	if (NODE_CMP_NAME (node, "include")) {
		/* add to the list */
		axl_list_append (ptr, node);
	}
	

	return axl_true; /* never stop until the last node is
			  * visited */
}

void valvulad_config_load_expand_nodes (ValvuladCtx * ctx)
{
	axlList    * include_nodes;
	int          iterator;
	axlNode    * node, * node2;
	axlDoc     * doc_aux;
	axlError   * error;
	const char * path;
	char       * full_path;

	/* directory reading */
	DIR           * dir;
	struct dirent * dirent;
	int             length;


	/* create the list and iterate over all nodes */
	include_nodes = axl_list_new (axl_list_always_return_1, NULL);
	axl_doc_iterate (ctx->config, DEEP_ITERATION, valvulad_config_find_include_nodes, include_nodes);
	msg ("Found include nodes %d, expanding..", axl_list_length (include_nodes));

	/* next position */
	iterator = 0;
	while (iterator < axl_list_length (include_nodes)) {

		/* get the node to replace */
		node = axl_list_get_nth (include_nodes, iterator);

		/* try to load document or directory */
		if (HAS_ATTR (node, "src")) {
			error = NULL;
			path  = ATTR_VALUE (node, "src");
			msg ("Loading document from: %s", path);
			doc_aux = axl_doc_parse_from_file (path, &error);
			if (doc_aux == NULL) {
				wrn ("Failed to load document at %s, error was: %s", path, axl_error_get (error));
				axl_error_free (error);
			} else {
				/* ok, now replace the content from the document into the original document */
				node2 = axl_doc_get_root (doc_aux);
				axl_node_deattach (node2);

				/* and */
				axl_node_replace (node, node2, axl_true);

				/* associate data to the node so it is released all memory when finished */
				axl_node_annotate_data_full (node2, "doc", NULL, doc_aux, (axlDestroyFunc) axl_doc_free);
					
			} /* end if */

		} else if (HAS_ATTR (node, "dir")) {
			msg ("Opening directory: %s", ATTR_VALUE (node, "dir"));
			dir    = opendir (ATTR_VALUE (node, "dir"));
			
			while (dir && (dirent = readdir (dir))) {

				/* build path */
				full_path = axl_strdup_printf ("%s/%s", ATTR_VALUE (node, "dir"), dirent->d_name);

				/* check for regular file */
				if (! valvula_support_file_test (full_path, FILE_IS_REGULAR)) {
					axl_free (full_path);
					continue;
				} /* end if */
					
				/* found regular file */
				msg ("Found regular file: %s..loading", full_path);
				length = strlen (full_path);
				if (full_path[length - 1] == '~') {
					wrn ("Skipping file %s", full_path);
					axl_free (full_path);
					continue;
				}

				/* load document */
				error   = NULL;
				doc_aux = axl_doc_parse_from_file (full_path, &error);
				if (doc_aux == NULL) {
					wrn ("Failed to load document at %s, error was: %s", full_path, axl_error_get (error));
					axl_error_free (error);
					axl_free (full_path);
					continue;
				} 
				axl_free (full_path);

				/* ok, now replace the content from the document into the original document */
				node2 = axl_doc_get_root (doc_aux);
				axl_node_deattach (node2);
					
				/* set the node content following reference node */
				axl_node_set_child_after (node, node2);

				/* associate data to the node so it is released all memory when finished */
				axl_node_annotate_data_full (node2, "doc", NULL, doc_aux, (axlDestroyFunc) axl_doc_free);
					
				/* next directory */
			}

			/* close directory */
			closedir (dir);
			
			/* remove include node */
			axl_node_remove (node, axl_true);
		}



		/* next iterator */
		iterator++;
	} /* end while */

	axl_list_free (include_nodes);
	return;
}

/** 
 * @brief Allows to open configuration file.
 *
 * @param ctx The valvulad context.
 *
 * @param config Full path to the configuration file to be opened.
 */
axl_bool        valvulad_config_load     (ValvuladCtx * ctx, 
					  const char    * config)
{
	axlError   * error;

	/* check null value */
	if (config == NULL) {
		error ("config file not defined, terminating valvulad");
		return axl_false;
	} /* end if */

	/* get a reference to the configuration path used for this context */
	ctx->config_path = axl_strdup (config);

	/* load the file */
	ctx->config = axl_doc_parse_from_file (config, &error);
	if (ctx->config == NULL) {
		error ("unable to load file (%s), it seems a xml error: %s", 
		       config, axl_error_get (error));

		/* free resources */
		axl_error_free (error);

		/* call to finish valvulad */
		return axl_false;

	} /* end if */

	/* drop a message */
	msg ("file %s loaded, ok", config);

	/* now process inclusions */
	valvulad_config_load_expand_nodes (ctx);

	msg ("server configuration is valid..");
	
	return axl_true;
}


/** 
 * @brief Allows to check if an xml attribute is positive, that is,
 * have 1, true or yes as value.
 *
 * @param ctx The valvulad context.
 *
 * @param node The node to check for positive attribute value.
 *
 * @param attr_name The node attribute name to check for positive
 * value.
 */
axl_bool        valvulad_config_is_attr_positive (ValvuladCtx * ctx,
						  axlNode       * node,
						  const char    * attr_name)
{
	if (ctx == NULL || node == NULL)
		return axl_false;

	/* check for yes, 1 or true */
	if (HAS_ATTR_VALUE (node, attr_name, "yes"))
		return axl_true;
	if (HAS_ATTR_VALUE (node, attr_name, "1"))
		return axl_true;
	if (HAS_ATTR_VALUE (node, attr_name, "true"))
		return axl_true;

	/* none of them was found */
	return axl_false;
}

/**
 * @brief Allows to check if an xml attribute is positive, that is,
 * have 1, true or yes as value.
 *
 * @param ctx The valvulad context.
 *
 * @param node The node to check for positive attribute value.
 *
 * @param attr_name The node attribute name to check for positive
 * value.
 */
axl_bool        valvulad_config_is_attr_negative (ValvuladCtx * ctx,
						  axlNode       * node,
						  const char    * attr_name)
{
	if (ctx == NULL || node == NULL)
		return axl_false;

	/* check for yes, 1 or true */
	if (HAS_ATTR_VALUE (node, attr_name, "no"))
		return axl_true;
	if (HAS_ATTR_VALUE (node, attr_name, "0"))
		return axl_true;
	if (HAS_ATTR_VALUE (node, attr_name, "false"))
		return axl_true;

	/* none of them was found */
	return axl_false;
}


/** 
 * @brief Allows to get the value found on provided config path at the
 * selected attribute.
 *
 * @param ctx The valvulad context where to get the configuration value.
 *
 * @param path The path to the node where the config is found.
 *
 * @param attr_name The attribute name to be returned as a number.
 *
 * @return The function returns the value configured or -1 in the case
 * the configuration is wrong. The function returns -2 in the case
 * path, ctx or attr_name are NULL. The function returns -3 in the
 * case the path is not found so the user can take default action.
 */
int             valvulad_config_get_number (ValvuladCtx * ctx, 
					    const char    * path,
					    const char    * attr_name)
{
	axlNode * node;
	int       value;
	char    * error = NULL;

	/* check values received */
	v_return_val_if_fail (ctx && path && attr_name, -2);

	msg ("Getting value at path %s (%s)", path, attr_name);

	/* get the node */
	node = axl_doc_get (ctx->config, path);
	if (node == NULL) {
		wrn ("  Path %s was not found in config (%p)", path, ctx->config);
		return -3;
	}

	msg ("  Translating value to a number %s=%s", attr_name, ATTR_VALUE (node, attr_name));

	/* now get the value */
	value = valvula_support_strtod (ATTR_VALUE (node, attr_name), &error);
	if (error && strlen (error) > 0) 
		return -1;
	return value;
}
