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
#include <valvulad.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

ValvuladCtx * ctx = NULL;

/** 
 * @brief Init function, perform all the necessary code to register
 * profiles, configure Vortex, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  ticket_init (ValvuladCtx * _ctx)
{
	/* configure the module */
	ctx = _ctx;

	msg ("Valvulad ticket module: init");

	/* create databases to be used by the module */
	msg ("Calling valvulad_db_ensure_table() ..");
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "ticket_plan", 
				  /* attributes */
				  "id", "autoincrement int", 
				  "name", "varchar(250)",
				  "description", "varchar(500)",
				  /* how many tickets does the domain have */
				  "total_limit", "int",
				  /* day limit that can be consumed */
				  "day_limit", "int",
				  /* month limit that can be consumed */
				  "month_limit", "int",
				  /* applies to sasl authenticated sends too */
				  "applies_sasl", "int",
				  /* applies to sasl authenticated sends too */
				  "applies_without_sasl", "int",
				  NULL);

	/* global domain plan */
	msg ("Calling valvulad_db_ensure_table() ..2..");
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "domain_ticket", 
				  /* attributes */
				  "id", "autoincrement int", 
				  /* sending domain these tickets applies to */
				  "domain", "varchar(512)",
				  /* sending sasl user these tickets applies to */
				  "sasl_user", "varchar(512)",
				  /* how many tickets were used by this module */
				  "total_used", "int",
				  /* current day usage */
				  "current_day_usage", "int", 
				  /* current month usage */
				  "current_month_usage", "int",
				  /* valid until (if -1 no valid until limit, if defined, epoch until it is valid) */
				  "valid_until", "int",
				  NULL);

	return axl_true;
}

/** 
 * @brief Process request for the module.
 */
ValvulaState ticket_process_request (ValvulaCtx        * _ctx, 
				     ValvulaConnection * connection, 
				     ValvulaRequest    * request,
				     axlPointer          request_data,
				     char             ** message)
{
	axl_bool domain_in_tickets    = axl_false;
	axl_bool sasl_user_in_tickets = axl_false;

	msg ("calling to process at mod ticket");
	
	/* check if the domain is limited by ticket */
	if (valvula_get_sender_domain (request))
		domain_in_tickets   = valvulad_db_boolean_query (ctx, "SELECT * FROM domain_ticket WHERE domain = '%s'", valvula_get_sender_domain (request));
	if (valvula_get_sasl_user (request))
		sasl_user_in_tickets = valvulad_db_boolean_query (ctx, "SELECT * FROM domain_ticket WHERE sasl_user = '%s'", valvula_get_sasl_user (request));

	/* skip if the domain or the sasl user in the request is not
	 * limited by the domain request */
	if (! domain_in_tickets && ! sasl_user_in_tickets)
		return VALVULA_STATE_DUNNO;

	
	/* by default report return dunno */
	return VALVULA_STATE_DUNNO;
}

/** 
 * @brief Close function called once the valvulad server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
void ticket_close (ValvuladCtx * ctx)
{
	msg ("Valvulad ticket module: close");
	return;
}

/** 
 * @brief The reconf function is used by valvulad to notify to all
 * its modules loaded that a reconfiguration signal was received and
 * modules that could have configuration and run time change support,
 * should reread its files. It is an optional handler.
 */
void ticket_reconf (ValvuladCtx * ctx) {
	msg ("Valvulad configuration have change");
	return;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the valvulad will lookup to load the rest of items.
 */
ValvuladModDef module_def = {
	"mod-ticket",
	"Valvulad ticket module",
	ticket_init,
	ticket_close,
	ticket_process_request,
	NULL,
	NULL
};

END_C_DECLS
