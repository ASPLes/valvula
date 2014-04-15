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
static int  bwl_init (ValvuladCtx * _ctx)
{
	/* configure the module */
	ctx = _ctx;

	msg ("Valvulad bwl module: init");

	/* create databases to be used by the module */
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "bwl_global", 
				  /* attributes */
				  "id", "autoincrement int", 
				  /* rule status */
				  "is_active", "int",
				  /* source domain or account to apply restriction */
				  "source", "varchar(1024)",
				  "description", "varchar(500)",
				  /* status: reject, discard, ok */
				  "status", "varchar(32)",
				  NULL);

	/* now create domain level table, rules that only affects when
	 * the destination domain matches with the stored value */
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "bwl_by_domain", 
				  /* attributes */
				  "id", "autoincrement int",
				  /* rule status */
				  "is_active", "int",
				  /* destination domain or account to apply restriction */
				  "destination", "varchar(1024)", 
				  /* source domain or account to apply restriction */
				  "source", "varchar(1024)",
				  "description", "varchar(500)",
				  /* status: reject, discard, ok */
				  "status", "varchar(32)",
				  NULL);

	/* now create account level table, rules that only affects
	 * when the destination account matches with the stored
	 * value */
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "bwl_by_account", 
				  /* attributes */
				  "id", "autoincrement int",
				  /* rule status */
				  "is_active", "int",
				  /* destination account to apply restriction */
				  "account", "varchar(1024)", 
				  /* source domain or account to apply restriction */
				  "source", "varchar(1024)",
				  "description", "varchar(500)",
				  /* status: reject, discard, ok */
				  "status", "varchar(32)",
				  NULL);

	return axl_true;
}

/** 
 * @brief Process request for the module.
 */
ValvulaState bwl_process_request (ValvulaCtx        * _ctx, 
				     ValvulaConnection * connection, 
				     ValvulaRequest    * request,
				     axlPointer          request_data,
				     char             ** message)
{
	const char    * sender_domain = valvula_get_sender_domain (request);
	const char    * sender        = request->sender;
	const char    * status;
	ValvulaState    state;
	char          * query;

	/* get result and row */
	ValvuladRes     result  = NULL;
	ValvuladRow     row;

	/* get current status*/
	result = valvulad_db_run_query (ctx, "SELECT status FROM bwl_global WHERE is_active = '1' AND (source = '%s' OR source = '%s')",
					sender, sender);
	if (! result) {
		/* maybe the database configuration was removed before checking previous request, no problem */
		return VALVULA_STATE_DUNNO;
	} /* end if */

	/* get row and then status */
	row = GET_ROW (result);
	if (row == NULL || row[0] == NULL)
		return VALVULA_STATE_DUNNO;

	/* get status */
	status = row[0];

	/* now check values */
	if (axl_stream_casecmp (status, "ok"))
		return VALVULA_STATE_OK;
	if (axl_stream_casecmp (status, "reject")) {
		valvulad_reject (ctx, request, "Rejecting due to blacklist");
		return VALVULA_STATE_REJECT;
	} /* end if */
	if (axl_stream_casecmp (status, "discard")) {
		valvulad_reject (ctx, request, "Discard due to blacklist");
		return VALVULA_STATE_DISCARD;
	} /* end if */
		

	/* by default report return dunno */
	return VALVULA_STATE_DUNNO;
}

/** 
 * @brief Close function called once the valvulad server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
void bwl_close (ValvuladCtx * ctx)
{
	msg ("Valvulad bwl module: close");
	return;
}

/** 
 * @brief The reconf function is used by valvulad to notify to all
 * its modules loaded that a reconfiguration signal was received and
 * modules that could have configuration and run time change support,
 * should reread its files. It is an optional handler.
 */
void bwl_reconf (ValvuladCtx * ctx) {
	msg ("Valvulad configuration have change");
	return;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the valvulad will lookup to load the rest of items.
 */
ValvuladModDef module_def = {
	"mod-bwl",
	"Valvulad Black and white lists module",
	bwl_init,
	bwl_close,
	bwl_process_request,
	NULL,
	NULL
};

END_C_DECLS
