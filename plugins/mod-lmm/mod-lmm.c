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
				  "lmm_global", 
				  /* attributes */
				  "id", "autoincrement int", 
				  /* rule status */
				  "is_active", "int",
				  /* source domain or account to apply restriction. If defined
				   * applies. If not defined, applies to all sources. When source 
				   is a local destination, it must be autenticated */
				  "domain", "varchar(1024)",
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
	const char    * sender_domain        = valvula_get_sender_domain (request);
	char          * sender_local_part    = valvula_get_sender_local_part (request);

	/* recipient and local part */
	const char    * recipient_domain     = valvula_get_recipient_domain (request);
	char          * recipient_local_part = valvula_get_recipient_local_part (request);

	const char    * sender           = request->sender;
	const char    * recipient        = request->recipient;
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
	"Valvulad plugin to limit mail from usage/forge from unauthorized sources",
	lmm_init,
	lmm_close,
	lmm_process_request,
	NULL,
	NULL
};

END_C_DECLS



/** 
 * \page valvulad_mod_lmm mod-lmm : Valvula blacklisting module
 *
 * \section valvulad_mod_lmm_intro Introduction
 *
 * mod-lmm is a handy module that allows implementing blacklisting
 * rules that are based on source and destination at the same time. As
 * opposed postfix which implements only source OR destination rules.
 * This allow implementing rules that accept (whitelist) or blocks
 * (blacklist) traffic for certain domains or even certain accounts.
 *
 * At the same time, mod-lmm implement different blocking/whitelist
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
 * \section valvulad_mod_lmm_how_it_works How mod-lmm works
 *
 * The module install three tables to handle different levels of
 * blacklists and whitelists. They are applied in the following order
 * and each one takes precedence:
 *
 * - <b>lmm_global</b> : server level table that includes whitelists
 *     and blacklists. If there is a rule here that matches, the rest
 *     of the tables do not applies. 
 *
 *  <b>NOTE:</b> This table should be only used     and accesible by machine system adminstrators.
 *
 *
 *  <span style='text-decoration: underline'>This is how this table is checked: </span><br>
 *    1) <b>This is the first table</b> that is checked before checking next tables (lmm_domain and lmm_account). <br>
 *    2) While checking this table, first <b>specific rules</b> are checked first before <b>general rules</b>. <br>
 *    3) This means that a rule with a source and a destination defined (<b>specific rule</b>) is checked before a rule that only has source or destination defined (<b>general rules</b>). <br>
 *    <b>NOTE:</b> This allows, for example, to block all traffic from certain domain, let's say gmail.com, <b>with a general rule</b>, that only has has source defined, and then accept traffic from certain.user@gmail.com to certain.local.user@loca-domain.com <b>with a specific rule</b> because it has both source and destination defined.
 *
 * - <b>lmm_domain</b> : domain level table that includes whitelists and
 *     blacklists that applies to a particular domain. Because it
 *     applies to a domain, the rule must have as source or
 *     destination an account of that domain or the domain
 *     itself. Rules applied at this level can only accept traffic
 *     received (delivered to its domain). 
 *
 *    <b>NOTE:</b> This table should be used or accesible by domain administrators. 
 *
 *  <span style='text-decoration: underline'>This is how this table is checked: </span><br>
 *    1) This table is checked before checking next tables lmm_account. <br>
 *    2) While checking this table, first <b>specific rules</b> are checked first before <b>general rules</b>. <br>
 *  
 *
 * - <b>lmm_account</b> : account level table that includes whitelists and
 *     blacklists that applies to a particular account. Because it
 *     applies to an account, the rule must have as source or as
 *     destination the provided account. Rules applied at this level
 *     can only accept traffic received on the account (not outgoing
 *     traffic). 
 *
 *    <b>NOTE:</b> This table should be used or accesible by end users / account owners.
 *
 *  <span style='text-decoration: underline'>This is how this table is checked: </span><br>
 *    1) This is the last table checked before checking next tables lmm_account. <br>
 *    2) While checking this table, first <b>specific rules</b> are checked first before <b>general rules</b>. <br>
 *
 * If no rule "reject"s or "discard"s the message, the request is let
 * to continue to the next module configure (by reporting internally
 * DUNNO).
 *
 * \section valvulad_mod_lmm_how_rules_are_differenciated mod-lmm How rules are differenciated (whitelists and blacklists)
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
 * \section valvulad_mod_lmm_how_to_block_sasl_user mod-lmm How to block SASL users
 *
 * To block an account, use the following SQL to update valvula database:
 *
 * \code
 * INSERT INTO lmm_global_sasl (is_active, sasl_user) VALUES ('1', 'certain.user@domain.com');
 * \endcode
 *
 * \section valvulad_mod_lmm_how_rules mod-lmm Rules examples
 *
 * To block a certain user from receiving any traffic (outgoing) globally run use the following SQL:
 * \code
 * -- Block  * -> certain.user@domain.com
 * INSERT INTO lmm_global (is_active, destination, status) VALUES ('1', 'certain.user@domain.com', 'reject')
 * \endcode
 *
 * To block a certain user from receiving traffic from a particular user globally run use the following SQL:
 *
 * \code
 * -- Block  anotheruser@anotherdomanin.com -> certain.user@domain.com
 * INSERT INTO lmm_global (is_active, source, destination, status) VALUES ('1', 'anotheruser@anotherdomain.com', 'certain.user@domain.com', 'reject')
 * \endcode
 *
 * To block globally generic accounts webmaster@  without considering destination domain use:
 *
 * \code
 * -- Block  * -> webmaster@*
 * INSERT INTO lmm_global (is_active, destination, status) VALUES ('1', 'webmaster@', 'reject')
 * \endcode
 *
 *
 * 
 * 
 * 
 *
 * 
 */
