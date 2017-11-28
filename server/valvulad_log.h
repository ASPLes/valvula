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
#ifndef __VALVULAD_LOG_H__
#define __VALVULAD_LOG_H__

#include <valvulad.h>

void      valvulad_log_init         (ValvuladCtx * ctx);

typedef enum {LOG_REPORT_GENERAL = 1, 
	      LOG_REPORT_ACCESS  = 1 << 2, 
	      LOG_REPORT_VORTEX  = 1 << 3,
	      LOG_REPORT_ERROR   = 1 << 4,
	      LOG_REPORT_WARNING = 1 << 5
} LogReportType;

void      valvulad_log_report (ValvuladCtx * ctx,
				 LogReportType   type, 
				 const char    * message,
				 va_list         args,
				 const char    * file,
				 int             line);

void      valvulad_log_configure (ValvuladCtx * ctx,
				    LogReportType   type,
				    int             descriptor);

void      valvulad_log_manager_start (ValvuladCtx * ctx);

void      valvulad_log_manager_register (ValvuladCtx * ctx,
					 LogReportType   type,
					 int             descriptor);

axl_bool  valvulad_log_is_enabled    (ValvuladCtx * ctx);

void      valvulad_log_cleanup       (ValvuladCtx * ctx);

void      __valvulad_log_reopen      (ValvuladCtx * ctx);

#endif
