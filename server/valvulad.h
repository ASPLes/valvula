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
#ifndef __VALVULAD_H__
#define __VALVULAD_H__

/* library include */
#include <valvula.h>
#include <exarg.h>

/* support to record at syslog */
#include <syslog.h>

/** 
 * @brief ValvuladCtx server context. Do no confuse with \ref ValvulaCtx.
 */
typedef struct _ValvuladCtx {
	axlDoc         * config;

	axl_bool         console_debug;
	axl_bool         console_enabled;
	axl_bool         console_debug2;
	axl_bool         console_debug3;
	axl_bool         console_color_debug;
	axl_bool         use_syslog;
	axl_bool         debug_queries;

	int              pid;

	char           * config_path;

	ValvulaCtx     * ctx;

	/** 
	 * @brief List of registered modules currently loaded.
	 */
	axlList        * registered_modules;
	ValvulaMutex     registered_modules_mutex;

	/** 
	 * @brief List of on day change registered handlers 
	 */
	axlList        * on_day_change_handlers;

	/** 
	 * @brief List of on day change registered handlers 
	 */
	axlList        * on_month_change_handlers;

	/** 
	 * @brief Default location for the postfix configuration file.
	 */
	const char     * postfix_file;

	/** 
	 * @brief Local domains configurations.
	 */
	char           * ld_query;
	char           * ld_user;
	char           * ld_pass;
	char           * ld_dbname;
	char           * ld_host;
	/** 
	 * @brief This hash holds a set of domains that are found at
	 * configuration files or static files.
	 */
	axlHash        * ld_hash;

	/** 
	 * @brief Local accounts configurations.
	 */
	char           * la_query;
	char           * la_user;
	char           * la_pass;
	char           * la_dbname;
	char           * la_host;
	/** 
	 * @brief This hash holds a set of local accounts that are
	 * found at configuration files or static files.
	 */
	axlHash        * la_hash;

	/** 
	 * @brief Local aliases configurations.
	 */
	char           * ls_query;
	char           * ls_user;
	char           * ls_pass;
	char           * ls_dbname;
	char           * ls_host;
	/** 
	 * @brief This hash holds a set of aliases that are found at
	 * configuration files or static files.
	 */
	axlHash        * ls_hash;
} ValvuladCtx;

typedef struct _ValvuladHandlePtr {
	axlPointer handler;
	axlPointer ptr;
} ValvuladHandlerPtr;

/* common includes */
#include <valvulad_config.h>
#include <valvulad_log.h>
#include <valvulad_run.h>
#include <valvulad_moddef.h>
#include <valvulad_module.h>
#include <valvulad_db.h>

axl_bool  valvulad_log_enabled      (ValvuladCtx * ctx);

void      valvulad_log_enable       (ValvuladCtx * ctx, 
				       int  value);

axl_bool  valvulad_log2_enabled     (ValvuladCtx * ctx);

void      valvulad_log2_enable      (ValvuladCtx * ctx,
				       int  value);

axl_bool  valvulad_log3_enabled     (ValvuladCtx * ctx);

void      valvulad_log3_enable      (ValvuladCtx * ctx,
				       int  value);

void      valvulad_color_log_enable (ValvuladCtx * ctx,
				       int             value);

/** 
 * Drop an error msg to the console stderr.
 *
 * To drop an error message use:
 * \code
 *   error ("unable to open file: %s", file);
 * \endcode
 * 
 * @param m The error message to output.
 */
#define error(m,...) do{valvulad_error (ctx, axl_false, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  valvulad_error (ValvuladCtx * ctx, axl_bool ignore_debug, const char * file, int line, const char * format, ...);

/** 
 * Drop an error msg to the console stderr without taking into
 * consideration debug configuration.
 *
 * To drop an error message use:
 * \code
 *   abort_error ("unable to open file: %s", file);
 * \endcode
 * 
 * @param m The error message to output.
 */
#define abort_error(m,...) do{valvulad_error (ctx, axl_true, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)

/** 
 * Drop a msg to the console stdout.
 *
 * To drop a message use:
 * \code
 *   msg ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The console message to output.
 */
#define msg(m,...)   do{valvulad_msg (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  valvulad_msg   (ValvuladCtx * ctx, const char * file, int line, const char * format, ...);

/** 
 * Drop a second level msg to the console stdout.
 *
 * To drop a message use:
 * \code
 *   msg2 ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The console message to output.
 */
#define msg2(m,...)   do{valvulad_msg2 (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  valvulad_msg2   (ValvuladCtx * ctx, const char * file, int line, const char * format, ...);



/** 
 * Drop a warning msg to the console stdout.
 *
 * To drop a message use:
 * \code
 *   wrn ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The warning message to output.
 */
#define wrn(m,...)   do{valvulad_wrn (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  valvulad_wrn   (ValvuladCtx * ctx, const char * file, int line, const char * format, ...);

/** 
 * Drops to the console stdout a warning, placing the content prefixed
 * with the file and the line that caused the message, without
 * introducing a new line.
 *
 * To drop a message use:
 * \code
 *   wrn_sl ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The warning message to output.
 */
#define wrn_sl(m,...)   do{valvulad_wrn_sl (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  valvulad_wrn_sl   (ValvuladCtx * ctx, const char * file, int line, const char * format, ...);

/** 
 * Reports an access message, a message that is sent to the access log
 * file. The message must contain access to the server information.
 *
 * To drop a message use:
 * \code
 *   access ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The console message to output.
 */
#define tbc_access(m,...)   do{valvulad_access (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  valvulad_access   (ValvuladCtx * ctx, const char * file, int line, const char * format, ...);

void  valvulad_reject (ValvuladCtx * ctx, ValvulaRequest * request, const char * format, ...);

/** 
 * @internal The following definition allows to find printf like wrong
 * argument passing to nopoll_log function. To activate the depuration
 * just add the following header after this comment.
 *
 * #define SHOW_FORMAT_BUGS (1)
 */
#if defined(SHOW_FORMAT_BUGS)
# undef error
# undef msg
# undef msg2
# undef wrn
# undef wrn_sl
# undef tbc_access
#define error(m,...) do{printf (m, ##__VA_ARGS__);}while(0)
#define msg(m,...)   do{printf (m, ##__VA_ARGS__);}while(0)
#define msg2(m,...)   do{printf (m, ##__VA_ARGS__);}while(0)
#define wrn(m,...)   do{printf (m, ##__VA_ARGS__);}while(0)
#define wrn_sl(m,...)   do{printf (m, ##__VA_ARGS__);}while(0)
#define tbc_access(m,...)   do{printf (m, ##__VA_ARGS__);}while(0)
#endif

char          * valvulad_support_get_backtrace (ValvuladCtx * ctx, int pid);

axl_bool valvulad_init (ValvuladCtx ** result);

axl_bool valvulad_init_aux (ValvuladCtx * ctx);

void valvulad_exit (ValvuladCtx * ctx);

/** 
 * @brief Handler that represents those functions that are called to
 * notify day change.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param new_value The new value being notified: may it be day or month value.
 *
 * @param user_data User defined pointer to be defined at \ref valvulad_add_on_day_change
 */
typedef void (*ValvuladOnDateChange) (ValvuladCtx * ctx, long new_value, axlPointer user_data);

/** 
 * @brief Valvula date item that allows selecting which item must be
 * configured or notified.
 */
typedef enum {
	/** 
	 * @brief Changes and configurations that refers to day.
	 */
	VALVULAD_DATE_ITEM_DAY = 1,
	/** 
	 * @brief Chnages and configurations that refers to month.
	 */
	VALVULAD_DATE_ITEM_MONTH = 2,
} ValvuladDateItem;

void valvulad_add_on_day_change   (ValvuladCtx * ctx, ValvuladOnDateChange on_day_change, axlPointer ptr);

void valvulad_add_on_month_change (ValvuladCtx * ctx, ValvuladOnDateChange on_day_change, axlPointer ptr);

void valvulad_notify_date_change  (ValvuladCtx * ctx, long new_value, ValvuladDateItem item_type);

int  valvulad_get_system_id       (ValvuladCtx * ctx, const char * value, axl_bool get_user);

#endif
