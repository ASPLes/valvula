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
#include <valvulad_run.h>

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

	return axl_true; 
}


