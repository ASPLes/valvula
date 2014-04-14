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
	axl_bool        domain_in_bwls    = axl_false;
	axl_bool        sasl_user_in_bwls = axl_false;
	axl_bool        alternative_user     = axl_false;
	const char    * descriptive_user     = "<unknown>";

	/* get result and row */
	ValvuladRes    result  = NULL;
	ValvuladRow    row;

	long           current_day_usage;
	long           current_month_usage;
	long           valid_until;
	long           total_used;
	long           record_id;
	long           block_bwl;

	long           bwl_plan_id;
	long           total_limit;
	long           day_limit;
	long           month_limit;
	
	const char   * query;

	/* check if the domain is limited by bwl */
	if (valvula_get_sender_domain (request))
		domain_in_bwls    = valvulad_db_boolean_query (ctx, "SELECT * FROM domain_bwl WHERE domain = '%s'", valvula_get_sender_domain (request));
	if (valvula_get_sasl_user (request))
		sasl_user_in_bwls = valvulad_db_boolean_query (ctx, "SELECT * FROM domain_bwl WHERE sasl_user = '%s'", valvula_get_sasl_user (request));

	/* check for alternative names */
	if (! domain_in_bwls && ! sasl_user_in_bwls) {
		/* check for alternatives */
		if (valvula_get_sender_domain (request)) {
			result               = valvulad_db_run_query (ctx, "SELECT domain FROM domain_bwl WHERE domain like '#%s#'", valvula_get_sender_domain (request));
			domain_in_bwls    = mod_bwl_ensure_alternative_user (ctx, result, valvula_get_sender_domain (request));
			descriptive_user     = valvula_get_sender_domain (request);
		} /* end if */
		if (valvula_get_sasl_user (request)) {
			result               = valvulad_db_run_query (ctx, "SELECT sasl_user FROM domain_bwl WHERE sasl_user like '#%s#'", valvula_get_sasl_user (request));
			sasl_user_in_bwls = mod_bwl_ensure_alternative_user (ctx, result, valvula_get_sasl_user (request));
			descriptive_user     = valvula_get_sasl_user (request);
		} /* end if */

		/* update alternative user flag */
		alternative_user = domain_in_bwls || sasl_user_in_bwls;
	}

	/* skip if the domain or the sasl user in the request is not
	 * limited by the domain request */
	if (! domain_in_bwls && ! sasl_user_in_bwls) 
		return VALVULA_STATE_DUNNO;

	/* get current bwls for this domain */
	if (sasl_user_in_bwls) {
		/* prepare query to get the right user */
		if (alternative_user)
			query = "SELECT total_used, current_day_usage, current_month_usage, valid_until, bwl_plan_id, id, block_bwl FROM domain_bwl WHERE sasl_user LIKE '#%s#'";
		else 
			query = "SELECT total_used, current_day_usage, current_month_usage, valid_until, bwl_plan_id, id, block_bwl FROM domain_bwl WHERE sasl_user = '%s'";

		/* run query */
		result = valvulad_db_run_query (ctx,  query, valvula_get_sasl_user (request));
	} else if (domain_in_bwls) {
		/* prepare query to get the right domain */
		if (alternative_user) 
			query = "SELECT total_used, current_day_usage, current_month_usage, valid_until, bwl_plan_id, id, block_bwl FROM domain_bwl WHERE domain LIKE '#%s#'";
		else
			query = "SELECT total_used, current_day_usage, current_month_usage, valid_until, bwl_plan_id, id, block_bwl FROM domain_bwl WHERE domain = '%s'";

		/* run query */
		result = valvulad_db_run_query (ctx, query, valvula_get_sender_domain (request));
	} /* end if */

	if (! result) {
		/* maybe the database configurat was removed before checking previous request, no problem */
		return VALVULA_STATE_DUNNO;
	} /* end if */

	/* get the values we are interesting in */
	row = __bwl_get_row_or_fail (ctx, result);
	if (row == NULL) 
		return VALVULA_STATE_DUNNO;

	/* get if the limit is expired */
	valid_until = GET_CELL_AS_LONG (row, 3);
	if (valid_until != -1 && valvula_now () > valid_until) {
		/* release results */
		valvulad_db_release_result (result);

		/* not accepted */
		valvulad_reject (ctx, request, "Rejecting operation because bwls are expired (valid_until %d < %d)",
				 valid_until, valvula_now ());
		return VALVULA_STATE_REJECT;
	} /* end if */

	/* get daily use, monthly use and total usage */
	total_used          = GET_CELL_AS_LONG (row, 0);
	current_day_usage   = GET_CELL_AS_LONG (row, 1);
	current_month_usage = GET_CELL_AS_LONG (row, 2);
	bwl_plan_id      = GET_CELL_AS_LONG (row, 4);
	record_id           = GET_CELL_AS_LONG (row, 5);
	block_bwl        = GET_CELL_AS_LONG (row, 6);

	if (block_bwl) {
		/* release result */
		valvulad_db_release_result (result);

		valvulad_reject (ctx, request, "Rejecting operation because bwl (%d) is blocked(%d) for user (%s)",
				 bwl_plan_id, block_bwl, descriptive_user);
				 
		return VALVULA_STATE_REJECT;
	}

	/* release result */
	valvulad_db_release_result (result);

	/* now get limits from plan */
	result = valvulad_db_run_query (ctx, "SELECT total_limit, day_limit, month_limit FROM bwl_plan WHERE id = '%d'",
					bwl_plan_id);
	if (result == NULL) {
		/* don't record an error here because the database
		 * record may have vanished during the execution of
		 * this tests */

		return VALVULA_STATE_DUNNO;
	} /* end if */

	/* get row from result */
	row = __bwl_get_row_or_fail (ctx, result);
	if (row == NULL) 
		return VALVULA_STATE_DUNNO;

	/* get values */
	total_limit = GET_CELL_AS_LONG (row, 0);
	day_limit   = GET_CELL_AS_LONG (row, 1);
	month_limit = GET_CELL_AS_LONG (row, 2);

	/* release result */
	valvulad_db_release_result (result);

	/* apply operations */
	total_used ++;
	current_day_usage ++;
	current_month_usage ++;

	/* msg ("mod-bwl: %s total limit: %d (used: %d), day limit: %d (used: %d), month limit: %d (used: %d)",
	     valvula_get_sasl_user (request) ? valvula_get_sasl_user (request) : valvula_get_sender_domain (request),
	     total_limit, total_used, day_limit, current_day_usage, month_limit, current_month_usage);*/

	/* check total used limit */
	if (total_used > total_limit) {

		valvulad_reject (ctx, request, "Rejecting operation because total plan limit's reached (%d)", total_used);
		return VALVULA_STATE_REJECT;
	} /* end if */

	/* check day limit */
	if (current_day_usage > day_limit) {

		valvulad_reject (ctx, request, "Rejecting operation because day limit reached (%d)", day_limit);
		return VALVULA_STATE_REJECT;
	} /* end if */

	/* check month limit */
	if (current_month_usage > month_limit) {

		valvulad_reject (ctx, request, "Rejecting operation because month limit reached (%d)", month_limit);
		return VALVULA_STATE_REJECT;
	} /* end if */

	/* operation accepted, update database */
	if (! valvulad_db_run_non_query (ctx, "UPDATE domain_bwl SET current_day_usage = %d, current_month_usage = %d, total_used = %d WHERE id = %d",
					 current_day_usage, current_month_usage, total_used, record_id)) {
		error ("Failed to update record on mod-bwl");
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
