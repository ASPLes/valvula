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

	/* handle data to record history */
	now = valvula_now ();
	
	/* get next row */
	row = GET_ROW (res);
	while (row) {

		/* in the following sentence, we substract 100 to now to make sure it references to the previous day where the quota was consumed */
		if (! valvulad_db_run_query (ctx, "INSERT INTO domain_ticket_history (domain_ticket_id, stamp, day_usage) VALUES ('%d', '%d', '%d')",
					     GET_CELL_AS_LONG (row, 0), now - 100, GET_CELL_AS_LONG (row, 1))) {
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

	/* handle data to record history */
	now = valvula_now ();
	
	/* get next row */
	row = GET_ROW (res);
	while (row) {

		/* in the following sentence, we substract 100 to now to make sure it references to the previous day where the quota was consumed */
		if (! valvulad_db_run_query (ctx, "INSERT INTO domain_month_ticket_history (domain_ticket_id, stamp, month_usage) VALUES ('%d', '%d', '%d')",
					     GET_CELL_AS_LONG (row, 0), now - 100, GET_CELL_AS_LONG (row, 1))) {
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
				  "domain", "text",
				  /* sending sasl user these tickets applies to */
				  "sasl_user", "text",
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
				  /* flag to block the user */
				  "block_ticket", "int",
				  /* has fixed outgoing ip */
				  "has_outgoing_ip", "int",
				  /* reference to the outgoing ip if any */
				  "outgoing_ip_id", "int",
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

	/* out going ips */
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "outgoing_ip", 
				  /* attributes */
				  "id", "autoincrement int", 
				  /* is active */
				  "is_active", "int", 
				  /* out going ip */
				  "outgoing_ip", "varchar(32)",
				  /* transport */
				  "transport", "varchar(256)",
				  /* label */
				  "label", "varchar(256)",
				  /* description */
				  "description", "varchar(512)",
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

axl_bool mod_ticket_ensure_alternative_user (ValvuladCtx * ctx, ValvuladRes result, const char * user_or_domain)
{
	/* get result and row */
	ValvuladRow      row;
	const char     * str_value;
	char          ** items;
	int              iterator;

	/* check result received */
	if (result == NULL) {
		/* msg ("No alternative user found for %s", user_or_domain); */
		return axl_false;
	}

	/* trim value */
	axl_stream_trim ((char *) user_or_domain);

	/* get row */
	row = GET_ROW (result);
	while (row) {
		/* get cell */
		str_value = GET_CELL(row, 0);
		if (str_value) {
			/* split */
			items = axl_split (str_value, 1, ",");
			iterator = 0;
			while (items && items[iterator]) {

				/* trim value */
				axl_stream_trim (items[iterator]);
				/*msg ("Alternative checking %s with %s", user_or_domain, items[iterator]);*/
				
				if (axl_cmp (items[iterator], user_or_domain)) {
					/* release results */
					valvulad_db_release_result (result);

					/* found match, perfect! */
					axl_freev (items);
					return axl_true;
				}

				/* next position */
				iterator++;
			} /* end while */

			/* free this items */
			axl_freev (items);
		} /* end if */

		/* next row */
		row = GET_ROW (result);
	} /* end while */

	/* release results */
	valvulad_db_release_result (result);

	/* msg ("No alternative user found for %s (2)", user_or_domain); */
	return axl_false;

}

ValvulaState __mod_ticket_return_dunno_or_filter (ValvuladCtx * ctx, const char * descriptive_user, long has_outgoing_ip, long outgoing_ip_id, char ** message)
{
	/* get result and row */
	ValvuladRes    result  = NULL;
	ValvuladRow    row;
	const char   * transport;
	axl_bool       is_active;

	if (! has_outgoing_ip || outgoing_ip_id <= 0) {
		/* report dunno (accept) if it was not outgoing ip */
		return VALVULA_STATE_DUNNO;
	} /* end if */

	/* call to get values for transport */
	result  = valvulad_db_run_query (ctx, "SELECT transport, is_active FROM outgoing_ip WHERE id = '%d'", outgoing_ip_id);
	row     = __ticket_get_row_or_fail (ctx, result);
	if (row == NULL) {
		/* found error, report DUNNO for now */
		return VALVULA_STATE_DUNNO;
	} /* end if */

	/* get transport */
	transport = GET_CELL (row, 0);
	is_active = GET_CELL_AS_LONG (row, 1);

	if (! is_active) {
		/* found that this transport is not enabled, so tell
		 * postfix to use default transport that is, by
		 * reporting DUNNO */
		return VALVULA_STATE_DUNNO;
	} /* end if */

	(*message) = axl_strdup_printf ("%s:", transport);

	/* release result */
	valvulad_db_release_result (result);

	/* report dunno (accept) if it was not outgoing ip */
	return VALVULA_STATE_FILTER;
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
	axl_bool        domain_in_tickets    = axl_false;
	axl_bool        sasl_user_in_tickets = axl_false;
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
	long           block_ticket;
	long           has_outgoing_ip;
	long           outgoing_ip_id;

	long           ticket_plan_id;
	long           total_limit;
	long           day_limit;
	long           month_limit;
	
	const char   * query;
	int            count_update;

	/* check if the domain is limited by ticket */
	if (valvula_get_sender_domain (request))
		domain_in_tickets    = valvulad_db_boolean_query (ctx, "SELECT * FROM domain_ticket WHERE domain = '%s'", valvula_get_sender_domain (request));
	if (valvula_get_sasl_user (request))
		sasl_user_in_tickets = valvulad_db_boolean_query (ctx, "SELECT * FROM domain_ticket WHERE sasl_user = '%s'", valvula_get_sasl_user (request));

	/* check for alternative names */
	if (! domain_in_tickets && ! sasl_user_in_tickets) {
		/* check for alternatives */
		if (valvula_get_sender_domain (request)) {
			result               = valvulad_db_run_query (ctx, "SELECT domain FROM domain_ticket WHERE domain like '#%s#'", valvula_get_sender_domain (request));
			domain_in_tickets    = mod_ticket_ensure_alternative_user (ctx, result, valvula_get_sender_domain (request));
			descriptive_user     = valvula_get_sender_domain (request);
		} /* end if */
		if (valvula_get_sasl_user (request)) {
			result               = valvulad_db_run_query (ctx, "SELECT sasl_user FROM domain_ticket WHERE sasl_user like '#%s#'", valvula_get_sasl_user (request));
			sasl_user_in_tickets = mod_ticket_ensure_alternative_user (ctx, result, valvula_get_sasl_user (request));
			descriptive_user     = valvula_get_sasl_user (request);
		} /* end if */

		/* update alternative user flag */
		alternative_user = domain_in_tickets || sasl_user_in_tickets;
	}

	/* skip if the domain or the sasl user in the request is not
	 * limited by the domain request */
	if (! domain_in_tickets && ! sasl_user_in_tickets) {
		return VALVULA_STATE_DUNNO;
	}

	/* lock */
	valvula_mutex_lock (&work_mutex);

	/* get current tickets for this domain */
	if (sasl_user_in_tickets) {
		/* prepare query to get the right user */
		if (alternative_user)
			query = "SELECT total_used, current_day_usage, current_month_usage, valid_until, ticket_plan_id, id, block_ticket, has_outgoing_ip, outgoing_ip_id FROM domain_ticket WHERE sasl_user LIKE '#%s#'";
		else 
			query = "SELECT total_used, current_day_usage, current_month_usage, valid_until, ticket_plan_id, id, block_ticket, has_outgoing_ip, outgoing_ip_id FROM domain_ticket WHERE sasl_user = '%s'";

		/* run query */
		result = valvulad_db_run_query (ctx,  query, valvula_get_sasl_user (request));
	} else if (domain_in_tickets) {
		/* prepare query to get the right domain */
		if (alternative_user) 
			query = "SELECT total_used, current_day_usage, current_month_usage, valid_until, ticket_plan_id, id, block_ticket, has_outgoing_ip, outgoing_ip_id FROM domain_ticket WHERE domain LIKE '#%s#'";
		else
			query = "SELECT total_used, current_day_usage, current_month_usage, valid_until, ticket_plan_id, id, block_ticket, has_outgoing_ip, outgoing_ip_id FROM domain_ticket WHERE domain = '%s'";

		/* run query */
		result = valvulad_db_run_query (ctx, query, valvula_get_sender_domain (request));
	} /* end if */

	if (! result) {
		/* unlock */
		valvula_mutex_unlock (&work_mutex);

		/* maybe the database configuration was removed before
		 * checking previous request, no problem */
		return VALVULA_STATE_DUNNO;
	} /* end if */

	/* get the values we are interesting in */
	row = __ticket_get_row_or_fail (ctx, result);
	if (row == NULL) {
		return VALVULA_STATE_DUNNO;
	}

	/* get if the limit is expired */
	valid_until = GET_CELL_AS_LONG (row, 3);
	if (valid_until != -1 && valvula_now () > valid_until) {
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
	block_ticket        = GET_CELL_AS_LONG (row, 6);
	has_outgoing_ip     = GET_CELL_AS_LONG (row, 7);
	outgoing_ip_id      = GET_CELL_AS_LONG (row, 8);

	if (block_ticket) {
		/* release result */
		valvulad_db_release_result (result);

		/* unlock */
		valvula_mutex_unlock (&work_mutex);

		valvulad_reject (ctx, request, "Rejecting operation because ticket (%d) is blocked(%d) for user (%s)",
				 ticket_plan_id, block_ticket, descriptive_user);
			
		return VALVULA_STATE_REJECT;
	} /* end if */

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

		return __mod_ticket_return_dunno_or_filter (ctx, descriptive_user, has_outgoing_ip, outgoing_ip_id, message);
	} /* end if */

	/* get row from result */
	row = __ticket_get_row_or_fail (ctx, result);
	if (row == NULL) {
		return __mod_ticket_return_dunno_or_filter (ctx, descriptive_user, has_outgoing_ip, outgoing_ip_id, message);
	}

	/* get values */
	total_limit = GET_CELL_AS_LONG (row, 0);
	day_limit   = GET_CELL_AS_LONG (row, 1);
	month_limit = GET_CELL_AS_LONG (row, 2);

	/* release result */
	valvulad_db_release_result (result);

	/* apply operations */
	count_update         = request->recipient_count == 0 ? 1 : request->recipient_count;
	total_used          += count_update;
	current_day_usage   += count_update;
	current_month_usage += count_update;

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
	return __mod_ticket_return_dunno_or_filter (ctx, descriptive_user, has_outgoing_ip, outgoing_ip_id, message);
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

/** 
 * \page valvulad_mod_ticket mod-ticket : A valvula module to control massive mail deliveries by tickets
 *
 * \section valvulad_mod_ticket_intro Introduction
 *
 * <b>mod-ticket</b> is a handy module that allows to implement
 * massive mail services controlled by tickets. This tickets acts like
 * quotas where user consume them every time a send operation is done until ticket limit is reached.
 *
 * At the same time, these tickets allows to define a time limit to be
 * consumed, day limit, month limit and global limits. 
 *
 * NOTE: this module is recommended to be connected to
 * smtpd_data_restrictions to avoid problems with test phase software
 * (which first connects to simulate and then does the final send
 * operation). It can also work in smtpd_end_of_data_restrictions
 * though it is not recommended because your server will have to
 * accept the entire message first to make a final decision.
 *
 * 
 * 
 * 
 */
