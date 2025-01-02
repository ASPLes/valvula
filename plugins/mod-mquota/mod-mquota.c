/* 
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2025 Advanced Software Production Line, S.L.
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
#include <mod-mquota.h>

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
	VALVULA_MOD_MQUOTA_NO_MATCH_USE_FIRST = 1,
	VALVULA_MOD_MQUOTA_NO_MATCH_NO_LIMIT = 2,
	VALVULA_MOD_MQUOTA_NO_MATCH_REJECT = 3,
} ValvulaModMquotaNoMatchMode;

/* hashes to track activity */
ValvulaMutex hash_mutex;

axl_bool __mod_mquota_enable_debug = axl_false;

/* these hashes tracks activity */
/* account level */
axlHash * __mod_mquota_minute_hash;
axlHash * __mod_mquota_hour_hash;
/* domain level */
axlHash * __mod_mquota_domain_minute_hash;
axlHash * __mod_mquota_domain_hour_hash;

axlHash * __mod_mquota_instance_tracking;

/* reference to the list of mquotas lists */
axlList                      * __mod_mquota_limits = NULL;
int                            __mod_mquota_hour_track = 0;
ValvulaModMquotaNoMatchMode    __mod_mquota_no_match_conf = VALVULA_MOD_MQUOTA_NO_MATCH_USE_FIRST;
ModMquotaLimit               * __mod_mquota_current_period = NULL;

/** 
 * Allows to check if A(h,m) < B(h,m)
 */
axl_bool mod_mquota_time_is_before (long hour_a, long minute_a, 
				    long hour_b, long minute_b)
{
	if (hour_a < hour_b)
		return axl_true;
	if (hour_a == hour_b && minute_a <= minute_b)
		return axl_true;
	
	/* report pase */
	return axl_false;
}

ModMquotaLimit * mod_mquota_report_limit_select (ModMquotaLimit * limit)
{
	/* report some debug */
	if (limit && limit->label && __mod_mquota_enable_debug) {
		msg ("Selecting sending mquota period with label [%s] limits g: %d, h: %d, m: %d", limit->label,  
		     limit->global_limit, limit->hour_limit, limit->minute_limit); 
	} /* end if */

	if (limit == NULL && __mod_mquota_enable_debug) {
		wrn ("No sending mquota period was found.."); 
	} /* end if */
	return limit;
} /* end if */

/** 
 * Internal function used to select what's the current default period
 * to apply.
 */
ModMquotaLimit * mod_mquota_get_current_period (int current_minute, int current_hour) {
	ModMquotaLimit * limit;
	int              iterator;

	/* some debug */
	if (__mod_mquota_enable_debug) {
		msg ("mod_mquota_get_current_period (current_minute=%d, current_hour=%d) :: axl_list_length (__mod_mquota_limits) = %d",
		     current_minute, current_hour, axl_list_length (__mod_mquota_limits));
	}

	/* for all default registered period, find the right value */
	iterator = 0;
	while (iterator < axl_list_length (__mod_mquota_limits)) {
		
		/* get limit */
		limit = axl_list_get_nth (__mod_mquota_limits, iterator);

		/* check debug */
		if (__mod_mquota_enable_debug) {
			msg ("Checking now is %02d:%02d (start %02d:%02d - end %02d:%02d)", 
			     current_hour, current_minute, limit->start_hour, limit->start_minute, 
			     limit->end_hour, limit->end_minute); 
		} /* end if */

		if (limit && 
		    /* current hour:minute <= end time */
		    mod_mquota_time_is_before (current_hour, current_minute, limit->end_hour, limit->end_minute) && 
		    /* current hour:minute >= start time */
		    mod_mquota_time_is_before (limit->start_hour, limit->start_minute, current_hour, current_minute)) {

			/* limit found */
			return mod_mquota_report_limit_select (limit);
			
		} /* end if */

		/* inverse periods support */
		if (limit && 
		    ((limit->start_hour > limit->end_hour) || ((limit->start_hour == limit->end_hour) &&  (limit->start_minute < limit->end_minute)))) {

			/* inverse periods defined like this: 21:00 < 09:00 and we now it is 08:00 */
			if (mod_mquota_time_is_before (current_hour, current_minute, limit->end_hour, limit->end_minute))
				return mod_mquota_report_limit_select (limit);
			/* end if */

			/* inverse periods defined like this: 21:00 < 09:00 and we now it is 22:00 */
			if (mod_mquota_time_is_before (limit->start_hour, limit->start_minute, current_hour, current_minute))
				return mod_mquota_report_limit_select (limit);
			/* end if */
		} /* end if */

		/* next iterator */
		iterator++;
	} /* end while */

	if (__mod_mquota_enable_debug) {
		msg ("No period matched, checking for no match configuration..");
	} /* end if */

	/* reached this point, no limit was found, try to find what to
	 * do when this happens */
	switch (__mod_mquota_no_match_conf) {
	case VALVULA_MOD_MQUOTA_NO_MATCH_USE_FIRST:
		
		/* get first limit */
		limit = axl_list_get_nth (__mod_mquota_limits, 0);

		if (__mod_mquota_enable_debug) {
			msg ("No match found, reporting first period (current_hour=%d, current_minute=%d): %s (hour-limit=%d, minute-limit=%d)",
			     current_hour, current_minute, limit->label, limit->hour_limit, limit->minute_limit);
		} /* end if */

		/* get the first limit found */
		return mod_mquota_report_limit_select (limit);
	default:
		/* rest of options not not apply */
		break;
	}
	

	/* reached this point, no limit was found matching */
	return mod_mquota_report_limit_select (NULL);
}


/** 
 * Internal handler used to resent all hashes according to its current
 * configuration.
 */
axl_bool __mod_mquota_minute_handler        (ValvulaCtx  * _ctx, 
					     axlPointer   user_data,
					     axlPointer   user_data2)
{
	axlHash         * hash;
	ModMquotaLimit  * old_reference;
	ModMquotaLimit  * new_reference;
	int               current_minute;
	int               current_hour;

	if (__mod_mquota_enable_debug) {
		msg ("mod-quota: updating accounting info"); 
	} /* end if */

	/* lock */
	valvula_mutex_lock (&hash_mutex);

	/* reset minute hash */
	hash = __mod_mquota_minute_hash;
	__mod_mquota_minute_hash = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	axl_hash_free (hash);

	/* reset domain hash */
	hash = __mod_mquota_domain_minute_hash;
	__mod_mquota_domain_minute_hash = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	axl_hash_free (hash);

	/* reset hour hash if reached */
	__mod_mquota_hour_track ++;
	if (__mod_mquota_hour_track == 60) {
		if (__mod_mquota_enable_debug) {
			msg ("Found hour period change, calling to reset hour stats");
		} /* end if */

		/* reset minute counting */
		__mod_mquota_hour_track = 0;

		/* reset hash */
		hash = __mod_mquota_hour_hash;
		__mod_mquota_hour_hash = axl_hash_new (axl_hash_string, axl_hash_equal_string);
		axl_hash_free (hash);

		/* reset hash (domain) */
		hash = __mod_mquota_domain_hour_hash;
		__mod_mquota_domain_hour_hash = axl_hash_new (axl_hash_string, axl_hash_equal_string);
		axl_hash_free (hash);
	} /* end if */

	/* get minute and hour */
	current_minute = valvula_get_minute ();
	current_hour   = valvula_get_hour ();

	/* now check if global period have changed */
	new_reference = mod_mquota_get_current_period (current_minute, current_hour);
	if (new_reference != __mod_mquota_current_period) {

		/* with the period change, reset instance tracking */
		hash = __mod_mquota_instance_tracking;
		/* show some info */
		if (__mod_mquota_enable_debug) 
			msg ("[mod-mquota] releasing instance tracking (items stored=%d)", axl_hash_items (hash));
		/* init again */
		__mod_mquota_instance_tracking  = axl_hash_new (axl_hash_string, axl_hash_equal_string);
		/* release */
		axl_hash_free (hash);

		/* get old reference */
		old_reference = __mod_mquota_current_period;
		__mod_mquota_current_period = new_reference;

		/* report global period change */
		if (__mod_mquota_enable_debug) {
			msg ("Global period change detected, old is %p, new is %p, applying new values", 
			     old_reference, __mod_mquota_current_period);
		} /* end if */

		axl_hash_free (old_reference->accounting);
		axl_hash_free (old_reference->domain_accounting);
		old_reference->accounting = NULL;

		/* init hash */
		__mod_mquota_current_period->accounting        = axl_hash_new (axl_hash_string, axl_hash_equal_string);
		__mod_mquota_current_period->domain_accounting = axl_hash_new (axl_hash_string, axl_hash_equal_string);

	} /* end if */

	/* lock */
	valvula_mutex_unlock (&hash_mutex);
			
	return axl_false; /* signal the system to receive a call again */
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

	/* get domain limits */
	limit->domain_minute_limit = valvula_support_strtod (ATTR_VALUE (node, "domain-minute-limit"), NULL);
	limit->domain_hour_limit   = valvula_support_strtod (ATTR_VALUE (node, "domain-hour-limit"), NULL);
	limit->domain_global_limit = valvula_support_strtod (ATTR_VALUE (node, "domain-global-limit"), NULL);

	/* get label if defined */
	limit->label        = ATTR_VALUE (node, "label");

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
	int               current_minute;
	int               current_hour;

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
	__mod_mquota_minute_hash        = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	__mod_mquota_hour_hash          = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	__mod_mquota_domain_minute_hash = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	__mod_mquota_domain_hour_hash   = axl_hash_new (axl_hash_string, axl_hash_equal_string);

	/* instance tracking */
	__mod_mquota_instance_tracking  = axl_hash_new (axl_hash_string, axl_hash_equal_string);

	/* get debug support */
	node = axl_doc_get (_ctx->config, "/valvula/enviroment/default-sending-quota");
	if (node) {
		if (HAS_ATTR_VALUE (node, "debug", "yes")) {
			__mod_mquota_enable_debug = axl_true;
		} /* end if */

	} /* end if */

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

	/* get default no match limit */
	node = axl_doc_get (_ctx->config, "/valvula/enviroment/default-sending-quota");
	if (node) {
		/* detect use of 'first' */
		if (HAS_ATTR_VALUE (node, "if-no-match", "first"))
			__mod_mquota_no_match_conf = VALVULA_MOD_MQUOTA_NO_MATCH_USE_FIRST;

		/* detect use of 'no-limit' */
		else if (HAS_ATTR_VALUE (node, "if-no-match", "no-limit"))
			__mod_mquota_no_match_conf = VALVULA_MOD_MQUOTA_NO_MATCH_NO_LIMIT;

		/* detect use of 'reject' */
		else if (HAS_ATTR_VALUE (node, "if-no-match", "reject"))
			__mod_mquota_no_match_conf = VALVULA_MOD_MQUOTA_NO_MATCH_REJECT;

		else {
			error ("ERROR: Failed to start mod-mquota module, configured a if-no-match value that is not supported: %s\n", ATTR_VALUE (node, "if-no-match"));
			return axl_false;
		} /* end if */

	} /* end if */

	/* get minute and hour */
	current_minute = valvula_get_minute ();
	current_hour   = valvula_get_hour ();

	/** get default period that applies to every user that do not
	 * have exceptions or database configurations */
	__mod_mquota_current_period = mod_mquota_get_current_period (current_minute, current_hour);
	if (__mod_mquota_current_period == NULL) {
		wrn ("No default period was found, unable to start mod-mquota module..");
		return axl_false;
	} /* end if */

	/* init accounting */
	__mod_mquota_current_period->accounting        = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	__mod_mquota_current_period->domain_accounting = axl_hash_new (axl_hash_string, axl_hash_equal_string);

	/* create databases to be used by the module */
	valvulad_db_ensure_table (ctx, 
				  /* table name */
				  "mquota_exception", 
				  /* attributes */
				  "id", "autoincrement int", 
				  /* rule status */
				  "is_active", "int",
				  /* sasl user */
				  "sasl_user", "varchar(1024)",
				  /* rule description */
				  "description", "varchar(500)",
				  NULL);

	/* install time handlers */
	valvula_thread_pool_new_event (ctx->ctx, 60000000, __mod_mquota_minute_handler, NULL, NULL);

	return axl_true;
}

/* global handlers that reports when it is possible to do again the delivery when reached global quota */
int __mod_mquota_get_next_global_hour ();
int __mod_mquota_get_next_global_minute ();
/* hour handlers that reports when it is possible to do again the delivery when reached hour quota */
int __mod_mquota_get_next_hour_hour ();
int __mod_mquota_get_next_hour_minute ();
/* minute handlers that reports when it is possible to do again the delivery when reached minute quota */
int __mod_mquota_get_next_minute_hour ();
int __mod_mquota_get_next_minute_minute ();

int __mod_mquota_get_next_global_hour () {
	
	ModMquotaLimit * limit = __mod_mquota_current_period;
	if (limit == NULL)
		return __mod_mquota_get_next_minute_hour ();

	if (limit->end_minute == 59) {
		if (limit->end_hour == 23)
			return 0; /* 23:59 -> 00:00 */
		/* 10:59 -> 11:00 */
		return limit->end_hour + 1;
	}

	/* 10:10 -> 10:11 */
	return limit->end_hour;
}

int __mod_mquota_get_next_global_minute ()
{
	ModMquotaLimit * limit = __mod_mquota_current_period;
	if (limit == NULL)
		return __mod_mquota_get_next_minute_minute ();

	if (limit->end_minute == 59)
		return 0; /* 23:59 -> 00:00 */

	/* 23:30 -> 23:31 */
	return limit->end_minute + 1;
}

int __mod_mquota_get_next_hour_hour () {
	
	if (valvula_get_hour () == 23)
		return 0; /* 23:59 -> 00:00 */
	/* 10:59 -> 11:00 */
	return valvula_get_hour () + 1;
}

int __mod_mquota_get_next_hour_minute ()
{
	/* 13:30 -> 14:30: same minute */
	return valvula_get_minute ();
}

/** 
 * Internal function that allows to get the next hour when minute
 * limit is reached.
 */
int __mod_mquota_get_next_minute_hour () {
	
	if (valvula_get_minute () == 59) {
		if (valvula_get_hour () == 23)
			return 0; /* 23:59 -> 00:00 */
		/* 10:59 -> 11:00 */
		return valvula_get_hour () + 1;
	}

	/* 10:10 -> 10:11 */
	return valvula_get_hour ();
}

/** 
 * Internal function that allows to get the minute when minute limit
 * is reached.
 */
int __mod_mquota_get_next_minute_minute ()
{
	if (valvula_get_minute () == 59)
		return 0; /* 23:59 -> 00:00 */

	/* 23:30 -> 23:31 */
	return valvula_get_minute () + 1;
}

typedef enum {
	MOD_MQUOTA_GLOBAL_CHECK = 1,
	MOD_MQUOTA_HOUR_CHECK   = 2,
	MOD_MQUOTA_MINUTE_CHECK = 3,
} ModCheckType;

axl_bool __mod_mquota_check_reject (const char     * sasl_user, 
				    ValvulaRequest * request, 
				    int              limit, 
				    const char     * label_period, 
				    int              usage, 
				    ModCheckType     check_type, 
				    axl_bool         is_domain_limit)
{

	/* configure label */
	const char * label = "User";
	const char * info  = "account";
	if (is_domain_limit) {
		label = "Domain";
		info  = "domain";
	} /* end if */

	/* if limit is defined (!= -1) and usage is equal or higher,
	   then block operation */
	if (limit > 0 && usage >= limit) {
		/* release */
		valvula_mutex_unlock (&hash_mutex);

		switch (check_type) {
		case MOD_MQUOTA_GLOBAL_CHECK:
			/* report specific global check */
			valvulad_reject (ctx, VALVULA_STATE_REJECT, request, "REJECTED: sending mquota reached for %s %s from (%s). %s global limit=%d reached, user will have to wait until %02d:%02d (period: %s)", 
					 info, sasl_user, request->client_address, label,
					 usage, __mod_mquota_get_next_global_hour (), __mod_mquota_get_next_global_minute (), label_period);
			break;
		case MOD_MQUOTA_HOUR_CHECK:
			/* report specific hour check */
			valvulad_reject (ctx, VALVULA_STATE_REJECT, request, "REJECTED: sending mquota reached for %s %s from (%s). %s hour limit=%d reached, user will have to wait until %02d:%02d (period: %s)", 
					 info, sasl_user, request->client_address, label, 
					 usage, __mod_mquota_get_next_hour_hour (), __mod_mquota_get_next_hour_minute (), label_period);
			break;
		case MOD_MQUOTA_MINUTE_CHECK:
			/* report specific minute check */
			valvulad_reject (ctx, VALVULA_STATE_REJECT, request, "REJECTED: sending mquota reached for %s %s from (%s). %s minute limit=%d reached, user will have to wait until %02d:%02d (period: %s)", 
					 info, sasl_user, request->client_address, label,
					 usage, __mod_mquota_get_next_minute_hour (), __mod_mquota_get_next_minute_minute (), label_period);
			break;
		default:
			break; /* never reached */
		} 

		/* lock released */
		return axl_true;
	} /* end if */

	/* lock not released */
	return axl_false;
}

axl_bool __mod_mquota_check_reject_user (const char * sasl_user, ValvulaRequest * request)
{
	long             minute_usage;
	long             hour_usage;
	long             global_usage;
	const char     * sender;
	const char     * recipient;
	const char     * queue_id;
	int              count;

	/* check global limit */
	global_usage = PTR_TO_INT (axl_hash_get (__mod_mquota_current_period->accounting, (axlPointer) sasl_user));
	if (__mod_mquota_check_reject (sasl_user, request, __mod_mquota_current_period->global_limit, 
				       __mod_mquota_current_period->label,
				       global_usage, MOD_MQUOTA_GLOBAL_CHECK, axl_false))
		return axl_true; /* lock released by __mod_mquota_check_reject when it
				  * returns axl_true */

	/* check hour limit */
	hour_usage = PTR_TO_INT (axl_hash_get (__mod_mquota_hour_hash, (axlPointer) sasl_user));
	if (__mod_mquota_check_reject (sasl_user, request, __mod_mquota_current_period->hour_limit, 
				       __mod_mquota_current_period->label,
				       hour_usage, MOD_MQUOTA_HOUR_CHECK, axl_false))
		return axl_true; /* lock released by __mod_mquota_check_reject when it
				  * returns axl_true */

	/* check minute limit */
	minute_usage = PTR_TO_INT (axl_hash_get (__mod_mquota_minute_hash, (axlPointer) sasl_user));
	if (__mod_mquota_check_reject (sasl_user, request, __mod_mquota_current_period->minute_limit, 
				       __mod_mquota_current_period->label,
				       minute_usage, MOD_MQUOTA_MINUTE_CHECK, axl_false))
		return axl_true; /* lock released by __mod_mquota_check_reject when it
				  * returns axl_true */

	/* get count */
	count = request->recipient_count;
	if (count == 0)
		count = 1;

	/* reached this point, update record */
	global_usage += count;
	hour_usage   += count;
	minute_usage += count;

	/* get values and move then to printable values */
	sender = valvula_get_sender (request);
	if (! sender)
	  sender = "";
	recipient = valvula_get_recipient (request);
	if (! recipient)
	  recipient = "";
	queue_id = valvula_get_queue_id (request);
	if (! queue_id)
	  queue_id = "";

	/* show quota consumed */
	if (__mod_mquota_enable_debug)
		msg ("[mod-mquota] quota consumed rcpt count=%d, request-instance=%s sasl-username=%s (sender=%s, recipient=%s, queue-id=%s) -> global %d, hour %d, minute %d",
		     count, valvula_get_request_instance (request), sasl_user, sender, recipient, queue_id, global_usage, hour_usage, minute_usage); 

	axl_hash_insert_full (__mod_mquota_current_period->accounting, (axlPointer) axl_strdup (sasl_user), axl_free, INT_TO_PTR (global_usage), NULL);
	axl_hash_insert_full (__mod_mquota_hour_hash, (axlPointer) axl_strdup (sasl_user), axl_free, INT_TO_PTR (hour_usage), NULL);
	axl_hash_insert_full (__mod_mquota_minute_hash, (axlPointer) axl_strdup (sasl_user), axl_free, INT_TO_PTR (minute_usage), NULL);

	return axl_false; /* not rejected, not limited */
}

axl_bool __mod_mquota_check_reject_domain (const char * sasl_domain, ValvulaRequest * request)
{
	long             minute_usage;
	long             hour_usage;
	long             global_usage;

	/* check global limit */
	global_usage = PTR_TO_INT (axl_hash_get (__mod_mquota_current_period->domain_accounting, (axlPointer) sasl_domain));
	if (__mod_mquota_check_reject (sasl_domain, request, __mod_mquota_current_period->domain_global_limit, 
				       __mod_mquota_current_period->label,
				       global_usage, MOD_MQUOTA_GLOBAL_CHECK, axl_true))
		return axl_true;

	/* check hour limit */
	hour_usage = PTR_TO_INT (axl_hash_get (__mod_mquota_domain_hour_hash, (axlPointer) sasl_domain));
	if (__mod_mquota_check_reject (sasl_domain, request, __mod_mquota_current_period->domain_hour_limit, 
				       __mod_mquota_current_period->label,
				       hour_usage, MOD_MQUOTA_HOUR_CHECK, axl_true))
		return axl_true;

	/* check minute limit */
	minute_usage = PTR_TO_INT (axl_hash_get (__mod_mquota_domain_minute_hash, (axlPointer) sasl_domain));
	if (__mod_mquota_check_reject (sasl_domain, request, __mod_mquota_current_period->domain_minute_limit, 
				       __mod_mquota_current_period->label,
				       minute_usage, MOD_MQUOTA_MINUTE_CHECK, axl_true))
		return axl_true;

	/* reached this point, update record */
	global_usage += 1;
	hour_usage += 1;
	minute_usage += 1;

	/* msg ("Saving limits %s -> global %d, hour %d, minute %d", sasl_domain, global_usage, hour_usage, minute_usage);  */

	axl_hash_insert_full (__mod_mquota_current_period->domain_accounting, (axlPointer) axl_strdup (sasl_domain), axl_free, INT_TO_PTR (global_usage), NULL);
	axl_hash_insert_full (__mod_mquota_domain_hour_hash, (axlPointer) axl_strdup (sasl_domain), axl_free, INT_TO_PTR (hour_usage), NULL);
	axl_hash_insert_full (__mod_mquota_domain_minute_hash, (axlPointer) axl_strdup (sasl_domain), axl_free, INT_TO_PTR (minute_usage), NULL);

	return axl_false; /* not rejected, not limited */
}

axl_bool __mod_mquota_has_exception (const char * sasl_user, const char * sasl_domain)
{
	/* check if has an exception */
	if (valvulad_db_boolean_query (ctx, "SELECT * FROM mquota_exception WHERE sasl_user = '%s' OR sasl_user = '%s'", sasl_user, sasl_domain)) {
		return axl_true; /* report we have an exception */
	} /* end if */

	/* no exception found */
	return axl_false;
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
	const char     * sasl_user;
	const char     * sasl_domain;
	char           * key;
	int              count;

	/* do nothing if module is disabled */
	if (__mquota_mode == VALVULA_MOD_MQUOTA_DISABLED)
		return VALVULA_STATE_DUNNO;

	/* skip if request is not autenticated */
	if (! valvula_is_authenticated (request))
		return VALVULA_STATE_DUNNO;

	/* get sasl user */
	sasl_user   = valvula_get_sasl_user (request);
	sasl_domain = valvula_get_domain (sasl_user);
	/* msg ("Checking sasl user: %s", sasl_user); */

	/* check user exceptions to avoid applying quotas to him */
	if (__mod_mquota_has_exception (sasl_user, sasl_domain))
		return VALVULA_STATE_DUNNO;

	/* apply limits found, first global */
	if (__mod_mquota_current_period == NULL) {
		valvulad_reject (ctx, VALVULA_STATE_REJECT, request, "Rejecting due to internal server error");
		error ("Default period isn't defined, unable to apply limits");
		return VALVULA_STATE_REJECT;
	} /* end if */

	/* get the limit */
	valvula_mutex_lock (&hash_mutex);

	/* get count */
	count = request->recipient_count;
	if (count == 0) {
		/* not in DATA section, attempt to avoid counting twice (or more) same calls to same instance request */
		if (valvula_get_sender (request) && valvula_get_recipient (request) && valvula_get_request_instance (request)) {
			/* sender, recipient and instance request defined */
			
			/* check if we tracked this request instance */
			key = axl_strdup_printf ("%s.%s.%s", valvula_get_sender (request), valvula_get_recipient (request), valvula_get_request_instance (request));
			if (axl_hash_exists (__mod_mquota_instance_tracking, key)) {
				/* also release key in hash */
				axl_hash_remove (__mod_mquota_instance_tracking, key);		
				/* release key */
				axl_free (key);
				
				/* release mutex */
				valvula_mutex_unlock (&hash_mutex);

				/* show debug */
				if (__mod_mquota_enable_debug)
					msg ("[mod-mquota] already tracked, skipping quota consume, request-instance=%s sasl-username=%s (sender=%s, recipient=%s, queue-id=%s)",
					     valvula_get_request_instance (request),
					     valvula_get_sasl_user (request),
					     valvula_get_sender (request),
					     valvula_get_recipient (request),
					     valvula_get_queue_id (request));
				
				return VALVULA_STATE_DUNNO; /* not rejected, not limited */
			} /* end if */

			/* save this request to avoid counting quota for this case (reuse key pointer */
			axl_hash_insert_full (__mod_mquota_instance_tracking, (axlPointer) key, axl_free, INT_TO_PTR (axl_true), NULL);
			
		} /* end if */
	} /* end if */

	if (__mod_mquota_check_reject_user (sasl_user, request)) 
		return VALVULA_STATE_REJECT;  /* lock already released by
					       * __mod_mquota_check_reject_user
					       * when it returns axl_true */

	/* check domain limits */
	if (strstr (sasl_user, "@")) {
		/* check and reject if required */
		if (__mod_mquota_check_reject_domain (sasl_domain, request))
			return VALVULA_STATE_REJECT; /* lock already released by
						      * __mod_mquota_check_reject_user
						      * when it returns axl_true */
	} /* endi f */

	/* release */
	valvula_mutex_unlock (&hash_mutex);

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
	axl_hash_free (__mod_mquota_domain_minute_hash);
	axl_hash_free (__mod_mquota_domain_hour_hash);

	/* release instance tracking */
	axl_hash_free (__mod_mquota_instance_tracking);

	/* release accounting */
	axl_hash_free (__mod_mquota_current_period->accounting);
	axl_hash_free (__mod_mquota_current_period->domain_accounting);

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
 * \page valvulad_mod_mquota mod-mquota: Valvulad sending control quota module
 *
 * \section mquota_intro Introduction to mquota
 *
 * Mod-Mquota applies to sending mail operations when they are
 * authenticated. It is mainly designed for shared hosting solutions
 * where it is required to limit user sending rate and to control and
 * minimize the impact of compromised accounts.
 *
 * The plugin has a straightforward operation method were you configure
 * different time periods inside which you limit the amount of mails
 * that can be sent by minute, by hour and inside the total
 * period. 
 *
 * Those limits apply to account level and whole domain level (so
 * "smart" users cannot use user1@yourdomain.com,
 * user2@yourdomani.com..and so on to bypass limits).
 *
 * \section mquota_configuration_example mod-mquota Configuration examples
 *
 * Take a look inside &lt;default-sending-quota> node inside /etc/valvula/valvula.conf and you'll find something like this:
 * \code
 *    <!-- sending and receiving quotas: used by mod-mquota  -->
 *    <default-sending-quota status="full" if-no-match="first">
 *      <!-- account limit: 50/minute,  250/hour  and  750/global from 09:00 to 21:00 
 *           domain limit:  100/minute, 375/hour  and 1100/global 
 *
 *           note: use -1 to disable any of the limits.  
 *           For example, to disable global limit, use globa-limit="-1" 
 *      -->
 *      <limit label='day quota' from="9:00" to="21:00"  status="full" 
 *	     minute-limit="50" hour-limit="250" global-limit="750" 
 *	     domain-minute-limit="100" domain-hour-limit="375" domain-global-limit="1100" />
 *
 *      <!-- limit 15/minute, 50/hour  and 150/global from 21:00 to 09:00 -->
 *      <limit label='night quota' from="21:00" to="9:00"  status="full" 
 *	     minute-limit="15" hour-limit="50" global-limit="150" 
 *	     domain-minute-limit="15" domain-hour-limit="50" domain-global-limit="150" />
 *    </default-sending-quota>
 * \endcode
 *
 * Taking as a reference previous example, operation mode is applied following next rules:
 *
 * - 1. First it is found what period applies at this time by looking into <b>from</b> and <b>to</b> attribute on every <b>&lt;limit></b> node. 
 *
 * - 2. If no period matches, <b>&lt;if-no-match></b> attribute comes into play (we will talk about this later). 
 *
 * - 3. Once the period is selected, accounting is done to the user account and domain looking at the self-explaining limits. For example, if the user sends more that 50 mails by minute at 11:00 am, then valvula will reject accepting sending more.
 *
 * - 4. Again, if the total amount sent by a domain (including all accounts involved in previous send operations) reached provided limits (for example, domain-minute-limit) then valvula will reject accepting sending more. 
 *
 * - 5. Finally, if the minute limit is reached, then a minute after it will be restarted so the user only have to wait that time. The same applies to the hour limit and to the global limit. 
 *
 * \section mquota_if_no_match_period mod-mquota Selecting a default period when no match is found (<if-no-match>)
 *
 * When no period is found to apply, if-no-match attribute is used (at
 * <default-sending-quota>). This allows to define a particular period
 * where limits applies and then, outside that limit, a default
 * period, no limit or just reject is applied.
 *
 * Allowed values are:
 *
 * - 1) first : if no period matches, then the first period in the definition list is used.
 *
 * - 2) no-limit : if no period matches, then, apply no limit and let the user to send without limit. This is quite useful to define night limits. That is, you only have to define a period to cover nights period and then, during the day no limit is applied where you can have a better supervision.
 *
 * - 3) reject : if no period matches, then just reject the send operation.
 * 
 *
 * \section mquota_exceptions mod-mquota Configuring exceptions to certain users
 *
 * In the case you want to apply quotas in a general manner but need a
 * way to avoid applying these quotas to certain users, then connect
 * to the valvula database (take a look what's defined at
 * <b>/etc/valvula/valvula.conf</b>) and then run the following query:
 *
 * \code
 * INSERT INTO mquota_exception (is_active, sasl_user) VALUES ('1', 'test4@unlimited2.com');
 * \endcode
 *
 * This will allow sasl user test4@unlimited2.com to send without any restriction.
 *
 */
