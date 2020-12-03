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
#ifndef __MOD_MQUOTA_H__
#define __MOD_MQUOTA_H__

#include <valvulad.h>

typedef struct _ModMquotaLimit {
	/* this period id has a reference to the plan that this period
	 * applies. If it has -1 as value, it means it is a period
	 * defined at the valvula.conf file. Otherwise, this is a
	 * period declared at SQL tables */
	int            period_id;
	const char   * label;

	/* when this applies */
	int            start_hour;
	int            start_minute;

	/* when this ends */
	int            end_hour;
	int            end_minute;

	/* limits */
	int            minute_limit;
	int            hour_limit;
	int            global_limit;

	/* domain limits */
	int            domain_minute_limit;
	int            domain_hour_limit;
	int            domain_global_limit;

	/* mutex and hash that tracks global counting for this
	 * period. Each ModMquotaLimit represents a period that is
	 * being applied inside a plan. */
	axlHash      * accounting;
	axlHash      * domain_accounting;

} ModMquotaLimit;

extern ModMquotaLimit * mod_mquota_get_current_period (int current_minute, int current_hour);

#endif
