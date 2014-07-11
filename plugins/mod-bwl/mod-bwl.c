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

/* debug status */
axl_bool      __mod_bwl_enable_debug = axl_false;

/** 
 * @brief Init function, perform all the necessary code to register
 * profiles, configure Vortex, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  bwl_init (ValvuladCtx * _ctx)
{
	axlNode         * node;

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

	/* get debug status */
	node = axl_doc_get (_ctx->config, "/valvula/enviroment/mod-bwl");
	if (HAS_ATTR_VALUE (node, "debug", "yes"))
		__mod_bwl_enable_debug = axl_true;

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
	result = valvulad_db_run_query_s (ctx, query);
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

	if (__mod_bwl_enable_debug) {
		msg ("bwl (1): working with sender_domain=%s", sender_domain);
		msg ("bwl (1): working with sender=%s", sender);
		msg ("bwl (1): working with recipient_domain=%s", recipient_domain);
		msg ("bwl (1): working with recipient=%s", recipient);
	} /* end if */

	/* get current status at server level */
	state  = bwl_check_status (_ctx, request, VALVULA_MOD_BWL_SERVER, "global server lists", 
				   "SELECT status, source, destination FROM bwl_global WHERE is_active = '1' AND (source = '%s' OR source = '%s' OR destination = '%s' OR destination = '%s')",
				   sender_domain, sender, recipient_domain, recipient);
	/* check valvula state reported */
	if (state != VALVULA_STATE_DUNNO) 
		return state;

	/* check if recipient_domain is for a local delivery */
	if (valvulad_run_is_local_delivery (ctx, request)) {
		if (__mod_bwl_enable_debug) {
			msg ("bwl (2): working with sender_domain=%s", sender_domain);
			msg ("bwl (2): working with sender=%s", sender);
			msg ("bwl (2): working with recipient_domain=%s", recipient_domain);
			msg ("bwl (2): working with recipient=%s", recipient);
		} /* end if */

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
	"Valvulad Blacklists and whitelists module",
	bwl_init,
	bwl_close,
	bwl_process_request,
	NULL,
	NULL
};

END_C_DECLS



/** 
 * \page valvulad_mod_bwl mod-bwl : Valvula blacklisting module
 *
 * \section valvulad_mod_bwl Introduction
 *
 * mod-bwl is a handy module that allows implementing blacklisting
 * rules that are based on source and destination at the same time. As
 * opposed postfix which implements only source OR destination rules,
 * this allow implementing rules that accept (whitelist) or blocks
 * (blacklist) traffic for certain domains or even certain accounts.
 *
 * This way, domain administrators and end users can administrate
 * their own set of rules without affecting other domain and accounts.
 *
 * The module also uses valvula support to detect local users and
 * local domains to make better decisions while handling requests
 * received. These includes:
 *
 * - Avoiding a whitelist rule converting the mail server into an open relay. A whitelist can only work in the case it is configured with a local domain or local account as destination (that is whitelist to receive content). In the case the whitelist is for an external domain/account, that rule is skipped.
 *
 * - When you configure account or domain level rules they do not
 *     apply, no matter they are whitelist or blacklist, in the case
 *     the destination is not a local domain or address. This ensure that any user interface provided to the user account owner or
 *     domain administration to create rules will not led to have rules that may open
 *     the mail server or block/allow other's traffic in the server without their autorization.
 *
 * The module also support blocking SASL users. This allows to have a working account but temporally/permanently blocked.
 *
 * \section valvulad_mod_bwl_how_it_works How mod-bwl works
 *
 * The module install three tables to handle different levels of
 * blacklists and whitelists. They are applied in the following order
 * and each one takes precedence:
 *
 * - <b>bwl_global</b> : server level table that includes whitelists
 *     and blacklists. If there is a rule here that matches, the rest
 *     of the tables do not applies. 
 *
 *  <span style='text-decoration: underline'>This is how this table is checked: </span><br>
 *    1) <b>This is the first table</b> that is checked before checking next tables (bwl_domain and bwl_account). <br>
 *    2) While checking this table, first <b>specific rules</b> are checked first before <b>general rules</b>. <br>
 *    3) This means that a rule with a source and a destination defined (<b>specific rule</b>) is checked before a rule that only has source or destination defined (<b>general rules</b>). <br>
 *    <b>NOTE:</b> This allows, for example, to block all traffic from certain domain, let's say gmail.com, <b>with a general rule</b>, that only has has source defined, and then accept traffic from certain.user@gmail.com to certain.local.user@loca-domain.com <b>with a specific rule</b> because it has both source and destination defined.
 *
 * - <b>bwl_domain</b> : domain level table that includes whitelists and
 *     blacklists that applies to a particular domain. Because it
 *     applies to a domain, the rule must have as source or
 *     destination an account of that domain or the domain
 *     itself. Rules applied at this level can only accept traffic
 *     received (delivered to its domain).
 *
 *  <span style='text-decoration: underline'>This is how this table is checked: </span><br>
 *    1) This table is checked before checking next tables bwl_account. <br>
 *    2) While checking this table, first <b>specific rules</b> are checked first before <b>general rules</b>. <br>
 *  
 *
 * - <b>bwl_account</b> : account level table that includes whitelists and
 *     blacklists that applies to a particular account. Because it
 *     applies to an account, the rule must have as source or as
 *     destination the provided account. Rules applied at this level
 *     can only accept traffic received on the account (not outgoing
 *     traffic).
 *
 *  <span style='text-decoration: underline'>This is how this table is checked: </span><br>
 *    1) This is the last table checked before checking next tables bwl_account. <br>
 *    2) While checking this table, first <b>specific rules</b> are checked first before <b>general rules</b>. <br>
 *
 * \section valvulad_mod_bwl_how_rules_are_differenciated mod-bwl How rules are differenciated (whitelists and blacklists)
 *
 * Now, whitelists and blacklists are differenciated through the status field in every table (we will see examples later):
 *
 * - A whitelist rule includes a "ok" indication in the status field.
 *
 * - A Blacklist rule includes a "reject" or "discard" indication at the status field.
 *
 * \section valvulad_mod_bwl_how_to_block_sasl_user mod-bwl How to block SASL users
 *
 * To block an account, use the following SQL to update valvula database:
 *
 * \code
 * INSERT INTO bwl_global_sasl (is_active, sasl_user) VALUES ('1', 'certain.user@domain.com');
 * \endcode
 *
 * \section valvulad_mod_bwl_how_rules mod-bwl Rules examples
 *
 * To block a certain user from receiving any traffic (outgoing) globally run use the following SQL:
 * \code
 * -- Block  * -> certain.user@domain.com
 * INSERT INTO bwl_global (is_active, destination, status) VALUES ('1', 'certain.user@domain.com', 'reject')
 * \endcode
 *
 * To block a certain user from receiving traffic from a particular user globally run use the following SQL:
 * \code
 * -- Block  anotheruser@anotherdomanin.com -> certain.user@domain.com
 * INSERT INTO bwl_global (is_active, source, destination, status) VALUES ('1', 'anotheruser@anotherdomain.com', 'certain.user@domain.com', 'reject')
 * \endcode
 *
 *
 * 
 * 
 * 
 *
 * 
 */
