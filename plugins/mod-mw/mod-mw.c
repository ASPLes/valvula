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
#include <valvulad.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

ValvuladCtx * ctx = NULL;

/** 
 * @internal Init function, perform all the necessary code to register
 * profiles, configure Vortex, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  mw_init (ValvuladCtx * _ctx)
{
	/* configure the module */
	ctx = _ctx;

	msg ("Valvulad mw module: init");

	/* do some initialization code here */

	return axl_true;
}

/** 
 * @internal Close function called once the valvulad server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
void mw_close (ValvuladCtx * ctx)
{
	msg ("Valvulad mw module: close");
	return;
}

/** 
 * @internal Process request for the module. This is the function that
 * will be called every time postfix needs to module's opinion about
 * certain request.
 */
ValvulaState mw_process_request (ValvulaCtx        * _ctx, 
				   ValvulaConnection * connection, 
				   ValvulaRequest    * request,
				   axlPointer          request_data,
				   char             ** message)
{
	/* by default report return dunno */
	return VALVULA_STATE_DUNNO;
}


/** 
 * @internal The reconf function is used by valvulad to notify to all
 * its modules loaded that a reconfiguration signal was received and
 * modules that could have configuration and run time change support,
 * should reread its files. It is an optional handler.
 */
void mw_reconf (ValvuladCtx * ctx) {
	msg ("Valvulad configuration have change");
	return;
}


/** 
 * @internal Public entry point for the module to be loaded. This is the
 * symbol the valvulad will lookup to load the rest of items.
 */
ValvuladModDef module_def = {
	"mod-mw",
	"Valvulad mw module",
	mw_init,
	mw_close,
	mw_process_request,
	mw_reconf,
	NULL
};

END_C_DECLS

/** 
 * \page valvulad_mod_mw mod-mw: MySQL Works, a module to implement custom SQL queries on requests received
 *
 * \section mw_intro mod-MySQL Works introduction
 *
 * Have you ever wanted to run some SQL queries personalized with
 * request data received by postfix? Then mod-mw is for you. This
 * module allows you to run customized SQL queries with actual request
 * data on the valvula datbase, postfix database or some other mysql
 * database defined by you.
 *
 * \section mw_by_example mod-MySQL Works by example
 *
 * \htmlinclude mod-mw.example.conf.xml
 *
 */

