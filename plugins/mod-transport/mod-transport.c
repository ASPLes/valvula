 /* 
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
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
axl_bool      __mod_transport_enable_debug = axl_false;

/** 
 * @brief Init function, perform all the necessary code to register
 * profiles, configure Vortex, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  transport_init (ValvuladCtx * _ctx)
{
	axlNode         * node;

	/* configure the module */
	ctx = _ctx;

	msg ("Valvulad transport module: init");

	/* create databases to be used by the module */
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "transport_global", 
				  /* attributes */
				  "id", "autoincrement int", 
				  /* rule status */
				  "is_active", "int",
				  /* apply rule only to  authenticated requestes */
				  "only_auth", "int",
				  /* source domain or account to apply restriction. If defined
				   * applies. If not defined, applies to all sources. When source 
				   is a local destination, it must be autenticated */
				  "source", "varchar(1024)",
				  /* source domain or account to apply restriction. If defined
				   * applies. If not defined, applies to all local destinations. */
				  "destination", "varchar(1024)",
				  /* rule description */
				  "description", "varchar(500)",
				  /* transport: string with the postfix transport to apply to this 
				     rule (as defined in /etc/postfix/master.cf) */
				  "transport", "varchar(1024)",
				  NULL);

	/* get debug status */
	node = axl_doc_get (_ctx->config, "/valvula/enviroment/mod-transport");
	if (HAS_ATTR_VALUE (node, "debug", "yes"))
		__mod_transport_enable_debug = axl_true;

	return axl_true;
}



/** 
 * @brief Process request for the module.
 */
ValvulaState transport_process_request_aux (ValvulaCtx        * _ctx, 
					    ValvulaConnection * connection, 
					    ValvulaRequest    * request,
					    axlPointer          request_data,
					    char             ** message,
					    const char        * sender_domain,
					    const char        * sender_local_part,
					    const char        * recipient_domain,
					    const char        * recipient_local_part,
					    const char        * sender,
					    const char        * recipient)
{
	axl_bool is_authenticated = valvula_is_authenticated (request);

	/* build query with values received */

	


	/* by default report return dunno */
	return VALVULA_STATE_DUNNO;
}


/** 
 * @brief Process request for the module.
 */
ValvulaState transport_process_request (ValvulaCtx        * _ctx, 
				  ValvulaConnection * connection, 
				  ValvulaRequest    * request,
				  axlPointer          request_data,
				  char             ** message)
{
	/* sender and localpart */
	const char    * sender_domain        = valvula_get_sender_domain (request);
	char          * sender_local_part    = valvula_get_sender_local_part (request);

	/* recipient and local part */
	const char    * recipient_domain     = valvula_get_recipient_domain (request);
	char          * recipient_local_part = valvula_get_recipient_local_part (request);

	const char    * sender           = request->sender;
	const char    * recipient        = request->recipient;
	ValvulaState    state;

	/* get state */
	state = transport_process_request_aux (_ctx, connection, request, request_data, message, sender_domain, sender_local_part, recipient_domain, recipient_local_part, sender, recipient);

	axl_free (recipient_local_part);
	axl_free (sender_local_part);

	return state;
}

/** 
 * @brief Close function called once the valvulad server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
void transport_close (ValvuladCtx * ctx)
{
	msg ("Valvulad transport module: close");
	return;
}

/** 
 * @brief The reconf function is used by valvulad to notify to all
 * its modules loaded that a reconfiguration signal was received and
 * modules that could have configuration and run time change support,
 * should reread its files. It is an optional handler.
 */
void transport_reconf (ValvuladCtx * ctx) {
	msg ("Valvulad configuration have change");
	return;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the valvulad will lookup to load the rest of items.
 */
ValvuladModDef module_def = {
	"mod-transport",
	"Valvulad flexible Postfix transport map module",
	transport_init,
	transport_close,
	transport_process_request,
	NULL,
	NULL
};

END_C_DECLS



/** 
 * \page valvulad_mod_transport mod-transport : Valvula flexible Postfix transport map module
 *
 * \section valvulad_mod_transport_intro Introduction
 *
 * 
 */
