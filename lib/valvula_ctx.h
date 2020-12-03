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
#ifndef __VALVULA_CTX_H__
#define __VALVULA_CTX_H__

#include <valvula.h>

BEGIN_C_DECLS

ValvulaCtx * valvula_ctx_new                       (void);

ValvulaRequestRegistry *   valvula_ctx_register_request_handler (ValvulaCtx             * ctx, 
								 const char             * identifier,
								 ValvulaProcessRequest    process_handler, 
								 int                      priority, 
								 int                      port,
								 axlPointer               user_data);

void        valvula_ctx_set_final_state_handler   (ValvulaCtx              * ctx,
						   ValvulaReportFinalState   handler,
						   axlPointer                user_data);

void        valvula_ctx_set_request_line_limit    (ValvulaCtx       * ctx,
						   int                line_limit);

void        valvula_ctx_set_default_reply_state   (ValvulaCtx       * ctx,
						   ValvulaState       state);

void        valvula_ctx_set_data                  (ValvulaCtx       * ctx, 
						   const char      * key, 
						   axlPointer        value);

void        valvula_ctx_set_data_full             (ValvulaCtx       * ctx, 
						   const char      * key, 
						   axlPointer        value,
						   axlDestroyFunc    key_destroy,
						   axlDestroyFunc    value_destroy);

axlPointer  valvula_ctx_get_data                  (ValvulaCtx       * ctx,
						   const char      * key);

void        valvula_ctx_ref                       (ValvulaCtx  * ctx);

void        valvula_ctx_ref2                      (ValvulaCtx  * ctx, const char * who);

void        valvula_ctx_unref                     (ValvulaCtx ** ctx);

void        valvula_ctx_unref2                    (ValvulaCtx ** ctx, const char * who);

int         valvula_ctx_ref_count                 (ValvulaCtx  * ctx);

void        valvula_ctx_free                      (ValvulaCtx * ctx);

void        valvula_ctx_free2                     (ValvulaCtx * ctx, const char * who);

END_C_DECLS

#endif /* __VALVULA_CTX_H__ */
