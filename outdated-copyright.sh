#!/bin/bash

rm -rf doc/html
rm -rf debian/tmp
rm -rf debian/libvalvula
rm -rf debian/libvalvula-dev
rm -rf debian/valvulad-server
rm -rf debian/libvalvula-server-dev
rm -rf debian/libvalvula-server
rm -rf debian/valvulad-mod-ticket
rm -rf debian/valvulad-mod-slm
rm -rf debian/valvulad-mod-bwl
rm -rf debian/valvulad-mod-mquota
rm -rf debian/valvulad-mod-object-resolver
rm -rf debian/valvula-doc

# find all files that have copy right declaration associated to Aspl that don't have 
# the following declaration year
current_year="2025"
LANG=C rgrep "Copyright" debian-files doc lib plugins server rpm test web configure.ac 2>&1 | grep "Advanced" | grep -v "Permission denied" | grep -v '~:' | grep -v '/\.svn/' | grep -v "${current_year}"
