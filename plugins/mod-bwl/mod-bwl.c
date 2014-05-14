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
				  /* source domain or account to apply restriction. If defined
				   * applies. If not defined, applies to all sources. When source 
				   is a local destination, it must be autenticated */
				  "source", "varchar(1024)",
				  /* source domain or account to apply restriction. If defined
				   * applies. If not defined, applies to all local destinations. */
				  "destination", "varchar(1024)",
				  /* rule description */
				  "description", "varchar(500)",
				  /* status: reject, discard, ok */
				  "status", "varchar(32)",
				  NULL);

	/* create databases to be used by the module */
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "bwl_domain", 
				  /* attributes */
				  "id", "autoincrement int", 
				  /* rule status */
				  "is_active", "int",
				  /* rules that applies to this domain. */
				  "rules_for", "varchar(1024)",
				  /* source domain or account to apply restriction. If defined
				   * applies. If not defined, applies to all sources */
				  "source", "varchar(1024)",
				  "description", "varchar(500)",
				  /* status: reject, discard, ok */
				  "status", "varchar(32)",
				  NULL);

	/* create databases to be used by the module */
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "bwl_account", 
				  /* attributes */
				  "id", "autoincrement int", 
				  /* rule status */
				  "is_active", "int",
				  /* rules that applies to this domain. */
				  "rules_for", "varchar(1024)",
				  /* source domain or account to apply restriction. If defined
				   * applies. If not defined, applies to all sources */
				  "source", "varchar(1024)",
				  "description", "varchar(500)",
				  /* status: reject, discard, ok */
				  "status", "varchar(32)",
				  NULL);

	/** 
	 * bwl_global_sasl table allows to implement sasl user
	 * blocking. That is, even having a valid sasl user account,
	 * this table allows to blocking at server level.
	 */
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "bwl_global_sasl", 
				  /* attributes */
				  "id", "autoincrement int", 
				  /* rule status */
				  "is_active", "int",
				  /* source domain or account to apply restriction */
				  "sasl_user", "varchar(1024)",
				  "description", "varchar(500)",
				  NULL);

	return axl_true;
}

typedef enum {
	/**
	 * @internal Rules applied at server level.
	 */
	VALVULA_MOD_BWL_SERVER  = 1,
	/** 
	 * @internal Rules applied at domain level.
	 */
	VALVULA_MOD_BWL_DOMAIN  = 2,
	/** 
	 * @internal Rules applied at account level.
	 */
	VALVULA_MOD_BWL_ACCOUNT = 3,
} ValvulaModBwlLevel;

ValvulaState bwl_check_status_rules (ValvulaCtx         * _ctx,
				     ValvulaRequest     * request,
				     ValvulaModBwlLevel   level,
				     const char         * level_label,
				     ValvuladRes          result,
				     axl_bool             first_specific)
{

	ValvuladRow     row;
	const char    * status;
	const char    * source;
	const char    * destination;

	/* reset cursor */
	valvulad_db_first_row (ctx, result);

	/* get row and then status */
	row            = GET_ROW (result);
	while (row) {

		/* get status */
		status      = GET_CELL (row, 0);
		source      = GET_CELL (row, 1);
		destination = GET_CELL (row, 2);

		/* skip general rules first, look for specific rules where all attributes are defined */
		if (first_specific && (!source || strlen (source) == 0 || !destination || strlen (destination) == 0)) {
			/* get next row */
			row = GET_ROW (result);
			continue;
		} /* end if */

		/* ensure source matches */
		if (! valvula_address_rule_match (ctx->ctx, source, request->sender)) {
			wrn ("BWL: rule does not match(1) status=%s, source=%s, destination=%s", status, source, destination);
			/* get next row */
			row = GET_ROW (result);
			continue;
		} /* end if */

		if (! valvula_address_rule_match (ctx->ctx, destination, request->recipient)) {
			wrn ("BWL: rule does not match(2) status=%s, source=%s, destination=%s", status, source, destination);
			/* get next row */
			row = GET_ROW (result);
			continue;
		} /* end if */
			

		/* msg ("BWL: checking status=%s, source=%s, destination=%s", status, source, destination);
		   msg ("BWL:        with request source=%s, destination=%s", request->sender, request->recipient); */
		
		/* now check values */
		if (axl_stream_casecmp (status, "ok", 2)) {
			/* accept it if the sender or reception domain is local */
			if (valvulad_run_is_local_delivery (ctx, request)) {
				/* so, reached this point we have that
				 * the rule (whitelist) was added and
				 * it matches with a local delivery */
				return VALVULA_STATE_OK;
			} else {
				wrn ("Skipping rule because it is not a local delivery..");
			} /* end if */
		}
		
		if (axl_stream_casecmp (status, "reject", 6)) {
			valvulad_reject (ctx, request, "Rejecting due to blacklist (%s)", level_label);
			return VALVULA_STATE_REJECT;
			
		} /* end if */
		if (axl_stream_casecmp (status, "discard", 7)) {
			valvulad_reject (ctx, request, "Discard due to blacklist (%s)", level_label);
			return VALVULA_STATE_DISCARD;
			
		} /* end if */

		/* get next row */
		row = GET_ROW (result);

	} /* end while */

	/* report dunno state */
	return VALVULA_STATE_DUNNO;
}

ValvulaState bwl_check_status (ValvulaCtx         * _ctx,
			       ValvulaRequest     * request,
			       ValvulaModBwlLevel   level,
			       const char         * level_label,
			       const char         * format,
			       ...)
{
	/* get result and row */
	ValvuladRes     result  = NULL;
	char          * query;
	va_list         args;

	/* state with default value */
	ValvulaState    state;

	/* open arguments */
	va_start (args, format);

	query = axl_strdup_printfv (format, args);

	va_end (args);

	if (query == NULL) {
		error ("Unable to allocate query to check black/white lists, reporting dunno");
		return VALVULA_STATE_DUNNO;
	} /* end if */

	/* call to create the query */
	result = valvulad_db_run_query (ctx, query);
	axl_free (query);
	if (! result) {
		/* release result */
		valvulad_db_release_result (result);

		/* maybe the database configuration was removed before checking previous request, no problem */
		return VALVULA_STATE_DUNNO;
	} /* end if */

	/* msg ("Checking request from %s -> %s", request->sender, request->recipient); */

	/* first check specific rules */
	state = bwl_check_status_rules (_ctx, request, level, level_label, result, /* specific */ axl_true);
	if (state != VALVULA_STATE_DUNNO) {
		/* release result */
		valvulad_db_release_result (result);
		return state;
	} /* end if */

	/* now check rest of rules */
	state = bwl_check_status_rules (_ctx, request, level, level_label, result, /* generic */ axl_false);
	if (state != VALVULA_STATE_DUNNO) {
		/* release result */
		valvulad_db_release_result (result);
		return state;
	} /* end if */

	/* release result */
	valvulad_db_release_result (result);

	/* report status */
	return state;
}

axl_bool bwl_is_sasl_user_blocked (ValvuladCtx * ctx, ValvulaRequest * request)
{
	/* request is authenticated, check exceptions */
	if (valvulad_db_boolean_query (ctx, "SELECT sasl_user FROM bwl_global_sasl WHERE sasl_user = '%s'", request->sasl_username)) {
		valvulad_reject (ctx, request, "Rejecting sasl user (%s) due to administrative configuration (mod-bwl)", request->sasl_username);
		return axl_true; /* report rejected */
	} /* end if */

	return axl_false; /* not rejected */
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
	const char    * sender_domain    = valvula_get_sender_domain (request);
	const char    * recipient_domain = valvula_get_recipient_domain (request);
	const char    * sender           = request->sender;
	const char    * recipient        = request->recipient;
	ValvulaState    state;

	/* check if sasl user is blocked */
	if (valvula_is_authenticated (request) && bwl_is_sasl_user_blocked (ctx, request))
		return VALVULA_STATE_REJECT;

	/* get current status at server level */
	state  = bwl_check_status (_ctx, request, VALVULA_MOD_BWL_SERVER, "global server lists", "SELECT status, source, destination FROM bwl_global WHERE is_active = '1' AND (source = '%s' OR source = '%s' OR destination = '%s' OR destination = '%s')",
				   sender_domain, sender, recipient_domain, recipient);
	/* check valvula state reported */
	if (state != VALVULA_STATE_DUNNO) 
		return state;

	/* check if recipient_domain is for a local delivery */
	if (valvulad_run_is_local_delivery (ctx, request)) {
		/* get current status at domain level: rules that applies to recipient 
		   domain and has to do with source account or source domain */
		state  = bwl_check_status (_ctx, request, VALVULA_MOD_BWL_DOMAIN, "domain lists", 
					   "SELECT status, source, '%s' as destination FROM bwl_domain WHERE is_active = '1' AND rules_for = '%s' AND (source = '%s' OR source = '%s')",
					   recipient, recipient_domain, sender, sender_domain);
		/* check valvula state reported */
		if (state != VALVULA_STATE_DUNNO) 
			return state;

		/* get current status at domain level: rules that applies to recipient 
		   domain and has to do with source account or source domain */
		state  = bwl_check_status (_ctx, request, VALVULA_MOD_BWL_ACCOUNT, "account lists", 
					   "SELECT status, source, '%s' as destination FROM bwl_account WHERE is_active = '1' AND rules_for = '%s' AND (source = '%s' OR source = '%s')",
					   recipient, recipient, sender, sender_domain);
		/* check valvula state reported */
		if (state != VALVULA_STATE_DUNNO) 
			return state;
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



/** 
 * \section how_bwl_module_works How mod-bwl works
 *
 * The module install three tables to handle different levels of black
 * and white lists. They are applied in the following order and each
 * one takes precedence:
 *
 * - bwl_global : server level table that includes white lists and
 *     black lists. If there is a rule here that matches, the rest of
 *     the tables do not applies.
 *
 * - bwl_domain : domain level table that includes white lists and
 *     black lists that applies to a particular domain. Because it
 *     applies to a domain, the rule must have as source or
 *     destination an account of that domain or the domain
 *     itself. Rules applied at this level can only accept traffic
 *     received (delivered to its domain).
 *
 * - bwl_account : account level table that includes white lists and
 *     black lists that applies to a particular account. Because it
 *     applies to an account, the rule must have as source or as
 *     destination the provided account. Rules applied at this level
 *     can only accept traffic received on the account (not outgoing
 *     traffic).
 *
 * Now, white lists and black lists are differenciated through the status field:
 *
 * - A white list rule includes a "ok" indication in the status field.
 *
 * - A Black list rule includes a "reject" or "discard" indication at the status field.
 *
 * Now, there is a difference on how an white list or a black list
 * rule is applied. 
 *
 * - For a black list, there is no especial operation. The rule is
 *   applied at the provided level in the given order (server, then
 *   domain, then account table).
 * 
 * - For a white list it changes. When adding a whitelist rule, if it
 *   the destination is defined it must be an account or domain that
 *   is handled by the server (local domains). Rules that tries to
 *   accept traffic for domains that aren't handled by this server are
 *   ignored (because otherwise it will create an open relay rule).
 * 
 * 
 *
 * 
 */
