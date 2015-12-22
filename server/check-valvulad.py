#!/usr/bin/python

import os
import sys
import syslog
import commands

verbose  = False
pid_file = "/var/run/valvulad.pid"

def error (msg):
    syslog.syslog (syslog.LOG_ERR, "check-valvula: error: %s" % msg)
    if not verbose:
        return

    print "ERROR: %s" % msg
    return

def info (msg):
    syslog.syslog (syslog.LOG_INFO, "check-valvula: info: %s" % msg)

    if not verbose:
        return

    print "INFO: %s" % msg
    return

### MAIN ###

# detect verbose 
for arg in sys.argv:
    if arg in [ "--verbose", "-v", "verbose"]:
        verbose = True

# check if we have to implement the check
# if not os.path.exists (pid_file):
#     info ("Nothing to check, valvulad is not running and was not started")
#     sys.exit (0)

# call to ping server
status = os.system ("valvulad -p > /dev/null 2>&1")
if status == 0:
    info ("Valvulad server is working right")
    sys.exit (0)

error ("Valvula is failing, trying to recover it")
try:
    os.unlink (pid_file)
except Exception:
    pass

# kill all instances
os.system ("killall -9 valvulad  > /dev/null 2>&1")
os.system ("killall -9 valvulad  > /dev/null 2>&1")

# now stop and start
(status, output) = commands.getstatusoutput ("service --help")
if status == 0:
    os.system ("service valvulad stop > /dev/null 2>&1")
    (status, output) = commands.getstatusoutput ("service valvulad start")
else:
    if os.path.exists ("/etc/init.d/valvulad"):
        os.system ("/etc/init.d/valvulad stop")
        (status, output) = commands.getstatusoutput ("/etc/init.d/valvulad start")
    else:
        error ("Failed to recover valvula, unable to find service or /etc/init.d startup script to start the service")
        sys.exit (-1)
    # end if
# end if
    
if status:
    error ("Failed to recover valvula, error was: %s" % output)
    sys.exit (-1)
# end if

info ("Valvula recovered, output was: %s" % output)

# integration with core-admin (if present)







    
