#!/usr/bin/python
# -*- coding: utf-8 -*-
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
import axl
import commands

# command line arguments
from optparse import OptionParser

# configuration declaration
valvula_conf = "/etc/valvula/valvula.conf"

conf = None

def get_conf ():
    global conf

    # if was loaded, just report it
    if conf:
        return conf
    
    (conf, err) = axl.file_parse (valvula_conf)
    if not conf:
        print "ERROR: unable to open file located at %s, error was: %s" % (valvula_conf, err.msg)
        sys.exit (-1)

    return conf

def save_config ():
    """
    Allows to save current document opened
    """
    
    # dump current configuration
    conf.file_dump (valvula_conf, 4)
    
    return

def list_current_listeners ():
    # get current configuration
    doc = get_conf ()

    listen = doc.get ("/valvula/general/listen")
    while listen:

        print "Listen: %s:%s" % (listen.attr ("host"), listen.attr ("port"))

        # get first run
        run = listen.child_called ("run")
        while run:

            print "  Mod: %s" % run.attr ("module")

            # next run
            run = run.next_called ("run")
        # end while
        
        # next listener
        listen = listen.next_called ("listen")
    # end while
    
    return

def get_modules ():
    # get all modules
    items  = os.listdir ("/etc/valvula/mods-available")
    result = []
    for item in items:
        if ".xml" == item[-4:]:
            result.append (item[:-4])
        # end if
    # end for

    return result

def list_modules ():

    modules = get_modules ()
    for mod in modules:
        print "Module: %s" % mod

    sys.exit (0)

def filter_empty (x):
    return len (x.strip ()) > 0

def find_listener (host, port):
    """
    Allows to find the listener node cleration located at the provided host and port.
    """
    
    listen = get_conf ().get ("/valvula/general/listen")

    while listen:
        if listen.attr ("port") == str (port) and listen.attr ("host") == host:
            # port found
            return listen
        # end if

        # get next node
        listen = listen.next_called ("listen")
    # end while

    return None

def check_and_get_host_port (decl):

    if ":" in decl:
        items = filter (filter_empty, args.split (":"))
        host  = items[0]
        port  = items[1]
    else:
        host  = "127.0.0.1"
        port  = decl
    # end if

    # now check port
    if not port.isdigit ():
        print "ERROR: port configuration provided isn't a valid port declaration: %s" % port
        sys.exit (-1)

    try:
        port = int (port)
    except Exception, err:
        print "ERROR: port configuration provided isn't a valid port declaration: %s" % str (err)
        sys.exit (-1)

    if port <= 0 or port >= 65535:
        print "ERROR: port value is not in the range of 1..65534"
        sys.exit (-1)

    return (host, port)
    

def add_new_listener (options, args):

    if not args:
        print "ERROR: please, provide port or host:port declaration. Received empty declaration"
        sys.exit (-1)

    # get host and port declaration from user input
    (host, port) = check_and_get_host_port (args[0])

    if find_listener (host, port):
        print "INFO: port is already in use, unable to add the port"
        sys.exit (0)

    listen = axl.Node ("listen")
    listen.attr ("host", host)
    listen.attr ("port", "%s" % port)

    # open document
    general = get_conf ().get ("/valvula/general")

    # add the node
    general.set_child (listen)

    # save the document
    save_config ()

    print "INFO: listener added, now you have to restart valvula!"
    return

def module_declared_in_port (listen, module_name):
    while listen:
        # get first run module
        run = listen.child_called ("run")
        while run:

            if run.attr ("module") == module_name:
                return True

            # next run module
            run = run.next_called ("run")
        # end while
    # end while

    # no module was declared on that listener
    return False
    

def add_module (options, args):
    
    if not args or len (args) != 2:
        print "ERROR: not all arguments were received. Unable to add module"
        sys.exit (-1)

    module_name  = args[0]
    # get host and port declaration from user input
    (host, port) = check_and_get_host_port (args[1])

    # find listener module
    listen = find_listener (host, port)
    if not listen:
        print "ERROR: unable to add module, listener %s wasn't found. Please add it first" % args[1]
        sys.exit (-1)

    # check module exists
    if module_name not in get_modules ():
        print "ERROR: unable to add module %s, it is not currently installed"
        sys.exit (-1)

    # check if the module was added previously
    if module_declared_in_port (listen, module_name):
        print "INFO: module %s already declared on that listener" % module_name
        sys.exit (-1)

    # create module
    run = axl.Node ("run")
    run.attr ("module", module_name)

    listen.set_child (run)

    save_config ()

    # add link if missing
    if not os.path.exists ("/etc/valvula/mods-enabled/%s.xml" % module_name):
        os.symlink ("/etc/valvula/mods-available/%s.xml" % module_name, "/etc/valvula/mods-enabled/%s.xml" % module_name)
    # end if

    print "INFO: module added, now you have to restart valvula!"
    return

postfix_sections = [
    ("smtpd_client_restrictions" , "Optional restrictions that the Postfix SMTP server applies in the context of a client connection request. "),
    ("smtpd_helo_restrictions" , "Optional restrictions that the Postfix SMTP server applies in the context of a client HELO command. "),
    ("smtpd_sender_restrictions" , "Optional restrictions that the Postfix SMTP server applies in the context of a client MAIL FROM command. "),
    ("smtpd_relay_restrictions" , "Access restrictions for mail relay control that the Postfix SMTP server applies in the context of the RCPT TO command before smtpd_recipient_restrictions."),
    ("smtpd_recipient_restrictions" , "Optional restrictions that the Postfix SMTP server applies in the context of a client RCPT TO command, after smtpd_relay_restrictions."),
    ("smtpd_data_restrictions" , "Optional access restrictions that the Postfix SMTP server applies in the context of the SMTP DATA command."),
    ("smtpd_end_of_data_restrictions" , "Optional access restrictions that the Postfix SMTP server applies in the context of the SMTP END-OF-DATA command."),
    ]

def list_postfix_sections (options, args):
    import textwrap

    print "INFO: the following are Postfix sections where you can configure valvula listeners."
    print "      These sections applies in the other as they are presented."
    print
    for (section, description) in postfix_sections:
        print "- %s :\n" % section
        for item in textwrap.wrap (description, 55):
            print "        %s" % item
        print 

    return

def postfix_section_supported (postfix_section):

    for (section, description) in postfix_sections:
        if section == postfix_section:
            return True
        # end if
    # end for

    return False

def _get_postfix_config_type (postfix_section, config_file = "/etc/postfix/main.cf"):
    """
    The function returns one of the following declaration according to
    the status and type of the conguration.
    
    not_found : declaration not found
    empty_decl : found declaration but empty
    multi_line_decl : found declarations but in many lines
    multi_line_decl_2 : found declarations but in many lines and a first declaration in the first line
    single_line_decl : found declarations in the same line
    """

    # process all files
    lines   = open (config_file).read ().split ("\n")

    found_declaration = False
    decl_found        = ""
    length            = len (postfix_section)
    first_line_decl   = False
    for line in lines:
        if postfix_section in line and line[:length] == postfix_section:
            found_declaration = True
            decl_found = line

            items = filter (filter_empty, decl_found.split ("="))
            if len (items) > 1:
                first_line_decl = True
            continue

        if not line:
            continue

        if line[0] == "\t" or line[0] == " " and found_declaration:
            if line.strip () == "#":
                continue

            if first_line_decl:
                return "multi_line_decl_2"
            
            return "multi_line_decl"

        if line[0] == "#":
            continue

        # reached this point, declaration was found but it is not multi line
        if found_declaration:
            items = filter (filter_empty, decl_found.split ("="))
            if len (items) == 1:
                return "empty_decl"
        
            return "single_line_decl"
        # end if
        
    # end for

    # reached this point, declaration was found but it is not multi line
    if found_declaration:
        items = filter (filter_empty, decl_found.split ("="))
        if len (items) == 1:
            return "empty_decl"
        
        return "single_line_decl"
    # end if

    return "not_found"

def _add_postfix_valvula_declaration_existing (postfix_section, host, port, order, config_file = "/etc/postfix/main.cf"):

    # get a classification about the configuration:
    clasification = _get_postfix_config_type (postfix_section, config_file)

    if clasification == "not_found":
        print "ERROR: not found declaration that should exist: %s at %s" % (postfix_section, config_file)
        sys.exit (-1)

    result  = []
    lines   = open (config_file).read ().split ("\n")
    decl    = "check_policy_service inet:%s:%s" % (host, port)

    length  = len (postfix_section)
    op_done = False
    for line in lines:

        if op_done:
            result.append (line)
            continue

        # process lines without 
        if line and line.strip ()[0] != '#':
            if postfix_section in line and line[:length] == postfix_section:

                if clasification == "empty_decl":
                    line    = "%s = %s" % (postfix_section, decl)
                    op_done = True
                    
                elif clasification in ["single_line_decl", "multi_line_decl_2"]:
                    content = line.split ("=")[1].strip ()
                    if order == "first":
                        line    = "%s = %s, %s" % (postfix_section, decl, content)
                    elif order == "last":
                        line    = "%s = %s, %s" % (postfix_section, content, decl)
                    op_done = True
                    
                elif clasification in ["multi_line_decl"]:
                    if order == "last":
                        print "ERROR: still not supported. "
                        sys.exit (-1)

                    # reached this point its is first
                    result.append (line)
                    result.append ("  %s," % decl)
                    op_done = True
                    continue
                # end if
            # end if
        # end if

        # add line
        result.append (line)
        
    # end for

    # save content
    open (config_file, "w").write ("\n".join (result))
    
    return

def get_postfix_normalized (config_file = "/etc/postfix/main.cf", section_name = None):
    if section_name:
        (status, output) = commands.getstatusoutput ("grep -v \# %s | grep -v ^$ | grep %s" % (config_file, section_name))
    else:
        (status, output) = commands.getstatusoutput ("grep -v \# %s | grep -v ^$" % config_file)
    return output.replace ("\n\t", " ").replace ("\n "," ").replace ("\n  ", " ")

def _add_postfix_valvula_declaration (postfix_section, host, port, order, config_file = "/etc/postfix/main.cf"):

    # check if the section already have this declaration
    output = get_postfix_normalized (config_file, postfix_section)

    # strip output
    output = output.strip ()

    # prepare declaration
    decl   = "check_policy_service inet:%s:%s" % (host,port)

    if output:
        if decl in output:
            print "found %s inside %s" % (decl, output)
            print "INFO: declaration already added into %s. Doing nothing" % postfix_section
            sys.exit (0)

        # now add declaration
        _add_postfix_valvula_declaration_existing (postfix_section, host, port, order, config_file)

        return
    # end if

    # postfix section is not present just added
    handler = open (config_file, "a")
    handler.write ("\n\n")
    handler.write ("%s = %s\n" % (postfix_section, decl))
    handler.close ()

    return


def do_connect_valvula_to_postfix (options, args):

    if len (args) != 3:
        print "ERROR: received wrong number of arguments to complete operation"
        sys.exit (-1)

    # get postfix section
    postfix_section = args[0]

    # get host and port declaration
    (host, port) = check_and_get_host_port (args[1])

    # order
    order = args[2]
    if order not in ["first", "last"]:
        print "ERROR: incorrect order provided: %s. Please, provide first or last" % order
        sys.exit (-1)

    # ok, find listener with that information
    if not find_listener (host, port):
        print "ERROR: provided listener location %s:%s which is not declared" % (host, port)
        sys.exit (-1)

    # check postfix section is supported
    if not postfix_section_supported (postfix_section):
        print "ERROR: postfix section provide isn't supported: %s. Try %s -s to know them" % (postfix_section, sys.argv[0])
        sys.exit (-1)

    # add declaration
    _add_postfix_valvula_declaration (postfix_section, host, port, order, "/etc/postfix/main.cf")

    print "INFO: connected valvulad at %s:%s to %s" % (host, port, postfix_section)
    print "INFO: now you must restart postfix"

    return

#### MAIN ####
parser = OptionParser()
parser.add_option("-l", "--list-listeners", action="store_true", dest="list_listeners", default=False,
                  help="Allows to listen currently declared valvula listeners and modules inside. ")
parser.add_option("-o", "--list-modules", action="store_true", dest="list_modules", default=False,
                  help="Allows to list current modules available. ")
parser.add_option("-a", "--add-listener", action="store_true", dest="add_listener", default=False,
                  help="Allows to add a listener to the current configuration. You can add just port or host:port. Note host must be an address running on this server.", metavar="HOST:PORT PORT")
parser.add_option("-m", "--add-module", action="store_true", dest="add_module", default=False,
                  help="Allows to add a module into a listener located at host:port. Use %s -l to list current listeners and then, use %s -m mod_name host:port to add it." % (sys.argv[0], sys.argv[0]), metavar="module_name HOST:PORT PORT" )
parser.add_option("-c", "--connect-postfix", action="store_true", dest="connect_postfix", default=False,
                  help="Usage: %s -c postfix_section host:port|port first|last. Allows to connect a valvula listener to postfix at some of its restriction sections. You can list current postfix sections by running %s -s. Here is an example %s -c smtpd_recipient_restrictions 3579 first" % (sys.argv[0], sys.argv[0], sys.argv[0]), metavar="postfix_section HOST:PORT" )
parser.add_option("-s", "--show-postfix-sections", action="store_true", dest="show_postfix_sections", default=False,
                  help="Allows to show postfix sections that can be used while connecting valvula listeners to its sections.")
parser.add_option("-n", "--show-postfix-conf", action="store_true", dest="show_postfix_conf", default=False,
                  help="Allows to show postfix configuration (like postconf -n) but allowing to change the file to inspect.")

# parse options received
(options, args) = parser.parse_args ()

if options.list_listeners:
    list_current_listeners ()
elif options.list_modules:
    list_modules ()
elif options.add_listener:
    add_new_listener (options, args)
elif options.add_module:
    add_module (options, args)
elif options.show_postfix_sections:
    list_postfix_sections (options, args)
elif options.connect_postfix:
    do_connect_valvula_to_postfix (options, args)
elif options.show_postfix_conf:
    print get_postfix_normalized ()
else:
    print "INFO: run %s --help to get additional information" % sys.argv[0]

