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
#include <valvulad.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

ValvuladCtx * ctx = NULL;

/* debug status */
axl_bool      __mod_lmm_enable_debug = axl_false;

/** 
 * @brief Init function, perform all the necessary code to register
 * profiles, configure Vortex, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  lmm_init (ValvuladCtx * _ctx)
{
	axlNode         * node;

	/* configure the module */
	ctx = _ctx;

	msg ("Valvulad lmm module: init");

	/* create databases to be used by the module */
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "lmm_by_domain", 
				  /* attributes */
				  "id", "autoincrement int", 
				  /* rule status */
				  "is_active", "int",
				  /* source domain or account to apply restriction. If defined
				   * applies. If not defined, applies to all sources. When source 
				   is a local destination, it must be autenticated */
				  "domain", "varchar(1024)",
				  /* source ips and networks: if allowed_networks is empty, then  */
				  "allowed_networks", "varchar(1024)",
				  /* rule description */
				  "description", "varchar(500)",
				  NULL);

	/* get debug status */
	node = axl_doc_get (_ctx->config, "/valvula/enviroment/mod-lmm");
	if (HAS_ATTR_VALUE (node, "debug", "yes"))
		__mod_lmm_enable_debug = axl_true;

	return axl_true;
}

/** 
 * @brief Process request for the module.
 */
ValvulaState lmm_process_request (ValvulaCtx        * _ctx, 
				  ValvulaConnection * connection, 
				  ValvulaRequest    * request,
				  axlPointer          request_data,
				  char             ** message)
{
	/* sender and localpart */
	/* const char    * sender_domain        = valvula_get_sender_domain (request); */
	char          * sender_local_part    = valvula_get_sender_local_part (request);

	/* recipient and local part */
	/* const char    * recipient_domain     = valvula_get_recipient_domain (request);*/
	char          * recipient_local_part = valvula_get_recipient_local_part (request);

	/* const char    * sender           = request->sender; */
	/* const char    * recipient        = request->recipient; */
	ValvulaState    state            = VALVULA_STATE_DUNNO;


	axl_free (recipient_local_part);
	axl_free (sender_local_part);

	return state;
}

/** 
 * @brief Close function called once the valvulad server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
void lmm_close (ValvuladCtx * ctx)
{
	msg ("Valvulad lmm module: close");
	return;
}

/** 
 * @brief The reconf function is used by valvulad to notify to all
 * its modules loaded that a reconfiguration signal was received and
 * modules that could have configuration and run time change support,
 * should reread its files. It is an optional handler.
 */
void lmm_reconf (ValvuladCtx * ctx) {
	msg ("Valvulad configuration have change");
	return;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the valvulad will lookup to load the rest of items.
 */
ValvuladModDef module_def = {
	"mod-lmm",
	"Valvulad plugin to limit mail from use/forge from unauthorized sources",
	lmm_init,
	lmm_close,
	lmm_process_request,
	NULL,
	NULL
};

END_C_DECLS



/** 
 * \page valvulad_mod_lmm mod-lmm : Valvulad plugin to limit mail from use/forge from unauthorized sources
 *
 * Still not documented.
 * 
 */
