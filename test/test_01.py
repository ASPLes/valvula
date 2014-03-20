#!/usr/bin/python
# -*- coding: utf-8 -*-
#
#   Valvula: a high performance policy daemon
#   Copyright (C) 2014 Advanced Software Production Line, S.L.
# 
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License as
#   published by the Free Software Foundation; either version 2.1 of
#   the License, or (at your option) any later version.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
#   02111-1307 USA
#   
#   You may find a copy of the license under this software is released
#   at COPYING file. 
# 
#   For comercial support about integrating valvula or any other ASPL
#   software production please contact as at:
#           
#       Postal address:
#          Advanced Software Production Line, S.L.
#          C/ Antonio Suarez Nº 10, 
#          Edificio Alius A, Despacho 102
#          Alcalá de Henares 28802 (Madrid)
#          Spain
# 
#       Email address:
#          info@aspl.es - http://www.aspl.es/valvula

import sys
import os
sys.path.append ("../server")

# import module
m = __import__ ("valvulad-mgr")

### tests ###

def test_00 ():

    # check single line decl
    info ("Checking for single line declarations..")
    value = m._get_postfix_config_type ("smtpd_data_restrictions", "config.example.cf")
    if value != "single_line_decl":
        print "ERROR: expected single_line_decl for postfix section but found: %s" % value
        return False

    # check for multi_line_decl_2
    info ("Checking for multi line declaration with first declaration on first line")
    value = m._get_postfix_config_type ("smtpd_sender_restrictions", "config.example.cf")
    if value != "multi_line_decl_2":
        print "ERROR: expected multi_line_decl_2 for postfix section but found: %s" % value
        return False

    # check for multi_line_decl
    info ("Checking usual multi line declarations..")
    value = m._get_postfix_config_type ("smtpd_recipient_restrictions", "config.example.cf")
    if value != "multi_line_decl":
        print "ERROR: expected multi_line_decl for postfix section but found: %s" % value
        return False

    # check for empty declarations
    info ("Checking for empty declarations..")
    value = m._get_postfix_config_type ("smtpd_relay_restrictions", "config.example.cf")
    if value != "empty_decl":
        print "ERROR: expected empty_decl for postfix section but found: %s" % value
        return False

    # check for not found example
    info ("Checking for not found examples..")
    value = m._get_postfix_config_type ("not_found", "config.example.cf")
    if value != "not_found":
        print "ERROR: expected not_found for postfix section but found: %s" % value
        return False

    # check for single line decl
    info ("Checking for single line decl examples..")
    value = m._get_postfix_config_type ("smtpd_data_restrictions", "test_01.base")
    if value != "single_line_decl":
        print "ERROR: expected single_line_decl for postfix section but found: %s" % value
        return False
    
    return True

def test_01_do_update_and_check (base_file, postfix_section, host, port, order):
    import shutil
    
    # copy file
    shutil.copyfile (base_file, "%s.cf" % base_file)
    m._add_postfix_valvula_declaration (postfix_section, host, port, order, "%s.cf" % base_file)

    if not os.path.exists ("%s.ref" % base_file):
        print "ERROR: file %s.ref doesn't exist. Please run this command to check differences" % base_file
        print "       >> diff %s %s.cf" % (base_file, base_file)
        return False
        
    
    if open ("%s.cf" % base_file).read () != open ("%s.ref" % base_file).read ():
        print "ERROR: reference files %s.cf and %s.ref differs" % (base_file, base_file)
        print "       run: "
        print "       >> diff %s.cf %s.ref" % (base_file, base_file)
        return False

    return True

def test_01 ():

    # test single line first
    if not test_01_do_update_and_check ("test_01.base", "smtpd_data_restrictions", "127.0.0.1", "3579", "first"):
        return False

    # test single line last
    if not test_01_do_update_and_check ("test_01_last.base", "smtpd_data_restrictions", "127.0.0.1", "3579", "last"):
        return False

    # test multi line first
    if not test_01_do_update_and_check ("test_01_multi_first.base", "smtpd_data_restrictions", "127.0.0.1", "3579", "first"):
        return False
    
    return True


def info (msg):
    print "[ INFO  ] : " + msg

def error (msg):
    print "[ ERROR ] : " + msg

def ok (msg):
    print "[  OK   ] : " + msg

def run_all_tests ():
    test_count = 0
    for test in tests:
        
         # print log
        info ("TEST-" + str(test_count) + ": Running " + test[1])
        
        # call test
        if not test[0]():
            error ("detected test failure at: " + test[1])
            return False

        # next test
        test_count += 1
    
    ok ("All tests ok!")
    return True

# declare list of tests available
tests = [
   (test_00,   "Check postfix section type detection"),
   (test_01,   "Check postfix section updating")
]

### MAIN ###

if __name__ == '__main__':
    # drop a log
    info ("Running tests..")

    # call to run all tests
    run_all_tests ()
