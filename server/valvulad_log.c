/* 
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2013 Advanced Software Production Line, S.L.
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
#include <stdlib.h>
#include <syslog.h>

/** 
 * @brief Init the valvulad log module.
 */
void valvulad_log_init (ValvuladCtx * ctx)
{
	/* get current valvulad configuration */
	axlDoc  * doc = ctx->config;
	axlNode * node;

	/* check log reporting */
	node = axl_doc_get (doc, "/valvulad/global-settings/log-reporting");
	if (node == NULL) {
		abort_error ("Unable to find log configuration <valvulad/global-settings/log-reporting>");
		return;
	} /* end if */

	/* check enabled attribute */
	if (! HAS_ATTR (node, "enabled")) {
		abort_error ("Missing attribute 'enabled' located at <valvulad/global-settings/log-reporting>. Unable to determine if log is enabled");
		return;
	}

	/* check if log reporting is enabled or not */
	if (! HAS_ATTR_VALUE (node, "enabled", "yes")) {
		msg ("log reporting to file disabled");
		return;
	}

	/* check for syslog usage */
	ctx->use_syslog = HAS_ATTR_VALUE (node, "use-syslog", "yes");
	msg ("Checking for usage of syslog %d", ctx->use_syslog);
	if (ctx->use_syslog) {
		/* open syslog */
		openlog ("valvulad", LOG_PID, LOG_DAEMON);
		msg ("Using syslog facility for logging");
		return;
	} /* end if */

	return;
}


/** 
 * @internal macro that allows to report a message to the particular
 * log, appending date information.
 */
void REPORT (axl_bool use_syslog, LogReportType type, const char * message, va_list args, const char * file, int line) 
{
	/* get valvulad context */
	char            * string;

	if (use_syslog) {
		string = axl_strdup_printfv (message, args);
		if (string == NULL)
			return;
		if (type == LOG_REPORT_ERROR) {
			syslog (LOG_ERR, "**ERROR**: %s", string);
		} else if (type == LOG_REPORT_WARNING) {
			syslog (LOG_INFO, "warning: %s", string);
		} else if (type == LOG_REPORT_ACCESS) {
			syslog (LOG_INFO, "access: %s", string);
		} else if (type == LOG_REPORT_VORTEX) {
			syslog (LOG_INFO, "vortex: %s", string);
		} else {
 		        syslog (LOG_INFO, "info: %s", string);
		}
		axl_free (string);
		return;
	} /* end if */
	
	return;
} 

/** 
 * @brief Reports a single line to the particular log, configured by
 * "type".
 * 
 * @param type The log to select for reporting. The function do not
 * support reporting at the same call to several targets. You must
 * call one time for each target to report.
 *
 * @param message The message to report.
 */
void valvulad_log_report (ValvuladCtx   * ctx,
			    LogReportType     type, 
			    const char      * message, 
			    va_list           args,
			    const char      * file,
			    int               line)
{
	/* according to the type received report */
	if ((type & LOG_REPORT_GENERAL) == LOG_REPORT_GENERAL) 
		REPORT (ctx->use_syslog, LOG_REPORT_GENERAL, message, args, file, line);
	
	/* handle error and warning through the same log file */
	if ((type & LOG_REPORT_ERROR) == LOG_REPORT_ERROR) 
		REPORT (ctx->use_syslog, LOG_REPORT_ERROR, message, args, file, line);

	if ((type & LOG_REPORT_WARNING) == LOG_REPORT_WARNING) 
		REPORT (ctx->use_syslog, LOG_REPORT_WARNING, message, args, file, line);
	
	if ((type & LOG_REPORT_ACCESS) == LOG_REPORT_ACCESS) 
		REPORT (ctx->use_syslog, LOG_REPORT_ACCESS, message, args, file, line);

	if ((type & LOG_REPORT_VORTEX) == LOG_REPORT_VORTEX) {
		REPORT (ctx->use_syslog, LOG_REPORT_VORTEX, message, args, file, line);
	}
	return;
}

/** 
 * @brief Allows to check if the log to file is enabled on the
 * provided context.
 *
 * @param ctx The context where file log is checked to be enabled or
 * not.
 */
axl_bool   valvulad_log_is_enabled    (ValvuladCtx * ctx)
{
	axlDoc  * config;
	axlNode * node;

	/* check context received */
	if (ctx == NULL)
		return axl_false;
	
	/* get configuration */
	config = ctx->config;
	node   = axl_doc_get (config, "/valvulad/global-settings/log-reporting");

	/* check value returned */
	return valvulad_config_is_attr_positive (ctx, node, "enabled");
}

void __valvulad_log_close (ValvuladCtx * ctx)
{

	/* check if we are running with syslog support */
	if (ctx->use_syslog) {
		closelog ();
		return;
	}

	return;
}

void __valvulad_log_reopen (ValvuladCtx * ctx)
{
	msg ("Reload received, reopening log references..");

	/* call to close all logs opened at this moment */
	__valvulad_log_close (ctx);

	/* call to open again */
	valvulad_log_init (ctx);

	msg ("Log reopening finished..");

	return;
}

/** 
 * @internal
 * @brief Stops and dealloc all resources hold by the module.
 */
void valvulad_log_cleanup (ValvuladCtx * ctx)
{
	/* call to close current logs */
	__valvulad_log_close (ctx);

	return;
}


