/*
 *  Valvula: a high performance policy daemon
 *  Copyright (C) 2014 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; version 2.1 of the
 *  License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
 *
 *  For comercial support about integrating valvula or any other ASPL
 *  software production please contact as at:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº10, Edificio Alius A, Despacho 102
 *         Alcala de Henares, 28802 (MADRID)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/valvula
 */

/* include local turbulence header */
#include <valvulad.h>

axl_bool test_common_enable_debug = axl_false;

ValvuladCtx *  test_valvula_load_config (const char * label, const char * path, axl_bool run_config)
{
	ValvuladCtx * result;

	/* show a message */
	printf ("%s: loading configuration file at: %s\n", label, path);

	if (! valvulad_init (&result)) {
		printf ("ERROR: failed to initialize Valvulad context..\n");
		return NULL;
	}

	/* load config provided */
	if (! valvulad_config_load (result, path)) {
		printf ("ERROR: failed to load configuration at: %s\n", path);
		return NULL;
	} /* end if */

	/* try run configuration */
	if (run_config && ! valvulad_run_config (result)) {
		printf ("ERROR: failed to run configuration found at: %s\n", path);
		return NULL;
	} /* end if */

	return result;
}

void common_finish (ValvuladCtx * ctx)
{
	valvula_exit_ctx (ctx->ctx, axl_true);
	valvulad_exit (ctx);

	return;
}


/** 
 * @brief Check the turbulence db list implementation.
 * 
 * 
 * @return axl_true if the dblist implementation is ok, otherwise false is
 * returned.
 */
axl_bool  test_01 (void)
{
	ValvuladCtx * ctx;
	const char  * path;

	/* load basic configuration */
	path = "../server/valvula.example.conf";
	ctx  = test_valvula_load_config ("Test 01: ", path, axl_true);
	if (! ctx) {
		printf ("ERROR: unable to load configuration file at %s\n", path);
		return axl_false;
	} /* end if */

	/* wait a bit to let the system to startup */
	printf ("Test 01: waiting a bit..\n");
	sleep (2);

	/* free valvula server context */
	printf ("tst 01: finishing configuration..\n");
	common_finish (ctx);
	
	return axl_true;
}

#define CHECK_TEST(name) if (run_test_name == NULL || axl_cmp (run_test_name, name))

typedef axl_bool (* ValvulaTestHandler) (void);

/** 
 * @brief Helper handler that allows to execute the function provided
 * with the message associated.
 * @param function The handler to be called (test)
 * @param message The message value.
 */
int run_test (ValvulaTestHandler function, const char * message) {

	if (function ()) {
		printf ("%s [   OK   ]\n", message);
	} else {
		printf ("%s [ FAILED ]\n", message);
		exit (-1);
	}
	return 0;
}

/** 
 * @brief General regression test to check all features inside
 * valvula.
 */
int main (int argc, char ** argv)
{
	char * run_test_name = NULL;

	printf ("** test_01: Valvula: a high performance policy daemon\n");
	printf ("** Copyright (C) 2014 Advanced Software Production Line, S.L.\n**\n");
	printf ("** Regression tests: valvula: %s \n",
		VERSION);
	printf ("** To gather information about time performance you can use:\n**\n");
	printf ("**     time ./test_01 [--help] [--debug] [--run-test=NAME]\n**\n");
	printf ("** To gather information about memory consumed (and leaks) use:\n**\n");
	printf ("**     >> libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes --error-limit=no ./test_01 [--debug]\n**\n");
	printf ("** Providing --run-test=NAME will run only the provided regression test.\n");
	printf ("** Available tests: test_01\n");
	printf ("**\n");
	printf ("** Report bugs to:\n**\n");
	printf ("**     <valvula@lists.aspl.es> Valvula Mailing list\n**\n");

	/* uncomment the following four lines to get debug */
	while (argc > 0) {
		if (axl_cmp (argv[argc], "--help")) 
			exit (0);
		if (axl_cmp (argv[argc], "--debug")) 
			test_common_enable_debug = axl_true;

		if (argv[argc] && axl_memcmp (argv[argc], "--run-test", 10)) {
			run_test_name = argv[argc] + 11;
			printf ("INFO: running test: %s\n", run_test_name);
		}
		argc--;
	} /* end if */

	/* run tests */
	CHECK_TEST("test_01")
	run_test (test_01, "Test 01: basic server startup (using default configuration)");

	printf ("All tests passed OK!\n");

	/* terminate */
	return 0;
	
} /* end main */
