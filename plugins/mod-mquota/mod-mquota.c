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
	VALVULA_MOD_MQUOTA_FULL            = 1,
	VALVULA_MOD_MQUOTA_DISABLED        = 4,
} ValvulaModMquotaMode;

ValvulaModMquotaMode __mquota_mode = VALVULA_MOD_MQUOTA_DISABLED;

typedef enum {
	VALVULA_MOD_MQUOTA_NO_MATCH_FIRST = 1,
	VALVULA_MOD_MQUOTA_NO_MATCH_NO_LIMIT = 2,
} ValvulaModMquotaNoMatchMode;

/* hashes to track activity */
ValvulaMutex hash_mutex;

/* these hashes tracks activity */
axlHash * __mod_mquota_minute_hash;
axlHash * __mod_mquota_hour_hash;
axlHash * __mod_mquota_global_hash;

typedef struct _ModMquotaLimit {
	/* when this applies */
	int       start_hour;
	int       start_minute;

	/* when this ends */
	int       end_hour;
	int       end_minute;

	/* limits */
	int       minute_limit;
	int       hour_limit;
	int       global_limit;

} ModMquotaLimit;

/* reference to the list of mquotas lists */
axlList * __mod_mquota_limits = NULL;
int       __mod_mquota_hour_track = 0;

/** 
 * Internal handler used to resent all hashes according to its current
 * configuration.
 */
axl_bool __mod_mquota_minute_handler        (ValvulaCtx  * ctx, 
					     axlPointer   user_data,
					     axlPointer   user_data2)
{
	axlHash * hash;

	/* reset minute hash */
	hash                     = __mod_mquota_minute_hash;
	__mod_mquota_minute_hash = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	axl_hash_free (hash);

	/* reset hour hash if reached */
	__mod_mquota_hour_track ++;
	if (__mod_mquota_hour_track == 60) {
		/* reset minute counting */
		__mod_mquota_hour_track = 0;

		/* reset hash */
		hash                     = __mod_mquota_hour_hash;
		__mod_mquota_hour_hash = axl_hash_new (axl_hash_string, axl_hash_equal_string);
		axl_hash_free (hash);
	} /* end if */

	/* now check global hash reseting */
		
	
	return axl_true; /* signal the system to receive a call again */
}


ModMquotaLimit * __mod_mquota_parse_node (axlNode * node) {
	ModMquotaLimit  * limit;
	char           ** values;

	/* if not enabled, skip rule */
	if (! HAS_ATTR_VALUE (node, "status", "full"))
		return NULL;

	/* create node limit */
	limit = axl_new (ModMquotaLimit, 1);
	if (! limit)
		return NULL;

	/* parse from */
	values = axl_split (ATTR_VALUE (node, "from"), 1, ":");
	if (values == NULL || values[0] == NULL || values[1] == NULL) {
		axl_free (limit);
		error ("Unable to parse from='%s' attribute, skipping rule", ATTR_VALUE (node, "from"));
		return NULL;
	} /* end if */

	/* get start hour and minute */
	limit->start_hour   = valvula_support_strtod (values[0], NULL);
	limit->start_minute = valvula_support_strtod (values[1], NULL);

	axl_freev (values);

	/* parse from */
	values = axl_split (ATTR_VALUE (node, "to"), 1, ":");
	if (values == NULL || values[0] == NULL || values[1] == NULL) {
		axl_free (limit);
		error ("Unable to parse to='%s' attribute, skipping rule", ATTR_VALUE (node, "to"));
		return NULL;
	} /* end if */

	/* get end hour and minute */
	limit->end_hour   = valvula_support_strtod (values[0], NULL);
	limit->end_minute = valvula_support_strtod (values[1], NULL);

	axl_freev (values);

	/* get limits */
	limit->minute_limit = valvula_support_strtod (ATTR_VALUE (node, "minute-limit"), NULL);
	limit->hour_limit   = valvula_support_strtod (ATTR_VALUE (node, "hour-limit"), NULL);
	limit->global_limit = valvula_support_strtod (ATTR_VALUE (node, "global-limit"), NULL);

	/* install time handlers */
	valvula_thread_pool_new_event (ctx->ctx, 60000000, __mod_mquota_minute_handler, NULL, NULL);

	return limit;
}

/** 
 * @brief Init function, perform all the necessary code to register
 * profiles, configure Vortex, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  mquota_init (ValvuladCtx * _ctx)
{
	axlNode         * node;
	const char      * mode;
	ModMquotaLimit  * limit;

	/* configure the module */
	ctx = _ctx;

	msg ("Valvulad mquota module: init");
	
	/* init mutex */
	valvula_mutex_create (&hash_mutex);
	
	/* get node */
	node = axl_doc_get (_ctx->config, "/valvula/enviroment/default-sending-quota");
	mode = ATTR_VALUE (node, "status");

	/* configure module */
	if (axl_cmp (mode, "full"))
		__mquota_mode = VALVULA_MOD_MQUOTA_FULL;

	/* init tracking hashes */
	__mod_mquota_minute_hash = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	__mod_mquota_hour_hash   = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	__mod_mquota_global_hash = axl_hash_new (axl_hash_string, axl_hash_equal_string);

	/* parse limits */
	__mod_mquota_limits = axl_list_new (axl_list_always_return_1, axl_free);
	node = axl_doc_get (_ctx->config, "/valvula/enviroment/default-sending-quota/limit");
	while (node) {
		if (HAS_ATTR_VALUE (node, "status", "full")) {
			/* create a node */
			limit = __mod_mquota_parse_node (node);
			if (limit) 
				axl_list_append (__mod_mquota_limits, limit);
			
		} /* end if */

		/* next <limit /> node */
		node = axl_node_get_next_called (node, "limit");
	} /* end while */

	

	return axl_true;
}

/** 
 * @brief Process request for the module.
 */
ValvulaState mquota_process_request (ValvulaCtx        * _ctx, 
				     ValvulaConnection * connection, 
				     ValvulaRequest    * request,
				     axlPointer          request_data,
				     char             ** message)
{

	/* do nothing if module is disabled */
	if (__mquota_mode == VALVULA_MOD_MQUOTA_DISABLED)
		return VALVULA_STATE_DUNNO;

	/* skip if request is not autenticated */
	if (! valvula_is_authenticated (request))
		return VALVULA_STATE_DUNNO;

	

	/* by default report return dunno */
	return VALVULA_STATE_DUNNO;
}

/** 
 * @brief Close function called once the valvulad server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
void mquota_close (ValvuladCtx * ctx)
{
	msg ("Valvulad mquota module: close");

	/* release hash */
	axl_hash_free (__mod_mquota_minute_hash);
	axl_hash_free (__mod_mquota_hour_hash);
	axl_hash_free (__mod_mquota_global_hash);

	/* release the list */
	axl_list_free (__mod_mquota_limits);

	/* release mutex */
	valvula_mutex_destroy (&hash_mutex);
	return;
}

/** 
 * @brief The reconf function is used by valvulad to notify to all
 * its modules loaded that a reconfiguration signal was received and
 * modules that could have configuration and run time change support,
 * should reread its files. It is an optional handler.
 */
void mquota_reconf (ValvuladCtx * ctx) {
	msg ("Valvulad configuration have change");
	return;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the valvulad will lookup to load the rest of items.
 */
ValvuladModDef module_def = {
	"mod-mquota",
	"Valvulad mail quotas module",
	mquota_init,
	mquota_close,
	mquota_process_request,
	NULL,
	NULL
};

END_C_DECLS


/** 
 * \page mquota_intro mod-mquota Introduction
 *
 *
 */
