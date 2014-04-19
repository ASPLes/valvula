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

typedef enum {
	VALVULA_MOD_SLM_FULL            = 1,
	VALVULA_MOD_SLM_SAME_DOMAIN     = 2,
	VALVULA_MOD_SLM_VALID_MAIL_FROM = 3,
	VALVULA_MOD_SLM_DISABLED        = 4,

} ValvulaModSlmMode;

ValvulaModSlmMode __slm_mode = VALVULA_MOD_SLM_DISABLED;

/** 
 * @brief Init function, perform all the necessary code to register
 * profiles, configure Vortex, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  slm_init (ValvuladCtx * _ctx)
{
	axlNode    * node;
	const char * mode;

	/* configure the module */
	ctx = _ctx;

	msg ("Valvulad slm module: init");
	
	/* get node */
	node = axl_doc_get (_ctx->config, "/valvula/enviroment/sender-login-mismatch");
	mode = ATTR_VALUE (node, "mode");

	/* configure module */
	if (axl_cmp (mode, "full"))
		__slm_mode = VALVULA_MOD_SLM_FULL;
	else if (axl_cmp (mode, "same-domain"))
		__slm_mode = VALVULA_MOD_SLM_SAME_DOMAIN;
	else if (axl_cmp (mode, "valid-mail-from"))
		__slm_mode = VALVULA_MOD_SLM_VALID_MAIL_FROM;

	/* ensure tables */
	if (! valvulad_db_ensure_table (ctx, "slm_exception", 
					"mail_from", "text", 
					"sasl_username", "text", NULL)) {
		printf ("ERROR: unable to create slm_exception table..\n");
		return axl_false;
	} /* end if */

	return axl_true;
}

axl_bool slm_has_exception (ValvuladCtx * ctx, ValvulaRequest * request)
{
	/* request is authenticated, check exceptions */
	if (valvulad_db_boolean_query (ctx, "SELECT mail_from, sasl_username FROM slm_exception WHERE sasl_username = '%s' AND mail_from = '%s'",
				       request->sasl_username, request->sender))
		return axl_true;

	/* no exception found */
	return axl_false;
}

/** 
 * @brief Process request for the module.
 */
ValvulaState slm_process_request (ValvulaCtx        * _ctx, 
				  ValvulaConnection * connection, 
				  ValvulaRequest    * request,
				  axlPointer          request_data,
				  char             ** message)
{

	/* do nothing if module is disabled */
	if (__slm_mode == VALVULA_MOD_SLM_DISABLED)
		return VALVULA_STATE_DUNNO;

	/* skip if request is not autenticated */
	if (! valvula_is_authenticated (request))
		return VALVULA_STATE_DUNNO;

	/* check here for the exception first */
	if (slm_has_exception (ctx, request))
		return VALVULA_STATE_DUNNO;

	if (__slm_mode == VALVULA_MOD_SLM_FULL) {
		/* apply same domain restriction */
		if (! axl_cmp (request->sasl_username, request->sender)) {
			valvulad_reject (ctx, request, "Rejecting because SASL username <%s> do not match mail from <%s> (mod-slm=full)", request->sasl_username, request->sender);
			return VALVULA_STATE_REJECT;
		} /* end if */

		/* full mode enabled and it passes the test */
		return VALVULA_STATE_DUNNO;
	} else if (__slm_mode == VALVULA_MOD_SLM_SAME_DOMAIN) {
		/* ensure at domain matches */
		if (! axl_cmp (valvula_get_domain (request->sasl_username), valvula_get_domain (request->sender))) {
			valvulad_reject (ctx, request, "Rejecting because SASL username domain <%s> do not match mail from domain <%s> (mod-slm=same-domain)", 
					 valvula_get_domain (request->sasl_username), valvula_get_domain (request->sender));
			return VALVULA_STATE_REJECT;
		} /* end if */

		/* and now ensure the mail from account is valid */
		if (! valvulad_run_is_local_address (ctx, request->sender)) {
			valvulad_reject (ctx, request, "Rejecting because SASL username <%s> is sending with an unknown account mail from <%s> (mod-slm=same-domain)", 
					 request->sasl_username, request->sender);
			return VALVULA_STATE_REJECT;
		} /* end if */
	} else if (__slm_mode == VALVULA_MOD_SLM_VALID_MAIL_FROM) {
		/* and now ensure the mail from account is valid */
		if (! valvulad_run_is_local_address (ctx, request->sender)) {
			valvulad_reject (ctx, request, "Rejecting because SASL username <%s> is sending with an unknown account mail from <%s> (mod-slm=valid-mail-from)", 
					 request->sasl_username, request->sender);
			return VALVULA_STATE_REJECT;
		} /* end if */
	}

	/* by default report return dunno */
	return VALVULA_STATE_DUNNO;
}

/** 
 * @brief Close function called once the valvulad server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
void slm_close (ValvuladCtx * ctx)
{
	msg ("Valvulad slm module: close");
	return;
}

/** 
 * @brief The reconf function is used by valvulad to notify to all
 * its modules loaded that a reconfiguration signal was received and
 * modules that could have configuration and run time change support,
 * should reread its files. It is an optional handler.
 */
void slm_reconf (ValvuladCtx * ctx) {
	msg ("Valvulad configuration have change");
	return;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the valvulad will lookup to load the rest of items.
 */
ValvuladModDef module_def = {
	"mod-slm",
	"Valvulad Sender Login Mismatch module",
	slm_init,
	slm_close,
	slm_process_request,
	NULL,
	NULL
};

END_C_DECLS


/** 
 * \page slm_intro mod-slm Introduction
 *
 * mod-slm allows to implement sender login mismatch restrictions
 * which allows limiting mail-from's spoofing.
 *
 * The module, in full mode, provides same function as postfix
 * restrictions reject_authenticated_sender_login_mismatch and
 * reject_sender_login_mismatch. 
 *
 * However, the module implements an adaptative approach allow to
 * provide different enforce levels that makes it suitable to more
 * situations.
 *
 * Along this, mod-slm provides support for restrictions which allows
 * convering those situations where most accounts must be limited
 * though some of them shouldn't (for example, because they are relay
 * accounts).
 *
 * The module has different general modes with exceptions that can be
 * handled by updating the database. Current modes are:
 *
 * - full : mail from and sasl auth must match or request will be rejected.
 *
 * - same-domain : sasl auth user and mail from's domain must match
 *     and sender address (mail from) must be a valid account.
 * 
 * - valid-mail-from : mail from and sasl auth user can mismatch but
 * mail from should be a valid account at current mail server.
 *
 * - disabled : disables mod-slm application. Removing <sender-login-mismatch /> node also disables mod-slm module.
 * 
 * These values are configured inside <enviroment> node like follows:
 *
 * \code
 * <enviroment>
 *    ...
 *    <sender-login-mismatch mode='full' />
 *    ...
 * </enviroment>
 * \endcode
 *
 */
