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
axl_bool      __mod_bwl_enable_debug = axl_false;
/* by default support to deny-unknown-mail-local-from is enabled */
axl_bool      __mod_bwl_enable_deny_unknown_local_mail_from = axl_true;

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
				  /* stamp: when this rule was added (not used by valvula but useful for management) */
				  "stamp", "int",
				  /* who: who added this rule (not used by valvula but useful for management) */
				  "who", "varchar(1024)",
				  /* sasl_users_to_skip : list of sasl_users that this rule will not be applied to. Leave this empty to make this rule apply to all users, authenticated or not.*/
				  "sasl_users_to_skip", "text",
				  /* limit_rule_to_sasl_users : list of sasl_users that will apply this rule and only this rule. Leave this empty to make this rule apply all users, authenticated or not*/
				  "limit_rule_to_sasl_users", "text",
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
				  /* stamp: when this rule was added (not used by valvula but useful for management) */
				  "stamp", "int",
				  /* who: who added this rule (not used by valvula but useful for management) */
				  "who", "varchar(1024)",
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
				  /* stamp: when this rule was added (not used by valvula but useful for management) */
				  "stamp", "int",
				  /* who: who added this rule (not used by valvula but useful for management) */
				  "who", "varchar(1024)",
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
				  /* stamp: when this rule was added (not used by valvula but useful for management) */
				  "stamp", "int",
				  /* who: who added this rule (not used by valvula but useful for management) */
				  "who", "varchar(1024)",
				  NULL);

	/* get debug status */
	node = axl_doc_get (_ctx->config, "/valvula/enviroment/mod-bwl");
	if (HAS_ATTR_VALUE (node, "debug", "yes"))
		__mod_bwl_enable_debug = axl_true;
	
	/* support to deny unknown mail froms that has destinations
	   for valid domain accounts:
	   DUNNO: VoiceMessage@valid.domain.com -> info@valid.domain.com
	*/
	if (HAS_ATTR_VALUE (node, "disable-deny-unknown-local-mail-from", "yes"))
		__mod_bwl_enable_deny_unknown_local_mail_from = axl_false;

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

axl_bool bwl_list_has_users (const char * limit_rule_to_sasl_users) {

	char ** items;
	int     iterator;
	int     count;


	/* avoid checking if parameter is not defined or has no
	   content */
	if (! limit_rule_to_sasl_users)
		return axl_false;
	if (strlen (limit_rule_to_sasl_users) <= 0)
		return axl_false;

	/* split items from list */
	items = axl_split (limit_rule_to_sasl_users, 5, ",", " ", ";", "\\n", "\\t");
	if (items == NULL)
		return axl_false;

	/* iterate over all items from string */
	iterator = 0;
	count    = 0;
	while (items[iterator] != 0) {
		
		if (items[iterator] && strlen (items[iterator]) > 0) {
			axl_stream_trim (items[iterator]);
			if (strlen (items[iterator]) > 0) {
				count++;
			} /* end if */
		} /* end if */
		iterator++;
	}
	axl_freev (items);
	
	/* return if we have found elements in the list */
	return count > 0;
}

axl_bool bwl_sasl_user_request_in_list (const char * sasl_user, const char * sasl_user_list) {
	char       ** items;
	int           iterator;
	axl_bool      found;
	
	if (! sasl_user) {
		/* found that this request has no sasl user, so we
		   directly return axl_false because it cannot not be
		   in the list */
		return axl_false;
	} /* end if */

	if (! sasl_user_list)
		return axl_false;
	if (strlen (sasl_user_list) <= 0)
		return axl_false;

	items = axl_split (sasl_user_list, 5, ",", " ", ";", "\\n", "\\t");
	if (items == NULL)
		return axl_false;
	
	iterator = 0;
	found    = axl_false;
	while (items[iterator] != 0) {
		/* check if position has content */
		if (items[iterator] && strlen (items[iterator]) > 0) {
			/* cleanup string */
			axl_stream_trim (items[iterator]);
			/* check position with sasl user */
			if (axl_casecmp (items[iterator], sasl_user)) {
				found = axl_true;
				break;
			} /* end if */
		} /* end if */

		/* next iterator position */
		iterator++;
	}
	axl_freev (items);
	
	return found;
}

ValvulaState bwl_check_status_rules (ValvulaCtx          * _ctx,
				     ValvulaRequest      * request,
				     ValvulaModBwlLevel    level,
				     const char          * level_label,
				     char               ** message,
				     ValvuladRes           result,
				     axl_bool              first_specific)
{

	ValvuladRow     row;
	const char    * status;
	const char    * source;
	const char    * destination;
	const char    * sasl_users_to_skip = NULL;
	const char    * limit_rule_to_sasl_users = NULL;

	/* get sasl user from request received */
	const char    * sasl_user = valvula_get_sasl_user (request);

	/* reset cursor */
	valvulad_db_first_row (ctx, result);

	/* get row and then status */
	row            = GET_ROW (result);
	while (row) {

		/* get status */
		status      = GET_CELL (row, 0);
		source      = GET_CELL (row, 1);
		destination = GET_CELL (row, 2);

		/* sasl users application or limitation */
		sasl_users_to_skip = GET_CELL (row, 3);
		if (sasl_users_to_skip && bwl_sasl_user_request_in_list (sasl_user, sasl_users_to_skip)) {
			/* found user to skip */
			row = GET_ROW (result);
			continue;
		} /* end if */
		limit_rule_to_sasl_users = GET_CELL (row, 4);
		if (limit_rule_to_sasl_users && bwl_list_has_users (limit_rule_to_sasl_users) && ! bwl_sasl_user_request_in_list (sasl_user, limit_rule_to_sasl_users)) {
			/* found rule defined for a list of users, but user does not match */
			row = GET_ROW (result);
			continue;
		}

		/* skip general rules first, look for specific rules where all attributes are defined */
		if (first_specific && (!source || strlen (source) == 0 || !destination || strlen (destination) == 0)) {
			/* get next row */
			row = GET_ROW (result);
			continue;
		} /* end if */

		/* ensure source matches */
		if (! valvula_address_rule_match (ctx->ctx, source, request->sender)) {
			if (__mod_bwl_enable_debug) 
				wrn ("BWL: rule does not match(1) !valvula_address_rule_match(source=%s, request->sender=%s) :: status=%s, source=%s, sender=%s, destination=%s",
				     source, request->sender,
				     status, source, request->sender, destination);
			/* get next row */
			row = GET_ROW (result);
			continue;
		} /* end if */

		if (! valvula_address_rule_match (ctx->ctx, destination, request->recipient)) {
			if (__mod_bwl_enable_debug) 
				wrn ("BWL: rule does not match(2) !valvula_address_rule_match(destination=%s, request->recipient=%s) :: status=%s, source=%s, sender=%s, destination=%s",
				     destination, request->recipient,
				     status, source, request->sender, destination);
			/* get next row */
			row = GET_ROW (result);
			continue;
		} /* end if */
			

		/* msg ("BWL: checking status=%s, source=%s, destination=%s", status, source, destination);
		   msg ("BWL:        with request source=%s, destination=%s", request->sender, request->recipient); */
		
		/* 
		   NOTES about the following check:

		   now check values: the following checks if an OK
		   rule will create us Open Relay problems, what we
		   check here is:
		   
		   1) Is a OK rule (which accepts everything and skips
		   every possible check that postfix will do
		   later). That is, it can create an OpenRelay
		   situation.

		   2) The operation is not authenticated because if it
		   is, indeed we have to accept it because we have
		   already authenticated this user so there is no
		   point in blocking it: he/she already has the power
		   to send to anyone without restriction.

		*/
		
		if (axl_stream_casecmp (status, "ok", 2)) {
			/* accept it if the sender or reception domain
			 * is local or operation is authenticated */
			if (valvulad_run_is_local_delivery (ctx, request) || valvula_get_sasl_user (request)) {
				/* so, reached this point we have that
				 * the rule (whitelist) was added and
				 * it matches with a local delivery */
				
				/* ONLY ACCEPT OK: for rules that are
				   directed to local delivery:
				   otherwise, open-relay will be
				   allowed */
				return VALVULA_STATE_OK;
			} else {
				/* if (__mod_bwl_enable_debug) { */
				/* do not make the following warning to be avoided if 
				   debug is enabled because it confuses people: it is 
				   better to drop some log when a rule is discarded to 
				   help people track/trace the problem */
				wrn ("Skipping rule because it is not a local delivery and it is not SASL authenticated, rule: [status=%s, source=%s, destination=%s], request: [source=%s, destination=%s]",
				     status, source, destination,
				     request->sender, request->recipient); 
				/* } */ /* end if */
			} /* end if */
		}
		
		if (axl_stream_casecmp (status, "reject", 6)) {
			valvulad_reject (ctx, VALVULA_STATE_REJECT, request, "Rejecting due to blacklist (%s)", level_label);
			return VALVULA_STATE_REJECT;
			
		} /* end if */
		if (axl_stream_casecmp (status, "discard", 7)) {
			valvulad_reject (ctx, VALVULA_STATE_DISCARD, request, "Discard due to blacklist (%s)", level_label); 
			return VALVULA_STATE_DISCARD;
			
		} /* end if */

		if (axl_stream_casecmp (status, "filter-discard", 14)) {
			/* do a discard but instaed or returning DISCARD, use a filter to the transport discard: */
			(*message) = axl_strdup ("discard:");
			valvulad_reject (ctx, VALVULA_STATE_FILTER, request, "Discard (using filter-discard) due to blacklist (%s)", level_label); 
			return VALVULA_STATE_FILTER;
			
		} /* end if */

		/* get next row */
		row = GET_ROW (result);

	} /* end while */

	/* report dunno state */
	return VALVULA_STATE_DUNNO;
}

ValvulaState bwl_check_status (ValvulaCtx          * _ctx,
			       ValvulaRequest      * request,
			       ValvulaModBwlLevel    level,
			       const char          * level_label,
			       char               ** message,
			       const char          * format,
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

	if (__mod_bwl_enable_debug) 
		msg ("(bwl) Running query: %s", query);

	/* call to create the query */
	result = valvulad_db_run_query_s (ctx, query);
	axl_free (query);
	if (! result) {
		/* release result */
		valvulad_db_release_result (result);

		/* maybe the database configuration was removed before checking previous request, no problem */
		return VALVULA_STATE_DUNNO;
	} /* end if */

	if (__mod_bwl_enable_debug) 
		msg ("(bwl) Checking request from %s -> %s", request->sender, request->recipient); 

	/* first check specific rules */
	state = bwl_check_status_rules (_ctx, request, level, level_label, message, result, /* specific */ axl_true);
	if (__mod_bwl_enable_debug)
		msg ("(bwl) bwl_check_setatus_rules [source=%s, destination=%s, sasl-username=%s], reported status=%s  (specific rules)",
		     request->sender, request->recipient, request->sasl_username ? request->sasl_username : "", valvula_support_state_str (state));
	if (state != VALVULA_STATE_DUNNO) {
		/* release result */
		valvulad_db_release_result (result);
		return state;
	} /* end if */

	/* now check rest of rules */
	state = bwl_check_status_rules (_ctx, request, level, level_label, message, result, /* generic */ axl_false);
	if (__mod_bwl_enable_debug)
		msg ("(bwl) bwl_check_setatus_rules [source=%s, destination=%s, sasl-username=%s], reported status=%s  (generic rules)",
		     request->sender, request->recipient, request->sasl_username ? request->sasl_username : "", valvula_support_state_str (state));
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
		valvulad_reject (ctx, VALVULA_STATE_REJECT, request, "Rejecting sasl user (%s) due to administrative configuration (mod-bwl)", request->sasl_username);
		return axl_true; /* report rejected */
	} /* end if */

	return axl_false; /* not rejected */
}

ValvulaState bwl_process_check_for_deny_unknown_local_mail_from (ValvulaCtx        * _ctx, 
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
	/* This function attempts to reject unknown accounts that are
	   attempting to send content to valid local domains but using
	   as mail from unknown accounts, which is a sign of spam and
	   forgery. 
	   
	   The classical example is:
	   
	   mail from: unknown-mail-account@aspl.es
	   rcpt to: info@aspl.es

	   In this case, info@aspl.es is valid and local. We want to
	   reject unknown-mail-account@aspl.es because it is known to
	   be an account that is not valid.
	   
	   If the user wants to receive this message, then two
	   options are available:
	   
	   1) Make mail from: to be a valid account
	   2) Add a rule into bwl to accept this content.
	*/

	/* authenticated operations (valid customers and users) do not apply for deny-unknown-local-mail-from */
	if (valvula_is_authenticated (request))
		return VALVULA_STATE_DUNNO;

	/* check sender domain to be valid */
	if (! valvulad_run_is_local_domain (ctx, sender_domain)) {
		/* sender domain is not local, so this check cannot be
		   applied */
		return VALVULA_STATE_DUNNO;
	} /* end if */

	/* reached this point, domain is a local valid domain, then
	   check remote sender to be valid too */
	if (valvulad_run_is_local_address (ctx, sender)) {

		if (__mod_bwl_enable_debug) 
			msg ("bwl :: deny-unknown-local-mail-from :: check OK %s -> %s", sender, recipient);
		
		/* found mail-from value which is a valid
		   local-address */ 
		return VALVULA_STATE_DUNNO;
	} /* end if */

	/* reject :: deny-unknown-local-mail-from :: found */
	valvulad_reject (ctx, VALVULA_STATE_REJECT, request, "Rejecting remote mail-from (%s) because it is using an unknown local-address for local domain %s to deliver to %s (deny-unknown-local-mail-from)",
			 sender, sender_domain, recipient);
	return VALVULA_STATE_REJECT;
}


/** 
 * @brief Process request for the module.
 */
ValvulaState bwl_process_request_aux (ValvulaCtx        * _ctx, 
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
	ValvulaState    state;
	axl_bool        is_local;

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
	state  = bwl_check_status (_ctx, request, VALVULA_MOD_BWL_SERVER, "global server lists", message,
				   "SELECT status, source, destination, sasl_users_to_skip, sasl_users_to_skip FROM bwl_global WHERE is_active = '1' AND (source = '%s' OR source = '%s' OR source = '%s@' OR source = '%s' OR destination = '%s' OR destination = '%s' OR destination = '%s@' OR destination = '%s')",
				   sender_domain, sender, sender_local_part, valvula_get_tld_extension (sender_domain),
				   recipient_domain, recipient, recipient_local_part, valvula_get_tld_extension (recipient_domain));
	
	/* check valvula state reported */
	if (state != VALVULA_STATE_DUNNO) {
	        if (__mod_bwl_enable_debug) 
		       msg ("bwl : finishing %s -> %s reporting %s (state: %d) ", sender, recipient, valvula_support_state_str  (state), state);
		return state;
	} /* end if */

	/* call to check if it is local delivery */
	is_local = valvulad_run_is_local_delivery (ctx, request);
	if (__mod_bwl_enable_debug) {
	        msg ("bwl : checking local-delivery %s -> %s reporting (valvulad_run_is_local_delivery (%p, %p) = %d : %s", 
		     sender, recipient, ctx, request, is_local, is_local ? "is-local" : "non-local");
	} /* end if */

	/* check if recipient_domain is for a local delivery */
	if (is_local) {
		if (__mod_bwl_enable_debug) {
			msg ("bwl (2): working with sender_domain=%s", sender_domain);
			msg ("bwl (2): working with sender=%s", sender);
			msg ("bwl (2): working with recipient_domain=%s", recipient_domain);
			msg ("bwl (2): working with recipient=%s", recipient);
		} /* end if */

		/* get current status at domain level: rules that applies to recipient 
		   domain and has to do with source account or source domain */
		state  = bwl_check_status (_ctx, request, VALVULA_MOD_BWL_DOMAIN, "domain lists", message,
					   "SELECT status, source, '%s' as destination, NULL as sasl_users_to_skip, NULL as sasl_users_to_skip FROM bwl_domain WHERE is_active = '1' AND rules_for = '%s' AND (source = '%s' OR source = '%s' OR source = '%s')",
					   recipient, recipient_domain, sender, sender_domain, valvula_get_tld_extension (sender_domain));
		/* check valvula state reported */
		if (state != VALVULA_STATE_DUNNO) 
			return state;

		/* get current status at domain level: rules that applies to recipient 
		   domain and has to do with source account or source domain */
		state  = bwl_check_status (_ctx, request, VALVULA_MOD_BWL_ACCOUNT, "account lists", message,
					   "SELECT status, source, '%s' as destination, NULL as sasl_users_to_skip, NULL as sasl_users_to_skip FROM bwl_account WHERE is_active = '1' AND rules_for = '%s' AND (source = '%s' OR source = '%s' OR source = '%s')",
					   recipient, recipient, sender, sender_domain, valvula_get_tld_extension (sender_domain));
		/* check valvula state reported */
		if (state != VALVULA_STATE_DUNNO) 
			return state;

		/* NOTE :: THIS FUNCTION :: must be called only when is_local=axl_true :: check for deny-unknown-local-mail-from */
		state = bwl_process_check_for_deny_unknown_local_mail_from (_ctx, request, request_data, message, sender_domain, sender_local_part, recipient_domain, recipient_local_part, sender, recipient);
		if (state != VALVULA_STATE_DUNNO)
			return state;
		
	} /* end if */

	/* by default report return dunno */
	return VALVULA_STATE_DUNNO;
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
	state = bwl_process_request_aux (_ctx, connection, request, request_data, message, sender_domain, sender_local_part, recipient_domain, recipient_local_part, sender, recipient);

	axl_free (recipient_local_part);
	axl_free (sender_local_part);

	return state;
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
 * \section valvulad_mod_bwl_index mod-bwl Index
 *
 * - \ref valvulad_mod_bwl_intro
 * - \ref valvulad_mod_bwl_how_it_works
 * - \ref valvulad_mod_bwl_how_rules_are_differenciated
 * - \ref valvulad_mod_bwl_how_to_block_sasl_user
 * - \ref valvulad_mod_bwl_how_rules
 * - \ref valvulad_mod_bwl_deny_unknown_local_mail_from
 *
 * \section valvulad_mod_bwl_intro mod-bwl Introduction
 *
 * mod-bwl is a handy module that allows implementing blacklisting
 * rules that are based on source and destination at the same time. As
 * opposed postfix which implements only source OR destination rules.
 * This allow implementing rules that accept (whitelist) or blocks
 * (blacklist) traffic for certain domains or even certain accounts.
 *
 * At the same time, mod-bwl implement different blocking/whitelist
 * levels (global, domain and account). This way, domain
 * administrators and end users can administrate their own set of
 * rules without affecting other domain and accounts. This allows:
 *
 *  - <b>System administrators:</b> to globally block or whitelist accounts
 *
 *  - <b>Domain administrators:</b> to accept mail traffic from detain domains without causing this rule to accept that traffic for another domain.
 *
 *  - <b>End user:</b> to accept traffic from especific accounts or domains that is directed to a particular end user account.
 *
 * The module also uses valvula support to detect local users and
 * local domains to make better decisions while handling requests
 * received. These includes:
 *
 * - Avoiding a whitelist rule converting the mail server into an open relay. A whitelist can only work in the case it is configured with a local domain or local account as destination (that is, whitelist to receive content). In the case the whitelist is for an external domain/account, that rule is skipped.
 *
 * - When you configure account or domain level rules they do not
 *     apply, no matter what they are white or blacklist, in the case
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
 *  <b>NOTE:</b> This table should be only used     and accesible by machine system adminstrators.
 *
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
 *    <b>NOTE:</b> This table should be used or accesible by domain administrators. 
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
 *    <b>NOTE:</b> This table should be used or accesible by end users / account owners.
 *
 *  <span style='text-decoration: underline'>This is how this table is checked: </span><br>
 *    1) This is the last table checked before checking next tables bwl_account. <br>
 *    2) While checking this table, first <b>specific rules</b> are checked first before <b>general rules</b>. <br>
 *
 * If no rule "reject"s or "discard"s the message, the request is let
 * to continue to the next module configure (by reporting internally
 * DUNNO).
 *
 * \section valvulad_mod_bwl_how_rules_are_differenciated mod-bwl How rules are differenciated (whitelists and blacklists)
 *
 * Now, whitelists and blacklists are differenciated through the status field in every table (we will see examples later):
 *
 * - A whitelist rule includes a "ok" indication in the status
 *     field. Note that reporting "ok" means a terminal indication and
 *     will make postifx to accept it right away without checking rest
 *     of the modules (and postfix restrictions).
 *
 * - A Blacklist rule includes a "reject" or "discard" indication at
 *     the status field. This is a terminal indication that will make
 *     postfix to finally discard or reject the message without
 *     checking the rest modules (and postfix restrictions).
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
 *
 * \code
 * -- Block  anotheruser@anotherdomanin.com -> certain.user@domain.com
 * INSERT INTO bwl_global (is_active, source, destination, status) VALUES ('1', 'anotheruser@anotherdomain.com', 'certain.user@domain.com', 'reject')
 * \endcode
 *
 * To block globally generic accounts webmaster@  without considering destination domain use:
 *
 * \code
 * -- Block  * -> webmaster@*
 * INSERT INTO bwl_global (is_active, destination, status) VALUES ('1', 'webmaster@', 'reject')
 * \endcode
 *
 * You can also block globally, or domain or account level generic top level domains like:
 *
 * \code
 * -- Block all .top domains  *.top -> *
 * -- Block all .top domains  *.us -> *
 * INSERT INTO bwl_global (is_active, source, status) VALUES ('1', 'top', 'reject')
 * INSERT INTO bwl_global (is_active, source, status) VALUES ('1', 'us', 'reject')
 * \endcode
 *
 *
 * \section valvulad_mod_bwl_deny_unknown_local_mail_from mod-bwl Support to deny unknown accounts attempting to deliver to known local accounts (deny-unknown-local-mail-from)
 *
 *
 * <b>mod-bwl</b> includes by default enabled, a protection to deny
 * spoofed mail from accounts for valid local domains, targeting valid
 * local addresses.
 *
 * A typical example are forged accounts like:
 *
 * \code
 * Sep  6 04:41:19 mailserver02 valvulad[21581]: info: DUNNO: VoiceMessage@asplhosting.com -> info@asplhosting.com (sasl_user=), port 3579, rcpt count=0, queue-id , from 11X.Y.Z.W1, no-tls
 * \endcode
 * 
 * In this case, <b>info@asplhosting.com</b> exists, but <b>VoiceMessage@asplhosting.com</b> doesn't. 
 *
 * For such situations, if you enable <b>mod-bwl</b>, it will activate
 * by default <b>deny-unknown-local-mail-from</b> protection,
 * rejecting this account.
 *
 * Of course, you still have choice to configure an exception using
 * regular bwl rules (with this module) or make remote software to use
 * a valid mail account as mail-from, or to create such account.
 *
 * Protection provided by <b>deny-unknown-local-mail-from</b> do no
 * apply to authenticated users. In such situations you must use
 * <b>mod-bwl</b> rules, or <b>mod-slm</b> to track and control send
 * operations.
 * 
 * 
 *
 * 
 */
