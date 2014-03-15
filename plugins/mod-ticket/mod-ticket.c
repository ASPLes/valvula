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

ValvulaMutex  work_mutex;

/** 
 * @brief Handler called when day changes.
 */
void ticket_change_day (ValvuladCtx * ctx, long new_value, axlPointer user_data)
{
	ValvuladRes res;
	ValvuladRow row;
	long        now;

	/* get all usage records before setting them to 0 */
	res = valvulad_db_run_query (ctx,  "SELECT id, current_day_usage FROM domain_ticket WHERE current_day_usage > 0");
	
	/* get next row */
	row = GET_ROW (res);
	while (row) {
		/* handle data to record history */
		now = valvula_now ();

		if (! valvulad_db_run_query (ctx, "INSERT INTO domain_ticket_history (domain_ticket_id, stamp, day_usage) VALUES ('%d', '%d', '%d')",
					     GET_CELL_AS_LONG (row, 0), now, GET_CELL_AS_LONG (row, 1))) {
			error ("Failed to insert domain ticket history..");
		} /* end if */

		/* next row */
		row = GET_ROW (res);
	} /* end while */

	/* release previous data */
	valvulad_db_release_result (res);

	/* reset day usage for all mail plans */
	valvulad_db_run_query (ctx, "UPDATE domain_ticket SET current_day_usage = '0'");

	return;
}

/** 
 * @brief Handler called when month changes.
 */
void ticket_change_month (ValvuladCtx * ctx, long new_value, axlPointer user_data)
{
	ValvuladRes res;
	ValvuladRow row;
	long        now;

	/* get all usage records before setting them to 0 */
	res = valvulad_db_run_query (ctx,  "SELECT id, current_month_usage FROM domain_ticket WHERE current_month_usage > 0");
	
	/* get next row */
	row = GET_ROW (res);
	while (row) {
		/* handle data to record history */
		now = valvula_now ();

		if (! valvulad_db_run_query (ctx, "INSERT INTO domain_month_ticket_history (domain_ticket_id, stamp, month_usage) VALUES ('%d', '%d', '%d')",
					     GET_CELL_AS_LONG (row, 0), now, GET_CELL_AS_LONG (row, 1))) {
			error ("Failed to insert domain ticket history (month)..");
		} /* end if */

		/* next row */
		row = GET_ROW (res);
	} /* end while */

	/* release previous data */
	valvulad_db_release_result (res);

	/* reset day usage for all mail plans */
	valvulad_db_run_query (ctx, "UPDATE domain_ticket SET current_month_usage = '0'");

	return;
}

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
				  NULL);

	/* global domain plan */
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
				  /* ticket plan id */
				  "ticket_plan_id", "int",
				  NULL);

	/* day tracking */
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "domain_ticket_history", 
				  /* attributes */
				  "id", "autoincrement int", 
				  /* sending domain these tickets applies to */
				  "domain_ticket_id", "int",
				  /* history stamp */
				  "stamp", "int",
				  /* total day used */
				  "day_usage", "int",
				  NULL);

	/* month tracking */
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "domain_month_ticket_history", 
				  /* attributes */
				  "id", "autoincrement int", 
				  /* sending domain these tickets applies to */
				  "domain_ticket_id", "int",
				  /* history stamp */
				  "stamp", "int",
				  /* total day used */
				  "month_usage", "int",
				  NULL);

	/* add on day and on month change */
	valvulad_add_on_day_change (ctx, ticket_change_day, NULL);
	valvulad_add_on_month_change (ctx, ticket_change_month, NULL);

	/* init lock */
	valvula_mutex_create (&work_mutex);

	return axl_true;
}

axlPointer __ticket_get_row_or_fail (ValvuladCtx * ctx, axlPointer result) {
	axlPointer row;

	/* get row */
	row = valvulad_db_get_row (ctx, result);
	if (row == NULL) {
		/* release results */
		valvulad_db_release_result (result);

		/* unlock */
		valvula_mutex_unlock (&work_mutex);

		/* maybe the database configurat was removed before checking previous request, no problem */
		return NULL;
	} /* end if */

	/* report pointer found */
	return row;
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

	/* get result and row */
	ValvuladRes    result  = NULL;
	ValvuladRow    row;

	long           current_day_usage;
	long           current_month_usage;
	long           valid_until;
	long           total_used;
	long           record_id;

	long           ticket_plan_id;
	long           total_limit;
	long           day_limit;
	long           month_limit;

	/* check if the domain is limited by ticket */
	if (valvula_get_sender_domain (request))
		domain_in_tickets   = valvulad_db_boolean_query (ctx, "SELECT * FROM domain_ticket WHERE domain = '%s'", valvula_get_sender_domain (request));
	if (valvula_get_sasl_user (request))
		sasl_user_in_tickets = valvulad_db_boolean_query (ctx, "SELECT * FROM domain_ticket WHERE sasl_user = '%s'", valvula_get_sasl_user (request));

	/* skip if the domain or the sasl user in the request is not
	 * limited by the domain request */
	if (! domain_in_tickets && ! sasl_user_in_tickets) 
		return VALVULA_STATE_DUNNO;

	/* lock */
	valvula_mutex_lock (&work_mutex);

	/* get current tickets for this domain */
	if (sasl_user_in_tickets) 
		result = valvulad_db_run_query (ctx, "SELECT total_used, current_day_usage, current_month_usage, valid_until, ticket_plan_id, id FROM domain_ticket WHERE sasl_user = '%s'",
						valvula_get_sasl_user (request));
	else if (domain_in_tickets)
		result = valvulad_db_run_query (ctx, "SELECT total_used, current_day_usage, current_month_usage, valid_until, ticket_plan_id, id FROM domain_ticket WHERE domain = '%s'",
						valvula_get_sender_domain (request));

	if (! result) {
		/* unlock */
		valvula_mutex_unlock (&work_mutex);

		/* maybe the database configurat was removed before checking previous request, no problem */
		return VALVULA_STATE_DUNNO;
	} /* end if */

	/* get the values we are interesting in */
	row = __ticket_get_row_or_fail (ctx, result);
	if (row == NULL) 
		return VALVULA_STATE_DUNNO;

	/* get if the limit is expired */
	valid_until = GET_CELL_AS_LONG (row, 3);
	if (valvula_now () > valid_until) {
		/* release results */
		valvulad_db_release_result (result);

		/* unlock */
		valvula_mutex_unlock (&work_mutex);

		/* not accepted */
		valvulad_reject (ctx, request, "Rejecting operation because tickets are expired (valid_until %d < %d)",
				 valid_until, valvula_now ());
		return VALVULA_STATE_REJECT;
	} /* end if */

	/* get daily use, monthly use and total usage */
	total_used          = GET_CELL_AS_LONG (row, 0);
	current_day_usage   = GET_CELL_AS_LONG (row, 1);
	current_month_usage = GET_CELL_AS_LONG (row, 2);
	ticket_plan_id      = GET_CELL_AS_LONG (row, 4);
	record_id           = GET_CELL_AS_LONG (row, 5);

	/* release result */
	valvulad_db_release_result (result);

	/* now get limits from plan */
	result = valvulad_db_run_query (ctx, "SELECT total_limit, day_limit, month_limit FROM ticket_plan WHERE id = '%d'",
					ticket_plan_id);
	if (result == NULL) {
		/* don't record an error here because the database
		 * record may have vanished during the execution of
		 * this tests */

		/* unlock */
		valvula_mutex_unlock (&work_mutex);

		return VALVULA_STATE_DUNNO;
	} /* end if */

	/* get row from result */
	row = __ticket_get_row_or_fail (ctx, result);
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

	/* msg ("mod-ticket: %s total limit: %d (used: %d), day limit: %d (used: %d), month limit: %d (used: %d)",
	     valvula_get_sasl_user (request) ? valvula_get_sasl_user (request) : valvula_get_sender_domain (request),
	     total_limit, total_used, day_limit, current_day_usage, month_limit, current_month_usage);*/

	/* check total used limit */
	if (total_used > total_limit) {
		/* unlock */
		valvula_mutex_unlock (&work_mutex);

		valvulad_reject (ctx, request, "Rejecting operation because total plan limit's reached (%d)", total_used);
		return VALVULA_STATE_REJECT;
	} /* end if */

	/* check day limit */
	if (current_day_usage > day_limit) {
		/* unlock */
		valvula_mutex_unlock (&work_mutex);

		valvulad_reject (ctx, request, "Rejecting operation because day limit reached (%d)", day_limit);
		return VALVULA_STATE_REJECT;
	} /* end if */

	/* check month limit */
	if (current_month_usage > month_limit) {
		/* unlock */
		valvula_mutex_unlock (&work_mutex);

		valvulad_reject (ctx, request, "Rejecting operation because month limit reached (%d)", month_limit);
		return VALVULA_STATE_REJECT;
	} /* end if */

	/* operation accepted, update database */
	if (! valvulad_db_run_non_query (ctx, "UPDATE domain_ticket SET current_day_usage = %d, current_month_usage = %d, total_used = %d WHERE id = %d",
					 current_day_usage, current_month_usage, total_used, record_id)) {
		error ("Failed to update record on mod-ticket");
	} /* end if */

	/* unlock */
	valvula_mutex_unlock (&work_mutex);
	
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
