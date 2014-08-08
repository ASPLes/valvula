#!/usr/bin/python

import os
import sys

verbose  = False
pid_file = "/var/run/valvulad.pid"

def error (msg):
    if not verbose:
        return

    print "ERROR: %s" % msg
    return

def info (msg):
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
if not os.path.exists (pid_file):
    info ("Nothing to check, valvulad is not running and was not started")
    sys.exit (0)

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
os.system ("service valvulad stop > /dev/null 2>&1")
os.system ("service valvulad start > /dev/null 2>&1")






    