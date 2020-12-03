/* 
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2020 Advanced Software Production Line, S.L.
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
axl_bool     __mod_object_resolver_enable_debug    = axl_false;
const char * __object_resolver_plesk_accounts_file = "/var/spool/postfix/plesk/passwd.db";

axl_bool object_resolver_plesk_accounts (ValvuladCtx * ctx, const char * item_name, ValvuladObjectRequest request_type, axlPointer data)
{
	const char * domain       = valvula_get_domain (item_name);
	char       * local_part   = valvula_get_local_part (item_name);
	ValvuladRes  res          = NULL;
	ValvuladRow  row;
	axl_bool     result       = axl_false;
	
	
	/* run query */
	switch (request_type) {
	case VALVULAD_OBJECT_ACCOUNT:
		/* get account */
		res = valvulad_db_sqlite_run_query (ctx, __object_resolver_plesk_accounts_file,
						    "SELECT users.name || '@' || domains.name AS account FROM domains, users WHERE domains.id = users.dom_id AND domains.name = '%s' and users.name = '%s'",
						    domain, local_part);
		break;
	case VALVULAD_OBJECT_DOMAIN:
		/* get account */
		res = valvulad_db_sqlite_run_query (ctx, __object_resolver_plesk_accounts_file,
						    "SELECT domains.name FROM domains WHERE domains.name = '%s'",
						    domain);
		break;
	case VALVULAD_OBJECT_ALIAS:
		/* get alias */
		/* no support yet */
		break;
	}
	/* release memory */
	axl_free (local_part);

	/* check result */
	if (res) {
		/* get first row */
		row = valvulad_db_sqlite_get_row (ctx, res);
		if (row) {
			/* check result */
			result = axl_cmp (valvulad_db_sqlite_get_cell (ctx, row, 0), item_name);
		} /* end if */

		/* release result */
		valvulad_db_sqlite_release_result (res);
		
	} /* end if */

	/* return result */
	return result;
}

void object_resolver_change_group_uid (const char * path) {
	/* run chown operation */
	int result;

	/* avoid changing anything if running_gid is not defined */
	if (ctx->running_gid <= 0 || ctx->running_uid <= 0)
		return;
	
	result = chown (path, -1, ctx->running_gid);
	if (result != 0)
		error ("Failed to change group gid=%d to %s", ctx->running_gid, path);
	else
		msg ("Changed gid=%d to %s", ctx->running_gid, path);
	return;
}

void object_resolver_add_exec_to_dir (const char * path) {
	int         result;
	struct stat value;

	/* avoid changing anything if running_gid is not defined */
	if (ctx->running_gid <= 0 || ctx->running_uid <= 0)
		return;

	/* get stat */
	result = stat (path, &value);
	if (result != 0) {
		error ("Failed to get stat for %s, errno=%d, errocode=%d", path, errno, result);
		return;
	} /* end if */

	/* clear read and write to others (chmod o-rw) */
	value.st_mode = value.st_mode & 0xfff8;
	/* add exec to others (chmod o+x) */
	value.st_mode = value.st_mode | 0x0001;
	
	msg ("Changed mode=%o to path %s", value.st_mode, path);
	result = chmod (path, value.st_mode);
	if (result != 0) {
		error ("Failed to configure permissions (%o) to folder %s, errno=%d, errorcode=%d", value.st_mode, path, errno, result);
		return;
	} /* end if */
	return;
}

/** 
 * @brief Init function, perform all the necessary code to register
 * profiles, configure Vortex, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  object_resolver_init (ValvuladCtx * _ctx)
{
	if (access (__object_resolver_plesk_accounts_file, F_OK) != -1 ) {
		msg ("Detected Plesk Postfix database for user accounts and domains: %s", __object_resolver_plesk_accounts_file);
	
		/* register resolver */
		valvulad_run_add_object_resolver (_ctx, object_resolver_plesk_accounts, NULL);

		/* configure permissions */
		/* change group permissions for folder and file */
		object_resolver_change_group_uid ("/var/spool/postfix/plesk/passwd.db");
		object_resolver_change_group_uid ("/var/spool/postfix/plesk/");
		object_resolver_add_exec_to_dir ("/var/spool/postfix/plesk/");
	} /* end if */
	
	return axl_true;
}

/** 
 * @brief Process request for the module.
 */
ValvulaState object_resolver_process_request (ValvulaCtx        * _ctx, 
					      ValvulaConnection * connection, 
					      ValvulaRequest    * request,
					      axlPointer          request_data,
					      char             ** message)
{
	return VALVULA_STATE_DUNNO;
}

/** 
 * @brief Close function called once the valvulad server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
void object_resolver_close (ValvuladCtx * ctx)
{
	msg ("Valvulad object_resolver module: close");
	return;
}

/** 
 * @brief The reconf function is used by valvulad to notify to all
 * its modules loaded that a reconfiguration signal was received and
 * modules that could have configuration and run time change support,
 * should reread its files. It is an optional handler.
 */
void object_resolver_reconf (ValvuladCtx * ctx) {
	msg ("Valvulad configuration have change");
	return;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the valvulad will lookup to load the rest of items.
 */
ValvuladModDef module_def = {
	"mod-object-resolver",
	"Valvulad Valvula external object resolution to detect local domain and accounts.",
	object_resolver_init,
	object_resolver_close,
	object_resolver_process_request,
	NULL,
	NULL
};

END_C_DECLS



/** 
 * \page valvulad_mod_object_resolver mod-object_resolver : Valvula external object resolution to detect local domain and accounts.
 *
 * \section valvulad_mod_object_resolver_intro Introduction
 *
 * Mod-Object-Resolver is a module that tries to aid valvulad engine
 * to detect local domain and sasl user accounts that are not
 * available using normal configurations with MySQL.
 *
 * In context, the module attempts to detect known configurations
 * (like Plesk Postfix configuration), reading in each case databases
 * with different format than MySQL, and inyecting that information
 * into the Valvulad engine (by registering a object resolver handler
 * \ref valvulad_run_add_object_resolver).
 *
 *
 *
 * \section valvulad_mod_object_resolver_how_it_works How mod-object_resolver works
 *
 * For know module function is pretty simple. Just enable it by running:
 *
 * \code
 * >> valvulad-mgr.py -e mod-object-resolver
 * \endcode
 *
 * Then, restart valvula:
 *
 * \code
 * >> service valvulad restart
 * \endcode
 *
 * 
 */
